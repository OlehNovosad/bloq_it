import logging
from pathlib import Path

from paho.mqtt import client as mqtt_client

logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s"
)


class MQTTClientManager:
    """
    MQTT gateway client manager for forwarding commands and events.

    Responsibilities:
    - Connect to an MQTT broker.
    - Subscribe to events and command topics.
    - Forward commands from the gateway to the QR device command topic.
    - Provide TLS/SSL configuration for secure connections (optional).
    """

    TOPIC_EVENTS = "olno/events"
    """MQTT topic for receiving events from other services."""

    TOPIC_COMMANDS = "olno/commands"
    """MQTT topic for receiving control commands."""

    TOPIC_DEV_CMD = "olno/dev_cmd"
    """MQTT topic used to forward commands to the QR device."""

    # Base and certificate directories
    BASE_DIR = Path(__file__).resolve().parent
    """Base directory of the application."""

    CERTS_DIR = BASE_DIR / ".." / "certs"
    """Directory containing TLS/SSL certificates."""

    CA_CERT = str(CERTS_DIR / "ca.crt")
    CLIENT_CERT = str(CERTS_DIR / "client.crt")
    CLIENT_KEY = str(CERTS_DIR / "client.key")

    def __init__(self, broker: str = "test.mosquitto.org", port: int = 1883):
        """
        Initialize the MQTT gateway client.

        :param broker: Hostname of the MQTT broker
        :param port: Port number of the MQTT broker
        """
        self.broker = broker
        self.port = port
        self.client_id = "gateway-py_process"

        self.client = mqtt_client.Client(
            mqtt_client.CallbackAPIVersion.VERSION2, client_id=self.client_id
        )

        """ 
        Uncomment to enable TLS/SSL (broker port must match).
        Currently disabled for testing with public broker.
        """
        # self.client.tls_set(
        #     ca_certs=self.CA_CERT,
        #     certfile=self.CLIENT_CERT,
        #     keyfile=self.CLIENT_KEY,
        #     cert_reqs=ssl.CERT_REQUIRED
        # )

        # Assign callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

    def _on_connect(self, client, userdata, flags, rc, properties):
        """
        MQTT callback when the client connects to the broker.

        Subscribes to events and command topics on successful connection.

        :param client: MQTT client instance
        :param userdata: User data (not used)
        :param flags: Connection flags
        :param rc: Connection result code
        :param properties: MQTT v5 properties (optional)
        """
        if rc == 0:
            logging.info("Successfully connected to broker")
            client.subscribe(MQTTClientManager.TOPIC_EVENTS)
            client.subscribe(MQTTClientManager.TOPIC_COMMANDS)
        else:
            logging.error(f"Connection failed with code {rc}")

    def _on_disconnect(self, client, userdata, disconnect_flags, rc, properties):
        """
        MQTT callback executed when the client disconnects.

        :param client: MQTT client instance
        :param userdata: User data (not used)
        :param disconnect_flags: Disconnect flags
        :param rc: Disconnect result code
        :param properties: MQTT v5 properties (optional)
        """
        logging.warning(f"Disconnected from broker (RC: {rc}). Retrying...")

    def _on_message(self, client, userdata, msg):
        """
        MQTT callback executed when a message is received.

        Forwards commands from the gateway to the QR device topic.

        :param client: MQTT client instance
        :param userdata: User data (not used)
        :param msg: MQTT message object containing topic and payload
        """
        if msg.topic == MQTTClientManager.TOPIC_COMMANDS:
            self.client.publish(MQTTClientManager.TOPIC_DEV_CMD, msg.payload)
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    def run(self):
        """
        Starts the MQTT client network loop.

        Connects asynchronously to the broker and processes messages indefinitely.
        Retries first connection automatically and logs fatal errors.
        """
        logging.info(f"Connecting to {self.broker}...")
        try:
            self.client.connect_async(self.broker, self.port)
            self.client.loop_forever(retry_first_connection=True)
        except Exception as e:
            logging.error(f"Fatal error in network loop: {e}")


def main():
    """
    Entry point for the MQTT gateway application.

    Initializes the MQTTClientManager and starts its network loop.
    Handles KeyboardInterrupt to gracefully shut down the client.
    """
    manager = MQTTClientManager()
    try:
        manager.run()
    except KeyboardInterrupt:
        logging.info("Shutting down...")


if __name__ == "__main__":
    main()
