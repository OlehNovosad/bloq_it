import sys
import json
import logging
from paho.mqtt import client as mqtt_client

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


class MQTTClientManager:
    def __init__(self, broker: str = "test.mosquitto.org", port: int = 1883, topic: str = "olno/events"):
        self.broker = broker
        self.port = port
        self.topic = topic
        self.client_id = "qr-c_process"

        self.client = mqtt_client.Client(
            mqtt_client.CallbackAPIVersion.VERSION2,
            client_id=self.client_id
        )

    def connect(self):
        try:
            self.client.connect(self.broker, self.port)
            self.client.loop_start()
            logging.info(f"Connected to {self.broker} on topic {self.topic}")
        except Exception as e:
            logging.error(f"Failed to connect to MQTT broker: {e}")
            raise

    def disconnect(self):
        """Explicitly stop the loop and disconnect."""
        self.client.loop_stop()
        self.client.disconnect()
        logging.info("MQTT connection closed.")

    def publish(self, payload: str):
        result = self.client.publish(self.topic, payload)
        if result.rc != mqtt_client.MQTT_ERR_SUCCESS:
            logging.warning(f"Failed to send message: {result.rc}")


def process_line(line: str) -> str:
    try:
        data = json.loads(line)

        qr_data = data.get('qr-data', {})
        code = qr_data.get('code')
        ts = qr_data.get('ts')

        logging.info(f"JSON Received - Code: {code}, TS: {ts}")
        return json.dumps(data)
    except (json.JSONDecodeError, TypeError):
        logging.info(f"Non-JSON input received: {line}")
        return line


def main():
    client = MQTTClientManager()
    try:
        client.connect()
        for line in sys.stdin:
            clean_line = line.strip()
            if not clean_line:
                continue

            payload = process_line(clean_line)
            client.publish(payload)

    except KeyboardInterrupt:
        logging.info("Process interrupted by user.")
    except Exception as e:
        logging.error(f"Unexpected error: {e}")
    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
