# ------------------------------
# Build Python dependencies
# ------------------------------
FROM alpine:3.21 AS builder

# Install Python and pip
RUN apk add --no-cache python3 py3-pip

WORKDIR /app

# Create a virtual environment for isolated dependencies
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

# Copy requirements and install Python dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt


# ------------------------------
# Final lightweight image
# ------------------------------
FROM alpine:3.21

# Install runtime Python and create a non-root user
RUN apk add --no-cache python3 \
    && adduser -D -u 1001 qruser

WORKDIR /app

# Copy virtual environment and application code from builder
COPY --from=builder /opt/venv /opt/venv
COPY main.py .

# Ensure the app uses the virtual environment and unbuffered output
ENV PATH="/opt/venv/bin:$PATH" \
    PYTHONUNBUFFERED=1

# Switch to non-root user
USER qruser

# Run the Python application
ENTRYPOINT ["python", "main.py"]
