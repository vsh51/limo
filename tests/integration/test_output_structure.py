"""Tests that verify the shape and fields of the JSON output."""
from conftest import run, make_input, constraint


def test_top_level_fields(classic_max):
    out = run(classic_max)
    assert set(out.keys()) == {"status", "error", "input", "basisFinding", "iterations", "solution"}


def test_status_ok_on_success(classic_max):
    out = run(classic_max)
    assert out["status"] == "ok"
    assert out["error"] is None


def test_input_summary(classic_max):
    out = run(classic_max)
    inp = out["input"]
    assert inp["variableCount"] == 2
    assert inp["constraintCount"] == 3
    assert inp["objectiveSense"] == "maximize"


def test_basis_finding_fields(classic_max):
    out = run(classic_max)
    bf = out["basisFinding"]
    assert bf["method"] == "big-m"
    result = bf["result"]
    assert "originalVariableCount" in result
    assert "basisColumns" in result
    assert "slackColumns" in result
    assert "surplusColumns" in result
    assert "artificialColumns" in result
    assert "augmented" in result
    assert result["originalVariableCount"] == 2


def test_iterations_is_list(classic_max):
    out = run(classic_max)
    assert isinstance(out["iterations"], list)
    assert len(out["iterations"]) > 0


def test_iteration_fields(classic_max):
    out = run(classic_max)
    it = out["iterations"][0]
    assert "iterationNumber" in it
    assert "tableau" in it
    assert "basisVariables" in it
    assert "pivotRow" in it
    assert "pivotColumn" in it
    assert "objectiveValue" in it
    assert "ratios" in it


def test_solution_fields(classic_max):
    out = run(classic_max)
    sol = out["solution"]
    assert "status" in sol
    assert "objectiveValue" in sol
    assert "variableValues" in sol
    assert len(sol["variableValues"]) == 2


def test_no_iterations_when_already_optimal():
    # A trivially optimal LP: min x  s.t.  x >= 0 (just x itself, already at 0)
    # Actually in our setup we need at least one binding constraint.
    # Use a one-constraint problem where the initial basis is already optimal.
    inp = make_input(
        "minimize",
        [0],
        [constraint([1], "<=", 5)],
    )
    out = run(inp)
    assert out["solution"]["status"] == "optimal"


def test_final_iteration_has_no_pivot(classic_max):
    out = run(classic_max)
    last = out["iterations"][-1]
    assert last["pivotRow"] is None
    assert last["pivotColumn"] is None


def test_artificial_method_label(classic_max):
    classic_max["solver"]["basisMethod"] = "artificial"
    out = run(classic_max)
    assert out["basisFinding"]["method"] == "artificial"


def test_infeasible_solution_fields(infeasible):
    out = run(infeasible)
    sol = out["solution"]
    assert sol["status"] == "infeasible"
