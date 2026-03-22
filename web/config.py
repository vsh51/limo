"""Centralized configuration for the LIMO web application.

All environment-driven settings live here. Import from this module instead
of scattering os.environ.get() calls across the codebase.
"""

import os

# ── Paths ──────────────────────────────────────────────────────────────────────

LIMO_BINARY = os.environ.get(
    "LIMO_BINARY",
    os.path.normpath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build", "bin", "limo")
    ),
)

# ── Redis / Celery ─────────────────────────────────────────────────────────────

REDIS_URL = os.environ.get("REDIS_URL", "redis://localhost:6379/0")

CELERY_RESULT_EXPIRES = int(os.environ.get("CELERY_RESULT_EXPIRES", "3600"))

# ── Solver ─────────────────────────────────────────────────────────────────────

SOLVER_TIMEOUT = int(os.environ.get("SOLVER_TIMEOUT", "300"))

SUPPORTED_FORMATS = {"json", "csv", "txt"}
PARSE_TIMEOUT = 10
