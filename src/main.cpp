#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>
#include "DFRobotDFPlayerMini.h"

/* Configurazione pin e tipologia sensore ambientale */
#define DHTPIN 33
#define DHTTYPE DHT22
/* Pin di input digitale per il sensore di movimento PIR */
#define PIRPIN 27

DHT dht(DHTPIN, DHTTYPE);

/* Parametri di configurazione del segnale PWM per il servomotore */
const int servoPin = 18;
const int pwmChannel = 0;
const int pwmFreq = 50;
const int pwmResolution = 16;
uint32_t dutyMin = 1638;
uint32_t dutyMax = 8192;

/* Credenziali di rete WiFi e parametri del broker MQTT */
const char* ssid = "Mi 10T Lite";
const char* wifi_password = "bccd15185a22x";
const char* mqtt_server = "0b9a48980b684944aabfe4adc2ebc36b.s1.eu.hivemq.cloud";
constexpr int mqtt_port = 8883;
const char* mqtt_user = "IOT-Project";
const char* mqtt_password = "Ccmt1234";

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

/* Variabili di stato per il controllo cinematico e timing */
float angoloAttuale = 0;
int angoloTarget = 0;
unsigned long ultimoStep = 0;
unsigned long ultimoTentativoMqtt = 0;
const int velocita = 15;
int ora = 0;

/* Calcola il modulo di un valore forzandolo all'interno di un range definito */
int wrap(int amt, int min, int max) {
    int range = max - min;
    return (amt - min) % range + min;
}

/* Converte l'angolo in un valore duty cycle proporzionale e aggiorna l'uscita PWM */
void impostaPWM(float ang) {
    uint32_t duty = dutyMin + (dutyMax - dutyMin) * ang / 270;
    ledcWrite(pwmChannel, duty);
}

/* Imposta il nuovo target limitando l'escursione massima a 270 gradi per sicurezza meccanica */
void impostaAngolo(int nuovoAngolo) {
    angoloTarget = constrain(nuovoAngolo, 0, 270);
}

/* Esegue l'interpolazione lineare dell'angolo a step temporizzati per un movimento fluido */
void gestisciMovimentoServo() {
    if (millis() - ultimoStep >= velocita) {
        ultimoStep = millis();
        if (angoloAttuale < angoloTarget) {
            angoloAttuale += 1;
            impostaPWM(angoloAttuale);
        } else if (angoloAttuale > angoloTarget) {
            angoloAttuale -= 1;
            impostaPWM(angoloAttuale);
        }
    }
}

/* Aggiorna l'indice di posizione e suddivide l'escursione totale di 270 gradi in 6 sezioni */
void prossimaOra() {
    /* Limita il contatore in un range da 0 a 5 (6 posizioni totali) */
    ora = wrap(ora + 1, 0, 6);
    /* Calcola l'angolo target basato sulla partizione a blocchi di 45 gradi (270 / 6) */
    impostaAngolo(270 / 6 * ora);
}

/* Inizializza il modulo WiFi saltando la verifica del certificato SSL del server */
void impostaWifi() {
    wifiClient.setInsecure();
    WiFi.begin(ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi OK");
}

/* Gestisce il keep-alive MQTT e tenta la riconnessione automatica in caso di timeout */
void gestisciConnessioneMQTT() {
    if (!client.connected()) {
        if (millis() - ultimoTentativoMqtt > 5000) {
            ultimoTentativoMqtt = millis();
            if (client.connect("esp32-123", mqtt_user, mqtt_password)) {
                client.subscribe("esp32/comandi");
            }
        }
    } else {
        client.loop();
    }
}

/* Esegue il parsing del payload MQTT ricevuto ed innesca l'azione hardware associata */
void callback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    if (String(topic) == "esp32/comandi" && msg == "prossimaOra") {
        prossimaOra();
    }
}

void setup() {
    Serial.begin(115200);

    /* Inizializzazione input hardware e bus dati sensori */
    pinMode(PIRPIN, INPUT);
    dht.begin();

    /* Configurazione e aggancio del segnale PWM al GPIO designato per il servo */
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(servoPin, pwmChannel);
    impostaPWM(angoloAttuale);

    /* Avvio stack di rete e configurazione client MQTT */
    impostaWifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    /* Mantenimento ciclico delle routine hardware e di rete */
    gestisciConnessioneMQTT();
    gestisciMovimentoServo();

    /* Scheduler non bloccante per campionamento dati e trasmissione telemetrica (5Hz) */
    static unsigned long lastMsg = 0;
    if (millis() - lastMsg > 5000) {
        lastMsg = millis();

        float t = dht.readTemperature();
        float h = dht.readHumidity();
        bool movimento = digitalRead(PIRPIN);

        /* Generazione dinamica del payload in formato JSON */
        String payload = "{";
        payload += "\"temperatura\":" + (isnan(t) ? "null" : String(t));
        payload += ",\"umidita\":" + (isnan(h) ? "null" : String(h));
        payload += ",\"angolo\":" + String(angoloAttuale);
        payload += ",\"movimento\":" + String(movimento ? "true" : "false");
        payload += "}";

        /* Esegue il publish solo se il socket MQTT è in stato connected */
        if (client.connected()) {
            client.publish("esp32/dati", payload.c_str());
            Serial.println("MQTT: " + payload);
        }
    }
}