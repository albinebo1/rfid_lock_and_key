#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>


#define SS_PIN 5
#define RST_PIN 21

// LEDs
#define GREEN_LED_PIN 26
#define RED_LED_PIN 27

// Buzzer
#define BUZZER_PIN 25

// Servo
#define SERVO_PIN 14
#define SERVO_LOCKED_ANGLE 0
#define SERVO_UNLOCKED_ANGLE 90
#define UNLOCK_TIME_MS 3000  // how long door stays unlocked

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo lockServo;

// Wi-Fi creds
const char* WIFI_SSID = "your_username";
const char* WIFI_PASS = "your_password";

// Apps Script Web App URL 
const char* SCRIPT_URL = "google apps script URL";

// Authorized UID
byte authorizedUID[] = { 0x73, 0x84, 0xE6, 0x0B };
byte authorizedUIDSize = sizeof(authorizedUID);

String uidToString(byte* uid, byte uidSize) {
  String s;
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
  }
  s.toUpperCase();
  return s;
}

void logToSheet(const String& uidStr, const String& resultStr) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping log.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();  // demo-friendly

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(2500);

  // Build GET URL with parameters
  String url = String(SCRIPT_URL) + "?uid=" + uidStr + "&result=" + resultStr;

  Serial.print("GET: ");
  Serial.println(url);

  if (!http.begin(client, url)) {
    Serial.println("HTTP begin failed");
    return;
  }

  int code = http.GET();
  Serial.print("Sheets HTTP code: ");
  Serial.println(code);

  if (code > 0) {
    String resp = http.getString();
    Serial.print("Sheets response (first 80): ");
    Serial.println(resp.substring(0, min((int)resp.length(), 80)));
  }

  http.end();
}



bool isAuthorized(byte* uid, byte uidSize) {
  if (uidSize != authorizedUIDSize) return false;
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] != authorizedUID[i]) return false;
  }
  return true;
}

void allOff() {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

void beepOnce(int freq, int ms) {
  tone(BUZZER_PIN, freq);
  delay(ms);
  noTone(BUZZER_PIN);
}

void unlockDoor() {
  lockServo.write(SERVO_UNLOCKED_ANGLE);
  delay(UNLOCK_TIME_MS);
  lockServo.write(SERVO_LOCKED_ANGLE);
}

void feedbackAuthorized() {
  allOff();
  digitalWrite(GREEN_LED_PIN, HIGH);
  beepOnce(2000, 120);
  unlockDoor();
  allOff();
}

void feedbackDenied() {
  allOff();
  digitalWrite(RED_LED_PIN, HIGH);
  beepOnce(1500, 100);
  delay(120);
  beepOnce(1500, 100);
  delay(300);
  allOff();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed (continuing without logging).");
  }

  SPI.begin();
  mfrc522.PCD_Init();

  lockServo.setPeriodHertz(50);
  lockServo.attach(SERVO_PIN);
  lockServo.write(SERVO_LOCKED_ANGLE);  // start locked

  Serial.println("RFID system ready.");
  Serial.println("Tap a card...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  Serial.print("Scanned UID:");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(" ");
    if (mfrc522.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  String uidStr = uidToString(mfrc522.uid.uidByte, mfrc522.uid.size);

  if (isAuthorized(mfrc522.uid.uidByte, mfrc522.uid.size)) {
    Serial.println(" AUTHORIZED");
    feedbackAuthorized();
    logToSheet(uidStr, "AUTHORIZED");
  } else {
    Serial.println(" DENIED");
    feedbackDenied();
    logToSheet(uidStr, "DENIED");
  }



  Serial.println("--------------------");

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(700);
}
