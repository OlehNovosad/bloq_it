import json
import logging
import sys
import threading
import time

import serial
from paho.mqtt import client as mqtt_client

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)


class MQTTClientManager:
    TOPIC_DEV_CMD = "olno/dev_cmd"
    TOPIC_EVENTS = "olno/events"

    def __init__(
            self,
            broker: str = "test.mosquitto.org",
            port: int = 1883,
            serial_port: str = "/dev/ttyS2",
            qr_sim_port: str = "/dev/ttyQR2",
    ):
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
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message

    def on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            logging.info(f"Successfully connected to {self.broker}")
            client.subscribe(self.TOPIC_DEV_CMD)
        else:
            logging.error(f"Connection failed with code {rc}")

    def on_disconnect(self, client, userdata, disconnect_flags, rc, properties=None):
        logging.warning(f"Disconnected from MQTT broker (rc: {rc}). Retrying...")

    def on_message(self, client, userdata, msg):
        command = msg.payload.decode()
        logging.info(f"Received `{command}` from `{msg.topic}` topic")

        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.write(f"{command}\n".encode())
            self.serial_conn.flush()

            if command == "START":
                self.simulate_qr_scan("SIMULATED_QR_12345")

    def simulate_qr_scan(self, data: str, delay: float = 0.5):
        """Simulate a QR scan by writing to the QR simulator port."""

        def _send():
            time.sleep(delay)
            if self.qr_sim_conn and self.qr_sim_conn.is_open:
                self.qr_sim_conn.write(f"{data}\n".encode())
                self.qr_sim_conn.flush()
                logging.info(f"Simulated QR scan: {data}")

        threading.Thread(target=_send, daemon=True).start()

    def connect(self):
        try:
            self.serial_conn = serial.Serial(
                self.serial_port, baudrate=115200, timeout=1
            )
            logging.info(f"Serial port {self.serial_port} opened")

            self.qr_sim_conn = serial.Serial(
                self.qr_sim_port, baudrate=115200, timeout=1
            )
            logging.info(f"QR simulator port {self.qr_sim_port} opened")

            self.client.reconnect_delay_set(min_delay=1, max_delay=60)
            self.client.connect(self.broker, self.port, keepalive=60)
            self.client.loop_start()
        except Exception as e:
            logging.error(f"Connection failed: {e}")

    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()
        if self.serial_conn:
            self.serial_conn.close()
        if self.qr_sim_conn:
            self.qr_sim_conn.close()
        logging.info("Connections closed.")

    def publish(self, topic: str, payload: str):
        if not self.client.is_connected():
            logging.warning("Not connected to broker. Message dropped.")
            return

        result = self.client.publish(topic, payload, qos=1)
        result.wait_for_publish()


def process_line(line: str) -> str:
    try:
        data = json.loads(line)
        qr_data = data.get("qr-data", {})
        logging.info(f"JSON Received - Code: {qr_data.get('code')}")
        return json.dumps(data)
    except (json.JSONDecodeError, TypeError):
        logging.info(f"Non JSON Received - {line}")
        return line


def main():
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
