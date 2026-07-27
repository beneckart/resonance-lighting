"""ESP-NOW RF channel model: parameter-controlled noise a playa deployment might see.

Directed per-packet model (all dB):

    RSSI(i->j, pkt) = P0 - 10 n log10(d_ij)
                      + b_pair(ij) + b_dir(i->j)         static multipath bias
                      - fade_pair(ij)                    occasional deep fade
                      + t_i + r_j                        per-device TX/RX offsets
                      - panel_i(az out) - panel_j(az in) directional solar-panel shadow
                      - trunk(ij)                        bamboo trunk occlusion
                      + two_ray(ij)                      optional ground-reflection term
                      + eps(pkt)                         per-packet fading
    -> quantize to 1 dB -> PDR(rssi) reception lottery -> sample kept or lost

Calibration to the 2026-06-07 5-node bench report: TX ~+19 dBm full power, floor
~-90 dBm; short-window per-board spread 2-8 dB (sigma_pkt); same-placement shifts
8-17 dB across minutes indoors (sigma_link upper band; open playa estimated 2-6);
solar panel over the antenna ~20 dB and directional (panel_depth_db); RSSI is
path-asymmetric (sigma_asym_db); doorway/oak-class dips (p_fade deep-fade mixture).
"""

from dataclasses import dataclass
from typing import Dict, List, Tuple

import numpy as np

C_LIGHT = 299792458.0
FREQ_HZ = 2.412e9          # ESP-NOW channel 1
LAMBDA_M = C_LIGHT / FREQ_HZ


@dataclass
class RfParams:
    p0_dbm: float = -40.0          # RSSI at 1 m
    n: float = 2.7                 # path-loss exponent (true)
    sigma_pkt_db: float = 2.0      # per-packet fading
    sigma_link_db: float = 4.0     # static per-link multipath bias (the killer knob)
    sigma_asym_db: float = 1.5     # directional component of the per-link bias
    p_fade: float = 0.05           # probability of a deep-fade link
    fade_lo_db: float = 10.0
    fade_hi_db: float = 25.0
    sigma_dev_db: float = 3.0      # per-device TX/RX hardware offsets
    panel_depth_db: float = 20.0   # max directional solar-panel attenuation
    panel_mode: str = "spin"       # "spin" (orientation varies over the campaign),
                                   # "frozen" (worst case), "off"
    quant_db: float = 1.0
    floor_dbm: float = -90.0       # receiver sensitivity floor
    floor_width_db: float = 2.0    # PDR logistic width around the floor
    k_packets: int = 50            # packets per directed link over the campaign
    trunk_on: bool = True
    trunk_radius_m: float = 0.3    # bamboo trunk cylinder
    trunk_loss_db: float = 10.0
    two_ray: bool = False          # deterministic ground-reflection stressor
    two_ray_gamma: float = -0.7    # ground reflection coefficient


def _panel_atten(depth_db: float, daz: np.ndarray) -> np.ndarray:
    """Raised-cosine azimuthal shadow: 0 dB at the clear side, depth_db at the
    panel side. daz = azimuth of the link minus the device's panel azimuth."""
    return depth_db * 0.5 * (1.0 + np.cos(daz))


def _trunk_hit(pi: np.ndarray, pj: np.ndarray, radius: float) -> bool:
    """Does the xy projection of segment i-j pass within radius of the origin?"""
    a, b = pi[:2], pj[:2]
    ab = b - a
    denom = float(ab @ ab)
    t = 0.0 if denom < 1e-12 else float(np.clip(-(a @ ab) / denom, 0.0, 1.0))
    closest = a + t * ab
    return bool(np.linalg.norm(closest) < radius)


def _two_ray_db(pi: np.ndarray, pj: np.ndarray, gamma: float) -> float:
    d = float(np.linalg.norm(pi - pj))
    if d < 1e-6:
        return 0.0
    # path difference between direct ray and ground bounce
    d_dir = float(np.hypot(np.linalg.norm(pi[:2] - pj[:2]), pi[2] - pj[2]))
    d_ref = float(np.hypot(np.linalg.norm(pi[:2] - pj[:2]), pi[2] + pj[2]))
    phi = 2 * np.pi * (d_ref - d_dir) / LAMBDA_M
    gain = abs(1.0 + gamma * np.exp(1j * phi))
    return float(20 * np.log10(max(gain, 1e-3)))


def simulate_rssi(
    truth_pos: np.ndarray, params: RfParams, seed: int = 0
) -> Dict[Tuple[int, int], List[float]]:
    """Returns directed[(tx, rx)] = list of received per-packet RSSI samples."""
    rng = np.random.default_rng(seed)
    n = len(truth_pos)
    p = params

    t_off = rng.normal(0, p.sigma_dev_db, n)
    r_off = rng.normal(0, p.sigma_dev_db, n)
    panel_az = rng.uniform(0, 2 * np.pi, n)

    directed: Dict[Tuple[int, int], List[float]] = {}
    for i in range(n):
        for j in range(i + 1, n):
            d = float(np.linalg.norm(truth_pos[i] - truth_pos[j]))
            base = p.p0_dbm - 10 * p.n * np.log10(max(d, 1e-3))

            b_pair = rng.normal(0, p.sigma_link_db)
            if rng.random() < p.p_fade:
                b_pair -= rng.uniform(p.fade_lo_db, p.fade_hi_db)
            b_dir = rng.normal(0, p.sigma_asym_db, 2)      # i->j, j->i

            if p.trunk_on and _trunk_hit(truth_pos[i], truth_pos[j], p.trunk_radius_m):
                base -= p.trunk_loss_db
            if p.two_ray:
                base += _two_ray_db(truth_pos[i], truth_pos[j], p.two_ray_gamma)

            az_ij = float(np.arctan2(truth_pos[j][1] - truth_pos[i][1],
                                     truth_pos[j][0] - truth_pos[i][0]))
            az_ji = az_ij + np.pi

            for (tx, rx, bd, az_out, az_in, to, ro) in (
                (i, j, b_dir[0], az_ij, az_ji, t_off[i], r_off[j]),
                (j, i, b_dir[1], az_ji, az_ij, t_off[j], r_off[i]),
            ):
                if p.panel_mode == "frozen":
                    pan = (_panel_atten(p.panel_depth_db, np.array([az_out - panel_az[tx]]))
                           + _panel_atten(p.panel_depth_db, np.array([az_in - panel_az[rx]])))
                    pan = np.full(p.k_packets, float(pan))
                elif p.panel_mode == "spin":
                    pan = (_panel_atten(p.panel_depth_db,
                                        rng.uniform(0, 2 * np.pi, p.k_packets))
                           + _panel_atten(p.panel_depth_db,
                                          rng.uniform(0, 2 * np.pi, p.k_packets)))
                else:
                    pan = np.zeros(p.k_packets)

                rssi = (base + b_pair + bd + to + ro - pan
                        + rng.normal(0, p.sigma_pkt_db, p.k_packets))
                if p.quant_db > 0:
                    rssi = np.round(rssi / p.quant_db) * p.quant_db
                pdr = 1.0 / (1.0 + np.exp(-(rssi - p.floor_dbm) / p.floor_width_db))
                kept = rssi[rng.random(p.k_packets) < pdr]
                if len(kept):
                    directed[(tx, rx)] = [float(v) for v in kept]
    return directed
