"""Shared test helpers."""

import numpy as np


def procrustes_residual(X, Y, allow_scale=True):
    """RMS residual after optimally aligning X onto Y (rotation+reflection+
    translation, optional scale) -- Kabsch/Umeyama. Used to test embeddings
    up to their inherent gauge freedom."""
    Xc = X - X.mean(axis=0)
    Yc = Y - Y.mean(axis=0)
    U, S, Vt = np.linalg.svd(Xc.T @ Yc)
    R = U @ Vt
    s = S.sum() / (Xc ** 2).sum() if allow_scale else 1.0
    resid = s * Xc @ R - Yc
    return float(np.sqrt(np.mean(np.sum(resid ** 2, axis=1))))


def random_config(n, seed, spread=5.0):
    rng = np.random.default_rng(seed)
    return rng.uniform(-spread, spread, size=(n, 3))
