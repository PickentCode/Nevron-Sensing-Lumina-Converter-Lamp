Adafruit_NeoPixel ws2812b(NUM_LEDS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

unsigned long time_to_update_LEDs = 0;
const uint8_t step_interval_ms = 40;
const float tail_weights[3] = {1.0f, 0.35f, 0.12f};

// color swap
unsigned long lastSwap = 0;
const int swapInterval = 3000;
const int swapTime = 500;
const int max_color_count = 4;
bool isSwapping = false;
unsigned long swapStartMs = 0;
int next_col_id = 0;
uint8_t from_color[3] = {0, 0, 0};
uint8_t target_color[3] = {0, 0, 0};

// runner effect
unsigned long lastCall = 0;
int callFreq = 100;
int runnerHeads[32];
const uint8_t spawn_interval_steps = 20;
uint8_t activeRunnerCount = 0;
unsigned int stepsSinceLastSpawn = spawn_interval_steps;

// breathing effect
const float breathingInterval = 80.0f;

// sparkle effect
const int maxActiveSparkles = 15;
const float fadeUpPerMS = 1.0f / 200.0f;
const float fadeDownPerMS = 1.0f / 600.0f;
const int spawnChance = 50;
float bright[NUM_LEDS];
float vel[NUM_LEDS];
uint32_t last = 0;
bool seeded = false;

void setupLEDs() {
    ws2812b.begin();
    ws2812b.clear();
    ws2812b.show();
}

void updateLEDs() {

    unsigned long time = millis();

    if (time < time_to_update_LEDs) return;
    time_to_update_LEDs = time + step_interval_ms;

    if (time - last_player_ping < player_timeout) {
        playerColor(player_color[0], player_color[1], player_color[2]);
        return;
    }

    if (active_nevron == NEV_OFF) {
        ws2812b.clear();
        ws2812b.show();
        current_color[0] = 0;
        current_color[1] = 0;
        current_color[2] = 0;
        return;
    }

    swapColors();

    switch (nevronEffects[active_nevron]) {
        case FX_SOLID:
            solidColor(current_color[0], current_color[1], current_color[2]);
            break;
        case FX_BREATH:
            breathingEffect(current_color[0], current_color[1], current_color[2]);
            break;
        case FX_RUNNERS:
            runnerEffect(current_color[0], current_color[1], current_color[2]);
            break;
        case FX_SPARKLE:
            sparkleEffect(current_color[0], current_color[1], current_color[2]);
            break;
    }
}

int nextValidIndex(int start, int count) {
    int idx = start;
    for (int k = 0; k < count; ++k) {
        idx = (idx + 1) % count;
        const uint8_t* c = nevronColors[active_nevron][idx];
        if ((c[0] | c[1] | c[2]) != 0) return idx;
    }
    return start;
}

void swapColors() {
    unsigned long now = millis();

    if (!isSwapping) {
        if (now - lastSwap < (unsigned long)swapInterval) return;

        next_col_id = nextValidIndex(current_col_id, max_color_count);
        for (int i = 0; i < 3; ++i) {
            from_color[i]   = current_color[i];
            target_color[i] = nevronColors[active_nevron][next_col_id][i];
        }
        swapStartMs = now;
        isSwapping  = true;
    }
    float t = (float)(now - swapStartMs) / (float)swapTime;
    if (t > 1.0f) t = 1.0f;

    for (int i = 0; i < 3; ++i) {
        float v = (1.0f - t) * (float)from_color[i] + t * (float)target_color[i];
        if (v < 0.0f) v = 0.0f; else if (v > 255.0f) v = 255.0f;
        current_color[i] = (uint8_t)(v + 0.5f);
    }
    if (t >= 1.0f) {
        current_col_id = next_col_id;
        lastSwap = now;
        isSwapping = false;
    }
}

void solidColor(uint8_t baseR, uint8_t baseG, uint8_t baseB) {
    ws2812b.clear();
    for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex+=3) {
        uint32_t cv = ws2812b.Color(baseR, baseG, baseB);
        cv = ws2812b.gamma32(cv);
        ws2812b.setPixelColor(ledIndex, cv);
    }
    ws2812b.show();
}

