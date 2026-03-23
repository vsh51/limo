FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake make g++ git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY CMakeLists.txt CMakePresets.json ./
COPY libs/ ./libs/

RUN cmake -S . -B /build \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIMO_BUILD_TESTS=OFF \
        -DLIMO_BUILD_DOCS=OFF \
        -DLIMO_BUILD_CLI=OFF \
    || true

COPY src/ ./src/
RUN cmake -S . -B /build \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIMO_BUILD_TESTS=OFF \
        -DLIMO_BUILD_DOCS=OFF \
        -DLIMO_BUILD_CLI=ON \
    && cmake --build /build --target limo -j"$(nproc)"


FROM python:3.12-slim AS runtime

LABEL org.opencontainers.image.title="LIMO" \
      org.opencontainers.image.description="Linear Method Optimizer — async solver with web UI" \
      org.opencontainers.image.source="https://github.com/vsh51/limo"

RUN groupadd --gid 1000 limo \
    && useradd --uid 1000 --gid limo --create-home limo

WORKDIR /app

COPY web/requirements.txt ./web/requirements.txt
RUN pip install --no-cache-dir -r ./web/requirements.txt

COPY --from=builder /build/bin/limo ./limo
RUN chmod +x ./limo

COPY web/        ./web/
COPY limo_io/    ./limo_io/

RUN chown -R limo:limo /app
USER limo

ENV LIMO_BINARY=/app/limo \
    PYTHONUNBUFFERED=1

EXPOSE 5050

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD ["python3", "-c", "import urllib.request; urllib.request.urlopen('http://localhost:5050/api/status')"]

CMD gunicorn \
     --workers  "${GUNICORN_WORKERS:-1}" \
     --threads  "${GUNICORN_THREADS:-2}" \
     --bind     0.0.0.0:5050 \
     --timeout  30 \
     --keep-alive 5 \
     --access-logfile - \
     --error-logfile  - \
     web:flask_app
