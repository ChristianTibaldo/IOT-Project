#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include <esp32-hal.h>

// SERVO
const int servoPin = 18;

const int pwmChannel = 0;
const int pwmFreq = 50;
const int pwmResolution = 16;

uint32_t dutyMin = 1638;
uint32_t dutyMax = 8192;

// WIFI
const char* ssid = "Mi 10T Lite";
const char* wifi_password = "bccd15185a22x";

// MQTT
const char* mqtt_server = "ffd628ec3a4545039e59114a3e36449f.s1.eu.hivemq.cloud:8883";
constexpr int mqtt_port = 8883;
const char* mqtt_user = "IOT-Project";
const char* mqtt_password = "Ccmt1234";

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connessione al WiFi");

    wifiClient.setInsecure();
    WiFi.begin(ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connesso");
}

void reconnect() {

    while (!client.connected()) {

        Serial.print("Connessione MQTT...");

        if (client.connect("esp32-123", mqtt_user, mqtt_password)) {

            Serial.println("connesso");

            client.publish("esp32/status", "ESP32 connessa");

            client.subscribe("esp32/comandi");

        } else {

            Serial.print("Errore connessione, rc=");
            Serial.print(client.state());
            Serial.println(" riprovo tra 5 secondi");

            delay(5000);
        }
    }
}

void setAngle(int angle) {
    // Limita tra 0 e 270
    angle = constrain(angle, 0, 270);

    uint32_t duty = dutyMin + (dutyMax - dutyMin) * angle / 270;
    ledcWrite(pwmChannel, duty);
}

boolean ruota = false;

void callback(char* topic, byte* payload, unsigned int length) {

    // 1. Converti il payload (array di byte) in String leggibile
    String messaggio = "";
    for (unsigned int i = 0; i < length; i++) {
        messaggio += (char)payload[i];
    }

    // 2. Stampa topic e messaggio
    Serial.println("Topic:    " + String(topic));
    Serial.println("Payload:  " + messaggio);

    // 3. Confronta il topic (utile se sei iscritto a più topic)
    if (String(topic) == "esp32/comandi") {

        // 4. Leggi il contenuto del messaggio
        if (messaggio == "ruota") {
            if (ruota)
                setAngle(180);
            else
                setAngle(0);
            ruota = !ruota;
            Serial.println("LED acceso");
        }
        else {
            Serial.println("Comando non riconosciuto: " + messaggio);
        }
    }
}

void setupServo() {
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(servoPin, pwmChannel);
}

void setup() {

    Serial.begin(115200);

    setupServo();

    setup_wifi();

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {

    if (!client.connected()) {
        reconnect();
    }

    client.loop();

    static long lastMsg = 0;
    long now = millis();

    if (now - lastMsg > 5000) {

        lastMsg = now;

        client.publish("esp32/dati", "ciao dal esp32");
    }
}
