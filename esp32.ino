#include <BLEDevice.h>
#include <BLEServer.h>
#include <Adafruit_NeoPixel.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- HARDWARE CONFIGURATION (4 Pins for 4 Zones) ---
#define REAR_PIN   5
#define LEFT_PIN   18
#define RIGHT_PIN  19
#define FRONT_PIN  21

#define REAR_COUNT  45
#define LEFT_COUNT  60
#define RIGHT_COUNT 60
#define FRONT_COUNT 60

Adafruit_NeoPixel pixels_rear(REAR_COUNT, REAR_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_left(LEFT_COUNT, LEFT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_right(RIGHT_COUNT, RIGHT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_front(FRONT_COUNT, FRONT_PIN, NEO_GRB + NEO_KHZ800);

enum Mode { STEADY, RAINBOW, BLINK_SLOW, BLINK_FAST, BREATHING, HEARTBEAT, POLICE, OFF };

struct ZoneState {
  Mode currentMode = STEADY;
  uint32_t currentColor = 0xFF0000;
  int brightness = 50;
  int speedValue = 100;
  float animPos = 0;
};
ZoneState zones[4];

unsigned long lastMs = 0;

// Server Connection Callback
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Device Connected");
    };
    void onDisconnect(BLEServer* pServer) {
      Serial.println("Device Disconnected");
      pServer->getAdvertising()->start();
    }
};

// BLE Characteristic Callback
// Payload format: [ZoneID, Cmd, Data...]
// ZoneID: 0=Rear, 1=Left, 2=Right, 3=Front, 4=All Zones
// Cmd: 1=Mode, 2=Color, 3=Brightness, 4=Speed
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* data = pCharacteristic->getData();
      size_t len = pCharacteristic->getLength();
      
      if (len < 3) return;
      
      uint8_t targetZone = data[0];
      uint8_t cmd = data[1];
      
      for (int z = 0; z < 4; z++) {
        if (targetZone == 4 || targetZone == z) {
          if (cmd == 1 && len >= 3) {
            zones[z].currentMode = (Mode)data[2];
          }
          else if (cmd == 2 && len >= 5) {
            if (zones[z].currentMode == RAINBOW || zones[z].currentMode == POLICE || zones[z].currentMode == OFF) {
              zones[z].currentMode = STEADY;
            }
            zones[z].currentColor = pixels_rear.Color(data[2], data[3], data[4]);
          }
          else if (cmd == 3 && len >= 3) {
            zones[z].brightness = data[2];
            if (z == 0) pixels_rear.setBrightness(zones[z].brightness);
            if (z == 1) pixels_left.setBrightness(zones[z].brightness);
            if (z == 2) pixels_right.setBrightness(zones[z].brightness);
            if (z == 3) pixels_front.setBrightness(zones[z].brightness);
          }
          else if (cmd == 4 && len >= 3) {
            zones[z].speedValue = data[2];
          }
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  
  pixels_rear.begin();
  pixels_left.begin();
  pixels_right.begin();
  pixels_front.begin();
  
  pixels_rear.setBrightness(zones[0].brightness);
  pixels_left.setBrightness(zones[1].brightness);
  pixels_right.setBrightness(zones[2].brightness);
  pixels_front.setBrightness(zones[3].brightness);
  
  BLEDevice::init("S3-BT-Ultimate");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pChar = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pChar->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
  
  lastMs = millis();
}

void applyZonesAndShow() {
  pixels_rear.show();
  pixels_left.show();
  pixels_right.show();
  pixels_front.show();
}

void renderZone(int z, float delta, Adafruit_NeoPixel &pixels, int count) {
  ZoneState &s = zones[z];
  float speedFactor = s.speedValue / 50.0;
  s.animPos += delta * speedFactor;
  if (s.animPos > 1000) s.animPos -= 1000;

  if (s.currentMode == STEADY) {
    for (int i = 0; i < count; i++) pixels.setPixelColor(i, s.currentColor);
  }
  else if (s.currentMode == RAINBOW) {
    for (int i = 0; i < count; i++) pixels.setPixelColor(i, pixels.ColorHSV((s.animPos * 10000) + (i * 800)));
  }
  else if (s.currentMode == BLINK_SLOW) {
    uint32_t c = (fmod(s.animPos, 2.0) < 1.0) ? s.currentColor : 0;
    for (int i = 0; i < count; i++) pixels.setPixelColor(i, c);
  }
  else if (s.currentMode == BLINK_FAST) {
    uint32_t c = (fmod(s.animPos, 0.4) < 0.2) ? s.currentColor : 0;
    for (int i = 0; i < count; i++) pixels.setPixelColor(i, c);
  }
  else if (s.currentMode == BREATHING) {
    float val = (exp(sin(s.animPos * PI)) - 0.36787944) * 108.0;
    uint8_t r = (s.currentColor >> 16) & 0xFF;
    uint8_t g = (s.currentColor >> 8) & 0xFF;
    uint8_t b = s.currentColor & 0xFF;
    uint32_t c = pixels.Color((r * val) / 255, (g * val) / 255, (b * val) / 255);
    for (int i = 0; i < count; i++) pixels.setPixelColor(i, c);
  }
  else if (s.currentMode == HEARTBEAT) {
    float time = fmod(s.animPos, 1.0);
    uint32_t c = (time < 0.1 || (time > 0.2 && time < 0.3)) ? s.currentColor : 0;
    for (int i = 0; i < count; i++) pixels.setPixelColor(i, c);
  }
  else if (s.currentMode == POLICE) {
    bool showRed = (fmod(s.animPos, 0.6) < 0.3);
    uint32_t cL = showRed ? pixels.Color(0, 0, 255) : 0;
    uint32_t cR = showRed ? 0 : pixels.Color(255, 0, 0);
    int half = count / 2;
    for (int i = 0; i < count; i++) {
      pixels.setPixelColor(i, (i < half) ? cL : cR);
    }
  }
  else if (s.currentMode == OFF) {
    pixels.clear();
  }
}

void loop() {
  unsigned long currentMs = millis();
  float delta = (currentMs - lastMs) / 1000.0;
  lastMs = currentMs;

  renderZone(0, delta, pixels_rear, REAR_COUNT);
  renderZone(1, delta, pixels_left, LEFT_COUNT);
  renderZone(2, delta, pixels_right, RIGHT_COUNT);
  renderZone(3, delta, pixels_front, FRONT_COUNT);

  applyZonesAndShow();
  delay(10);
}
