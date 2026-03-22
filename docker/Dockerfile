# ── Stage 1: Build the C++ binary ────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake make g++ git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy only the files cmake needs so layer cache is reused on source changes
COPY CMakeLists.txt CMakePresets.json ./
COPY src/   ./src/
COPY libs/  ./libs/

RUN cmake -S . -B /build \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIMO_BUILD_TESTS=OFF \
        -DLIMO_BUILD_DOCS=OFF \
    && cmake --build /build --target limo -j"$(nproc)"


# ── Stage 2: Python runtime + binary ────────────────────────────────────────
FROM python:3.12-slim

WORKDIR /app

# Copy compiled solver
COPY --from=builder /build/bin/limo ./limo
RUN chmod +x ./limo

# Copy Python application
COPY web/        ./web/
COPY limo_io/    ./limo_io/

# Install Python dependencies
RUN pip install --no-cache-dir -r ./web/requirements.txt

ENV LIMO_BINARY=/app/limo \
    PYTHONUNBUFFERED=1

EXPOSE 5050

CMD ["gunicorn", \
     "--workers",       "2",               \
     "--threads",       "4",               \
     "--bind",          "0.0.0.0:5050",    \
     "--timeout",       "30",              \
     "--keep-alive",    "5",               \
     "--access-logfile","-",               \
     "--error-logfile", "-",               \
     "web.server:app"]
