import os

class Settings:
    HOST: str = os.environ.get("HOST", "0.0.0.0")
    PORT: int = int(os.environ.get("PORT", "8000"))
    DATABASE_PATH: str = os.environ.get(
        "DATABASE_PATH",
        os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "data", "hab_telemetry.db")
    )

settings = Settings()
