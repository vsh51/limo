# ── Stage 1: Build the C++ binary ────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake make g++ git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy build system first for better layer caching
COPY CMakeLists.txt CMakePresets.json ./
COPY libs/ ./libs/

# Fetch dependencies in a separate layer (rarely changes)
RUN cmake -S . -B /build \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIMO_BUILD_TESTS=OFF \
        -DLIMO_BUILD_DOCS=OFF \
        -DLIMO_BUILD_CLI=OFF \
    || true

# Now copy source and build the binary
COPY src/ ./src/
RUN cmake -S . -B /build \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIMO_BUILD_TESTS=OFF \
        -DLIMO_BUILD_DOCS=OFF \
    && cmake --build /build --target limo -j"$(nproc)"


# ── Stage 2: Python runtime + binary ──────────────────────────────────────────
FROM python:3.12-slim AS runtime

LABEL org.opencontainers.image.title="LIMO" \
      org.opencontainers.image.description="Linear Method Optimizer — async solver with web UI" \
      org.opencontainers.image.source="https://github.com/vsh51/limo"

# Create non-root user for security
RUN groupadd --gid 1000 limo \
    && useradd --uid 1000 --gid limo --create-home limo

WORKDIR /app

# Install Python dependencies first (better layer caching)
COPY web/requirements.txt ./web/requirements.txt
RUN pip install --no-cache-dir -r ./web/requirements.txt

# Copy compiled solver
COPY --from=builder /build/bin/limo ./limo
RUN chmod +x ./limo

# Copy Python application
COPY web/        ./web/
COPY limo_io/    ./limo_io/

# Switch to non-root user
RUN chown -R limo:limo /app
USER limo

ENV LIMO_BINARY=/app/limo \
    PYTHONUNBUFFERED=1

EXPOSE 5050

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD ["python3", "-c", "import urllib.request; urllib.request.urlopen('http://localhost:5050/api/status')"]

CMD ["gunicorn", \
     "--workers",       "2",               \
     "--threads",       "4",               \
     "--bind",          "0.0.0.0:5050",    \
     "--timeout",       "30",              \
     "--keep-alive",    "5",               \
     "--access-logfile","-",               \
     "--error-logfile", "-",               \
     "web.server:app"]
