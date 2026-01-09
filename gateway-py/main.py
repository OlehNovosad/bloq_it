import logging

from paho.mqtt import client as mqtt_client

logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s"
)


class MQTTClientManager:
    TOPIC_EVENTS = "olno/events"
    TOPIC_COMMANDS = "olno/commands"
    TOPIC_DEV_CMD = "olno/dev_cmd"

    def __init__(self, broker: str = "test.mosquitto.org", port: int = 1883):
        self.broker = broker
        self.port = port
        self.client_id = "gateway-py_process"

        self.client = mqtt_client.Client(
            mqtt_client.CallbackAPIVersion.VERSION2, client_id=self.client_id
        )

        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message

    def on_connect(self, client, userdata, flags, rc, properties):
        if rc == 0:
            logging.info("Successfully connected to broker")

            client.subscribe(MQTTClientManager.TOPIC_EVENTS)
            client.subscribe(MQTTClientManager.TOPIC_COMMANDS)
        else:
            logging.error(f"Connection failed with code {rc}")

    def on_disconnect(self, client, userdata, disconnect_flags, rc, properties):
        logging.warning(f"Disconnected from broker (RC: {rc}). Retrying...")

    def on_message(self, client, userdata, msg):
        if msg.topic == MQTTClientManager.TOPIC_COMMANDS:
            self.client.publish(MQTTClientManager.TOPIC_DEV_CMD, msg.payload)
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    def run(self):
        logging.info(f"Connecting to {self.broker}...")
        try:
            self.client.connect_async(self.broker, self.port)
            self.client.loop_forever(retry_first_connection=True)
        except Exception as e:
            logging.error(f"Fatal error in network loop: {e}")


def main():
    manager = MQTTClientManager()
    try:
        manager.run()
    except KeyboardInterrupt:
        logging.info("Shutting down...")


if __name__ == "__main__":
    main()
