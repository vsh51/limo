import time

from web.solver import run
from web.worker import celery_app


@celery_app.task(bind=True, name="limo.solve")
def solve_task(self, limo_input: dict) -> dict:
    started_at = time.time()
    self.update_state(state="PROGRESS", meta={"step": "solving", "started_at": started_at})
    return run(limo_input)
