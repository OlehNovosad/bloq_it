# -------- Build stage --------
FROM alpine:3.21 AS builder

RUN apk add --no-cache \
    build-base \
    cmake \
    git

WORKDIR /build

COPY CMakeLists.txt .
COPY src ./src
COPY inc ./inc

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build

# -------- Runtime stage --------
FROM alpine:3.21

RUN apk add --no-cache libstdc++ socat

WORKDIR /app

COPY --from=builder /build/build/qr_serial /app/qr_serial

RUN adduser -D -u 1001 qruser

ENV SERIAL_PORT=/dev/ttyS1
ENV SERIAL_BAUD=9600
ENV READ_TIMEOUT_MS=5000

ENTRYPOINT ["/app/qr_serial"]
