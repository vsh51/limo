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
import os
import subprocess
import time

from celery import Celery

REDIS_URL   = os.environ.get("REDIS_URL",   "redis://localhost:6379/0")
LIMO_BINARY = os.environ.get("LIMO_BINARY", "./build/bin/limo")

# One Celery application shared by both the Flask server (to dispatch tasks)
# and the worker processes (to execute tasks).
celery_app = Celery(
    "limo_tasks",
    broker=REDIS_URL,
    backend=REDIS_URL,
)

celery_app.conf.update(
    task_serializer="json",
    result_serializer="json",
    accept_content=["json"],
    result_expires=3600,           # keep results in Redis for 1 hour
    task_track_started=True,       # STARTED state is reported while running
    worker_prefetch_multiplier=1,  # each worker takes one task at a time
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

    The task is executed by a Celery worker process.  The result (or any
    exception) is stored in Redis and retrieved by the Flask server via
    GET /api/result/<task_id>.
    """
    started_at = time.time()

    # ── Step 1: launch the solver process ────────────────────────────────────
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

    # ── Step 2: feed input and wait for result ────────────────────────────────
    self.update_state(
        state="PROGRESS",
        meta={"step": "solving", "started_at": started_at},
    )

    try:
        stdout, stderr = proc.communicate(
            input=json.dumps(limo_input),
            timeout=300,
        )
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.communicate()
        raise RuntimeError("Solver timed out (300 s limit)")

    if proc.returncode != 0 and not stdout.strip():
        raise RuntimeError(stderr or "Solver process crashed")

    return json.loads(stdout)
