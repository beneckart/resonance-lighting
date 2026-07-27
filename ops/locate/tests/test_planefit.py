import numpy as np

from locate.planefit import fit_ground_plane


def _zone_points(height, tilt_deg=0.0, n=8, fov_deg=45.0, downtilt_deg=15.0):
    """Synthesize 8x8 zone hits: device at origin (z up), ground plane below at
    -height (optionally tilted about y), boresight along +x pitched down."""
    half = np.tan(np.deg2rad(fov_deg / 2))
    u = np.linspace(-half, half, n)
    pitch = np.deg2rad(downtilt_deg)
    tilt = np.deg2rad(tilt_deg)
    # ground plane: z = tan(tilt) * x - height
    nrm = np.array([-np.tan(tilt), 0.0, 1.0])
    d0 = -height
    pts = []
    for uy in u:
        for uz in u:
            ray = np.array([1.0, uy, uz])
            ray /= np.linalg.norm(ray)
            # pitch down about y
            c, s = np.cos(pitch), np.sin(pitch)
            ray = np.array([c * ray[0] + s * ray[2], ray[1], -s * ray[0] + c * ray[2]])
            denom = nrm @ ray
            if abs(denom) < 1e-9:
                continue
            t = (d0 - 0.0) / denom  # plane: nrm.p = d0, origin at sensor
            if t > 0:
                pts.append(t * ray)
    return np.array(pts)


def test_flat_ground_exact_height():
    for h in (1.0, 1.52, 2.5):
        pts = _zone_points(h)
        fit = fit_ground_plane(pts)
        assert fit.ok
        assert abs(fit.height_m - h) < 1e-9, (h, fit.height_m)
        assert fit.resid_rms_m < 1e-9


def test_tilted_ground_recovers_perpendicular_height():
    h, tilt = 1.52, 4.0
    pts = _zone_points(h, tilt_deg=tilt)
    fit = fit_ground_plane(pts)
    # perpendicular distance from origin to plane z = tan(t) x - h is
    # h * cos(t) (normalize [-tan t, 0, 1])
    expect = h * np.cos(np.deg2rad(tilt))
    assert abs(fit.height_m - expect) < 1e-9


def test_outlier_zones_rejected():
    pts = _zone_points(1.52)
    pts[3] = [0.2, 0.1, 5.0]   # a zone that hit a passer-by, wildly off-plane
    fit = fit_ground_plane(pts)
    assert fit.n_used < fit.n_total
    assert abs(fit.height_m - 1.52) < 1e-6


def test_too_few_points_not_ok():
    pts = _zone_points(1.52)[:4]
    fit = fit_ground_plane(pts)
    assert not fit.ok
