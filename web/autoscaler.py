"""Autoscaler for LIMO Celery workers.

Periodically reads the Redis queue depth and adjusts the number of running
worker containers via `docker compose up --scale worker=N`.

Scaling rules
-------------
- desired = ceil(queue_depth / TASKS_PER_WORKER), clamped to [MIN, MAX]
- Scale up  immediately when desired > current
- Scale down after the queue has been empty for IDLE_BEFORE_SCALE_DOWN seconds
"""

import logging
import os
import subprocess
import time

import redis

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)-8s  %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("autoscaler")

# ── Configuration (all tuneable via environment variables) ────────────────────

REDIS_URL             = os.environ.get("REDIS_URL",             "redis://redis:6379/0")
COMPOSE_FILE          = os.environ.get("COMPOSE_FILE",          "/app/project/docker-compose.yml")
PROJECT_NAME          = os.environ.get("COMPOSE_PROJECT_NAME",  "limo")
MIN_WORKERS           = int(os.environ.get("MIN_WORKERS",           "1"))
MAX_WORKERS           = int(os.environ.get("MAX_WORKERS",           "8"))
TASKS_PER_WORKER      = int(os.environ.get("TASKS_PER_WORKER",      "2"))
IDLE_BEFORE_SCALE_DOWN = int(os.environ.get("IDLE_BEFORE_SCALE_DOWN","30"))
CHECK_INTERVAL        = int(os.environ.get("CHECK_INTERVAL",        "10"))

# ── Redis connection ──────────────────────────────────────────────────────────

def connect_redis(url: str, retries: int = 12, delay: float = 5.0) -> redis.Redis:
    for attempt in range(1, retries + 1):
        try:
            r = redis.from_url(url)
            r.ping()
            log.info("Connected to Redis at %s", url)
            return r
        except Exception as exc:
            log.warning("Redis not ready (attempt %d/%d): %s", attempt, retries, exc)
            time.sleep(delay)
    raise RuntimeError(f"Cannot connect to Redis after {retries} attempts")


r = connect_redis(REDIS_URL)

# ── Docker Compose scaling ────────────────────────────────────────────────────

def scale_workers(n: int) -> None:
    cmd = [
        "docker", "compose",
        "-f", COMPOSE_FILE,
        "-p", PROJECT_NAME,
        "up",
        "--scale", f"worker={n}",
        "-d",
        "--no-recreate",
    ]
    log.info("Running: %s", " ".join(cmd))
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        log.error("Scale command failed:\n%s", result.stderr)
    else:
        log.info("Scaled workers to %d", n)

# ── Main loop ─────────────────────────────────────────────────────────────────

current_workers = MIN_WORKERS
idle_ticks       = 0            # consecutive ticks with empty queue

log.info(
    "Autoscaler started — workers [%d–%d], %d tasks/worker, "
    "scale-down after %ds idle, check every %ds",
    MIN_WORKERS, MAX_WORKERS, TASKS_PER_WORKER,
    IDLE_BEFORE_SCALE_DOWN, CHECK_INTERVAL,
)

while True:
    try:
        depth = int(r.llen("celery"))   # Celery default queue key

        if depth > 0:
            idle_ticks = 0
            # Ceiling division: how many workers do we need?
            desired = min(MAX_WORKERS, max(MIN_WORKERS,
                          -(-depth // TASKS_PER_WORKER)))
            if desired > current_workers:
                log.info("Queue depth %d → scaling up %d → %d workers",
                         depth, current_workers, desired)
                scale_workers(desired)
                current_workers = desired
        else:
            idle_ticks += 1
            idle_seconds = idle_ticks * CHECK_INTERVAL
            if idle_seconds >= IDLE_BEFORE_SCALE_DOWN and current_workers > MIN_WORKERS:
                log.info("Queue empty for %ds → scaling down to %d workers",
                         idle_seconds, MIN_WORKERS)
                scale_workers(MIN_WORKERS)
                current_workers = MIN_WORKERS
                idle_ticks = 0

        log.debug("queue=%d  workers=%d  idle=%ds",
                  depth, current_workers, idle_ticks * CHECK_INTERVAL)

    except Exception as exc:
        log.error("Autoscaler error: %s", exc)

    time.sleep(CHECK_INTERVAL)
