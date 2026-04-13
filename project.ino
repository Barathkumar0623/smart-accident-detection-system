#include <Wire.h>
#include <MPU6050.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// -------- OBJECTS --------
MPU6050 mpu;
TinyGPSPlus gps;
SoftwareSerial gpsSerial(D5, D6); // RX, TX

// -------- PIN --------
#define LED D7

// -------- WIFI --------
const char* ssid = "Redmi 12 5G";
const char* password = "12345678";

// -------- TELEGRAM --------
#define BOT_TOKEN ""

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// -------- USERS --------
String chatIDs[] = {
  "6624999824",
  "1181343738","1884479012"
};
int totalUsers = 3;

// -------- VARIABLES --------
int16_t ax, ay, az;
float lastSpeed = 0;
float threshold = 2.5;
bool accidentTriggered = false;

// -------- SETUP --------
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== SMART ACCIDENT SYSTEM START =====");

  Wire.begin(D2, D1);
  gpsSerial.begin(9600);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 Connected ✅");
  } else {
    Serial.println("MPU6050 FAILED ❌");
  }

  // WiFi
  Serial.print("Connecting WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  client.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected ✅");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// -------- SEND ALERT --------
void sendAlert(String message) {
  Serial.println("\n📩 Sending Alert...");
  for (int i = 0; i < totalUsers; i++) {
    bot.sendMessage(chatIDs[i], message, "");
  }
  Serial.println("Alert Sent ✅");
}

// -------- LOOP --------
void loop() {

  Serial.println("\n------ LOOP ------");

  // -------- GPS --------
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.speed.isValid()) {
    lastSpeed = gps.speed.kmph();
  }

  Serial.print("Speed: ");
  Serial.print(lastSpeed);
  Serial.println(" km/h");

  if (gps.location.isValid()) {
    Serial.print("Location: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(", ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("GPS: No Fix ❌");
  }

  // -------- MPU6050 --------
  mpu.getAcceleration(&ax, &ay, &az);

  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  float totalAcc = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);

  Serial.print("Acceleration: ");
  Serial.println(totalAcc);

  // -------- ACCIDENT DETECTION --------
  if (totalAcc > threshold && !accidentTriggered) {

    accidentTriggered = true;
    digitalWrite(LED, HIGH);

    Serial.println("🚨 ACCIDENT DETECTED!");

    String message = "🚨 ACCIDENT ALERT 🚨\n";
    message += "Crash detected!\n\n";

    // Speed
    message += "🚗 Speed: ";
    message += String(lastSpeed);
    message += " km/h\n";

    // Location
    if (gps.location.isValid()) {
      float lat = gps.location.lat();
      float lng = gps.location.lng();

      message += "📍 Location:\n";
      message += "https://maps.google.com/?q=";
      message += String(lat, 6) + "," + String(lng, 6);
    } else {
      message += "📍 Location not available\n";
    }

    message += "\n⚡ Immediate help needed!";

    sendAlert(message);

    delay(10000); // avoid spam
    digitalWrite(LED, LOW);
  }

  // -------- RESET --------
  if (totalAcc < 1.2) {
    accidentTriggered = false;
  }

  delay(1000);
}