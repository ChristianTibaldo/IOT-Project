#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// Parametri hardware Servo
const int servoPin = 18;
const int pwmChannel = 0;
const int pwmFreq = 50;
const int pwmResolution = 16;
uint32_t dutyMin = 1638;
uint32_t dutyMax = 8192;

// Rete e MQTT
const char* ssid = "Mi 10T Lite";
const char* wifi_password = "bccd15185a22x";
const char* mqtt_server = "7b5efaa28c754223b5c5ae9bce2d6384.s1.eu.hivemq.cloud";
constexpr int mqtt_port = 8883;
const char* mqtt_user = "IOT-Project";
const char* mqtt_password = "Ccmt1234";

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

// Variabili di stato movimento
float angoloAttuale = 0;
int angoloTarget = 0;
unsigned long ultimoStep = 0;
const int velocita = 15;

int ora = 0;

int wrap(int amt, int min, int max) {
    int range = max - min;
    return (amt - min) % range + min;
}

void impostaPWM(float ang) {
    uint32_t duty = dutyMin + (dutyMax - dutyMin) * ang / 270;
    ledcWrite(pwmChannel, duty);
}

void impostaAngolo(int nuovoAngolo) {
    angoloTarget = constrain(nuovoAngolo, 0, 270);
}

void gestisciMovimento() {
    if (millis() - ultimoStep >= velocita) {
        ultimoStep = millis();

        if (angoloAttuale < angoloTarget) {
            angoloAttuale += 1;
            impostaPWM(angoloAttuale);
        }
        else if (angoloAttuale > angoloTarget) {
            angoloAttuale -= 1;
            impostaPWM(angoloAttuale);
        }
    }
}

void prossimaOra() {
    ora = wrap(ora + 1, 0, 8);
    impostaAngolo(270 / 8 * ora);
}

void impostaWifi() {
    wifiClient.setInsecure();
    WiFi.begin(ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connesso");
}

void riconnetti() {
    while (!client.connected()) {
        if (client.connect("esp32-123", mqtt_user, mqtt_password)) {
            client.subscribe("esp32/comandi");
        } else {
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

    if (String(topic) == "esp32/comandi") {
        if (msg == "prossimaOra") {
            prossimaOra();
        }
    }
}

void setup() {
    Serial.begin(115200);
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(servoPin, pwmChannel);

    impostaPWM(angoloAttuale);
    impostaWifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) riconnetti();
    client.loop();

    gestisciMovimento();

    // Invia dati a HiveMQ
    static unsigned long lastMsg = 0;
    if (millis() - lastMsg > 5000) {
        lastMsg = millis();
        client.publish("esp32/dati", String(angoloAttuale).c_str());
    }
}