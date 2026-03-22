"""Flask application for the LIMO web interface.

Routes
------
GET  /                  — serve the single-page frontend
POST /api/solve         — enqueue a solve task, return task ID
GET  /api/result/<id>   — poll task status / retrieve result
POST /api/import        — parse an uploaded file via the C++ binary
GET  /api/workers       — number of active Celery workers
GET  /api/status        — Redis queue depth (used by health checks)
"""

import json
import subprocess
import sys
from pathlib import Path

import redis as redis_lib
from celery.result import AsyncResult
from flask import Flask, jsonify, request, send_from_directory

# Ensure the project root is importable (for limo_io)
_project_root = str(Path(__file__).resolve().parent.parent)
if _project_root not in sys.path:
    sys.path.insert(0, _project_root)

from limo_io import convert_output, format_fraction, parse_fraction
from web.config import LIMO_BINARY, PARSE_TIMEOUT, REDIS_URL, SUPPORTED_FORMATS
from web.tasks import celery_app, solve_task

app = Flask(__name__, static_folder="static")

_redis = redis_lib.from_url(REDIS_URL)


# ── Helpers ──────────────────────────────────────────────────────────────────


def _build_limo_input(data: dict) -> dict:
    """Transform frontend JSON into the format expected by the C++ binary."""
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


def _get_queue_position(task_id: str) -> tuple[int | None, int]:
    """Return (1-based position in queue, total queue length).

    Scans the Redis list used by the Celery broker. Returns (None, total)
    if the task has already been picked up by a worker.
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


def _format_coefficient(c) -> str:
    """Format a coefficient (dict or scalar) as a human-readable string."""
    return format_fraction(c) if isinstance(c, dict) else str(c)


# ── Static pages ─────────────────────────────────────────────────────────────


@app.route("/")
def index():
    return send_from_directory("static", "index.html")


# ── Async solve ──────────────────────────────────────────────────────────────


@app.route("/api/solve", methods=["POST"])
def solve():
    """Enqueue a solve task and return its ID immediately."""
    try:
        user_input = request.get_json()
        limo_input = _build_limo_input(user_input)
        task = solve_task.delay(limo_input)
        return jsonify({"task_id": task.id, "state": "PENDING"})
    except Exception as e:
        return jsonify({"status": "error", "error": str(e)}), 500


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


# ── File import ──────────────────────────────────────────────────────────────


@app.route("/api/import", methods=["POST"])
def import_file():
    """Parse an uploaded LP file via the C++ binary's --parse-only mode."""
    if "file" not in request.files:
        return jsonify({"error": "No file uploaded"}), 400

    f = request.files["file"]
    filename = (f.filename or "").lower()

    try:
        content = f.read().decode("utf-8")
    except UnicodeDecodeError:
        return jsonify({"error": "File must be UTF-8 encoded"}), 400

    ext = filename.rsplit(".", 1)[-1] if "." in filename else ""
    if ext not in SUPPORTED_FORMATS:
        return jsonify({"error": f"Unsupported format. Use: {', '.join(sorted(SUPPORTED_FORMATS))}"}), 400

    try:
        result = subprocess.run(
            [LIMO_BINARY, "--format", ext, "--parse-only"],
            input=content,
            capture_output=True,
            text=True,
            timeout=PARSE_TIMEOUT,
        )
        raw = json.loads(result.stdout)
        if raw.get("status") == "error":
            return jsonify({"error": raw.get("error", "Parse error")}), 400

        obj = raw["objective"]
        constraints = raw["constraints"]

        return jsonify({
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
                for con in constraints
            ],
            "basisMethod": raw.get("basisMethod", "big-m"),
        })

    except subprocess.TimeoutExpired:
        return jsonify({"error": "Parser timed out"}), 500
    except Exception as e:
        return jsonify({"error": f"Import error: {e}"}), 500


# ── Queue status ─────────────────────────────────────────────────────────────


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
    """Health-check endpoint. Returns Redis queue depth."""
    try:
        queue_depth = int(_redis.llen("celery"))
    except Exception:
        queue_depth = -1
    return jsonify({"queue_depth": queue_depth})


# ── Development server ───────────────────────────────────────────────────────

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5050)
