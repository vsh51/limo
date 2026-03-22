import json
import os
import subprocess
import sys

# Make the project root importable so limo_io can be used here and in tests
sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(__file__), "..")))

import redis as redis_lib
from celery.result import AsyncResult
from flask import Flask, jsonify, request, send_from_directory

from limo_io import convert_output, format_fraction, parse_fraction
from web.tasks import celery_app, solve_task

app = Flask(__name__, static_folder="static")

LIMO_BINARY = os.environ.get(
    "LIMO_BINARY",
    os.path.normpath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build", "bin", "limo")
    ),
)
REDIS_URL = os.environ.get("REDIS_URL", "redis://localhost:6379/0")

_redis = redis_lib.from_url(REDIS_URL)


def build_limo_input(data: dict) -> dict:
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


# ── Static pages ──────────────────────────────────────────────────────────────

@app.route("/")
def index():
    return send_from_directory("static", "index.html")


# ── Async solve ───────────────────────────────────────────────────────────────

@app.route("/api/solve", methods=["POST"])
def solve():
    """Enqueue a solve task and return its ID immediately."""
    try:
        user_input = request.get_json()
        limo_input = build_limo_input(user_input)
        task = solve_task.delay(limo_input)
        return jsonify({"task_id": task.id, "state": "PENDING"})
    except Exception as e:
        return jsonify({"status": "error", "error": str(e)}), 500


def _get_queue_position(task_id: str) -> tuple[int | None, int]:
    """Return (1-based position in the Celery queue, total queue length).

    Scans the Redis list used by the Celery broker.  If the task is not found
    (e.g. it was already picked up by a worker), returns (None, total).
    """
    try:
        items = _redis.lrange("celery", 0, -1)
        total = len(items)
        needle = task_id.encode()
        for idx, raw in enumerate(items):
            if needle in raw:
                return idx + 1, total
        return None, total
    except Exception:
        return None, 0


@app.route("/api/result/<task_id>")
def get_result(task_id: str):
    """Poll the status / result of a previously submitted solve task."""
    task = AsyncResult(task_id, app=celery_app)

    if task.state == "PENDING":
        pos, total = _get_queue_position(task_id)
        return jsonify({"state": "PENDING", "position": pos, "total": total})

    if task.state in ("STARTED", "PROGRESS"):
        meta = task.info if isinstance(task.info, dict) else {}
        return jsonify({"state": "PROGRESS", "meta": meta})

    if task.state == "SUCCESS":
        result = convert_output(task.result)
        return jsonify({"state": "SUCCESS", "result": result})

    if task.state == "FAILURE":
        return jsonify({"state": "FAILURE", "error": str(task.result)})

    return jsonify({"state": task.state})


# ── File import (delegated to the C++ binary) ─────────────────────────────────

@app.route("/api/import", methods=["POST"])
def import_file():
    if "file" not in request.files:
        return jsonify({"error": "No file uploaded"}), 400

    f = request.files["file"]
    filename = (f.filename or "").lower()

    try:
        content = f.read().decode("utf-8")
    except UnicodeDecodeError:
        return jsonify({"error": "File must be UTF-8 encoded"}), 400

    ext = filename.rsplit(".", 1)[-1] if "." in filename else ""
    if ext not in ("json", "csv", "txt"):
        return jsonify({"error": "Unsupported format. Use .json, .csv, or .txt"}), 400

    try:
        result = subprocess.run(
            [LIMO_BINARY, "--format", ext, "--parse-only"],
            input=content,
            capture_output=True,
            text=True,
            timeout=10,
        )
        raw = json.loads(result.stdout)
        if raw.get("status") == "error":
            return jsonify({"error": raw.get("error", "Parse error")}), 400

        obj = raw["objective"]
        constraints = raw["constraints"]

        def coeff_str(c):
            return format_fraction(c) if isinstance(c, dict) else str(c)

        return jsonify({
            "numVars": len(obj["coefficients"]),
            "objective": {
                "sense": obj["sense"],
                "coefficients": [coeff_str(c) for c in obj["coefficients"]],
            },
            "constraints": [
                {
                    "coefficients": [coeff_str(c) for c in con["coefficients"]],
                    "sense": con["sense"],
                    "rhs": coeff_str(con["rhs"]),
                }
                for con in constraints
            ],
            "basisMethod": raw.get("basisMethod", "big-m"),
        })

    except subprocess.TimeoutExpired:
        return jsonify({"error": "Parser timed out"}), 500
    except Exception as e:
        return jsonify({"error": f"Import error: {e}"}), 500


# ── Queue status (for the web UI indicator) ───────────────────────────────────

@app.route("/api/workers")
def api_workers():
    """Return the number of reachable Celery workers."""
    try:
        result = celery_app.control.ping(timeout=0.5)
        count = len(result) if result else 0
    except Exception:
        count = -1
    return jsonify({"count": count})


@app.route("/api/status")
def api_status():
    try:
        queue_depth = int(_redis.llen("celery"))
    except Exception:
        queue_depth = -1
    return jsonify({"queue_depth": queue_depth})


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5050)
