import os
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent

LIMO_BINARY = os.environ.get("LIMO_BINARY", str(BASE_DIR / "build" / "bin" / "limo"))
REDIS_URL = os.environ.get("REDIS_URL", "redis://localhost:6379/0")
CELERY_RESULT_EXPIRES = int(os.environ.get("CELERY_RESULT_EXPIRES", "3600"))
SOLVER_TIMEOUT = int(os.environ.get("SOLVER_TIMEOUT", "300"))
PARSE_TIMEOUT = int(os.environ.get("PARSE_TIMEOUT", "10"))
SUPPORTED_FORMATS = {"csv", "txt"}
