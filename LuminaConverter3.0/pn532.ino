Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

unsigned long time_to_update_PN532 = 0;

void setupPN532() {
    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("Didn't find PN532 module. Check connections.");
        while (1);
    }

    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    
    nfc.SAMConfig();
    Serial.println("PN532 ready!");
}

void updatePN532() {
    unsigned long wait_time = 500;
    unsigned long time = millis();
    if (time < time_to_update_PN532) return;
    if (time - last_player_ping < player_timeout) return;

    boolean success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 300);

    if (success) {
        Serial.println("Found a nevron!");
        String uidStr = makeUIDString(uid, uidLength);
        Serial.print("UID: ");
        Serial.println(uidStr);

        time_to_update_PN532 = time + wait_time;

        active_nevron = getNevron(uidStr);
    }
    else
    {
        active_nevron = NEV_OFF;
    }
}

String makeUIDString(uint8_t* uid, uint8_t len) {
    String s;
    for (uint8_t i = 0; i < len; i++){
        if (i) s+=":";
        if(uid[i] < 0x10) s+="0";
        s += String(uid[i], HEX);
    }
    s.toUpperCase(); return s;
}

uint8_t getNevron(const String& uidStr) {
    for (uint8_t i = 0; i < nevronCount; i++) if (uidStr.equalsIgnoreCase(nevronIDs[i])) return i;
    return NEV_OFF;
}