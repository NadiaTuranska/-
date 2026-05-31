#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>

const char* ssid = "iPhone Надія";
const char* password = "11111111";

String GOOGLE_SCRIPT_ID = "AKfycbyNzsKUBTf-zja0nwoPbtIupphPx6-e_12-gWA6-87GkJ9z9IVzvb74qF9Od5e-UXqN";
String patient = "Pacient";

#define PULSE_PIN 34
#define ONE_WIRE_PIN 32

#define SDA_RTC 22
#define SCL_RTC 19

#define OLED_CS   5
#define OLED_DC   4
#define OLED_RST  15
#define OLED_MOSI 23
#define OLED_SCLK 18

#define SD_CS   26
#define SD_MISO 27
#define SD_CLK  14
#define SD_MOSI 13

#define BUZZER 25

#define BTN_START 21
#define BTN_CANCEL 35
#define BTN_SEND 33

RTC_DS3231 rtc;
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);

SPIClass spiSD(HSPI);
Adafruit_SSD1331 display(OLED_CS, OLED_DC, OLED_MOSI, OLED_SCLK, OLED_RST);

bool isRecording = false;
bool sdReady = false;
bool fingerDetected = false;

float temperature = 0;
int pulseBPM = 0;

unsigned long recordStart = 0;
const unsigned long RECORD_TIME = 180000; // 3 хвилини

String fileName = "/data.txt";
File dataFile;

int threshold = 550;
int fingerThreshold = 500;

unsigned long lastBeatTime = 0;
int beatCount = 0;
unsigned long bpmStartTime = 0;

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 1000;

void beep(int ms) {
  tone(BUZZER, 2000);
  delay(ms);
  noTone(BUZZER);
}

void showOLED(String status, float temp, int bpm) {
  display.fillScreen(0x0000);

  display.setCursor(0, 0);
  display.setTextColor(0xFFFF);
  display.setTextSize(1);
  display.print(status);

  display.setCursor(0, 16);
  display.print("Temp: ");
  display.print(temp, 1);

  display.setCursor(0, 32);
  display.print("BPM: ");
  display.print(bpm);

  display.setCursor(0, 48);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi: OK");
  } else {
    display.print("WiFi: OFF");
  }
}

String getDateTime() {
  DateTime now = rtc.now();

  char buf[25];
  sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());

  return String(buf);
}

bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("WiFi not connected");
    return false;
  }
}

bool sendDataToGoogle(String date, float temp, int bpm) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  date.replace(" ", "T");

  String url = "https://script.google.com/macros/s/";
  url += GOOGLE_SCRIPT_ID;
  url += "/exec?pacient=";
  url += patient;
  url += "&date=";
  url += date;
  url += "&temperature=";
  url += String(temp, 1);
  url += "&pulse=";
  url += String(bpm);

  Serial.println("Sending to Google:");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();
  String payload = http.getString();

  http.end();

  Serial.print("HTTP code: ");
  Serial.println(httpCode);
  Serial.print("Response: ");
  Serial.println(payload);

  if (httpCode > 0 && payload.indexOf("OK") >= 0) {
    return true;
  }

  return false;
}

void saveToSD(String date, float temp, int bpm) {
  if (!sdReady) {
    Serial.println("SD not ready");
    showOLED("SD not ready", temp, bpm);
    return;
  }

  dataFile = SD.open(fileName, FILE_APPEND);

  if (dataFile) {
    dataFile.print(date);
    dataFile.print(";");
    dataFile.print(temp, 1);
    dataFile.print(";");
    dataFile.println(bpm);
    dataFile.close();

    Serial.println("Saved one record to SD");
    showOLED("Saved to SD", temp, bpm);
  } else {
    Serial.println("SD write error");
    showOLED("SD error", temp, bpm);
  }
}

