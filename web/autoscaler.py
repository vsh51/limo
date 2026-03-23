import logging
import math
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

REDIS_URL = os.environ.get("REDIS_URL", "redis://redis:6379/0")
COMPOSE_FILE = os.environ.get("COMPOSE_FILE", "/app/project/docker-compose.yml")
PROJECT_NAME = os.environ.get("COMPOSE_PROJECT_NAME", "limo")
MIN_WORKERS = int(os.environ.get("MIN_WORKERS", "1"))
MAX_WORKERS = int(os.environ.get("MAX_WORKERS", "4"))
TASKS_PER_WORKER = int(os.environ.get("TASKS_PER_WORKER", "2"))
IDLE_BEFORE_SCALE_DOWN = int(os.environ.get("IDLE_BEFORE_SCALE_DOWN", "30"))
CHECK_INTERVAL = int(os.environ.get("CHECK_INTERVAL", "5"))
SCALE_COOLDOWN = int(os.environ.get("SCALE_COOLDOWN", "10"))


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


def _compose_cmd(*args: str) -> list[str]:
    return [
        "docker", "compose",
        "-f", COMPOSE_FILE,
        "-p", PROJECT_NAME,
        *args,
    ]


def get_running_worker_count() -> int:
    result = subprocess.run(
        _compose_cmd("ps", "--status", "running", "--format", "{{.Name}}", "worker"),
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        log.error("Failed to list workers: %s", result.stderr.strip())
        return -1
    names = [line for line in result.stdout.strip().splitlines() if line]
    return len(names)


def scale_workers(n: int, force_recreate: bool = False) -> bool:
    cmd = _compose_cmd(
        "up", "-d", "--no-deps",
        "--scale", f"worker={n}",
        "--remove-orphans",
    )
    if force_recreate:
        cmd.append("--force-recreate")
    cmd.append("worker")

    result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    if result.returncode != 0:
        log.error("Scale command failed:\n%s", result.stderr.strip())
        return False
    log.info("Scaled workers to %d%s", n, " (force-recreated)" if force_recreate else "")
    return True


def main() -> None:
    r = connect_redis(REDIS_URL)
    idle_ticks = 0
    last_scale_time = 0.0

    log.info(
        "Autoscaler started — workers [%d–%d], %d tasks/worker, "
        "scale-down after %ds idle, check every %ds, cooldown %ds",
        MIN_WORKERS, MAX_WORKERS, TASKS_PER_WORKER,
        IDLE_BEFORE_SCALE_DOWN, CHECK_INTERVAL, SCALE_COOLDOWN,
    )

    # On startup: force-recreate workers to ensure clean state
    log.info("Initial startup: force-recreating %d worker(s)…", MIN_WORKERS)
    scale_workers(MIN_WORKERS, force_recreate=True)
    last_scale_time = time.monotonic()

    while True:
        try:
            depth = int(r.llen("celery"))
            current_workers = get_running_worker_count()
            if current_workers < 0:
                current_workers = MIN_WORKERS

            now = time.monotonic()
            can_scale = (now - last_scale_time) >= SCALE_COOLDOWN

            if depth > 0:
                idle_ticks = 0
                desired = min(
                    MAX_WORKERS,
                    max(MIN_WORKERS, math.ceil(depth / TASKS_PER_WORKER)),
                )
                if desired > current_workers and can_scale:
                    log.info(
                        "Queue depth %d → scaling up %d → %d workers",
                        depth, current_workers, desired,
                    )
                    scale_workers(desired)
                    last_scale_time = time.monotonic()
            else:
                idle_ticks += 1
                idle_seconds = idle_ticks * CHECK_INTERVAL
                if (
                    idle_seconds >= IDLE_BEFORE_SCALE_DOWN
                    and current_workers > MIN_WORKERS
                    and can_scale
                ):
                    log.info(
                        "Queue empty for %ds → scaling down %d → %d workers",
                        idle_seconds, current_workers, MIN_WORKERS,
                    )
                    scale_workers(MIN_WORKERS)
                    last_scale_time = time.monotonic()
                    idle_ticks = 0

            log.debug(
                "queue=%d  workers=%d  idle=%ds",
                depth, current_workers, idle_ticks * CHECK_INTERVAL,
            )

        except Exception as exc:
            log.error("Autoscaler error: %s", exc)

        time.sleep(CHECK_INTERVAL)


if __name__ == "__main__":
    main()
