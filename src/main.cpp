#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <esp32-hal.h>

// Parametri hardware per il controllo del Servo
const int servoPin = 18;
const int pwmChannel = 0;
const int pwmFreq = 50;
const int pwmResolution = 16;
uint32_t dutyMin = 1638;
uint32_t dutyMax = 8192;

// Parametri di rete WiFi
const char* ssid = "Mi 10T Lite";
const char* wifi_password = "bccd15185a22x";

// Parametri di connessione MQTT
const char* mqtt_server = "7b5efaa28c754223b5c5ae9bce2d6384.s1.eu.hivemq.cloud";
constexpr int mqtt_port = 8883;
const char* mqtt_user = "IOT-Project";
const char* mqtt_password = "Ccmt1234";

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

// Variabili di stato per il movimento asincrono del servo
bool inMovimento = false;          // Flag di attivazione del ciclo di rotazione
int angoloAttuale = 0;             // Traccia la posizione corrente
int direzione = 1;                 // Moltiplicatore per il calcolo dell'angolo (1 o -1)
unsigned long tempoUltimoStep = 0; // Riferimento temporale per la temporizzazione
const int millisecondiPerGrado = 15; // Ritardo tra ogni grado di spostamento

// Converte l'angolo in segnale PWM e lo invia al pin
void setAngle(int angle) {
    // Vincola l'angolo ai limiti fisici previsti
    angle = constrain(angle, 0, 270);

    // Interpola linearmente l'angolo nel range dei duty cycle
    uint32_t duty = dutyMin + (dutyMax - dutyMin) * angle / 270;
    ledcWrite(pwmChannel, duty);
}

// Gestisce il movimento incrementale del servo senza bloccare l'esecuzione
void gestisciMovimentoServo() {
    // Interrompe l'esecuzione se il comando non è stato attivato
    if (!inMovimento) return;

    unsigned long tempoAttuale = millis();

    // Esegue lo step successivo solo se è trascorso il tempo definito
    if (tempoAttuale - tempoUltimoStep >= millisecondiPerGrado) {
        tempoUltimoStep = tempoAttuale;
        angoloAttuale += direzione;

        setAngle(angoloAttuale);

        // Inverte il senso di marcia al raggiungimento del limite superiore
        if (angoloAttuale >= 270) {
            direzione = -1;
        }
        // Termina la routine di movimento ritornando alla posizione iniziale
        else if (angoloAttuale <= 0) {
            angoloAttuale = 0;
            direzione = 1;
            inMovimento = false;
        }
    }
}

void setup_wifi() {
    delay(10);
    Serial.println("\nConnessione al WiFi");

    // Disabilita la validazione stretta del certificato SSL per semplificare il test
    wifiClient.setInsecure();
    WiFi.begin(ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connesso");
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Connessione MQTT...");

        if (client.connect("esp32-123", mqtt_user, mqtt_password)) {
            Serial.println("connesso");
            client.publish("esp32/status", "ESP32 connessa");

            // Sottoscrizione al topic per intercettare i comandi in ingresso
            client.subscribe("esp32/comandi");
        } else {
            Serial.print("Errore connessione, rc=");
            Serial.print(client.state());
            Serial.println(" riprovo tra 5 secondi");
            delay(5000);
        }
    }
}

// Funzione richiamata automaticamente alla ricezione di un messaggio MQTT
void callback(char* topic, byte* payload, unsigned int length) {
    // Ricostruzione della stringa dal buffer di byte
    String messaggio = "";
    for (unsigned int i = 0; i < length; i++) {
        messaggio += (char)payload[i];
    }

    Serial.println("Topic: " + String(topic));
    Serial.println("Payload: " + messaggio);

    // Valutazione logica del payload
    if (String(topic) == "esp32/comandi") {
        if (messaggio == "ruota") {
            // Imposta il flag a true per avviare la routine nel loop principale
            if (!inMovimento) {
                inMovimento = true;
                Serial.println("Routine di rotazione avviata");
            } else {
                Serial.println("Routine gia in esecuzione. Comando ignorato.");
            }
        } else {
            Serial.println("Comando non riconosciuto: " + messaggio);
        }
    }
}

void setupServo() {
    // Configura i parametri hardware per la generazione del segnale PWM
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(servoPin, pwmChannel);

    // Forza la posizione iniziale per coerenza di stato
    setAngle(0);
}

void setup() {
    Serial.begin(115200);

    setupServo();
    setup_wifi();

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    // Garantisce la continuità della connessione al broker
    if (!client.connected()) {
        reconnect();
    }

    // Elabora le code di rete ed esegue le callback necessarie
    client.loop();

    // Processa gli incrementi del motore in tempo reale (solo se inMovimento è true)
    gestisciMovimentoServo();

    // Schedulazione non bloccante per l'invio della telemetria periodica
    static unsigned long lastMsg = 0;
    unsigned long now = millis();

    if (now - lastMsg > 5000) {
        lastMsg = now;
        client.publish("esp32/dati", "ciao dal esp32");
    }
}