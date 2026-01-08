### 🧪 Testing Workflow (macOS/Linux)

Due to macOS limitations with direct PTY mapping in Docker, we use a TCP bridge to connect your Mac terminal to the
container's virtual serial port.

---

#### Step 1: Start the Host Listener

On your Mac, open a terminal to create a TCP server. This provides a virtual port `/tmp/ttyV_TEST` for interacting with
the application.

```bash
socat -d -d PTY,link=/tmp/ttyV_TEST,raw,echo=0 TCP-LISTEN:9999,reuseaddr,bind=0.0.0.0
```

> **Note:** Keep this terminal open to monitor the bridge connection.

---

#### Step 2: Run the Docker Container

Open a second terminal and launch the container with the following command. It installs `socat` inside the container,
ensures proper log directory permissions, and bridges the internal `/dev/ttyS1` to your host.

```bash
docker run -it --rm \
    --user root \
    --entrypoint /bin/sh \
    qr-serial-app \
    -c "apk add --no-cache socat && \
        mkdir -p logs && chmod 777 logs && \
        socat -d -d PTY,link=/dev/ttyS1,raw,echo=0 TCP:host.docker.internal:9999 & \
        sleep 2 && \
        /app/qr_serial | python3 process.py"
```

---

#### Step 3: Interaction and Commands

Open a third terminal to simulate hardware input.

**Monitor Serial Output**
View the data sent by the application:

```bash
cat /tmp/ttyV_TEST
```

**Send Serial Commands**
Send QR data or trigger app logic:

```bash
# Simulate QR code input
echo "ABC123" > /tmp/ttyV_TEST
```