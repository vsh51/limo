"""Tests that verify error responses for invalid inputs."""
import json
import subprocess
import os
import pytest

BINARY = os.environ.get("LIMO_BINARY", "./build/bin/limo")


def run_raw(text: str) -> dict:
    result = subprocess.run(
        [BINARY],
        input=text,
        capture_output=True,
        text=True,
    )
    return json.loads(result.stdout)


def test_malformed_json():
    out = run_raw("{not valid json")
    assert out["status"] == "error"
    assert out["error"]


def test_missing_objective():
    out = run_raw('{"constraints": [{"coefficients": [{"num": 1, "den": 1}], "sense": "<=", "rhs": {"num": 1, "den": 1}}]}')
    assert out["status"] == "error"


def test_missing_constraints():
    out = run_raw('{"objective": {"sense": "maximize", "coefficients": [{"num": 1, "den": 1}]}}')
    assert out["status"] == "error"


def test_empty_constraints():
    out = run_raw('{"objective": {"sense": "maximize", "coefficients": [{"num": 1, "den": 1}]}, "constraints": []}')
    assert out["status"] == "error"


def test_dimension_mismatch():
    # objective has 2 coefficients but constraint has 1
    out = run_raw(json.dumps({
        "objective": {"sense": "maximize", "coefficients": [{"num": 1, "den": 1}, {"num": 2, "den": 1}]},
        "constraints": [{"coefficients": [{"num": 1, "den": 1}], "sense": "<=", "rhs": {"num": 5, "den": 1}}],
    }))
    assert out["status"] == "error"


def test_zero_denominator():
    out = run_raw(json.dumps({
        "objective": {"sense": "maximize", "coefficients": [{"num": 1, "den": 0}]},
        "constraints": [{"coefficients": [{"num": 1, "den": 1}], "sense": "<=", "rhs": {"num": 5, "den": 1}}],
    }))
    assert out["status"] == "error"


def test_invalid_objective_sense():
    out = run_raw(json.dumps({
        "objective": {"sense": "sideways", "coefficients": [{"num": 1, "den": 1}]},
        "constraints": [{"coefficients": [{"num": 1, "den": 1}], "sense": "<=", "rhs": {"num": 5, "den": 1}}],
    }))
    assert out["status"] == "error"


def test_invalid_constraint_sense():
    out = run_raw(json.dumps({
        "objective": {"sense": "maximize", "coefficients": [{"num": 1, "den": 1}]},
        "constraints": [{"coefficients": [{"num": 1, "den": 1}], "sense": "!=", "rhs": {"num": 5, "den": 1}}],
    }))
    assert out["status"] == "error"


def test_empty_input():
    out = run_raw("")
    assert out["status"] == "error"
