import sys
import json
import logging
import time
from paho.mqtt import client as mqtt_client

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


class MQTTClientManager:
    TOPIC_DEV_CMD = "olno/dev_cmd"
    TOPIC_EVENTS = "olno/events"

    def __init__(self, broker: str = "test.mosquitto.org", port: int = 1883):
        self.broker = broker
        self.port = port
        self.client_id = "qr-c_process"

        self.client = mqtt_client.Client(
            mqtt_client.CallbackAPIVersion.VERSION2,
            client_id=self.client_id
        )

        # Define callbacks for reconnection logic
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message

    def on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            logging.info(f"Successfully connected to {self.broker}")

            client.subscribe(MQTTClientManager.TOPIC_DEV_CMD)
        else:
            logging.error(f"Connection failed with code {rc}")

    def on_disconnect(self, client, userdata, disconnect_flags, rc, properties=None):
        logging.warning(f"Disconnected from MQTT broker (rc: {rc}). Retrying in 5 seconds...")

    def on_message(self, client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    def connect(self):
        try:
            # Set reconnection delay (min/max seconds)
            self.client.reconnect_delay_set(min_delay=1, max_delay=60)
            self.client.connect(self.broker, self.port, keepalive=60)

            self.client.loop_start()
        except Exception as e:
            logging.error(f"Initial connection failed: {e}")

    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()
        logging.info("MQTT connection closed.")

    def publish(self, topic, payload: str):
        # We check is_connected() to avoid trying to publish while offline
        if not self.client.is_connected():
            logging.warning("Not connected to broker. Message dropped.")
            return

        result = self.client.publish(topic, payload, qos=1)
        result.wait_for_publish()


def process_line(line: str) -> str:
    try:
        data = json.loads(line)
        qr_data = data.get('qr-data', {})
        logging.info(f"JSON Received - Code: {qr_data.get('code')}")
        return json.dumps(data)
    except (json.JSONDecodeError, TypeError):
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
