#include <WiFi.h>
#include <HTTPClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ArduinoJson.h>

/*  THE LED MATRIX  */
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 5

MD_Parola matrix(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

/*  IO  */
#define BUTTON_PIN 13
#define LED_PIN 12

/*  WIFI  */
const char* ssid = "ssid";
const char* password = "password";

/*  SERVER  */
const char* SERVER_GET = "http://hostserver/get";
const char* SERVER_ACK = "http://hostserver/ack";

/*  STATE  */
String currentMessage = "";
bool unread = false;
bool scrolling = false;

bool lastButtonState = HIGH; // pull-up
unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 50;

unsigned long lastPoll = 0;
const unsigned long POLL_INTERVAL = 5000;

/*  SETUP  */
void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  matrix.begin();
  matrix.setIntensity(2); // low-mid brightness
  matrix.displayClear();
  matrix.displayText("READY", PA_CENTER, 50, 0, PA_PRINT, PA_NO_EFFECT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

/*  PING SERVER  */
void pollServer() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(SERVER_GET);
  int code = http.GET();
  if (code == 200) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, http.getString());

    String msg = doc["message"].as<String>();
    bool serverUnread = doc["unread"];

    // Only update if message changed
    if (msg != currentMessage) {
      currentMessage = msg;
      unread = serverUnread;
      digitalWrite(LED_PIN, unread ? HIGH : LOW);
    }
  }
  http.end();
}

/*  ACK MESSAGE  */
void ackMessage() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(SERVER_ACK);
  int code = http.POST(nullptr, 0); // send empty body safely
  if (code != 200) Serial.println("ACK failed");
  http.end();

  unread = false;
  digitalWrite(LED_PIN, LOW);
}

/*  START SCROLL  */
void startScroll() {
  if (currentMessage.length() == 0) return;

  matrix.displayClear();
  matrix.displayText(
    currentMessage.c_str(),
    PA_LEFT,
    40,      // speed
    0,       // pause
    PA_SCROLL_LEFT,
    PA_SCROLL_LEFT
  );
  scrolling = true;

  ackMessage();
}

void loop() {
  unsigned long now = millis();

  // check the server, ive really gotta optimise this next time uhh
  if (now - lastPoll > POLL_INTERVAL) {
    lastPoll = now;
    pollServer();
  }

  // Animate the text scrolling
  if (scrolling) {
    if (matrix.displayAnimate()) {
      scrolling = false;
      matrix.displayClear();
    }
  }

  // Button edge detection
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    if (now - lastDebounce > debounceDelay && !scrolling) {
      startScroll();
      lastDebounce = now;
    }
  }
  lastButtonState = currentButtonState;
}
