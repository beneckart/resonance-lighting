# Phase-2 CANOPY placement, corrected -- companion to solar_lights_setup.rb.
# From Ben + Claude (lighting workstream), 2026-07-17. See PLACEMENT_FIX.md.
#
# Replaces the canopy lantern set with the corrected 72-distinct-position
# canopy (rings 24/24/24): the raw export's 6 trunk strays are moved into the
# 6 ring holes and the 6 stacked duplicates are dropped. 66 lights keep their
# existing CL ids (position-matched within 2 cm); the 6 hole fills get fresh
# indices (CL-I29..I32, CL-M27..M28) so before/after datasets diff cleanly.
# The 12 ids that vanish are the artifacts: CL-I05/I09/I11/I12/I14/I16/I17/
# I18/I19 + CL-O05/O13/O14.
#
# Run via eval_ruby (or paste into the Ruby console) against the study model
# AFTER solar_lights_setup.rb has defined SOLAR_LIGHT (or standalone -- it
# reuses the definition if present). It DELETES all existing CL-* instances
# first, then places the 72 upright at the attach points (panels face-up,
# matching the phase-2 convention: all shipped canopy normals are [0,0,1]).
# Then re-run solar_access_analysis.rb to regenerate the dataset.

module ResonanceCanopyFix
  IN_M = 39.3701   # meters -> SketchUp native inches

  PLACEMENTS = [   # [id, x_m, y_m, z_m]  tree-centered, Z-up, 0.0985 m/unit frame
    ["CL-I01 006°N", 0.296, 2.612, 3.464],
    ["CL-I02 022°NNE", 0.97, 2.404, 3.401],
    ["CL-I03 037°NE", 1.577, 2.06, 3.388],
    ["CL-I04 052°NE", 2.118, 1.646, 3.437],
    ["CL-I06 067°ENE", 2.423, 1.01, 3.43],
    ["CL-I07 082°E", 2.611, 0.368, 3.406],
    ["CL-I08 097°E", 2.624, -0.318, 3.4],
    ["CL-I10 156°SSE", 0.988, -2.25, 3.452],
    ["CL-I13 172°S", 0.362, -2.591, 3.406],
    ["CL-I15 186°S", -0.294, -2.584, 3.466],
    ["CL-I20 202°SSW", -0.961, -2.42, 3.421],
    ["CL-I21 217°SW", -1.564, -2.078, 3.467],
    ["CL-I22 232°SW", -2.04, -1.613, 3.419],
    ["CL-I23 247°WSW", -2.375, -0.987, 3.463],
    ["CL-I24 261°W", -2.472, -0.372, 3.382],
    ["CL-I25 276°W", -2.426, 0.271, 3.427],
    ["CL-I26 306°NW", -2.075, 1.534, 3.46],
    ["CL-I27 322°NW", -1.636, 2.08, 3.47],
    ["CL-I28 351°N", -0.391, 2.61, 3.452],
    ["CL-I29 337°NNW", -1.032, 2.389, 3.461],  # NEW (ring-hole fill)
    ["CL-I30 141°SE", 1.622, -2.035, 3.439],  # NEW (ring-hole fill)
    ["CL-I31 127°SE", 2.09, -1.552, 3.426],  # NEW (ring-hole fill)
    ["CL-I32 112°ESE", 2.417, -0.964, 3.413],  # NEW (ring-hole fill)
    ["CL-M01 007°N", 0.528, 4.086, 3.928],
    ["CL-M02 023°NNE", 1.628, 3.779, 3.916],
    ["CL-M03 037°NE", 2.484, 3.265, 3.841],
    ["CL-M04 053°NE", 3.264, 2.458, 3.843],
    ["CL-M05 067°ENE", 3.808, 1.597, 3.896],
    ["CL-M06 083°E", 4.082, 0.537, 3.864],
    ["CL-M07 097°E", 4.073, -0.517, 3.888],
    ["CL-M08 113°ESE", 3.792, -1.602, 3.732],
    ["CL-M09 127°SE", 3.279, -2.506, 3.843],
    ["CL-M10 142°SE", 2.509, -3.243, 3.881],
    ["CL-M11 157°SSE", 1.627, -3.772, 3.905],
    ["CL-M12 173°S", 0.523, -4.065, 3.891],
    ["CL-M13 187°S", -0.602, -4.917, 2.917],
    ["CL-M14 203°SSW", -1.593, -3.842, 3.969],
    ["CL-M15 217°SW", -2.466, -3.258, 3.978],
    ["CL-M16 232°SW", -3.906, -3.049, 2.924],
    ["CL-M17 233°SW", -3.261, -2.454, 3.89],
    ["CL-M18 247°WSW", -3.799, -1.596, 3.893],
    ["CL-M19 263°W", -4.069, -0.516, 3.792],
    ["CL-M20 277°W", -4.107, 0.524, 3.913],
    ["CL-M21 292°WNW", -2.49, 1.012, 3.406],
    ["CL-M22 292°WNW", -4.602, 1.823, 2.881],
    ["CL-M23 293°WNW", -3.793, 1.628, 3.957],
    ["CL-M24 307°NW", -3.268, 2.482, 3.939],
    ["CL-M25 323°NW", -2.489, 3.29, 3.898],
    ["CL-M26 352°N", -0.552, 4.115, 3.99],
    ["CL-M27 338°NNW", -1.566, 3.806, 3.944],  # NEW (ring-hole fill)
    ["CL-M28 188°S", -0.544, -4.079, 3.93],  # NEW (ring-hole fill)
    ["CL-O01 007°N", 0.614, 4.953, 2.929],
    ["CL-O02 022°NNE", 1.842, 4.629, 2.854],
    ["CL-O03 037°NE", 2.981, 3.97, 2.778],
    ["CL-O04 052°NE", 3.947, 3.052, 2.843],
    ["CL-O06 067°ENE", 4.572, 1.935, 2.875],
    ["CL-O07 082°E", 4.926, 0.691, 2.814],
    ["CL-O08 097°E", 4.938, -0.595, 2.846],
    ["CL-O09 112°ESE", 4.603, -1.884, 2.911],
    ["CL-O10 127°SE", 3.974, -2.988, 2.957],
    ["CL-O11 142°SE", 3.067, -3.915, 2.956],
    ["CL-O12 157°SSE", 1.931, -4.597, 2.918],
    ["CL-O15 172°S", 0.702, -4.934, 2.814],
    ["CL-O16 202°SSW", -1.856, -4.616, 2.905],
    ["CL-O17 217°SW", -2.985, -3.991, 2.948],
    ["CL-O18 247°WSW", -4.631, -1.961, 2.92],
    ["CL-O19 262°W", -4.911, -0.712, 2.918],
    ["CL-O20 277°W", -4.958, 0.594, 2.915],
    ["CL-O21 307°NW", -3.97, 2.979, 2.824],
    ["CL-O22 322°NW", -3.072, 3.928, 3.003],
    ["CL-O23 337°NNW", -1.95, 4.598, 2.927],
    ["CL-O24 352°N", -0.703, 4.941, 2.862],
  ].freeze

  def self.fixture_definition(m)
    cd = m.definitions["SOLAR_LIGHT"]
    return cd if cd
    # minimal stand-in matching solar_lights_setup.rb geometry (panel top at
    # local z = 4.3in -- the analysis sample point)
    cd = m.definitions.add("SOLAR_LIGHT")
    e = cd.entities
    ym = m.materials["PanelYellow"] || m.materials.add("PanelYellow")
    ym.color = Sketchup::Color.new(255, 210, 0)
    p = 0.06 * IN_M; ph = 0.25 * IN_M
    w = 0.25 * IN_M; h = 0.175 * IN_M; t = 0.05 * IN_M
    pf = e.add_face([-w, -h, ph], [w, -h, ph], [w, h, ph], [-w, h, ph])
    pf.reverse! if pf.normal.z > 0
    pf.pushpull(t)
    e.grep(Sketchup::Face).each { |f| f.material = ym; f.back_material = ym }
    cd
  end

  def self.place_all(m = Sketchup.active_model)
    m.start_operation("corrected canopy", true)
    lay = m.layers.add("SOLAR_LIGHTS")
    cd = fixture_definition(m)
    removed = 0
    m.entities.grep(Sketchup::ComponentInstance).each do |ci|
      next unless ci.name.start_with?("CL-")
      ci.erase!
      removed += 1
    end
    PLACEMENTS.each do |id, x, y, z|
      pt = Geom::Point3d.new(x * IN_M, y * IN_M, z * IN_M)
      inst = m.entities.add_instance(cd, Geom::Transformation.translation(pt))
      inst.name = id
      inst.layer = lay
    end
    m.commit_operation
    puts "removed #{removed} old CL-* instances, placed #{PLACEMENTS.length} corrected canopy lights"
  end
end

ResonanceCanopyFix.place_all
