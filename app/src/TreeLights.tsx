import { useLayoutEffect, useMemo, useRef } from "react";
import { useFrame } from "@react-three/fiber";
import { useGLTF, useTexture } from "@react-three/drei";
import { mergeGeometries } from "three/examples/jsm/utils/BufferGeometryUtils.js";
import { AdditiveBlending, BufferAttribute, Color, ConeGeometry, DoubleSide, InstancedMesh, type BufferGeometry, Mesh, Object3D, Quaternion, SphereGeometry, SRGBColorSpace, Vector3 } from "three";
import { useTwin } from "./store";
import { litFor, type Lit } from "./patterns";
import { telemetry, type LightState } from "./telemetry";
import { updateField, fieldOut, type Attractor } from "./field";
import { updatePiano, keyBri, fixtureMidi } from "./piano";
import { updateAudio, setEqGains } from "./audio";
import { strobeGate, eqGain, lerp } from "./dj";
import { rippleIntensity } from "./interaction";
import { guestClamp } from "./guard";
import { relativeBeam } from "./photometry";
import { applyEnv } from "./sensors";
import { easeGroundTint } from "./groundtint";
import { createGlslPass, type GlslPass } from "./glslPass";

// Global pattern-motion slowdown (everything was tuned too fast). 1.0 = old
// frantic baseline; lower = calmer. The speed dial multiplies on top of this.
const TIME_SCALE = 0.35;

const dummy = new Object3D();
const col = new Color();
// beam-aim helpers: orient each cast cone along its real direction
const DOWN = new Vector3(0, -1, 0);
const aimV = new Vector3();
const aimQ = new Quaternion();
const lit: Lit = { r: 0, g: 0, b: 0 };
const litB: Lit = { r: 0, g: 0, b: 0 };
const GAIN = 1.65;
const DEG = Math.PI / 180;
// Tight "ray" cone: a narrow reference half-angle gives crisp light shafts
// instead of a washy flood. Per-fixture beamDeg is mapped against this below.
const RAY_HALF = 15 * DEG;
const refTan = Math.tan(RAY_HALF);

/** All lights as one InstancedMesh of lanterns + (optionally) one of beam cones.
 *  Render style (A7) is the `visualizer` mode: lanterns / orbs / wire.
 *  Tick = firmware stand-in (mock heartbeat, G1); render shows REPORTED state. */
