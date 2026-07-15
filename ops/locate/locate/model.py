"""Data model for the localization pipeline -- the sim/real abstraction contract.

The solver's entire input surface is:
    (list[Device], list[LinkObs], list[ZAnchor], CadModel, PathLossParams prior, config)
The simulator (ops/locate/sim/) and the real-data ingester (locate_ingest.py) both
produce exactly these types; nothing else crosses the boundary.
"""

from dataclasses import dataclass, field
from typing import Optional

import numpy as np

ROLES = ("downlight", "perimeter", "uplight", "chandelier")


@dataclass(frozen=True)
class Device:
    """One physical fleet device. Role is known from its hardware complement
    (downlight = TMF8820 down, perimeter = VL53L5CX out, ...)."""
    dev_id: str          # MAC-suffix style ("9F2690") or sim id ("S017")
    role: str            # one of ROLES


@dataclass
class LinkObs:
    """One SYMMETRIZED, aggregated RSSI link between devices i and j (i < j)."""
    i: int                                   # device index
    j: int                                   # device index, i < j
    rssi_dbm: float                          # mean of the per-direction medians
    n_samples: int                           # total packets behind the aggregate
    asym_db: Optional[float] = None          # |med_ij - med_ji|; None if one-directional
    samples: Optional[list] = None           # optional raw dB retention for bootstrap
    censored: bool = False                   # aggregated near the receiver floor:
                                             # survivor-biased, treat as a one-sided
                                             # "at least this far" constraint


@dataclass
class ZAnchor:
    """Metric height-above-ground measurement for one device (ToF-derived)."""
    idx: int             # device index
    z_m: float           # measured height above ground plane, meters
    sigma_m: float       # 1-sigma uncertainty (0.010 downlight TMF8820;
                         # plane-fit-propagated for perimeter VL53L5CX)


@dataclass
class PathLossParams:
    """Log-distance path loss: RSSI(d) = p0_dbm - 10 n log10(d / d0_m)."""
    p0_dbm: float = -40.0    # RSSI at d0
    n: float = 2.7           # path-loss exponent
    d0_m: float = 1.0


@dataclass
class CadFixture:
    fixture_id: str
    role: str
    pos_m: np.ndarray            # (3,) meters, scaled CAD frame, Z-up
    synthetic: bool = False      # True for the procedurally added perimeter ring


@dataclass
class CadModel:
    fixtures: list                          # list[CadFixture]
    duplicate_groups: list = field(default_factory=list)  # list[frozenset[str]]:
                                            # fixture_ids at (near-)identical positions;
                                            # within-group assignment swaps score as correct
    scale_m_per_unit: float = 1.0
    source: str = ""                        # provenance note

    def positions(self, role: Optional[str] = None) -> np.ndarray:
        fx = self.fixtures if role is None else [f for f in self.fixtures if f.role == role]
        if not fx:
            return np.zeros((0, 3))
        return np.array([f.pos_m for f in fx])

    def indices_by_role(self) -> dict:
        out = {}
        for k, f in enumerate(self.fixtures):
            out.setdefault(f.role, []).append(k)
        return out


@dataclass
class DeviceReport:
    dev_id: str
    role: str
    degree: int                      # observed links
    fixture_id: Optional[str]        # assigned CAD slot (None = unassigned)
    sigma_pos_m: float               # Hessian covariance proxy, sqrt(mean var)
    margin_cost: float               # LAP margin: extra cost if this edge is forbidden
    bootstrap_agreement: float       # fraction of bootstrap re-solves that agree (nan if not run)
    flagged: bool                    # True -> goes on the manual fix-up list


@dataclass
class SolveResult:
    pos_m: np.ndarray                # (N,3) estimated positions, CAD frame (post-registration)
    assignment: list                 # device idx -> fixture_id (str) or None
    pathloss: PathLossParams         # fitted model
    offsets_db: np.ndarray           # (N,) per-device combined TX/RX offsets
    per_device: list                 # list[DeviceReport]
    diagnostics: dict                # theta-cost curves, anchor-fit residual, iteration counts, ...
