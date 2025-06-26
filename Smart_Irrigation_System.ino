#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""
#define BLYNK_TEMPLATE_ID ""

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BlynkSimpleEsp32.h>
#include <ThingSpeak.h>

// === WiFi Credentials ===
char ssid[] = "";
char pass[] = "";

// === ThingSpeak ===
unsigned long myChannelNumber = ;
const char * myWriteAPIKey = "";
WiFiClient client;

// === Sensor & Actuator Pins ===
#define DHTPIN 15
#define DHTTYPE DHT11
#define SOIL_PIN 34
#define WATER_LEVEL_PIN 35
#define BUZZER_PIN 27
#define RED_PIN 18
#define GREEN_PIN 19
#define BLUE_PIN 23
#define RELAY_PIN 26

// === OLED ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// === Objects ===
DHT dht(DHTPIN, DHTTYPE);

// === Variables ===
bool pumpStatus = false;

// Function Prototypes
void beepBuzzer();
void setRGB(int r, int g, int b);
void drawMoistureBar(int moisture);

void setup() {
  Serial.begin(115200);

  // === Pin Modes ===
  pinMode(SOIL_PIN, INPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Keep pump OFF initially

  // === Begin Sensors & Display ===
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed"));
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart Irrigation");
  display.println("   (Auto Mode)");
  display.display();
  delay(2000);

  // === Connect WiFi ===
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  ThingSpeak.begin(client);
}

void loop() {
  Blynk.run();

  // === Read Sensors ===
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int soilRaw = analogRead(SOIL_PIN);
  int moisture = map(soilRaw, 3700, 2300, 0, 100);
  moisture = constrain(moisture, 0, 100);
  int waterLevel = digitalRead(WATER_LEVEL_PIN);

  // === OLED Display ===
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temp);
  display.println(" C");

  display.setCursor(0, 8);
  display.print("Hum: ");
  display.print(hum);
  display.println(" %");

  drawMoistureBar(moisture);

  display.setCursor(0, 24);
  display.print("Pump: ");
  display.println(pumpStatus ? "ON" : "OFF");

  display.setCursor(0, 55);
  display.print("Water: ");
  display.println(waterLevel == HIGH ? "OK" : "LOW");

  display.display();

  // === Buzzer Alert ===
  if (waterLevel == LOW) {
    beepBuzzer();
  }

  // === RGB Indicator ===
  if (moisture > 60) setRGB(0, 255, 0);
  else if (moisture > 50) setRGB(0, 0, 255);
  else setRGB(255, 0, 0);

  // === Pump Control ===
  if (moisture < 50 && waterLevel == HIGH) {
    digitalWrite(RELAY_PIN, LOW);
    pumpStatus = true;
  } else {
    digitalWrite(RELAY_PIN, HIGH);
    pumpStatus = false;
  }

  // === Send to Blynk ===
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V2, moisture);
  Blynk.virtualWrite(V3, waterLevel);
  Blynk.virtualWrite(V4, pumpStatus);

  // === Send to ThingSpeak ===
  ThingSpeak.setField(1, temp);
  ThingSpeak.setField(2, hum);
  ThingSpeak.setField(3, moisture);
  ThingSpeak.setField(4, waterLevel);
  ThingSpeak.setField(5, pumpStatus);
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x != 200) {
    Serial.println("ThingSpeak Error: " + String(x));
  } else {
    Serial.println("ThingSpeak updated.");
  }

  delay(2000);
}

void beepBuzzer() {
  tone(BUZZER_PIN, 1000);
  delay(200);
  noTone(BUZZER_PIN);
}

void setRGB(int r, int g, int b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

void drawMoistureBar(int moisture) {
  display.drawRect(0, 32, 100, 10, SSD1306_WHITE);
  int fillWidth = map(moisture, 0, 100, 0, 98);
  display.fillRect(1, 33, fillWidth, 8, SSD1306_WHITE);
  display.setCursor(0, 45);
  display.print("Moisture: ");
  display.print(moisture);
  display.print("%");
}
