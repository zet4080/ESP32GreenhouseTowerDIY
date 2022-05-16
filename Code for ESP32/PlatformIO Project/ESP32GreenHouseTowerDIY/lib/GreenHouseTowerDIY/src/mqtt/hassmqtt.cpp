#include "hassmqtt.hpp"

long lastReconnectAttempt = 0;

// Variables for MQTT
#define MQTT_SECURE_ENABrelay 0
#define MQTT_PORT 1883
#define MQTT_PORT_SECURE 8883
#define MQTT_MAX_TRANSFER_SIZE 1024
#define MQTT_KEEPALIVE 60
#define MQTT_RECONNECT_TIMEOUT 10000
#define MQTT_RECONNECT_RETRY_TIMEOUT 1000
#define MQTT_RECONNECT_RETRY_COUNT 3

/**
 * @brief Initialize the MQTT client
 * @param clientId The client ID to use
 * @param host The host to connect to
 * @param port The port to connect to
 * @param secure Whether to use TLS or not
 * @param callback The callback to use
 * <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
 * <component>: One of the supported MQTT components, eg. binary_sensor.
 * <node_id> (Optional): ID of the node providing the topic, this is not used by Home Assistant but may be used to structure the MQTT topic.
 * The ID of the node must only consist of characters from the character class [a-zA-Z0-9_-] (alphanumerics, underscore and hyphen).
 * <object_id>: The ID of the device. This is only to allow for separate topics for each device and is not used for the entity_id.
 * The ID of the device must only consist of characters from the character class [a-zA-Z0-9_-] (alphanumerics, underscore and hyphen).
 * Best practice for entities with a unique_id is to set <object_id> to unique_id and omit the <node_id>.
 **/
#define MQTT_DISCOVERY_PREFIX "homeassistant/"

bool mqttProcessing = false;

unsigned long lastReadAt = millis();
unsigned long lastAvailabilityToggleAt = millis();
bool lastInputState = false;

#if !ENABLE_MDNS_SUPPORT
#define BROKER_ADDR IPAddress(192, 168, 0, 17) // IP address of the MQTT broker - change to your broker IP address or enable MDNS support
#endif                                         // !ENABLE_MDNS_SUPPORT

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);
HASwitch relay("pump_relay", false); // is unique ID.

enum class MQTT_STATE
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
};

HASSMQTT::HASSMQTT()
{
    pump_relay_pin = PUMP_RELAY_PIN; // change this to your pin in the platformio.ini file
}

HASSMQTT::~HASSMQTT()
{
}

void onBeforeStateChanged(bool state, HASwitch *s)
{
    // this callback will be calrelay before publishing new state to HA
    // in some cases there may be delay before onStateChanged is calrelay due to network latency
}

void onRelayStateChanged(bool state, HASwitch *s)
{
    // Relay Control
    bool relay = state;
    for (int i = 0; i < sizeof(cfg.config.relays_pin) / sizeof(cfg.config.relays_pin[0]); i++)
    {
        log_i("switching state of pin: %s\n", relay ? "HIGH" : "LOW");
        cfg.config.relays[relay] = (cfg.config.relays[relay] == true) ? false : true;
    }
}

// ############## functions to update current server settings ###################
#if ENABLE_MDNS_SUPPORT
int HASSMQTT::DiscovermDNSBroker()
{
    IPAddress mqttServer;
    // check if there is a WiFi connection
    if (WiFi.status() == WL_CONNECTED)
    {
        log_i("[mDNS Broker Discovery]: connected!\n");

        log_i("[mDNS Broker Discovery]: Setting up mDNS: ");
        if (!MDNS.begin(mqtt_mDNS_clientId))
        {
            log_i("[Fail]\n");
        }
        else
        {
            log_i("[OK]\n");
            log_i("[mDNS Broker Discovery]: Querying MQTT broker: ");

            int n = MDNS.queryService("mqtt", "tcp") || MDNS.queryService("_mqtt", "_tcp");

            if (n == 0)
            {
                // No service found
                log_i("[Fail]\n");
                return 0;
            }
            else
            {
                int mqttPort;
                // Found one or more MQTT services - use the first one.
                log_i("[OK]\n");
                mqttServer = MDNS.IP(0);
                mqttPort = MDNS.port(0);
                heapStr(&(cfg.config.MQTTBroker), mqttServer.toString().c_str());
                cfg.config.MQTTPort = mqttPort;

                switch (mqttPort)
                {
                case MQTT_PORT:
                    log_i("[mDNS Broker Discovery]: MQTT port is insecure - running on port: %d\n", mqttPort);
                    break;

                case MQTT_PORT_SECURE:
                    log_i("[mDNS Broker Discovery]: MQTT port is secure - running on port: %d\n", mqttPort);
                    break;

                case 0:
                    log_i("[mDNS Broker Discovery]: MQTT port is not set - running on port: %d\n", mqttPort);
                    break;

                default:
                    log_i("[mDNS Broker Discovery]: MQTT port is on an unusual port - running on port: %d\n", mqttPort);
                    break;
                }

                log_i("[mDNS Broker Discovery]: MQTT broker found at: %s\n: %d", cfg.config.MQTTBroker, cfg.config.MQTTPort);
                return 1;
            }
        }
    }
    return 0;
}
#endif // ENABLE_MDNS_SUPPORT

