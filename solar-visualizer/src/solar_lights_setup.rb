# Solar light placement for the Resonance Tree solar-access study.
# Run via sketchup-mcp2 eval_ruby against a model containing the BuildSite
# component (Resonance_BuildSite.skp / BuildSite_light). Places 20 movable
# SOLAR_LIGHT component instances on tag SOLAR_LIGHTS:
#   - 12 trunk-base (TB-01..TB-12) at pole-pair azimuths 15° + 30°k, r=1.35m
#   - 4 root lights (RT-SE/NNE/NW/SSW) on the four root arms
#   - 4 door lights (DR-E-1/2, DR-W-1/2) atop the two portal pillars (±7.3, 0)
# Every instance z-snaps to geometry via raytest so panels sit ON the weave.
# Positions derive from tree_split.blend part centroids scaled 0.0985 m/unit
# (calibrated: blend height 67.84u ↔ BuildSite tree 6.66m).
#
# Sun setup gotcha (calibrated 2026-07-16): ShadowInfo applies TZOffset to the
# WALL-CLOCK fields of the Time you pass — pass local BRC time via Time.utc()
# or the sun lands 7h off. Verified: Time.utc(2026,8,30,12) -> SunDirection
# z=0.825 (alt 55.6°, SE) = correct for noon PDT at 40.786N.

module ResonanceSolar
  IN_M = 39.3701
  SCALE = 0.0985 # tree_split.blend units -> meters in BuildSite_light

  def self.setup_shadows(m)
    si = m.shadow_info
    si["Latitude"] = 40.7864
    si["Longitude"] = -119.2065
    si["TZOffset"] = -7.0
    si["NorthAngle"] = 0.0
    si["ShadowTime"] = Time.utc(2026, 8, 30, 12, 0, 0) # = noon PDT, see header
    si["DisplayShadows"] = true
    si["UseSunForAllShading"] = true
  end

  def self.fixture_definition(m)
    cd = m.definitions.add("SOLAR_LIGHT")
    e = cd.entities
    ym = m.materials.add("PanelYellow"); ym.color = Sketchup::Color.new(255, 210, 0)
    rm = m.materials.add("ArrowRed");    rm.color = Sketchup::Color.new(220, 30, 30)
    gm = m.materials.add("PostGray");    gm.color = Sketchup::Color.new(70, 70, 70)
    p = 0.06 * IN_M; ph = 0.25 * IN_M
    post = e.add_face([-p, -p, 0], [p, -p, 0], [p, p, 0], [-p, p, 0])
    post.reverse! if post.normal.z > 0
    post.pushpull(ph)
    e.grep(Sketchup::Face).each { |f| f.material = gm; f.back_material = gm }
    w = 0.25 * IN_M; h = 0.175 * IN_M; t = 0.05 * IN_M
    pf = e.add_face([-w, -h, ph], [w, -h, ph], [w, h, ph], [-w, h, ph])
    pf.reverse! if pf.normal.z > 0
    pf.pushpull(t)
    top_z = ph + t
    e.grep(Sketchup::Face).each { |f| next if f.material == gm; f.material = ym; f.back_material = ym }
    af = e.add_face([0.05 * IN_M, 0, top_z + 0.2], [-0.1 * IN_M, 0.08 * IN_M, top_z + 0.2],
                    [-0.1 * IN_M, -0.08 * IN_M, top_z + 0.2])
    af.material = rm; af.back_material = rm
    cd
  end

  def self.place_all(m)
    m.start_operation("solar lights", true)
    setup_shadows(m)
    lay = m.layers.add("SOLAR_LIGHTS")
    cd = fixture_definition(m)
    down = Geom::Vector3d.new(0, 0, -1)
    placed = []
    place = lambda do |name, xm, ym2, z_start, z_fallback|
      hit = m.raytest([Geom::Point3d.new(xm * IN_M, ym2 * IN_M, z_start * IN_M), down], false)
      z = hit ? hit[0].z / IN_M : z_fallback
      tr = Geom::Transformation.new(Geom::Point3d.new(xm * IN_M, ym2 * IN_M, z * IN_M))
      inst = m.entities.add_instance(cd, tr)
      inst.name = name
      inst.layer = lay
      placed << { name: name, x: xm.round(2), y: ym2.round(2), z: z.round(2) }
    end
    12.times do |k|
      ang = (15 + 30 * k) * Math::PI / 180
      place.call("TB-%02d (az %d)" % [k + 1, 15 + 30 * k], 1.35 * Math.cos(ang), 1.35 * Math.sin(ang), 2.5, 0.35)
    end
    { "RT-SE" => [30.0, -32.6], "RT-NNE" => [13.3, 42.3],
      "RT-NW" => [-29.7, 33.0], "RT-SSW" => [-13.3, -42.3] }.each do |nm, (bx, by)|
      place.call(nm, bx * SCALE, by * SCALE, 3.0, 0.3)
    end
    [["DR-E-1", 7.32, 0.4], ["DR-E-2", 7.32, -0.4],
     ["DR-W-1", -7.32, 0.4], ["DR-W-2", -7.32, -0.4]].each do |nm, x, y|
      place.call(nm, x, y, 4.0, 3.05)
    end
    ref = m.layers.add("SOLAR_REF")
    { "N" => [0, 12], "S" => [0, -12], "E" => [12, 0], "W" => [-12, 0] }.each do |txt, (x, y)|
      g = m.entities.add_group
      g.entities.add_3d_text(txt, TextAlignLeft, "Arial", true, false, 1.2 * IN_M, 0, 0, true, 0.05 * IN_M)
      g.transformation = Geom::Transformation.new(Geom::Point3d.new(x * IN_M, y * IN_M, 0.02 * IN_M))
      g.layer = ref; g.name = "compass_#{txt}"
    end
    m.commit_operation
    placed
  end
end

# ResonanceSolar.place_all(Sketchup.active_model)
