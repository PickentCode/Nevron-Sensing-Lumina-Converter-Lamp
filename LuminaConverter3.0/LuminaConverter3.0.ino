#include <Adafruit_NeoPixel.h>
#include <Adafruit_PN532.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <Preferences.h>

// Pins
#define SDA_PIN 21
#define SCL_PIN 22
#define PN532_DUMMY_IRQ_PIN 25
#define PN532_DUMMY_RST_PIN 26
#define PIN_WS2812B 16
#define NUM_LEDS 62

Preferences prefs;

enum Effects {
    FX_SOLID,
    FX_BREATH,
    FX_RUNNERS,
    FX_SPARKLE
};

enum Nevrons {
    NEV_OFF,
    NEV_STALACT,
    NEV_PETANK,
    NEV_GROSSE_TETE,
    NEV_NOIR,
    NEV_ROCHER
};
const char* nevronIDs[] = { "00:00:00:00", "CD:C5:D0:E2", "95:78:4F:06", "53:62:5E:F7", "D3:A0:53:F7", "D3:BB:2C:F7" };
const uint8_t nevronCount = sizeof(nevronIDs) / sizeof(nevronIDs[0]);
//uint8_t nevronColors[3][3] = { {0, 0, 0}, {0, 255, 255}, {255, 160, 0} };
uint8_t nevronColors[6][4][3] = {
    { {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} },
    { {0, 255, 255}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} },
    { {255, 160, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} },
    { {255, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} },
    { {160, 0, 255}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} },
    { {255, 255, 255}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} }
};
uint8_t nevronEffects[6] = { 0, 0, 0, 0, 0, 0 };
uint8_t active_nevron = NEV_OFF;
uint8_t current_col_id = 0;
uint8_t current_color[3] = { 0, 0, 0 };

void setup() {
    Serial.begin(115200);
    delay(200);
    setupPN532();
    delay(200);
    setupLEDs();
    delay(200);
    setupAccessPoint();
    delay(200);
    loadEffects();
    delay(200);
    loadColorMatrix();
}

void loop() {
    updatePN532();
    updateLEDs();
    updateAccessPoint();
}

void loadEffects() {
    for (int i = NEV_STALACT; i < nevronCount; i++) {
        loadEffect(i);
    }
}

void loadColorMatrix() {
    for (int i = NEV_STALACT; i < nevronCount; i++) {
        loadColors(i);
    }
}

void saveEffect(uint8_t id, uint8_t fx)
{
    if (id >= nevronCount || id == 0) return;
    char key[16];
    snprintf(key, sizeof(key), "f%u", id);

    prefs.begin("ledcfg", false);
    prefs.putUChar(key, fx);
    prefs.end();
}

bool loadEffect(uint8_t id)
{
    if (id >= nevronCount || id == 0) return false;
    char key[16];
    snprintf(key, sizeof(key), "f%u", id);

    prefs.begin("ledcfg", true);
    if (prefs.isKey(key)) {
        nevronEffects[id] = prefs.getUChar(key, 0);
        prefs.end();
        return true;
    }
    prefs.end();
    nevronEffects[id] = 0;
    return false;
}

void saveColors(uint8_t id)
{
    if (id >= nevronCount || id == 0) return;
    char key[16];
    snprintf(key, sizeof(key), "c%u", id);

    prefs.begin("ledcfg", false);
    prefs.putBytes(key, nevronColors[id], 12);
    prefs.end();
}

bool loadColors(uint8_t id) {
    bool loaded = false;
    if (id >= nevronCount || id == 0) return loaded;
    char key[16];
    snprintf(key, sizeof(key), "c%u", id);

    prefs.begin("ledcfg", true);
    if (prefs.isKey(key)) {
        loaded = prefs.getBytes(key, nevronColors[id], 12);
    }
    prefs.end();
    return loaded;
}