/**
 * @brief Check if the current hostname is the same as the one in the config file
 * Call in the Setup BEFORE the WiFi.begin()
 * @param None
 * @return None
 */
void HASSMQTT::loadMQTTConfig()
{
    log_i("Checking if hostname is set and valid");
    size_t size = sizeof(cfg.config.hostname);
    if (!cfg.isValidHostname(cfg.config.hostname, size - 1))
    {
        heapStr(&cfg.config.hostname, DEFAULT_HOSTNAME);
        cfg.setConfigChanged();
    }

    String MQTT_CLIENT_ID = generateDeviceID();
    char *mqtt_user = MQTT_USER;
    char *mqtt_pass = MQTT_PASS;
    char *mqtt_client_id = StringtoChar(MQTT_CLIENT_ID);
    heapStr(&cfg.config.MQTTUser, mqtt_user);
    heapStr(&cfg.config.MQTTPass, mqtt_pass);
    heapStr(&cfg.config.MQTTClientID, mqtt_client_id);
    WiFi.setHostname(cfg.config.hostname); // define hostname
    cfg.setConfigChanged();
    free(mqtt_user);
    free(mqtt_pass);
    free(mqtt_client_id);

    log_i("Loaded config: hostname %s, MQTT enabrelay %s, MQTT host %s, MQTT port %d, MQTT user %s, MQTT pass %s, MQTT topic %s, MQTT set topic %s, MQTT device name %s",
          cfg.config.hostname,
          cfg.config.MQTTBroker,
          cfg.config.MQTTPort,
          cfg.config.MQTTUser,
          cfg.config.MQTTPass,
          cfg.config.MQTTDeviceName);

    log_i("Loaded config: hostname %s", cfg.config.hostname);
}

void HASSMQTT::mqttSetup()
{
    lastInputState = digitalRead(pump_relay_pin);
    // Unique ID must be set!
    String mac = WiFi.macAddress();
    byte buf[100];
    mac.getBytes(buf, sizeof(buf));
    device.setUniqueId(buf, sizeof(buf));

    lastReadAt = millis();
    lastAvailabilityToggleAt = millis();

    /* for (int i = 0; i < 10; i++)
    {
        log_i("%c", buf[i]);
    }
    log_i("\n"); */
    // connect to broker
    log_i("Connecting to Broker");

    // set device's details (optional)
    device.setName(cfg.config.MQTTDeviceName);
    device.setSoftwareVersion(String(VERSION).c_str());

    // handle switch state
    relay.onBeforeStateChanged(onBeforeStateChanged); // optional
    relay.onStateChanged(onRelayStateChanged);
    relay.setName("Pump Relay"); // optional

    // This method enables availability for all device types registered on the device.
    // For example, if you have 5 sensors on the same device, you can enable
    // shared availability and change availability state of all sensors using
    // single method call "device.setAvailability(false|true)"
    device.enableSharedAvailability();

    // Optionally, you can enable MQTT LWT feature. If device will lose connection
    // to the broker, all device types related to it will be marked as offline in
    // the Home Assistant Panel.
    device.enableLastWill();
#if !MQTT_SECURE
#if ENABLE_MDNS_SUPPORT
    DiscovermDNSBroker();
    String mqtt_broker = cfg.config.MQTTBroker;
    IPAddress mqtt_ip;
    mqtt_ip.fromString(mqtt_broker);
    mqtt.begin(mqtt_ip);
#else
    mqtt.begin(BROKER_ADDR);
#endif // ENABLE_MDNS_SUPPORT
#else
#if ENABLE_MDNS_SUPPORT
    DiscovermDNSBroker();
    String mqtt_broker = cfg.config.MQTTBroker;
    IPAddress mqtt_ip;
    mqtt_ip.fromString(mqtt_broker);
    mqtt.begin(mqtt_ip, cfg.config.MQTTUser, cfg.config.MQTTPass);
#else
    mqtt.begin(BROKER_ADDR, cfg.config.MQTTUser, cfg.config.MQTTPass);
#endif // ENABLE_MDNS_SUPPORT
#endif // !MQTT_SECURE
}

void HASSMQTT::mqttLoop()
{
    mqtt.loop();
    if ((millis() - lastReadAt) > 30)
    { // read in 30ms interval
        // library produces MQTT message if a new state is different than the previous one
        relay.setState(digitalRead(pump_relay_pin));
        lastInputState = relay.getState();
        lastReadAt = millis();
    }

    if ((millis() - lastAvailabilityToggleAt) > 5000)
    {
        device.setAvailability(!device.isOnline());
        lastAvailabilityToggleAt = millis();
    }
}

HASSMQTT hassmqtt;