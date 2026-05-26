#include <BLEDevice.h>
#include <BLEServer.h>
#include <Adafruit_NeoPixel.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LED_PIN             5
#define LED_COUNT           45

Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

enum Mode { STEADY, RAINBOW, BLINK_SLOW, BLINK_FAST, BREATHING, HEARTBEAT, POLICE, OFF };
Mode currentMode = STEADY;
uint32_t currentColor = 0xFF0000;
int brightness = 100;
int speedValue = 100;

float animPos = 0;
unsigned long lastMs = 0;

// --- เพิ่มส่วนนี้: คลาสจัดการการเชื่อมต่อ ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Device Connected");
    };
    void onDisconnect(BLEServer* pServer) {
      Serial.println("Device Disconnected");
      // เมื่อหลุดการเชื่อมต่อ ให้เริ่มประกาศชื่อ (Advertising) ใหม่อีกครั้งทันที!
      pServer->getAdvertising()->start();
    }
};
// ------------------------------------

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      size_t len = pCharacteristic->getLength();
      if (len == 1) currentMode = (Mode)data[0];
      else if (len == 3) {
        if (currentMode == RAINBOW || currentMode == POLICE || currentMode == OFF) currentMode = STEADY;
        currentColor = pixels.Color(data[0], data[1], data[2]);
      }
      else if (len == 2) {
        if (data[0] == 0xFE) { brightness = data[1]; pixels.setBrightness(brightness); }
        else if (data[0] == 0xFD) { speedValue = data[1]; }
      }
    }
};

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(brightness);
  
  BLEDevice::init("S3-BT-Ultimate");
  BLEServer *pServer = BLEDevice::createServer();
  
  // --- เพิ่มส่วนนี้: กำหนด Callback ให้กับ Server ---
  pServer->setCallbacks(new MyServerCallbacks());
  // -------------------------------------------
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pChar = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pChar->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
  
  lastMs = millis();
}

void loop() {
  unsigned long currentMs = millis();
  float delta = (currentMs - lastMs) / 1000.0;
  lastMs = currentMs;

  float s = speedValue / 50.0;
  animPos += delta * s;
  if (animPos > 1000) animPos -= 1000;

  if (currentMode == STEADY) {
    for (int i = 0; i < LED_COUNT; i++) {
      pixels.setPixelColor(i, currentColor);
    }
  }
  else if (currentMode == RAINBOW) {
    for (int i = 0; i < LED_COUNT; i++) {
      pixels.setPixelColor(i, pixels.ColorHSV((animPos * 10000) + (i * 800)));
    }
  }
  else if (currentMode == BLINK_SLOW) {
    uint32_t color = (fmod(animPos, 2.0) < 1.0) ? currentColor : 0;
    for (int i = 0; i < LED_COUNT; i++) {
      pixels.setPixelColor(i, color);
    }
  }
  else if (currentMode == BLINK_FAST) {
    uint32_t color = (fmod(animPos, 0.4) < 0.2) ? currentColor : 0;
    for (int i = 0; i < LED_COUNT; i++) {
      pixels.setPixelColor(i, color);
    }
  }
  else if (currentMode == BREATHING) {
    float val = (exp(sin(animPos * PI)) - 0.36787944) * 108.0;
    uint8_t r = (currentColor >> 16) & 0xFF;
    uint8_t g = (currentColor >> 8) & 0xFF;
    uint8_t b = currentColor & 0xFF;
    uint32_t breathColor = pixels.Color((r * val) / 255, (g * val) / 255, (b * val) / 255);
    for (int i = 0; i < LED_COUNT; i++) {
      pixels.setPixelColor(i, breathColor);
    }
  }
  else if (currentMode == HEARTBEAT) {
    float time = fmod(animPos, 1.0);
    uint32_t color = (time < 0.1 || (time > 0.2 && time < 0.3)) ? currentColor : 0;
    for (int i = 0; i < LED_COUNT; i++) {
      pixels.setPixelColor(i, color);
    }
  }
  else if (currentMode == POLICE) {
    int half = LED_COUNT / 2;
    bool showRed = (fmod(animPos, 0.6) < 0.3);
    for (int i = 0; i < LED_COUNT; i++) {
      if ((i < half) == showRed) {
        pixels.setPixelColor(i, 255, 0, 0);
      } else {
        pixels.setPixelColor(i, 0, 0, 255);
      }
    }
  }
  else if (currentMode == OFF) {
    pixels.clear();
  }
  
  pixels.show();
  delay(10);
}
