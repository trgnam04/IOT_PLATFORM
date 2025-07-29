#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include  "certs.h"

const char* ssid = "BKIT_CS2";
const char* password = "cselabc5c6";

const char* mqtt_server = "192.168.1.74"; // IP của máy chạy Mosquitto
const int mqtt_port = 8883;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  setup_wifi();

  espClient.setCACert((const char*)ca_pem);
  espClient.setCertificate((const char*)chain_pem);
  espClient.setPrivateKey((const char*)device_key_pem);

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(5000);
    }
  }
  else
  {
    Serial.println("Already connected to MQTT broker.");
    char payload[50];
    sprintf(payload, "{\"temperature\":%d}", random(20, 30));
    Serial.print("Publishing message: ");
    Serial.println(payload);
    // Publish a message to the topic "v1/devices/me/telemetry"
    client.publish("v1/devices/me/telemetry", payload);
  }

  delay(1000); // Delay to avoid flooding the broker

  client.loop();
}
