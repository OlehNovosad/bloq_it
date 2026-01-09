import json
import logging
import sys
import threading
import time
import ssl
from pathlib import Path
from time import sleep

import serial
from paho.mqtt import client as mqtt_client

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)


class MQTTClientManager:
    """
    Manages MQTT communication and serial interaction with the QR device.

    Responsibilities:
    - Connect to an MQTT broker
    - Subscribe to device command topics
    - Forward commands to the QR device via serial
    - Simulate QR scan responses when requested
    - Publish QR scan events back to MQTT
    """

    TOPIC_DEV_CMD = "olno/dev_cmd"
    """MQTT topic used to receive device commands."""

    TOPIC_EVENTS = "olno/events"
    """MQTT topic used to publish QR scan events."""

    # Return path to qr-c directory
    BASE_DIR = Path(__file__).resolve().parent
    """Base directory of the application."""

    # Get the path to the cert directory
    CERTS_DIR = BASE_DIR / ".." / "certs"
    """Directory containing TLS certificates."""

    CA_CERT = str(CERTS_DIR / "ca.crt")
    CLIENT_CERT = str(CERTS_DIR / "client.crt")
    CLIENT_KEY = str(CERTS_DIR / "client.key")

    def __init__(
            self,
            broker: str = "test.mosquitto.org",
            port: int = 1883,
            serial_port: str = "/dev/ttyS2",
            qr_sim_port: str = "/dev/ttyQR2",
    ):
        """
        Initialize the MQTT client manager.

        :param broker: MQTT broker hostname
        :param port: MQTT broker port
        :param serial_port: Serial port connected to the QR C application
        :param qr_sim_port: Serial port used to simulate QR scans
        """
        self.broker = broker
        self.port = port
        self.client_id = "qr-c_process"
        self.serial_port = serial_port
        self.qr_sim_port = qr_sim_port
        self.serial_conn = None
        self.qr_sim_conn = None

        self.client = mqtt_client.Client(
            mqtt_client.CallbackAPIVersion.VERSION2,
            client_id=self.client_id,
        )

        """ 
        Uncomment to enable TLS/SSL (broker port must match)
        Currently disabled for testing with public broker
        """
        # self.client.tls_set(
        #     ca_certs=self.CA_CERT,
        #     certfile=self.CLIENT_CERT,
        #     keyfile=self.CLIENT_KEY,
        #     cert_reqs=ssl.CERT_REQUIRED
        # )

        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        """
        MQTT callback executed on successful or failed connection.

        Subscribes to the device command topic on success.
        """
        if rc == 0:
            logging.info(f"Successfully connected to {self.broker}")
            client.subscribe(self.TOPIC_DEV_CMD)
        else:
            logging.error(f"Connection failed with code {rc}")

    def _on_disconnect(self, client, userdata, disconnect_flags, rc, properties=None):
        """
        MQTT callback executed when the client disconnects from the broker.
        """
        logging.warning(f"Disconnected from MQTT broker (rc: {rc}). Retrying...")

    def _on_message(self, client, userdata, msg):
        """
        MQTT callback executed when a message is received.

        Forwards received commands to the QR device via serial.
        """
        command = msg.payload.decode()
        logging.info(f"Received `{command}` from `{msg.topic}` topic")

        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.write(f"{command}\n".encode())
            self.serial_conn.flush()

            if command == "START":
                """
                Simulate a QR scan after a delay when START command is received.
                To be able to send STOP command before the simulated scan,
                the simulation is run in a separate thread.
                
                Ensure that READ_TIMEOUT_MS from env is longer than this delay. 
                Recommended to set READ_TIMEOUT_MS to at least 10000 ms.
                """
                sleep(5)
                self.__simulate_qr_scan("SIMULATED_QR_12345")

    def __simulate_qr_scan(self, data: str, delay: float = 0.5):
        """
        Simulates a QR scan by writing data to the QR simulator serial port.

        :param data: QR code payload to simulate
        :param delay: Delay before sending the simulated scan (seconds)
        """

        def _send():
            time.sleep(delay)
            if self.qr_sim_conn and self.qr_sim_conn.is_open:
                self.qr_sim_conn.write(f"{data}\n".encode())
                self.qr_sim_conn.flush()
                logging.info(f"Simulated QR scan: {data}")

        threading.Thread(target=_send, daemon=True).start()

    def __connect_serial(self):
        """
        Opens serial connections for both the QR device and QR simulator.
        """
        self.serial_conn = serial.Serial(
            self.serial_port, baudrate=115200, timeout=1
        )
        logging.info(f"Serial port {self.serial_port} opened")

        self.qr_sim_conn = serial.Serial(
            self.qr_sim_port, baudrate=115200, timeout=1
        )
        logging.info(f"QR simulator port {self.qr_sim_port} opened")

    def connect(self):
        """
        Establishes serial and MQTT connections.
        """
        try:
            self.__connect_serial()
            self.client.reconnect_delay_set(min_delay=1, max_delay=60)
            self.client.connect(self.broker, self.port, keepalive=60)
            self.client.loop_start()
        except Exception as e:
            logging.error(f"Connection failed: {e}")

    def disconnect(self):
        """
        Closes MQTT and serial connections.
        """
        self.client.loop_stop()
        self.client.disconnect()
        if self.serial_conn:
            self.serial_conn.close()
        if self.qr_sim_conn:
            self.qr_sim_conn.close()
        logging.info("Connections closed.")

    def publish(self, topic: str, payload: str):
        """
        Publishes a message to an MQTT topic.

        :param topic: MQTT topic
        :param payload: Message payload
        """
        if not self.client.is_connected():
            logging.warning("Not connected to broker. Message dropped.")
            return

        result = self.client.publish(topic, payload, qos=1)
        result.wait_for_publish()


def process_line(line: str) -> str:
    """
    Processes a single line of input.

    Attempts to parse JSON-formatted QR data and logs extracted values.
    Falls back to returning the raw line if parsing fails.

    :param line: Input line from stdin
    :return: JSON string or raw line
    """
    try:
        data = json.loads(line)
        qr_data = data.get("qr-data", {})
        logging.info(f"JSON Received - Code: {qr_data.get('code')}")
        return json.dumps(data)
    except (json.JSONDecodeError, TypeError):
        logging.info(f"Non JSON Received - {line}")
        return line


def main():
    """
    Application entry point.

    Reads output from the QR C application via stdin and publishes
    QR scan events to MQTT.
    """
    client = MQTTClientManager()
    client.connect()

    try:
        for line in sys.stdin:
            clean_line = line.strip()
            if not clean_line:
                continue

            payload = process_line(clean_line)
            client.publish(client.TOPIC_EVENTS, payload)
    except KeyboardInterrupt:
        logging.info("Process interrupted by user.")
    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
