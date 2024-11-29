#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TM1637Display.h>
#include <ArduinoJson.h>
#include <TimeLib.h> // Pre prácu s časom
#include <Timezone.h> // Pre správu letného a zimného času

// WiFi pripojenie
const char* ssid     = "YourSSID";
const char* password = "YourPASS";

// TM1637 displej
#define CLK_PIN  D5
#define DIO_PIN  D7
TM1637Display display(CLK_PIN, DIO_PIN);

// Bzučiak
#define BUZZER_PIN D6

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Offset sa nastaví dynamicky

// Časové pásmo a pravidlá letného/zimného času
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time (UTC+2)
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time (UTC+1)
Timezone myTZ(CEST, CET);  // Časová zóna pre strednú Európu (prispôsobte podľa vašej lokality)

// Premenné pre displej
bool colonVisible = true;
unsigned long previousMillis = 0;
const long interval = 1000;

String previousHour = "";
bool buzzedAt550 = false;

// Funkcia na načítanie časového pásma cez IP-API
void fetchTimezoneOffset() {
  WiFiClient client;
  if (client.connect("ip-api.com", 80)) {
    client.println("GET /json HTTP/1.1");
    client.println("Host: ip-api.com");
    client.println("Connection: close");
    client.println();

    String response = "";
    while (client.connected() || client.available()) {
      response += client.readString();
    }
    client.stop();

    // Parsovanie JSON odpovede
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, response);
    if (doc["status"] == "success") {
      String timezone = doc["timezone"];
      Serial.println("Detected timezone: " + timezone);

      // Tu by ste mohli nastaviť inú časovú zónu na základe detekovaného časového pásma,
      // napr. myTZ nastavte podľa krajiny.
    } else {
      Serial.println("Failed to fetch timezone. Using default Central European Time.");
    }
  } else {
    Serial.println("Connection to IP-API failed.");
  }
}

void setup() {
  Serial.begin(115200);

  // Inicializácia displeja
  display.setBrightness(0x0f);

  // Pripojenie k WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Načítanie časového pásma
  fetchTimezoneOffset();

  // Inicializácia NTP klienta
  timeClient.begin();

  // Nastavenie bzučiaka
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  timeClient.update();

  // Získanie aktuálneho UTC času z NTP
  time_t rawTime = timeClient.getEpochTime();
  // Prevod na lokálny čas (s letným/zimným časom)
  time_t localTime = myTZ.toLocal(rawTime);

  // Extrakcia hodín a minút
  int hours = hour(localTime);
  int minutes = minute(localTime);

  // Preklopenie dvojbodky
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    colonVisible = !colonVisible;
  }

  // Zobrazenie času na displeji
  int displayTime = (hours * 100) + minutes;
  if (colonVisible) {
    display.showNumberDecEx(displayTime, 0x40, true);
  } else {
    display.showNumberDec(displayTime);
  }

  // Kontrola zmeny hodiny
  if (previousHour != String(hours)) {
    previousHour = String(hours);
    buzzedAt550 = false;

    if (hours < 23 && hours >= 6) {
      for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
      }
    }
  }

  // Kontrola na čas 5:50 a zabezpečenie, že bzučiak zabzučí len raz
  if (hours == 5 && minutes == 50 && !buzzedAt550) {
    buzzedAt550 = true;
    for (int i = 0; i < 10; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(1000);
      digitalWrite(BUZZER_PIN, LOW);
      delay(1000);
    }
  }
}
