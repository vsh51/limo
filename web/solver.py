import json
import signal
import subprocess

from limo_io import convert_output, format_fraction, parse_fraction

from web.config import LIMO_BINARY, PARSE_TIMEOUT, SOLVER_TIMEOUT

# Track the running solver process so it can be killed on SIGTERM
_current_proc = None


def _sigterm_handler(signum, frame):
    """Kill the solver subprocess when the worker receives SIGTERM."""
    if _current_proc and _current_proc.poll() is None:
        _current_proc.kill()
    raise SystemExit(1)


signal.signal(signal.SIGTERM, _sigterm_handler)


class SolverError(RuntimeError):
    pass


class SolverTimeout(SolverError):
    pass


def build_input(data: dict) -> dict:
    return {
        "objective": {
            "sense": data["objective"]["sense"],
            "coefficients": [parse_fraction(c) for c in data["objective"]["coefficients"]],
        },
        "constraints": [
            {
                "coefficients": [parse_fraction(c) for c in con["coefficients"]],
                "sense": con["sense"],
                "rhs": parse_fraction(con["rhs"]),
            }
            for con in data["constraints"]
        ],
        "solver": {
            "method": "simplex",
            "basisMethod": data.get("basisMethod", "big-m"),
        },
    }


def run(limo_input: dict) -> dict:
    global _current_proc
    try:
        proc = subprocess.Popen(
            [LIMO_BINARY],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        _current_proc = proc
    except FileNotFoundError:
        raise SolverError(
            f"LIMO binary not found at '{LIMO_BINARY}'. "
            "Make sure the Docker image was built correctly."
        )

    try:
        stdout, stderr = proc.communicate(input=json.dumps(limo_input), timeout=SOLVER_TIMEOUT)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.communicate()
        raise SolverTimeout(f"Solver timed out ({SOLVER_TIMEOUT}s limit)")
    finally:
        _current_proc = None

    if proc.returncode != 0 and not stdout.strip():
        raise SolverError(stderr or "Solver process crashed")

    return json.loads(stdout)


def parse_file(content: str, fmt: str) -> dict:
    try:
        proc = subprocess.run(
            [LIMO_BINARY, "--format", fmt, "--parse-only"],
            input=content,
            capture_output=True,
            text=True,
            timeout=PARSE_TIMEOUT,
        )
    except subprocess.TimeoutExpired:
        raise SolverTimeout("Parser timed out")

    raw = json.loads(proc.stdout)
    if raw.get("status") == "error":
        raise SolverError(raw.get("error", "Parse error"))
    return raw


def format_result(raw_result: dict) -> dict:
    return convert_output(raw_result)


def _format_coefficient(c) -> str:
    return format_fraction(c) if isinstance(c, dict) else str(c)


def serialize_problem(raw: dict) -> dict:
    obj = raw["objective"]
    return {
        "numVars": len(obj["coefficients"]),
        "objective": {
            "sense": obj["sense"],
            "coefficients": [_format_coefficient(c) for c in obj["coefficients"]],
        },
        "constraints": [
            {
                "coefficients": [_format_coefficient(c) for c in con["coefficients"]],
                "sense": con["sense"],
                "rhs": _format_coefficient(con["rhs"]),
            }
            for con in raw["constraints"]
        ],
        "basisMethod": raw.get("basisMethod", "big-m"),
    }
