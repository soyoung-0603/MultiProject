import logging
import os
from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI
from fastapi.exceptions import RequestValidationError
from fastapi.responses import FileResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from sqlalchemy.exc import DBAPIError

from app.cues.migrations import initialize_database
from app.cues.router import router
from app.cues.service import CueError
from app.database import build_engine


APP_DIR = Path(__file__).resolve().parent
DEFAULT_DB = APP_DIR.parent / "data" / "cues.sqlite3"
ADMIN_DIR = APP_DIR / "admin"
logger = logging.getLogger(__name__)


def error_response(status, code, message, field_errors=None):
    return JSONResponse(
        status_code=status,
        content={"code": code, "message": message, "fieldErrors": field_errors or []},
    )


def create_app(database_path: Path | None = None):
    database = database_path or Path(os.environ.get("CUE_DB_PATH", DEFAULT_DB))

    @asynccontextmanager
    async def lifespan(application):
        engine = build_engine(database)
        try:
            initialize_database(engine)
            application.state.engine = engine
            yield
        finally:
            engine.dispose()

    application = FastAPI(title="Cue Server", version="1.0.0", lifespan=lifespan)

    @application.exception_handler(RequestValidationError)
    async def validation_error(request, exc):
        errors = [
            {"field": str(error["loc"][1]) if len(error["loc"]) > 1 and error["type"] != "json_invalid" else "",
             "message": error["msg"]}
            for error in exc.errors()
        ]
        path = request.url.path
        code = "invalid_show"
        if path.endswith("/cue-order"):
            code = "invalid_order"
        elif "/cues" in path:
            code = "invalid_cue"
        return error_response(422, code, "입력값과 요청 형식을 확인하세요.", errors)

    @application.exception_handler(CueError)
    async def cue_error(_, exc):
        return error_response(exc.status, exc.code, str(exc), exc.field_errors)

    @application.exception_handler(DBAPIError)
    async def storage_error(_, exc):
        logger.error("Cue database operation failed: %s", exc.orig)
        return error_response(503, "storage_unavailable", "DB를 사용할 수 없습니다. 잠시 후 다시 시도하세요.")

    application.include_router(router)
    application.mount("/static", StaticFiles(directory=ADMIN_DIR / "static"), name="static")

    @application.get("/", include_in_schema=False)
    @application.get("/admin", include_in_schema=False)
    def admin():
        return FileResponse(ADMIN_DIR / "index.html", headers={"Cache-Control": "no-cache"})

    return application
