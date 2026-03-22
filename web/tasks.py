"""Celery task definitions for the LIMO solver.

Each solve request is dispatched as a Celery task so that:
  - The Flask web server returns immediately with a task ID.
  - Workers pick up tasks from the Redis queue independently.
  - Users can keep using the interface while a solve is in progress.
  - Multiple workers run in parallel (one simplex solve per worker process).

Progress states emitted during task execution
---------------------------------------------
  PROGRESS / step="starting"  — binary is being launched
  PROGRESS / step="solving"   — binary is running, waiting for output
  SUCCESS                     — result stored in Redis
"""

import json
import subprocess
import time

from celery import Celery

from web.config import CELERY_RESULT_EXPIRES, LIMO_BINARY, REDIS_URL, SOLVER_TIMEOUT

celery_app = Celery(
    "limo_tasks",
    broker=REDIS_URL,
    backend=REDIS_URL,
)

celery_app.conf.update(
    task_serializer="json",
    result_serializer="json",
    accept_content=["json"],
    result_expires=CELERY_RESULT_EXPIRES,
    task_track_started=True,
    worker_prefetch_multiplier=1,
    broker_connection_retry_on_startup=True,
    broker_transport_options={
        "socket_timeout": 5,
        "socket_connect_timeout": 5,
        "socket_keepalive": True,
        "retry_on_timeout": True,
    },
    result_backend_transport_options={
        "socket_timeout": 5,
        "socket_connect_timeout": 5,
        "socket_keepalive": True,
        "retry_on_timeout": True,
    },
)


@celery_app.task(bind=True, name="limo_tasks.solve")
def solve_task(self, limo_input: dict) -> dict:
    """Run the limo binary on *limo_input* and return the raw solver output.

    The task is executed by a Celery worker process. The result (or any
    exception) is stored in Redis and retrieved by the Flask server via
    GET /api/result/<task_id>.
    """
    started_at = time.time()

    self.update_state(
        state="PROGRESS",
        meta={"step": "starting", "started_at": started_at},
    )

    try:
        proc = subprocess.Popen(
            [LIMO_BINARY],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except FileNotFoundError:
        raise RuntimeError(
            f"LIMO binary not found at '{LIMO_BINARY}'. "
            "Make sure the Docker image was built correctly."
        )

    self.update_state(
        state="PROGRESS",
        meta={"step": "solving", "started_at": started_at},
    )

    try:
        stdout, stderr = proc.communicate(
            input=json.dumps(limo_input),
            timeout=SOLVER_TIMEOUT,
        )
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.communicate()
        raise RuntimeError(f"Solver timed out ({SOLVER_TIMEOUT}s limit)")

    if proc.returncode != 0 and not stdout.strip():
        raise RuntimeError(stderr or "Solver process crashed")

    return json.loads(stdout)
