const char* ssid     = "Lumina Converter";
const char* password = "Parry it!";
const char* access_point_name = "lumina";
const int http_port = 80;
const int ws_port = 81;

int player_timeout = 5000;
unsigned long last_player_ping = -player_timeout;
float audioLevel = 0.0f;
uint8_t player_color[3] = { 255, 255, 255 };

WebServer server(http_port);
WebSocketsServer webSocket(ws_port);

static String mimeFor(const String& path) {
    if (path.endsWith(".html")) return "text/html";
    else if (path.endsWith(".css")) return "text/css";
    else if (path.endsWith(".js")) return "application/javascript";
    else if (path.endsWith(".png")) return "image/png";
    else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
    else if (path.endsWith(".svg")) return "image/svg+xml";
    else if (path.endsWith(".woff2")) return "font/woff2";
    return "application/octet-stream";
}

static void streamOr404(const char* path) {
    File f = LittleFS.open(path, "r");
    if (!f) { server.send(404, "text/plain", String("Not found: ") + path); return; }
    server.streamFile(f, mimeFor(path));
    f.close();
}

static void handle_root() { streamOr404("/index.html"); }
static void handle_static() {
    String path = server.uri();
    if (path == "/") path = "/index.html";
    File f = LittleFS.open(path, "r");
    if (!f) { server.send(404, "text/plain", "Not found"); return; }
    server.streamFile(f, mimeFor(path));
    f.close();
}

static void wsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) {
    IPAddress ip;
    String msg;
    uint8_t nev, fx, r, g, b;

    switch (type) {
        case WStype_CONNECTED:
            ip = webSocket.remoteIP(num);
            Serial.printf("[WS] Client %u connected from %s\n", num, ip.toString().c_str());
            //webSocket.sendTXT(num, "You connected");
            break;
        case WStype_TEXT:
            msg = String((const char*)payload, len);
            //Serial.printf("[WS] INPUT from #%u: %s\n", num, msg.c_str());
            if (msg == "PLAYER_PING") {
                unsigned long time = millis();
                last_player_ping = time;
            }
            break;
        case WStype_BIN:
            if (payload[0] == 0xF0) {
                nev = payload[1];
                fx = payload[2];
                nevronEffects[nev] = fx;
                saveEffect(nev, fx);
                Serial.printf("Effect data received!");
            }
            else if (payload[0] == 0xC0) {
                nev = payload[1];
                if (nev < nevronCount) {
                    const size_t HEADER = 2;
                    const size_t CH = 3;
                    const size_t SLOTS  = 4;

                    size_t bytes_after = (len > HEADER) ? (len - HEADER) : 0;
                    size_t triplets = bytes_after / CH;
                    if (triplets > SLOTS) triplets = SLOTS;

                    for (size_t slot = 0; slot < triplets; slot++) {
                        size_t base = HEADER + slot * CH;
                        nevronColors[nev][slot][0] = payload[base + 0];
                        nevronColors[nev][slot][1] = payload[base + 1];
                        nevronColors[nev][slot][2] = payload[base + 2];
                    }

                    for (size_t slot = triplets; slot < SLOTS; slot++) {
                        nevronColors[nev][slot][0] = 0;
                        nevronColors[nev][slot][1] = 0;
                        nevronColors[nev][slot][2] = 0;
                    }
                }
                saveColors(nev);
                Serial.printf("Color data received!");
            }
            else if (payload[0] == 0xA0) {
                memcpy(&audioLevel, payload + 1, sizeof(float));
            }
            else if (payload[0] == 0xCA) {
                player_color[0] = payload[1];
                player_color[1] = payload[2];
                player_color[2] = payload[3];
            }
            break;
        case WStype_DISCONNECTED:
            Serial.printf("[WS] Client %u disconnected\n", num);
            break;
        default: break;
    }
}

void setupAccessPoint() {
    // Reset
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    delay(50);
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    delay(100);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount FAILED");
    } else {
        Serial.println("LittleFS mounted");
    }

    WiFi.mode(WIFI_AP);
    bool apOK;
    apOK = WiFi.softAP(ssid);
    WiFi.setSleep(false);
    if (apOK) {
        Serial.print("AP: "); Serial.println(ssid);
        Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
        if (MDNS.begin("lumina")) {
            MDNS.addService("http", "tcp", http_port);
            MDNS.addService("ws",   "tcp", ws_port);
            Serial.println("mDNS: http://lumina.local/");
        } else {
            Serial.println("mDNS start failed");
        }
    } else {
        Serial.println("AP start FAILED");
    }

    server.on("/", handle_root);
    server.onNotFound(handle_static);
    server.begin();
    Serial.println("HTTP server started");

    webSocket.begin();
    webSocket.onEvent(wsEvent);
    Serial.printf("WebSocket: ws://%s:%u/\n", WiFi.softAPIP().toString().c_str(), ws_port);
}

void updateAccessPoint() {
    webSocket.loop();
    server.handleClient();
}