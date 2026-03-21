import json
import os
import subprocess
import pytest

BINARY = os.environ.get("LIMO_BINARY", "./build/bin/limo")


def run(input_dict: dict) -> dict:
    result = subprocess.run(
        [BINARY],
        input=json.dumps(input_dict),
        capture_output=True,
        text=True,
    )
    assert result.returncode in (0, 1), f"unexpected exit code {result.returncode}"
    return json.loads(result.stdout)


def frac(num: int, den: int = 1) -> dict:
    return {"num": num, "den": den}


def make_input(
    sense: str,
    coeffs: list[int | tuple],
    constraints: list[dict],
    *,
    method: str = "big-m",
    big_m: int = 1000,
) -> dict:
    obj_coeffs = [frac(c) if isinstance(c, int) else frac(*c) for c in coeffs]
    solver = {"method": "simplex", "basisMethod": method}
    if method == "big-m":
        solver["bigM"] = frac(big_m)
    return {
        "objective": {"sense": sense, "coefficients": obj_coeffs},
        "constraints": constraints,
        "solver": solver,
    }


def constraint(coeffs: list[int | tuple], sense: str, rhs: int | tuple) -> dict:
    return {
        "coefficients": [frac(c) if isinstance(c, int) else frac(*c) for c in coeffs],
        "sense": sense,
        "rhs": frac(rhs) if isinstance(rhs, int) else frac(*rhs),
    }


@pytest.fixture
def classic_max():
    """max 30x1 + 20x2  s.t.  2x1+x2<=100, x1+x2<=80, x1<=40  →  x1=20,x2=60,z=1800"""
    return make_input(
        "maximize",
        [30, 20],
        [
            constraint([2, 1], "<=", 100),
            constraint([1, 1], "<=", 80),
            constraint([1, 0], "<=", 40),
        ],
    )


@pytest.fixture
def classic_min():
    """min x1 + 2x2  s.t.  x1+x2>=4, x1>=2  →  x1=4,x2=0,z=4"""
    return make_input(
        "minimize",
        [1, 2],
        [
            constraint([1, 1], ">=", 4),
            constraint([1, 0], ">=", 2),
        ],
    )


@pytest.fixture
def infeasible():
    """x1+x2<=4 AND x1+x2>=6 — infeasible"""
    return make_input(
        "minimize",
        [1, 1],
        [
            constraint([1, 1], "<=", 4),
            constraint([1, 1], ">=", 6),
        ],
    )
