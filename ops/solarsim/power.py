"""v5 power chain -- port of the solar-visualizer formulas (Elliot, calibrated
on Ben's bench: ADR 0026 panel-side INA + the July field-cycle ledger).

Per panel, per 10-min slot:
  DNI  = 1361 * 0.7^(AM^0.678) * 1.012          (Meinel; Kasten-Young air mass;
                                                 +1.2% for 1.19 km elevation)
  DHI  = 0.10 * DNI * sin(alt)
  GHI  = DNI * sin(alt) + DHI
  beam = DNI * max(0, n . s) * lit^2 * (0.75 if 0 < lit < 1 else 1)
  diff = DHI * svf * (1 + cos tilt)/2
  refl = GHI * 0.30 * (1 - cos tilt)/2
  POA  = beam + diff + refl                      [W/m^2]
  P_dc = POA/1000 * 5.0 * 0.77                   (= 3.85 W measured full sun)
         * min(1, (1 - 0.004*(Tcell-25)) / 0.93) * 0.95
  Tcell = ambient(t) + 0.025 * POA               (ambient 17->35 degC curve)
  wh_day_batt = sum(P_dc * dt_h) * 0.63          (field-cycle chain into cell)
  runtime_full_h = wh_day_batt / 1.364           (measured full-RGBW draw)
"""

import numpy as np

NAMEPLATE_W = 5.0
TOL_CTRL = 0.77
DUST = 0.95
HEAT_REF = 0.93
CHAIN_TO_CELL = 0.63
LED_FULL_W = 1.364
ALBEDO = 0.30
SLOT_H = 10.0 / 60.0


def clear_sky(suns: np.ndarray):
    """(DNI, DHI, GHI, alt) per slot from the sun unit vectors."""
    alt = np.arcsin(np.clip(suns[:, 2], -1, 1))
    up = suns[:, 2] > 0.01
    alt_safe = np.where(up, alt, np.pi / 2)
    am = np.where(up, 1.0 / (np.sin(alt_safe)
                             + 0.50572 * (np.degrees(alt_safe) + 6.07995) ** -1.6364), 40.0)
    dni = np.where(up, 1361.0 * 0.7 ** (am ** 0.678) * 1.012, 0.0)
    dhi = 0.10 * dni * np.maximum(np.sin(alt), 0)
    ghi = dni * np.maximum(np.sin(alt), 0) + dhi
    return dni, dhi, ghi, alt


def ambient_curve(n_slots: int, slot0_min: int = 360, step_min: int = 10):
    """17->35 degC playa day: min at 06:00, max at 15:00 (sinusoid)."""
    tmin = (slot0_min + np.arange(n_slots) * step_min) / 60.0
    return 26.0 + 9.0 * np.sin((tmin - 9.0) / 12.0 * np.pi)


def panel_power(lit: np.ndarray, normals: np.ndarray, svf: np.ndarray,
                suns: np.ndarray):
    """Returns (w, wh_day_batt, runtime_full_h); w is (n_panels, n_slots)."""
    dni, dhi, ghi, _ = clear_sky(suns)
    amb = ambient_curve(lit.shape[1])
    cos_inc = np.clip(normals @ suns.T, 0, None)         # (p, t)
    tilt_cos = np.clip(normals[:, 2], -1, 1)
    mismatch = np.where((lit > 0) & (lit < 1), 0.75, 1.0)
    beam = dni[None, :] * cos_inc * lit ** 2 * mismatch
    diff = dhi[None, :] * svf[:, None] * (1 + tilt_cos[:, None]) / 2
    refl = ghi[None, :] * ALBEDO * (1 - tilt_cos[:, None]) / 2
    poa = beam + diff + refl
    tcell = amb[None, :] + 0.025 * poa
    heat = np.minimum(1.0, (1 - 0.004 * (tcell - 25.0)) / HEAT_REF)
    w = poa / 1000.0 * NAMEPLATE_W * TOL_CTRL * heat * DUST
    wh_day_batt = w.sum(axis=1) * SLOT_H * CHAIN_TO_CELL
    return w, wh_day_batt, wh_day_batt / LED_FULL_W
