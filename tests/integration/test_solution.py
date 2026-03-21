"""Tests that verify the optimal solution values for various LP problems."""
import pytest
from conftest import run, make_input, constraint, frac


def var(values: list, index: int) -> int:
    """Return numerator of variableValues[index] (denominator assumed 1)."""
    v = values[index]
    assert v["den"] == 1, f"expected integer value at index {index}, got {v}"
    return v["num"]


def obj_val(sol: dict) -> float:
    return sol["objectiveValue"]["num"] / sol["objectiveValue"]["den"]


# --- BigM method ---

def test_bigm_maximisation(classic_max):
    out = run(classic_max)
    assert out["status"] == "ok"
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 1800
    assert var(sol["variableValues"], 0) == 20  # x1
    assert var(sol["variableValues"], 1) == 60  # x2


def test_bigm_minimisation(classic_min):
    out = run(classic_min)
    assert out["status"] == "ok"
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 4
    assert var(sol["variableValues"], 0) == 4   # x1
    assert var(sol["variableValues"], 1) == 0   # x2


def test_bigm_infeasible(infeasible):
    out = run(infeasible)
    assert out["status"] == "ok"
    assert out["solution"]["status"] == "infeasible"


def test_bigm_single_variable():
    inp = make_input(
        "minimize",
        [3],
        [constraint([1], ">=", 5)],
    )
    out = run(inp)
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 15
    assert var(sol["variableValues"], 0) == 5


def test_bigm_equality_constraint():
    # min x1 + x2  s.t.  x1 + x2 = 10, x1 >= 3  →  x1=3, x2=7, z=10
    inp = make_input(
        "minimize",
        [1, 1],
        [
            constraint([1, 1], "=", 10),
            constraint([1, 0], ">=", 3),
        ],
    )
    out = run(inp)
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 10


def test_bigm_custom_big_m():
    # Same classic_min but with a small bigM — still feasible and correct
    inp = make_input(
        "minimize",
        [1, 2],
        [
            constraint([1, 1], ">=", 4),
            constraint([1, 0], ">=", 2),
        ],
        big_m=500,
    )
    out = run(inp)
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 4


def test_bigm_fractional_coefficients():
    # min (1/2)x1 + (3/4)x2  s.t.  x1+x2>=2, x1>=1  →  x1=2,x2=0,z=1
    inp = make_input(
        "minimize",
        [(1, 2), (3, 4)],
        [
            constraint([1, 1], ">=", 2),
            constraint([1, 0], ">=", 1),
        ],
    )
    out = run(inp)
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 1.0


# --- Artificial (two-phase) method ---

def test_artificial_maximisation(classic_max):
    classic_max["solver"]["basisMethod"] = "artificial"
    out = run(classic_max)
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 1800
    assert var(sol["variableValues"], 0) == 20
    assert var(sol["variableValues"], 1) == 60


def test_artificial_minimisation(classic_min):
    classic_min["solver"]["basisMethod"] = "artificial"
    out = run(classic_min)
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 4
    assert var(sol["variableValues"], 0) == 4
    assert var(sol["variableValues"], 1) == 0


def test_artificial_infeasible(infeasible):
    infeasible["solver"]["basisMethod"] = "artificial"
    out = run(infeasible)
    assert out["solution"]["status"] == "infeasible"


def test_artificial_equality_constraint():
    inp = make_input(
        "minimize",
        [1, 1],
        [
            constraint([1, 1], "=", 10),
            constraint([1, 0], ">=", 3),
        ],
        method="artificial",
    )
    out = run(inp)
    sol = out["solution"]
    assert sol["status"] == "optimal"
    assert obj_val(sol) == 10


def test_both_methods_agree(classic_max):
    bigm = dict(classic_max)
    bigm["solver"] = {"method": "simplex", "basisMethod": "big-m", "bigM": frac(1000)}
    art = dict(classic_max)
    art["solver"] = {"method": "simplex", "basisMethod": "artificial"}

    out_bigm = run(bigm)["solution"]
    out_art = run(art)["solution"]

    assert out_bigm["status"] == out_art["status"] == "optimal"
    assert out_bigm["objectiveValue"] == out_art["objectiveValue"]
    assert out_bigm["variableValues"] == out_art["variableValues"]
