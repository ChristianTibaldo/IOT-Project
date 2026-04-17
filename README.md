![Descrizione immagine](tiba.jpeg) 

![Descrizione immagine](freccia.jpg)
 
![Descrizione immagine](cortisol.webp)

![Descrizione immagine](freccia.jpg)

![Descrizione immagine](Tiba%20MezzoPrime.png)

![Descrizione immagine](Tiba%20Prime.png)



PER AVVIARE:
- impostare ssid e wifi_password con quelli del proprio hotspot
- assicurarsi che l'hotspot giri a 2.4 GHz
- impostare upload_port con la porta a cui è collegato l'esp
- creare il cluster HiveMQ e aggiungerci le credenziali corrette
- impostare mqtt_server con quello del cluster


PER COLLEGAMENTO SERVO MOTORE AD ESP32 (Cavo servo -> Pin esp32):
- Rosso -> VIN/VCC
- Nero -> GND
- Giallo -> PIN esp32 a scelta (in questo momento stiamo usando il pin 18)


L'esp ha 2 pin per alimentazione. VIN 5V    3V3 3V
servo motore e pir muovimento usano 5V
sensore umidità/temperatura usa 3V


pir collegato al pin 27 dell'esp
dht collegato al pin 33 dell'esp
