#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
const char* ssid = "Mi 10T Lite";
const char* password = "";


Servo motoreServo;

// Sostituisci questo valore con il pin effettivo.
// Puoi usare un intero (es. 18 per D18) o la costante (es. D4 se la board lo mappa).
const int pinSegnale = 18;



void setup_wifi() {
    // Configura il chip Wi-Fi in modalità client
    WiFi.mode(WIFI_STA);

    // Inizializza il processo di associazione al router
    WiFi.begin(ssid, password);

    // Loop bloccante che interroga lo stato della connessione ogni 500ms
    // Il firmware non procede finché l'handshake WPA2 non è completato
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void setup() {
    // Alloca il timer hardware 0 per la generazione del segnale PWM
    ESP32PWM::allocateTimer(0);

    // Imposta la frequenza del segnale PWM a 50Hz, standard per i servomotori
    motoreServo.setPeriodHertz(50);

    // Associa il servomotore al pin specificato
    // Imposta la durata degli impulsi: 500us (0°) e 2400us (180°)
    motoreServo.attach(pinSegnale, 500, 2400);

    setup_wifi();
}

void loop() {
    // Genera il movimento continuo da 0 a 180 gradi
    for (int angolo = 0; angolo <= 180; angolo += 1) {
        motoreServo.write(angolo);
        // Attende 15ms per consentire l'attuazione meccanica della posizione
        delay(15);
    }

    // Genera il movimento continuo da 180 a 0 gradi
    for (int angolo = 180; angolo >= 0; angolo -= 1) {
        motoreServo.write(angolo);
        delay(15);
    }
}
