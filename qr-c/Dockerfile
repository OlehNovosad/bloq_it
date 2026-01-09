# ------------------------------
# Build Stage
# ------------------------------
FROM alpine:3.21 AS builder

# Install build tools and CMake
RUN apk add --no-cache build-base cmake git

WORKDIR /build

# Copy CMake project files
COPY CMakeLists.txt .
COPY src ./src
COPY inc ./inc

# Build the QR C application
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build


# ------------------------------
# Runtime Stage
# ------------------------------
FROM alpine:3.21

# Install runtime dependencies: C++ lib and Python
RUN apk add --no-cache libstdc++ python3 py3-pip

WORKDIR /app

# Copy compiled binary and Python processor
COPY --from=builder /build/build/qr_serial /app/qr_serial
COPY requirements.txt /app/requirements.txt
COPY process.py /app/process.py

# Create a non-root user
RUN adduser -D -u 1001 qruser

# Install Python dependencies
RUN pip3 install --no-cache-dir --break-system-packages -r /app/requirements.txt

# Set default environment variables
ENV SERIAL_PORT=/dev/ttyS1
ENV SERIAL_BAUD=9600
ENV READ_TIMEOUT_MS=10000
ENV PYTHONUNBUFFERED=1

# Entry point: run C app piped into Python processor
ENTRYPOINT ["sh", "-c", "/app/qr_serial | python3 /app/process.py"]
