import redis as redis_lib
from celery import Celery

from web.config import CELERY_RESULT_EXPIRES, REDIS_URL, SOLVER_TIMEOUT

celery_app = Celery("limo", broker=REDIS_URL, backend=REDIS_URL, include=["web.worker.tasks"])

celery_app.conf.update(
    task_serializer="json",
    result_serializer="json",
    accept_content=["json"],
    result_expires=CELERY_RESULT_EXPIRES,
    task_track_started=True,
    task_acks_late=True,
    task_reject_on_worker_lost=True,
    task_soft_time_limit=SOLVER_TIMEOUT + 30,
    task_time_limit=SOLVER_TIMEOUT + 60,
    worker_max_tasks_per_child=50,
    worker_prefetch_multiplier=1,
    worker_send_task_events=True,
    task_send_sent_event=True,
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

_redis_pool = redis_lib.ConnectionPool.from_url(REDIS_URL)


def get_redis() -> redis_lib.Redis:
    return redis_lib.Redis(connection_pool=_redis_pool)


def get_queue_position(task_id: str) -> tuple[int | None, int]:
    try:
        r = get_redis()
        items = r.lrange("celery", 0, -1)
        total = len(items)
        needle = task_id.encode()
        for idx, raw in enumerate(items):
            if needle in raw:
                return idx + 1, total
        return None, total
    except Exception:
        return None, 0
