from celery.result import AsyncResult
from flask import Blueprint, current_app, jsonify, request, send_from_directory

from web.config import SUPPORTED_FORMATS
from web.solver import (
    SolverError,
    SolverTimeout,
    build_input,
    format_result,
    parse_file,
    serialize_problem,
)
from web.worker import celery_app, get_queue_position
from web.worker.tasks import solve_task

bp = Blueprint("api", __name__)


@bp.route("/")
def index():
    return send_from_directory(current_app.static_folder, "index.html")


@bp.route("/api/solve", methods=["POST"])
def solve():
    try:
        limo_input = build_input(request.get_json())
        task = solve_task.delay(limo_input)
        return jsonify({"task_id": task.id, "state": "PENDING"})
    except Exception as e:
        return jsonify({"status": "error", "error": str(e)}), 500


@bp.route("/api/result/<task_id>")
def get_result(task_id: str):
    task = AsyncResult(task_id, app=celery_app)

    if task.state == "PENDING":
        pos, total = get_queue_position(task_id)
        if pos is None:
            return jsonify({"state": "LOST"})
        return jsonify({"state": "PENDING", "position": pos, "total": total})

    if task.state in ("STARTED", "PROGRESS"):
        meta = task.info if isinstance(task.info, dict) else {}
        return jsonify({"state": "PROGRESS", "meta": meta})

    if task.state == "SUCCESS":
        return jsonify({"state": "SUCCESS", "result": format_result(task.result)})

    if task.state == "FAILURE":
        return jsonify({"state": "FAILURE", "error": str(task.result)})

    if task.state == "REVOKED":
        return jsonify({"state": "REVOKED"})

    return jsonify({"state": task.state})


@bp.route("/api/import", methods=["POST"])
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
    if ext not in SUPPORTED_FORMATS:
        fmts = ", ".join(sorted(SUPPORTED_FORMATS))
        return jsonify({"error": f"Unsupported format. Use: {fmts}"}), 400

    try:
        raw = parse_file(content, ext)
        return jsonify(serialize_problem(raw))
    except SolverTimeout as e:
        return jsonify({"error": str(e)}), 500
    except SolverError as e:
        return jsonify({"error": str(e)}), 400
    except Exception as e:
        return jsonify({"error": f"Import error: {e}"}), 500


@bp.route("/api/cancel/<task_id>", methods=["POST"])
def cancel_task(task_id: str):
    celery_app.control.revoke(task_id, terminate=True, signal="SIGTERM")
    return jsonify({"state": "REVOKED", "task_id": task_id})