export function TreeLights() {
  const fixtures = useTwin((s) => s.fixtures);
  const treeSize = useTwin((s) => s.size);
  const center = useTwin((s) => s.center);
  const viz = useTwin((s) => s.control.visualizer);
  const dotSize = treeSize * (viz === "orbs" ? 0.022 : viz === "wire" ? 0.009 : 0.012);
  const beamH = treeSize * 0.38;
  const showBeams = viz !== "wire";

  const lightRef = useRef<InstancedMesh>(null);
  const beamRef = useRef<InstancedMesh>(null);
  const groundRef = useRef<InstancedMesh>(null); // per-fixture petal-gobo "light shades" on the floor
  const glslRef = useRef<GlslPass | null>(null); // lazy GPU pattern pass (glslMode)
  const groundY = center[1] - treeSize * 0.5;
  const gobo = useTexture("/gobo.png");
  gobo.colorSpace = SRGBColorSpace;

  // The REAL downlight fixture body (blender's downlight_lantern.glb) — the LED
  // is buried in the tube, so the BODY itself is what we see, glowing with the
  // pattern colour (the source is hidden, not a bare floating dot). Swappable:
  // drop a new glb in /public and it re-instances. The glb hangs along -Y.
  const { scene: lanternScene } = useGLTF("/downlight_lantern.glb");
  const lanternGeom = useMemo<BufferGeometry>(() => {
    const geos: BufferGeometry[] = [];
    lanternScene.updateMatrixWorld(true);
    lanternScene.traverse((o) => {
      const m = o as Mesh;
      if (m.isMesh && m.geometry) {
        const g = m.geometry.clone();
        g.applyMatrix4(m.matrixWorld);
        for (const k of Object.keys(g.attributes)) if (k !== "position" && k !== "normal") g.deleteAttribute(k);
        if (!g.getAttribute("normal")) g.computeVertexNormals();
        geos.push(g);
      }
    });
    const g = geos.length ? (geos.length === 1 ? geos[0] : mergeGeometries(geos, false)) : new SphereGeometry(1, 12, 12);
    g.computeBoundingBox();
    return g;
  }, [lanternScene]);
  // normalize the body to a visible fixture size (≈4.5% of treeSize tall)
  const lanternScale = useMemo(() => {
    const bb = lanternGeom.boundingBox;
    const h = bb ? Math.max(1e-4, bb.max.y - bb.min.y) : 0.25;
    return (treeSize * 0.045) / h;
  }, [lanternGeom, treeSize]);

  const beamGeom = useMemo(() => {
    const g = new ConeGeometry(beamH * refTan, beamH, 18, 1, true);
    g.translate(0, -beamH / 2, 0); // apex (source) at y=0, base (far) at y=-beamH
    // volumetric falloff: bright at the source, fading to nothing into the air
    // (additive × this vertex gradient = a real light shaft, not a flat cone)
    const pos = g.attributes.position;
    const col = new Float32Array(pos.count * 3);
    for (let i = 0; i < pos.count; i++) {
      const v = Math.max(0, Math.min(1, (pos.getY(i) + beamH) / beamH)); // 1 source → 0 far
      const f = v * v; // sharper near-source concentration
      col[i * 3] = f; col[i * 3 + 1] = f; col[i * 3 + 2] = f;
    }
    g.setAttribute("color", new BufferAttribute(col, 3));
    return g;
  }, [beamH]);

  const repR = useRef<Float32Array>(new Float32Array(0));
  const repG = useRef<Float32Array>(new Float32Array(0));
  const repB = useRef<Float32Array>(new Float32Array(0));
  // displayed (smoothed) colour — a low-pass slew toward the reported value so
  // every transition (pattern / cue / crossfade / colour) eases instead of cutting
  const dispR = useRef<Float32Array>(new Float32Array(0));
  const dispG = useRef<Float32Array>(new Float32Array(0));
  const dispB = useRef<Float32Array>(new Float32Array(0));
  const lastReport = useRef<Float32Array>(new Float32Array(0));
  const statsAt = useRef(0);
  const lastTele = useRef(0); // throttle the telemetry/data-log snapshot (~5 Hz)
  const teleBuf = useRef<LightState[]>([]);

  useLayoutEffect(() => {
    const lm = lightRef.current;
    if (!lm) return;
    const bm = beamRef.current;
    const n = fixtures.length;
    repR.current = new Float32Array(n);
    repG.current = new Float32Array(n);
    repB.current = new Float32Array(n);
    dispR.current = new Float32Array(n);
    dispG.current = new Float32Array(n);
    dispB.current = new Float32Array(n);
    lastReport.current = new Float32Array(n).fill(-1e9);
    fixtures.forEach((f, i) => {
      dummy.position.set(f.pos[0], f.pos[1], f.pos[2]);
      dummy.rotation.set(0, 0, 0); // glb hangs along -Y → straight-down lantern (downlight, never up)
      dummy.scale.setScalar(lanternScale);
      dummy.updateMatrix();
      lm.setMatrixAt(i, dummy.matrix);
      lm.setColorAt(i, col.setRGB(0, 0, 0));
      if (bm) {
        // map the fixture's real beam angle to a TIGHT visual ray: clamp wide
        // floods (≤70°) and cap width at 1.6× the reference so rays stay crisp.
        const s = Math.max(0.5, Math.min(1.6, Math.tan(Math.min(70, Math.max(12, f.beamDeg)) * 0.5 * DEG) / refTan));
        // real cast geometry: use the fixture's true aim (schema 0.2) when the
        // Blender export provides it; else fall back to the heuristic (canopy
        // downlights aim DOWN + fan radially OUTWARD from the trunk).
        if (f.aim) {
          aimV.set(f.aim[0], f.aim[1], f.aim[2]).normalize();
        } else {
          const rx = f.pos[0] - center[0], rz = f.pos[2] - center[2];
          const rl = Math.hypot(rx, rz) || 1;
          aimV.set((rx / rl) * 0.5, -1, (rz / rl) * 0.5).normalize();
        }
        // downlights cast DOWN only — never project upward (uplights are separate)
        if (aimV.y > -0.1) { aimV.y = -0.1; aimV.normalize(); }
        aimQ.setFromUnitVectors(DOWN, aimV);
        dummy.quaternion.copy(aimQ);
        dummy.scale.set(s, 1, s);
        dummy.updateMatrix();
        bm.setMatrixAt(i, dummy.matrix);
        bm.setColorAt(i, col.setRGB(0, 0, 0));
        dummy.quaternion.identity(); // reset for next light-mesh write
      }
      const gm = groundRef.current;
      if (gm) {
        // the "light shade": a petal-gobo cookie on the floor below each fixture,
        // sized by its throw to the ground (≈ half the drop height) — overlapping
        // colour-tinted cookies = the real projected petal shapes
        const h = Math.max(0.2, f.pos[1] - groundY);
        const sz = h * 0.5;
        dummy.position.set(f.pos[0], groundY + treeSize * 0.003, f.pos[2]);
        dummy.rotation.set(-Math.PI / 2, 0, 0);
        dummy.scale.set(sz, sz, sz);
        dummy.updateMatrix();
        gm.setMatrixAt(i, dummy.matrix);
        gm.setColorAt(i, col.setRGB(0, 0, 0));
      }
    });
    lm.instanceMatrix.needsUpdate = true;
    if (lm.instanceColor) lm.instanceColor.needsUpdate = true;
    if (bm) {
      bm.instanceMatrix.needsUpdate = true;
      if (bm.instanceColor) bm.instanceColor.needsUpdate = true;
    }
    const gm = groundRef.current;
    if (gm) {
      gm.instanceMatrix.needsUpdate = true;
      if (gm.instanceColor) gm.instanceColor.needsUpdate = true;
    }
  }, [fixtures, dotSize, groundY, treeSize]);

  useFrame((state, delta) => {
    const lm = lightRef.current;
    const n = fixtures.length;
    if (!lm || n === 0) return;
    // low-pass slew factor: ~110ms time-constant → soft transitions, clamped
    const k = Math.min(1, 1 - Math.exp(-Math.max(0, delta) / 0.11));
    const bm = beamRef.current;
    const gm = groundRef.current;
    const t = state.clock.elapsedTime;
    // GLOBAL SLOWDOWN: the per-pattern motion constants were tuned too hot —
    // everything read as frantic. Scale the time fed into the patterns so the
    // baseline is calm; the speed dial (c.speed) still rides on top of this.
    // (strobe + ripples below keep real `t` — only pattern motion is slowed.)
    const pt = t * TIME_SCALE;
    // throttle the data-log snapshot (~5 Hz); (re)size the buffer with the fixture set
    const writeTele = t - lastTele.current > 0.18;
    if (writeTele && teleBuf.current.length !== n) {
      teleBuf.current = fixtures.map((f) => ({ num: f.num, id: f.id, bri: 0, rgb: [0, 0, 0] as [number, number, number] }));
    }
    const st = useTwin.getState();
    const { overrides, view } = st;
    // fold live environmental sensors (crowd/temp/wind/daylight) into the look
    const ctrl = applyEnv(st.guest ? guestClamp(st.control) : st.control, st.sensors);
    const audio = updateAudio();
    setEqGains(ctrl.eqLow, ctrl.eqMid, ctrl.eqHigh); // EQ knobs also filter the live audio (DJ deck mixes)
    // DJ controller (C): crossfade to look B, master intensity, strobe gate
    const xfade = ctrl.xfade;
    const cb = xfade > 0.001 ? { ...ctrl, pattern: ctrl.djPatternB, hue: ctrl.djHueB } : null;
    // per-group/subset LAYERS: map each owned light number → its effective control
    // (the layer's control merged over the base). Last layer to claim a number wins.
    const layerCtrl = st.layers.length ? new Map<number, typeof ctrl>() : null;
    if (layerCtrl) for (const ly of st.layers) {
      const merged = { ...ctrl, ...ly.control };
      for (const nn of ly.nums) layerCtrl.set(nn, merged);
    }
    // decentralised "living" field — run once per frame if any active look uses it
    const useField = ctrl.pattern === "living" || st.layers.some((l) => l.control.pattern === "living");
    if (useField) {
      const c = st.center, sz = st.size;
      const fr = (x: number) => x - Math.floor(x);
      const att: Attractor[] = [
        { x: c[0] + Math.cos(t * 0.05) * sz * 0.3, y: c[1] + Math.sin(t * 0.037) * sz * 0.22, z: c[2] + Math.sin(t * 0.045) * sz * 0.3, hue: fr(t * 0.012) },
        { x: c[0] + Math.cos(t * 0.031 + 2) * sz * 0.34, y: c[1] + Math.cos(t * 0.052) * sz * 0.26, z: c[2] + Math.sin(t * 0.041 + 1) * sz * 0.3, hue: fr(t * 0.012 + 0.5) },
      ];
      updateField(fixtures, delta, ctrl.speed, att);
    }
    // piano: the canopy plays a score — refresh per-key brightness once per frame
    if (ctrl.pattern === "piano" || st.layers.some((l) => l.control.pattern === "piano")) {
      updatePiano(performance.now() / 1000);
    }
    const mg = ctrl.master * (ctrl.strobe ? strobeGate(t, ctrl.strobeHz) : 1);
    const ripples = st.ripples;
    const nowS = performance.now() / 1000;
    const rSpeed = treeSize * 0.55;
    const rWidth = treeSize * 0.06;
    let dead = 0;
    let stale = 0;
    let sumR = 0, sumG = 0, sumB = 0; // aggregate for the ground tint

    // GLSL mode (opt-in): run the GPU pattern pass once + read N×1 RGB back to
    // drive the fixtures, instead of the CPU litFor. Lazily built per fixture set.
    let glslBuf: Uint8Array | null = null;
    if (ctrl.glslMode) {
      try {
        if (!glslRef.current) glslRef.current = createGlslPass(fixtures, ctrl.glslPattern);
        else glslRef.current.setPattern(ctrl.glslPattern);
        glslBuf = glslRef.current.renderRead(state.gl, pt, ctrl, audio);
      } catch { glslBuf = null; } // any GL hiccup → fall back to CPU litFor
    }

    for (let i = 0; i < n; i++) {
      const f = fixtures[i];
      const fctrl = layerCtrl ? (layerCtrl.get(f.num) ?? ctrl) : ctrl;
      if (glslBuf) {
        lit.r = glslBuf[i * 4] / 255; lit.g = glslBuf[i * 4 + 1] / 255; lit.b = glslBuf[i * 4 + 2] / 255;
      } else if (fctrl.pattern === "living") {
        col.setHSL(fieldOut.hue[i], fctrl.sat, 0.5);
        const bv = fieldOut.bri[i] * fctrl.brightness;
        lit.r = col.r * bv; lit.g = col.g * bv; lit.b = col.b * bv;
      } else if (fctrl.pattern === "piano") {
        const m = fixtureMidi(fixtures, i);
        if (m < 0) { lit.r = lit.g = lit.b = 0; }
        else {
          const bv = keyBri[m] * fctrl.brightness;
          const hue = ((0.66 - ((m - 36) / 71) * 0.25) % 1 + 1) % 1; // low=deep blue → high=cyan
          col.setHSL(hue, 0.6, 0.5);
          lit.r = col.r * bv; lit.g = col.g * bv; lit.b = col.b * bv;
        }
      } else {
        litFor(pt, f, fctrl, audio, n, lit);
      }
      if (cb && !(layerCtrl && layerCtrl.has(f.num))) {
        litFor(pt, f, cb, audio, n, litB);
        lit.r = lerp(lit.r, litB.r, xfade);
        lit.g = lerp(lit.g, litB.g, xfade);
        lit.b = lerp(lit.b, litB.b, xfade);
      }
      const ov = overrides[i];
      if (ov) {
        if (ov.mode === "off") {
          lit.r = lit.g = lit.b = 0;
        } else if (ov.rgb) {
          const m = ctrl.brightness;
          lit.r = ov.rgb[0] * m;
          lit.g = ov.rgb[1] * m;
          lit.b = ov.rgb[2] * m;
        }
      }
      // EQ→zone gain × master × strobe
      const g = mg * eqGain(f.zone, ctrl.eqLow, ctrl.eqMid, ctrl.eqHigh, audio);
      lit.r *= g; lit.g *= g; lit.b *= g;

      // presence→ripple: brighten as a wavefront passes
      if (ripples.length) {
        let boost = 0;
        for (const rp of ripples) {
          const dx = f.pos[0] - rp.x, dy = f.pos[1] - rp.y, dz = f.pos[2] - rp.z;
          const b = rippleIntensity(Math.sqrt(dx * dx + dy * dy + dz * dz), nowS - rp.t0, rSpeed, rWidth);
          if (b > boost) boost = b;
        }
        if (boost > 0) { const m = 1 + 2.2 * boost; lit.r *= m; lit.g *= m; lit.b *= m; }
      }

      // BEACON safety preempt — force full white over everything (whiteout safety)
      if (ctrl.beaconPreempt) { const w = ctrl.master; lit.r = w; lit.g = w; lit.b = w; }
      // BLACKOUT preempt — force all-off (wins over beacon; instant dark)
      if (ctrl.blackout) { lit.r = lit.g = lit.b = 0; }

      const isDead = view.mock && f.seq < view.deadCount;
      if (!view.mock) {
        repR.current[i] = lit.r; repG.current[i] = lit.g; repB.current[i] = lit.b;
        lastReport.current[i] = t;
      } else if (isDead) {
        dead++;
      } else {
        const interval = 0.6 + f.rnd * 0.6;
        if (t - lastReport.current[i] >= interval) {
          repR.current[i] = lit.r; repG.current[i] = lit.g; repB.current[i] = lit.b;
          lastReport.current[i] = t;
        }
        if (t - lastReport.current[i] > 1.3) stale++;
      }

      // data-log snapshot: capture each light's REPORTED output (what really ships)
      if (writeTele && teleBuf.current.length === n) {
        const tb = teleBuf.current[i];
        tb.num = f.num; tb.id = f.id;
        const rr = isDead ? 0 : repR.current[i], gg = isDead ? 0 : repG.current[i], bb = isDead ? 0 : repB.current[i];
        tb.bri = Math.max(rr, gg, bb);
        tb.rgb[0] = rr; tb.rgb[1] = gg; tb.rgb[2] = bb;
      }

      if (view.monitor && isDead) {
        lm.setColorAt(i, col.setRGB(0.5, 0, 0));
        if (bm) bm.setColorAt(i, col.setRGB(0, 0, 0));
      } else {
        // ease displayed colour toward the reported value (smooth all transitions)
        dispR.current[i] += (repR.current[i] - dispR.current[i]) * k;
        dispG.current[i] += (repG.current[i] - dispG.current[i]) * k;
        dispB.current[i] += (repB.current[i] - dispB.current[i]) * k;
        const r = dispR.current[i], g = dispG.current[i], b = dispB.current[i];
        sumR += r; sumG += g; sumB += b;
        lm.setColorAt(i, col.setRGB(r * GAIN, g * GAIN, b * GAIN));
        if (bm) {
          // IES-ish: scale beam by the fixture's photometric intensity (lumens/beam angle)
          const bs = 0.55 * Math.min(2.5, Math.max(0.3, relativeBeam(f.lumens, f.beamDeg)));
          bm.setColorAt(i, col.setRGB(r * bs, g * bs, b * bs));
        }
        // the petal cookie on the floor, tinted by this fixture's colour
        if (gm) gm.setColorAt(i, col.setRGB(r * GAIN * 1.4, g * GAIN * 1.4, b * GAIN * 1.4));
      }
    }
    if (lm.instanceColor) lm.instanceColor.needsUpdate = true;
    if (bm?.instanceColor) bm.instanceColor.needsUpdate = true;
    if (gm?.instanceColor) gm.instanceColor.needsUpdate = true;

    // feed the live aggregate colour to the ground projection (real coloured
    // shapes on the floor): normalized hue + average luminance for intensity
    const inv = 1 / n;
    const ar = sumR * inv, ag = sumG * inv, ab = sumB * inv;
    const level = Math.min(1, (ar + ag + ab) / 3 * GAIN * 1.6);
    const mx = Math.max(ar, ag, ab, 1e-4);
    easeGroundTint(ar / mx, ag / mx, ab / mx, level, k);

    if (writeTele) { lastTele.current = t; telemetry.states = teleBuf.current; telemetry.t = t; }

    if (t - statsAt.current > 0.5) {
      statsAt.current = t;
      st.setMonitorStats({ reporting: n - dead - stale, dead, stale });
    }
  });

  if (fixtures.length === 0) return null;
  return (
    <group>
      {/* per-fixture petal-gobo cookies on the floor — the real "light shades" */}
      <instancedMesh ref={groundRef} args={[undefined as never, undefined as never, fixtures.length]} key={`grd${fixtures.length}`}>
        <planeGeometry args={[1, 1]} />
        <meshBasicMaterial map={gobo} transparent blending={AdditiveBlending} depthWrite={false} toneMapped={false} side={DoubleSide} opacity={0.95} />
      </instancedMesh>
      <instancedMesh
        ref={beamRef}
        args={[undefined as never, undefined as never, fixtures.length]}
        key={`beam${fixtures.length}`}
        visible={showBeams}
      >
        <primitive object={beamGeom} attach="geometry" />
        <meshBasicMaterial vertexColors transparent blending={AdditiveBlending} opacity={0.09} depthWrite={false} toneMapped={false} />
      </instancedMesh>
      {/* the REAL downlight lantern bodies, glowing with the live pattern colour
          (the LED source is hidden inside the tube — we see the lit housing, not
          a bare dot). wire mode → wireframe of the same body. */}
      <instancedMesh ref={lightRef} args={[undefined as never, undefined as never, fixtures.length]} key={`led${fixtures.length}`}>
        <primitive object={lanternGeom} attach="geometry" />
        <meshBasicMaterial toneMapped={false} wireframe={viz === "wire"} />
      </instancedMesh>
    </group>
  );
}
