#include <BLEDevice.h>
#include <BLEServer.h>
#include <Adafruit_NeoPixel.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Adafruit_NeoPixel pixels(1, 48, NEO_GRB + NEO_KHZ800);

enum Mode { STEADY, RAINBOW, BLINK_SLOW, BLINK_FAST, BREATHING, HEARTBEAT, POLICE, OFF };
Mode currentMode = STEADY;
uint32_t currentColor = 0xFF0000;
int brightness = 100;
int speedValue = 100;

float animPos = 0; // ตัวเก็บตำแหน่งแอนิเมชัน
unsigned long lastMs = 0;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      size_t len = pCharacteristic->getLength();
      if (len == 1) currentMode = (Mode)data[0];
      else if (len == 3) { currentMode = STEADY; currentColor = pixels.Color(data[0], data[1], data[2]); }
      else if (len == 2) {
        if (data[0] == 0xFE) { brightness = data[1]; pixels.setBrightness(brightness); }
        else if (data[0] == 0xFD) { speedValue = data[1]; }
      }
    }
};

void setup() {
  pixels.begin();
  pixels.setBrightness(brightness);
  BLEDevice::init("S3-BT-Ultimate");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pChar = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pChar->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
  lastMs = millis();
}

void loop() {
  unsigned long currentMs = millis();
  float delta = (currentMs - lastMs) / 1000.0; // เวลาที่ผ่านไปเป็นวินาที
  lastMs = currentMs;

  float s = speedValue / 50.0; // ปรับสเกลความเร็วให้เหมาะสม
  animPos += delta * s; // สะสมตำแหน่งแอนิเมชัน (ทำให้สมูท)
  if (animPos > 1000) animPos -= 1000; // ป้องกันเลขเยอะเกินไป

  if (currentMode == STEADY) {
    pixels.setPixelColor(0, currentColor);
  } 
  else if (currentMode == RAINBOW) {
    pixels.setPixelColor(0, pixels.ColorHSV(animPos * 10000));
  }
  else if (currentMode == BLINK_SLOW) {
    pixels.setPixelColor(0, (fmod(animPos, 2.0) < 1.0) ? currentColor : 0);
  }
  else if (currentMode == BLINK_FAST) {
    pixels.setPixelColor(0, (fmod(animPos, 0.4) < 0.2) ? currentColor : 0);
  }
  else if (currentMode == BREATHING) {
    float val = (exp(sin(animPos * PI)) - 0.36787944) * 108.0;
    pixels.setPixelColor(0, pixels.Color((currentColor>>16 & 0xFF)*val/255, (currentColor>>8 & 0xFF)*val/255, (currentColor & 0xFF)*val/255));
  }
  else if (currentMode == HEARTBEAT) {
    float time = fmod(animPos, 1.0);
    if (time < 0.1 || (time > 0.2 && time < 0.3)) pixels.setPixelColor(0, currentColor);
    else pixels.setPixelColor(0, 0);
  }
  else if (currentMode == POLICE) {
    if (fmod(animPos, 0.6) < 0.3) pixels.setPixelColor(0, 255, 0, 0);
    else pixels.setPixelColor(0, 0, 0, 255);
  }
  else if (currentMode == OFF) {
    pixels.clear();
  }
  
  pixels.show();
  delay(10);
}
