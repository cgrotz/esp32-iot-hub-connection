#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <MQTT.h>

const char ssid[] = "<Wifi SSID>";
const char pass[] = "<Wifi Password>";

// Connetion parameters from the provisioning json file
const char hubNamespace[] = "<the namespace we created>";
const char deviceId[] = "<deviceId>";
const char tenantId[] = "<IoT Hub tenant>";
const char mqttPassword[] = "<MQTT Password>";

// Since Bosch IoT Hub needs TLS, we need to use the WifiClient Secure instead of the normal Wifi Client
WiFiClientSecure net;
// We need to increase message size since the IoT Suite messages can be rather big
MQTTClient client(2048);

unsigned long lastMillis = 0;

void messageReceived(String &mqttTopic, String &payload) {
  StaticJsonDocument<2000> doc;
  deserializeJson(doc, payload);
  String topic = doc["topic"];
  String path = doc["path"];

  Serial.print("incoming topic: ");
  Serial.print(topic);
  Serial.print(" path: ");
  Serial.println(path);
  Serial.println("incoming: " + mqttTopic + " - " + payload);
  
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void connect() {
  Serial.print("\nchecking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print(" connected to wifi!");

  Serial.print("\nconnecting to mqtt...");
  client.begin("mqtt.bosch-iot-hub.com", 8883, net);
  client.onMessage(messageReceived);

  int wait = 0;
  char mqttDeviceId[100];
  char mqttUsername[100];
  sprintf(mqttDeviceId, "%s:%s", hubNamespace, deviceId);
  sprintf(mqttUsername, "%s_%s@%s", hubNamespace, deviceId, tenantId);
  while (!client.connect(mqttDeviceId, mqttUsername,mqttPassword, false) && wait < 10) {
    Serial.print(".");
    delay(1000);
    wait++;
  }
  if(!client.connected()) {
    Serial.println(" not connected to mqtt!");
  }
  else {
    Serial.println(" connected to mqtt!");
  }
  // Subscribe to commands from IoT Suite
  client.subscribe("command///req/#");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  connect();
}

void loop() {
  client.loop();
  delay(10);  // we add a few millisecond delay for wifi stability

  if (!client.connected()) {
    connect();
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    char payload[1000];
    sprintf(payload, "{\r\n\t\"topic\": \"%s/%s/things/twin/commands/modify\",\r\n\t\"path\": \"/features/version\",\r\n\t\"value\": {\r\n\t \t\"properties\": {\r\n \t\t \t\"version\": \"1.0.8\"\r\n\t\t}\r\n\t}\r\n}",hubNamespace, deviceId );
    if(!client.publish("event", payload, false, 1)) {
      Serial.print("Failed publishing ");
      Serial.println(client.lastError());
    }
  }
}
