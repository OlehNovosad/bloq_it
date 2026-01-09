# Stage 1: Build dependencies
FROM alpine:3.21 AS builder

RUN apk add --no-cache python3 py3-pip

WORKDIR /app

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Stage 2: Final lightweight image
FROM alpine:3.21

RUN apk add --no-cache python3 \
    && adduser -D -u 1001 qruser

WORKDIR /app

COPY --from=builder /opt/venv /opt/venv
COPY main.py .

# Ensure the app uses the virtual environment automatically
ENV PATH="/opt/venv/bin:$PATH" \
    PYTHONUNBUFFERED=1

USER qruser

ENTRYPOINT ["python", "main.py"]
