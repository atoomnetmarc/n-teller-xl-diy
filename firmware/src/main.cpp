/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>

#include "Colors.h"
#include "DisplayManager.h"
#include <AsyncMqttClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

#define ESP8266_DRD_USE_RTC       true
#define DOUBLERESETDETECTOR_DEBUG true
#define DRD_TIMEOUT               2
#define DRD_ADDRESS               0
#include <ESP_DoubleResetDetector.h>
DoubleResetDetector *drd;

DisplayManager display(4);

AsyncWebServer server(80);
DNSServer dns;

String name;

const char *mqtt_server = "revspace.nl";
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

/**
 * Connects to the MQTT server.
 * Initiates the MQTT client connection.
 */
void connectToMqtt() {
    Serial.println("Connecting to MQTT server");
    mqttClient.connect();
}

/**
 * Handles WiFi connection event.
 * Displays "viFi" on the LED strip and initiates MQTT connection.
 */
void onWifiConnect(const WiFiEventStationModeGotIP &event) {
    display.showText("viFi", COLOR_YELLOW);
    Serial.println("WiFi connected.");
    connectToMqtt();
}

/**
 * Handles WiFi disconnection event.
 * Logs the disconnection and detaches MQTT reconnect timer.
 */
void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    Serial.println("WiFi has disconnected.");
    mqttReconnectTimer.detach();
}

/**
 * Handles MQTT subscription acknowledgment.
 * Logs the successful subscription.
 */
void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
    Serial.println("Subscription to topic acknowledged.");
}

/**
 * Handles MQTT connection event.
 * Displays "MQtt" on LED strip, logs connection, and subscribes to topics.
 */
void onMqttConnect(bool sessionPresent) {
    display.showText("MQtt", COLOR_YELLOW);
    Serial.println("Connected to MQTT server.");
    Serial.println("Subscribing topics.");
    mqttClient.subscribe("revspace/doorduino/checked-in", 0);
    mqttClient.subscribe("revspace/state", 0);
}

/**
 * Handles MQTT disconnection event.
 * Logs disconnection and schedules reconnect if WiFi is connected.
 */
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("I seem to be disconnected from the MQTT server.");
    if (WiFi.isConnected()) {
        Serial.println("Lets reconnect to MQTT server.");
        mqttReconnectTimer.once(2, connectToMqtt);
    }
}

int16_t checkedin = 0;
uint8_t state = 0;

/**
 * Handles incoming MQTT messages.
 * Processes payloads for checked-in count and state topics, updates display accordingly.
 */
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    String topicString = topic;

    Serial.print("A MQTT message has arrived on topic: ");
    Serial.println(topicString);

    // Do nothing if unexpected length is being published.
    if (len > 32) {
        Serial.println("Ignoring payload.");
        return;
    }

    Serial.print("MQTT payload: ");

    char payloadChars[len + 1];
    for (uint8_t i = 0; i < len; i++) {
        payloadChars[i] = payload[i];
    }
    payloadChars[len] = '\0';

    String payloadString = payloadChars;

    Serial.println(payloadString);

    if (topicString == "revspace/doorduino/checked-in") {
        checkedin = payloadString.toInt();
    }

    if (topicString == "revspace/state") {
        state = (payloadString == "open");
    }

    display.displayNumber(checkedin, state);
}

/**
 * Arduino setup function.
 * Initializes serial, display, WiFi, MQTT, mDNS, and web server; handles double reset for WiFi config.
 */
void setup() {
    drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

    Serial.begin(115200);

    Serial.println("setup() starting.");

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    name = "nteller-" + mac;

    Serial.print("Hello there! My hostname is ");
    Serial.println(name);

    Serial.println("Configuring LED strip.");
    display.initialize();

    Serial.println("Starting WiFi.");
    WiFi.hostname(name.c_str());
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    if (drd->detectDoubleReset()) {
        Serial.println("Double reset detected. Starting wifimanager.");
        display.showText("_AP_", COLOR_YELLOW);

        AsyncWiFiManager wifiManager(&server, &dns);
        wifiManager.setTimeout(300);
        if (!wifiManager.startConfigPortal(name.c_str())) {
            Serial.println("Failed to connect and hit timeout.");
            ESP.restart();
        }

        Serial.println("Our new WiFi credentials seem to work!");
    } else {

        // All LEDS are currently blue
        delay(2500);

        // All LEDS green
        display.all(COLOR_GREEN);
        display.strip.Show();
        delay(2500);

        // All LEDS red
        display.all(COLOR_RED);
        display.strip.Show();
        delay(2500);

        // All LEDS yellow
        display.all(COLOR_YELLOW);
        display.strip.Show();

        display.showText("Conn", COLOR_YELLOW);

        WiFi.begin();
    }



    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setClientId(name.c_str());
    mqttClient.setServer(mqtt_server, 1883);

    if (!MDNS.begin(name.c_str())) {
        Serial.println("Error starting mDNS.");
    } else {
        Serial.println("mDNS started.");
    }

    Serial.println("Configuring async webserver.");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", name);
    });

    Serial.println("setup() done.");
}

/**
 * Arduino loop function.
 * Updates mDNS and double reset detector periodically.
 */
void loop() {
    MDNS.update();
    drd->loop();
}