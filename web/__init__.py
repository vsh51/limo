import sys
from pathlib import Path

_project_root = str(Path(__file__).resolve().parent.parent)
if _project_root not in sys.path:
    sys.path.insert(0, _project_root)

from web.worker import celery_app  # noqa: E402, F401


def create_app():
    from flask import Flask

    from web.routes import bp

    application = Flask(__name__, static_folder="static")
    application.register_blueprint(bp)
    return application


flask_app = create_app()
