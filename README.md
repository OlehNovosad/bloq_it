# bloq_it - QR Scanner IoT Gateway

A containerized IoT system for reading QR codes via serial hardware and publishing scan events over MQTT. The project demonstrates a multi-container architecture combining low-level C serial communication with Python-based MQTT message brokering.

---

## 📋 Table of Contents

- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Setup & Execution](#setup--execution)
- [Testing](#testing)
- [MQTT Topics & Commands](#mqtt-topics--commands)
- [Examples](#examples)
- [Design Decisions & Rationale](#design-decisions--rationale)
- [Future Improvements](#future-improvements)

---

### Component Flow

1. **External commands** arrive via MQTT topic `olno/commands`
2. **Gateway container** forwards commands to `olno/dev_cmd`
3. **process.py** receives commands and writes to named pipe → C application
4. **qr_serial (C)** reads from serial port, processes QR data, outputs JSON to stdout
5. **process.py** reads stdout, publishes QR events to `olno/events`

---

## 📁 Project Structure

```
bloq_it/
├── docker-compose.yml          # Multi-container orchestration
├── README.md                   # This file
├── diagrams/
│   └── architecture.md         # Architecture diagram (Mermaid)
│
├── gateway-py/                 # MQTT Gateway Service
│   ├── Dockerfile              # Multi-stage Alpine build
│   ├── main.py                 # MQTT client - forwards commands
│   └── requirements.txt        # Python dependencies (paho-mqtt)
│
└── qr-c/                       # QR Scanner Service
    ├── Dockerfile              # Multi-stage Alpine build (CMake + Python)
    ├── CMakeLists.txt          # CMake build configuration
    ├── CMakePresets.json       # CMake presets
    ├── process.py              # MQTT bridge & QR simulation
    ├── requirements.txt        # Python dependencies (paho-mqtt, pyserial)
    ├── README.md               # Testing workflow documentation
    ├── inc/
    │   └── qr_core.h           # Public API, types, logging macros
    └── src/
        ├── main.c              # Command dispatcher (INIT, PING, START, STOP)
        ├── qr_core.c           # Serial communication & QR handling logic
        └── logging.c           # Timestamped file logging
```

---

## 📦 Prerequisites

- **Docker** (v20.10+) and **Docker Compose** (v2.0+)
- **Linux host** (for serial port access via `network_mode: host`)
- Optional: Physical QR scanner connected via serial (or use mocked setup)

---

## 🚀 Setup & Execution

### 1. Clone the Repository

```bash
git clone <repository-url>
cd bloq_it
```

### 2. Build and Start All Services

```bash
docker compose up --build
```

This will:

- Build the `qr-c` container (CMake C application + Python processor)
- Build the `gateway` container (Python MQTT gateway)
- Start both containers with `network_mode: host`
- Create virtual serial ports using `socat` for hardware mocking

### 3. Stop Services

```bash
docker compose down
```

### 4. View Logs

```bash
# All services
docker compose logs -f

# Specific service
docker compose logs -f qr-c
docker compose logs -f gateway
```

---

## 🧪 Testing

### Hardware Mocking with Socat

The `docker-compose.yml` automatically mocks serial hardware using `socat`:

```bash
socat -d -d pty,raw,echo=0,link=/dev/ttyS1 pty,raw,echo=0,link=/dev/ttyS2
```

This creates a virtual serial port pair:

- `/dev/ttyS1` - Used by the C application
- `/dev/ttyS2` - Used by `process.py` to simulate QR scans

### Testing MQTT with Online Broker

Use the **[MQTT.Cool Test Client](https://testclient-cloud.mqtt.cool/)** to subscribe and publish messages:

1. **Connect** to broker: `test.mosquitto.org` (port `1883`)

2. **Subscribe** to topics:

   ```
   olno/events     # Receive QR scan results
   olno/dev_cmd    # Monitor forwarded commands
   ```

3. **Publish** commands to:

   ```
   olno/commands   # Send commands (INIT, START, STOP, PING)
   ```

### Test Sequence

1. **Initialize the QR device:**

   ```
   Topic: olno/commands
   Payload: INIT
   ```

   Expected response on `olno/events`: `OK`

2. **Health check:**

   ```
   Topic: olno/commands
   Payload: PING
   ```

   Expected response on `olno/events`: `PONG`

3. **Start QR scan:**

   ```
   Topic: olno/commands
   Payload: START
   ```

   Expected response on `olno/events` (after ~5s simulated delay):

   ```json
   {"qr-data": {"code":"SIMULATED_QR_CODE_123456","ts":1736505600}}
   ```

4. **Stop scan before timeout:**

   ```
   Topic: olno/commands
   Payload: STOP
   ```

   Expected response on `olno/events`: `OK`

### Local Testing (Without Docker)

#### Testing the C Application Directly

```bash
cd qr-c

# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Set environment variables
export SERIAL_PORT=/dev/ttyS1
export SERIAL_BAUD=115200
export READ_TIMEOUT_MS=10000

# Run with manual input
echo -e "INIT\nPING\nSTART" | ./build/qr_serial
```

#### macOS/Linux Virtual Serial Testing

See [qr-c/README.md](qr-c/README.md) for detailed instructions on TCP bridge testing.

---

## 📡 MQTT Topics & Commands

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `olno/commands` | Subscribe (Gateway) | External command entry point |
| `olno/dev_cmd` | Publish (Gateway) → Subscribe (qr-c) | Internal command forwarding |
| `olno/events` | Publish (qr-c) | QR scan results and responses |

### Supported Commands

| Command | Description | Response |
|---------|-------------|----------|
| `INIT` | Initialize serial port and QR subsystem | `OK` or error |
| `PING` | Health check | `PONG` |
| `START` | Begin QR scan (blocks until data/timeout/stop) | JSON with QR data |
| `STOP` | Abort ongoing scan | `OK` |

---

## 💡 Examples

### Example 1: Full Scan Cycle

```bash
# Using mosquitto_pub/sub CLI tools

# Terminal 1: Subscribe to events
mosquitto_sub -h test.mosquitto.org -t "olno/events" -v

# Terminal 2: Send commands
mosquitto_pub -h test.mosquitto.org -t "olno/commands" -m "INIT"
# Output: olno/events OK

mosquitto_pub -h test.mosquitto.org -t "olno/commands" -m "START"
# Output (after 5s): olno/events {"qr-data": {"code":"SIMULATED_QR_CODE_123456","ts":1736505600}}
```

### Example 2: Abort Scan

```bash
# Start scan
mosquitto_pub -h test.mosquitto.org -t "olno/commands" -m "START"

# Immediately stop
mosquitto_pub -h test.mosquitto.org -t "olno/commands" -m "STOP"
# Output: olno/events OK (scan aborted, no QR data)
```

### Example 3: QR Scan Response Format

```json
{
  "qr-data": {
    "code": "SIMULATED_QR_CODE_123456",
    "ts": 1736505600
  }
}
```

### Example 4: Timeout Response

If no QR code is scanned within `READ_TIMEOUT_MS`:

```json
{
  "qr-data": {
    "code": "TIMEOUT",
    "ts": 1736505610
  }
}
```

---

## 🎯 Design Decisions & Rationale

### 1. **C for Serial Communication**

- **Why:** Low-level serial I/O requires precise timing and minimal latency. C provides direct access to POSIX termios APIs.
- **Benefit:** Deterministic behavior, small memory footprint, suitable for embedded deployment.

### 2. **Python for MQTT Bridging**

- **Why:** Rapid development, excellent MQTT library support (paho-mqtt), easy JSON handling.
- **Benefit:** Faster iteration on protocol logic while C handles hardware.

### 3. **Pipe-Based IPC (stdin/stdout)**

- **Why:** Simple, UNIX-standard approach. C app reads commands from stdin, writes JSON to stdout.
- **Benefit:** Decouples components, enables easy testing, allows replacement of either component.

### 4. **Named Pipes for Command Forwarding**

- **Why:** MQTT messages arrive in Python; commands must reach the C application.
- **Benefit:** Non-blocking writes, standard FIFO semantics, no network overhead.

### 5. **Socat for Virtual Serial Ports**

- **Why:** Hardware QR scanners may not be available in development/testing environments.
- **Benefit:** Full end-to-end testing without physical hardware. Simulates realistic serial behavior.

### 6. **Multi-Stage Docker Builds**

- **Why:** Separate build-time dependencies (cmake, build-base) from runtime.
- **Benefit:** Minimal final image size (~50MB vs ~500MB), faster deployments, reduced attack surface.

### 7. **Alpine Linux Base Image**

- **Why:** Lightweight (5MB base), musl libc compatible with our use case.
- **Benefit:** Fast container startup, minimal resource usage.

### 8. **Host Network Mode**

- **Why:** Direct access to serial devices (`/dev/tty*`).
- **Benefit:** No complex device mapping; container behaves like host process for I/O.

### 9. **Thread-Safe Command Handling**

- **Why:** START command blocks during scan; must still receive STOP command.
- **Benefit:** Mutex-protected operations, pipe-based interruption for clean abort.

### 10. **Public MQTT Broker for Testing**

- **Why:** `test.mosquitto.org` requires no authentication, enables quick testing.
- **Benefit:** Zero setup for development. TLS support prepared but commented out.

---

## 🔮 Future Improvements

### Short-Term

- [ ] **TLS/SSL support** - Enable secure MQTT connections (certificates prepared in `certs/` directory)
- [ ] **Authentication** - Add MQTT username/password support
- [ ] **Health endpoint** - HTTP health check for container orchestration
- [ ] **Structured logging** - JSON-formatted logs for log aggregation systems

### Medium-Term

- [ ] **Configuration file** - YAML/JSON config instead of environment variables
- [ ] **Multiple scanner support** - Handle multiple serial devices simultaneously
- [ ] **Command queuing** - Buffer commands when device is busy
- [ ] **Retry logic** - Automatic reconnection with exponential backoff
- [ ] **Metrics endpoint** - Prometheus metrics for monitoring scan latency, success rate

### Long-Term

- [ ] **gRPC interface** - Alternative to MQTT for internal services
- [ ] **Web dashboard** - Real-time scan visualization
- [ ] **Database integration** - Persist scan history
- [ ] **OTA updates** - Remote firmware updates for C application
- [ ] **Kubernetes deployment** - Helm charts with proper device plugin support

### Code Quality

- [ ] **Unit tests** - Unit test for C, pytest for Python
- [ ] **Integration tests** - Docker-based end-to-end test suite
- [ ] **CI/CD pipeline** - GitHub Actions for automated builds and tests
- [ ] **Static analysis** - cppcheck, pylint, mypy integration