void sparkleEffect(uint8_t baseR, uint8_t baseG, uint8_t baseB) {
    if (!seeded) {
        #if defined(ESP32)
            randomSeed((uint32_t)esp_random());
        #else
            randomSeed(analogRead(A0));
        #endif
        for (int i = 0; i < NUM_LEDS; i++) {
            bright[i] = 0.0f;
            vel[i] = 0.0f;
        }
        seeded = true;
    }

    uint32_t now = millis();
    uint32_t dt  = (last == 0) ? 16 : (now - last);
    last = now;

    int active = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (bright[i] > 0.0f) active++;
    }
    if (active < maxActiveSparkles && random(100) < spawnChance) {
        for (int tries = 0; tries < 4; ++tries) {
            int i = (int)random(NUM_LEDS);
            if (bright[i] <= 0.0f) {
                bright[i] = 0.0f;
                vel[i] = fadeUpPerMS;
                break;
            }
        }
    }
    for (int i = 0; i < NUM_LEDS; i++) {
        if (vel[i] != 0.0f) {
            bright[i] += vel[i] * (float)dt;
            if (bright[i] >= 1.0f) {
                bright[i] = 1.0f;
                vel[i] = -fadeDownPerMS;
            }
            if (bright[i] <= 0.0f) {
                bright[i] = 0.0f;
                vel[i] = 0.0f;
            }
        }
        uint8_t r = (uint8_t)((float)baseR * bright[i]);
        uint8_t g = (uint8_t)((float)baseG * bright[i]);
        uint8_t b = (uint8_t)((float)baseB * bright[i]);
        uint32_t cv = ws2812b.Color(r, g, b);
        cv = ws2812b.gamma32(cv);
        ws2812b.setPixelColor(i, cv);
    }
    ws2812b.show();
}

void breathingEffect(uint8_t baseR, uint8_t baseG, uint8_t baseB) {
    ws2812b.clear();
    unsigned long time = millis();
    float weight = cosf((float)(time / 10) / breathingInterval) + 1;
    weight /= 2;
    for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex+=3) {
        uint8_t r = uint8_t(baseR * weight);
        uint8_t g = uint8_t(baseG * weight);
        uint8_t b = uint8_t(baseB * weight);
        uint32_t cv = ws2812b.Color(r, g, b);
        cv = ws2812b.gamma32(cv);
        ws2812b.setPixelColor(ledIndex, cv);
    }
    ws2812b.show();
}

void runnerEffect(uint8_t baseR, uint8_t baseG, uint8_t baseB) {
    unsigned long time = millis();
    if (time < lastCall + callFreq) return;
    lastCall = time;

    if (stepsSinceLastSpawn >= spawn_interval_steps && activeRunnerCount < 32) {
        runnerHeads[activeRunnerCount++] = 0;
        stepsSinceLastSpawn = 0;
    }
    ws2812b.clear();
    for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex++) {
        float maxWeight = 0.0f;
        for (uint8_t i = 0; i < activeRunnerCount; i++) {
            int d = abs(ledIndex - runnerHeads[i]);
            if (d <= 2 && tail_weights[d] > maxWeight) maxWeight = tail_weights[d];
        }
        if (maxWeight > 0.0f && (baseR | baseG | baseB)) {
            uint8_t r = uint8_t(baseR * maxWeight);
            uint8_t g = uint8_t(baseG * maxWeight);
            uint8_t b = uint8_t(baseB * maxWeight);
            uint32_t cv = ws2812b.Color(r, g, b);
            cv = ws2812b.gamma32(cv);
            ws2812b.setPixelColor(ledIndex, cv);
        }
    }
    ws2812b.show();

    for (uint8_t i = 0; i < activeRunnerCount; i++) runnerHeads[i]++;
    stepsSinceLastSpawn++;

    uint8_t w = 0;
    for (uint8_t i = 0; i < activeRunnerCount; i++) if (runnerHeads[i] <= (NUM_LEDS - 1 + 2)) runnerHeads[w++] = runnerHeads[i];
    activeRunnerCount = w;
}

void playerColor(uint8_t baseR, uint8_t baseG, uint8_t baseB) {
    ws2812b.clear();
    for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex+=3) {
        uint8_t r = uint8_t(baseR * audioLevel);
        uint8_t g = uint8_t(baseG * audioLevel);
        uint8_t b = uint8_t(baseB * audioLevel);
        uint32_t cv = ws2812b.Color(r, g, b);
        cv = ws2812b.gamma32(cv);
        ws2812b.setPixelColor(ledIndex, cv);
    }
    ws2812b.show();
}