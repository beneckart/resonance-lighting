import numpy as np

from locate.metrics import assignment_accuracy, position_error_stats, verdict_triple
from locate.model import CadFixture, CadModel


def _cad_with_dups():
    p = np.zeros(3)
    fixtures = [
        CadFixture("F000", "downlight", p),
        CadFixture("F001", "downlight", p),        # duplicate of F000
        CadFixture("F002", "downlight", np.ones(3)),
        CadFixture("F003", "perimeter", 2 * np.ones(3)),
    ]
    return CadModel(fixtures=fixtures, duplicate_groups=[frozenset({"F000", "F001"})])


def test_duplicate_group_swap_counts_correct():
    cad = _cad_with_dups()
    truth = ["F000", "F001", "F002", "F003"]
    swapped = ["F001", "F000", "F002", "F003"]
    roles = ["downlight", "downlight", "downlight", "perimeter"]
    acc = assignment_accuracy(swapped, truth, roles, cad)
    assert acc["overall"] == 1.0

    wrong = ["F002", "F001", "F000", "F003"]
    acc2 = assignment_accuracy(wrong, truth, roles, cad)
    assert acc2["overall"] == 0.5
    assert acc2["wrong_idx"] == [0, 2]


def test_verdict_triple_partitions():
    cad = _cad_with_dups()
    truth = ["F000", "F001", "F002", "F003"]
    assign = ["F000", "F001", "F003", "F002"]   # 2 right, 2 wrong
    flagged = np.array([False, True, True, False])
    v = verdict_triple(assign, truth, flagged, cad)
    assert v["auto_correct"] == 0.25   # only idx 0
    assert v["flagged"] == 0.5
    assert v["silent_wrong"] == 0.25   # idx 3
    assert abs(v["auto_correct"] + v["flagged"] + v["silent_wrong"] - 1.0) < 1e-12


def test_position_error_stats_shapes():
    est = np.zeros((4, 3))
    truth = np.array([[1.0, 0, 0], [0, 2.0, 0], [0, 0, 3.0], [0, 0, 0]])
    s = position_error_stats(est, truth, ["a", "a", "b", "b"])
    assert s["overall"]["n"] == 4
    assert abs(s["per_role"]["b"]["median_m"] - 1.5) < 1e-12
