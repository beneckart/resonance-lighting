# Solar-access analysis for the Resonance Tree solar-light study.
# Companion to solar_lights_setup.rb — run via eval_ruby against the model
# with SOLAR_LIGHT instances placed (reads LIVE positions, so re-run freely
# after dragging fixtures). Outputs ~/Downloads/Resonance_Solar_Access.csv.
#
# Method (validated against research + in-model sun verification 2026-07-16):
#  - ShadowInfo geolocated to BRC; ShadowTime passed as Time.utc(<PDT wall clock>)
#    (SketchUp double-applies TZOffset otherwise). Verified: solar noon Sep 2
#    12:57 PDT -> SunDirection (~0,-0.55,0.84), alt 56.9 deg, due south.
#  - SunDirection points TOWARD the sun. Occlusion = model.raytest from the
#    panel point offset 0.15" along the sun ray; nil hit = lit. The woven shell
#    is real strut geometry with real gaps, so raytest self-shading is physical.
#  - Beam: clear-sky DNI = 1361 * 0.7^(AM^0.678) * 1.012 (Meinel + Kasten-Young
#    air mass, +1.2% for 1.19 km elevation), gated by raytest, scaled cos(incidence).
#  - Diffuse: DHI ~= 0.10*DNI*sin(alt), scaled by per-fixture sky-view factor
#    (128 cosine-weighted hemisphere rays) and panel tilt view term.
#  - Reflected: GHI * playa albedo 0.3 * (1-cos tilt)/2.
#  - 15-min steps, 06:00-19:45 PDT, Aug 30 - Sep 8 2026 (10 days), 13 panel
#    orientations: FLAT + tilt {30,60,90} x azimuth {N,E,S,W}.
# Numbers are RELATIVE Wh/m2/day for ranking positions, not bankable yield.

module ResonanceSolarAnalysis
  IN_M = 39.3701
  DATES = [[2026,8,30],[2026,8,31],[2026,9,1],[2026,9,2],[2026,9,3],
           [2026,9,4],[2026,9,5],[2026,9,6],[2026,9,7],[2026,9,8]]
  ORIENTS = [["FLAT",0,0]] +
            [30,60,90].flat_map { |t| {"N"=>0,"E"=>90,"S"=>180,"W"=>270}.map { |c,az| ["#{c}#{t}",t,az] } }
  D2R = Math::PI / 180

  def self.verify_sun!(si)
    si["ShadowTime"] = Time.utc(2026, 9, 2, 12, 57, 0)
    sd = si["SunDirection"]
    ok = sd.z > 0.7 && sd.x.abs < 0.08 && sd.y < 0
    raise "sun convention broken: #{sd.to_a.inspect}" unless ok
  end

  # Fixtures are ORIENTED since v3 (panels follow the bark): sample point =
  # panel top center (local z=4.3in), and each returns its as-mounted normal.
  def self.fixtures(m)
    lay = m.layers["SOLAR_LIGHTS"]
    fx = {}
    m.entities.grep(Sketchup::ComponentInstance).each do |ci|
      next unless ci.layer == lay && ci.definition.name =~ /^SOLAR_LIGHT/
      t = ci.transformation
      nv = t * Geom::Vector3d.new(0, 0, 1)
      nv.normalize!
      fx[ci.name] = { pt: t * Geom::Point3d.new(0, 0, 4.3), n: [nv.x, nv.y, nv.z] }
    end
    fx
  end

  def self.sky_view(m, pt, n = 128)
    ga = Math::PI * (3 - Math.sqrt(5))
    wsum = vsum = 0.0
    n.times do |k|
      z = (k + 0.5) / n
      r = Math.sqrt(1 - z * z)
      dir = Geom::Vector3d.new(r * Math.cos(ga * k), r * Math.sin(ga * k), z)
      wsum += z
      vsum += z if m.raytest([pt.offset(dir, 0.15), dir], true).nil?
    end
    vsum / wsum
  end

  def self.run(m = Sketchup.active_model, out = File.expand_path("~/Downloads/Resonance_Solar_Access.csv"))
    si = m.shadow_info
    si["Latitude"] = 40.7864; si["Longitude"] = -119.2065
    si["TZOffset"] = -7.0; si["NorthAngle"] = 0.0; si["UseSunForAllShading"] = true
    verify_sun!(si)
    fx = fixtures(m)
    svf = fx.transform_values { |f| sky_view(m, f[:pt]) }
    order = fx.keys
    slices = []   # [sx,sy,sz,litmask]
    DATES.each do |y, mo, d|
      (0...56).each do |q|
        s = 6 * 3600 + q * 900
        si["ShadowTime"] = Time.utc(y, mo, d, s / 3600, (s % 3600) / 60, 0)
        sun = si["SunDirection"]
        next if sun.z <= 0
        mask = 0
        order.each_with_index do |nm, i|
          mask |= (1 << i) if m.raytest([fx[nm][:pt].offset(sun, 0.15), sun], true).nil?
        end
        slices << [sun.x, sun.y, sun.z, mask]
      end
    end
    nd = DATES.length.to_f
    rows = {}
    order.each_with_index do |nm, i|
      lit_q = 0
      e = Hash.new(0.0)
      an = fx[nm][:n]
      slices.each do |sx, sy, sz, mask|
        lit = (mask & (1 << i)) != 0
        lit_q += 1 if lit
        alt = Math.asin(sz)
        am = 1.0 / (Math.sin(alt) + 0.50572 * (6.07995 + alt / D2R) ** -1.6364)
        dni = 1361 * 0.7 ** (am ** 0.678) * 1.012
        dhi = 0.10 * dni * Math.sin(alt)
        ghi = dni * Math.sin(alt) + dhi
        cta = an[0] * sx + an[1] * sy + an[2] * sz
        beam_a = lit && cta > 0 ? dni * cta : 0.0
        e["ACTUAL"] += (beam_a + dhi * (1 + an[2]) / 2 * svf[nm] + ghi * 0.3 * (1 - an[2]) / 2) * 0.25
        ORIENTS.each do |onm, t, az|
          tr = t * D2R
          n = t == 0 ? [0, 0, 1.0] : [Math.sin(tr) * Math.sin(az * D2R), Math.sin(tr) * Math.cos(az * D2R), Math.cos(tr)]
          ct = n[0] * sx + n[1] * sy + n[2] * sz
          beam = lit && ct > 0 ? dni * ct : 0.0
          e[onm] += (beam + dhi * (1 + Math.cos(tr)) / 2 * svf[nm] + ghi * 0.3 * (1 - Math.cos(tr)) / 2) * 0.25
        end
      end
      daily = e.transform_values { |v| (v / nd).round }
      rows[nm] = { svf: svf[nm].round(3), lit_h: (lit_q * 0.25 / nd).round(2), daily: daily,
                   actual: daily["ACTUAL"],
                   best: daily.reject { |k, _| k == "ACTUAL" }.max_by { |_, v| v } }
    end
    csv = "fixture,svf,sun_h_per_day,ACTUAL_as_mounted," + ORIENTS.map(&:first).join(",") + ",grid_best\n"
    rows.each { |nm, r| csv << "#{nm},#{r[:svf]},#{r[:lit_h]},#{r[:actual]}," + ORIENTS.map { |o| r[:daily][o[0]] }.join(",") + ",#{r[:best][0]}\n" }
    File.write(out, csv)
    rows
  end
end

# ResonanceSolarAnalysis.run