void sendSavedDataFromSD() {
  if (!sdReady) {
    Serial.println("SD not ready");
    showOLED("SD not ready", temperature, pulseBPM);
    return;
  }

  if (!connectWiFi()) {
    Serial.println("No WiFi. Cannot send SD data.");
    showOLED("No WiFi", temperature, pulseBPM);
    return;
  }

  if (!SD.exists(fileName)) {
    Serial.println("No saved data on SD");
    showOLED("No SD data", temperature, pulseBPM);
    return;
  }

  File file = SD.open(fileName, FILE_READ);

  if (!file) {
    Serial.println("Cannot open SD file");
    showOLED("SD read error", temperature, pulseBPM);
    return;
  }

  bool allSent = true;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;

    int firstSep = line.indexOf(';');
    int secondSep = line.indexOf(';', firstSep + 1);

    if (firstSep == -1 || secondSep == -1) continue;

    String date = line.substring(0, firstSep);
    date.replace(" ", "T");

    float temp = line.substring(firstSep + 1, secondSep).toFloat();
    int bpm = line.substring(secondSep + 1).toInt();

    bool sent = sendDataToGoogle(date, temp, bpm);

    if (!sent) {
      allSent = false;
      break;
    }

    delay(500);
  }

  file.close();

  if (allSent) {
    SD.remove(fileName);
    Serial.println("All SD data sent. File cleared.");
    showOLED("SD sent OK", temperature, pulseBPM);
    beep(200);
  } else {
    Serial.println("Some data was not sent. File kept.");
    showOLED("Send error", temperature, pulseBPM);
    beep(500);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_START, INPUT);
  pinMode(BTN_CANCEL, INPUT);
  pinMode(BTN_SEND, INPUT);
  pinMode(PULSE_PIN, INPUT);

  Wire.begin(SDA_RTC, SCL_RTC);

  if (!rtc.begin()) {
    Serial.println("RTC not found!");
  } else {
    Serial.println("RTC OK");
  }

  ds18b20.begin();

  display.begin();
  display.fillScreen(0x0000);
  display.setCursor(0, 0);
  display.setTextColor(0xFFFF);
  display.setTextSize(1);
  display.print("System ready");

  spiSD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, spiSD)) {
    Serial.println("SD FAIL");
    sdReady = false;
  } else {
    Serial.println("SD OK");
    sdReady = true;
  }

  connectWiFi();

  showOLED("System ready", temperature, pulseBPM);
  beep(200);
}

void loop() {
  if (digitalRead(BTN_START) == HIGH && !isRecording) {
    isRecording = true;

    recordStart = millis();
    bpmStartTime = millis();
    lastBeatTime = millis();

    beatCount = 0;
    pulseBPM = 0;
    fingerDetected = false;
    lastDisplayUpdate = 0;

    beep(100);
    Serial.println("Recording started");

    showOLED("Recording...", temperature, pulseBPM);

    delay(500);
  }

  if (digitalRead(BTN_CANCEL) == HIGH && isRecording) {
    isRecording = false;

    beep(100);
    Serial.println("Recording canceled");

    showOLED("Canceled", temperature, pulseBPM);

    delay(500);
  }

  if (digitalRead(BTN_SEND) == HIGH) {
    beep(100);
    Serial.println("SEND button pressed");

    showOLED("Sending SD...", temperature, pulseBPM);

    sendSavedDataFromSD();

    delay(1000);
  }

  if (isRecording) {
    unsigned long now = millis();

    ds18b20.requestTemperatures();
    temperature = ds18b20.getTempCByIndex(0);

    int sensorValue = analogRead(PULSE_PIN);

    if (sensorValue < fingerThreshold) {
      fingerDetected = false;
      pulseBPM = 0;
      beatCount = 0;

      if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
        showOLED("No finger", temperature, pulseBPM);
        lastDisplayUpdate = now;
      }
    } else {
      fingerDetected = true;

      if (sensorValue > threshold && (now - lastBeatTime) > 300) {
        lastBeatTime = now;
        beatCount++;
      }

      if ((now - bpmStartTime) >= 5000) {
        pulseBPM = beatCount * 12;
        beatCount = 0;
        bpmStartTime = now;
      }

      if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
        showOLED("Recording...", temperature, pulseBPM);
        lastDisplayUpdate = now;
      }
    }

    if (now - recordStart >= RECORD_TIME) {
      isRecording = false;

      beep(200);
      Serial.println("Recording finished");

      if (fingerDetected && pulseBPM > 0) {
        saveToSD(getDateTime(), temperature, pulseBPM);
      } else {
        showOLED("No valid pulse", temperature, 0);
        Serial.println("No valid pulse. Data not saved.");
      }
    }
  }

  delay(200);
}