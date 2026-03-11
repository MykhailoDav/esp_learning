/**
 * LAB 2.3 — SPI Protocol on ESP32-S3
 * Module:  NXP PN532 NFC/RFID Reader (v3) via SPI
 *
 * PN532 DIP switches for SPI mode:
 *   SW1 = OFF (LOW)
 *   SW2 = OFF (LOW)
 *
 * Wiring (SPI2 / FSPI on ESP32-S3):
 *   PN532 SCK  → GPIO12
 *   PN532 MISO → GPIO13
 *   PN532 MOSI → GPIO11
 *   PN532 SS   → GPIO10
 *   PN532 VCC  → 3.3V
 *   PN532 GND  → GND
 *
 * Demo features:
 *   1. Firmware version read
 *   2. Continuous card scan loop
 *   3. UID display (hex + decimal)
 *   4. Card type identification (MIFARE Classic / Ultralight)
 *   5. MIFARE Classic: authenticate + read manufacturer block 0
 *   6. MIFARE Ultralight: read pages 0-9
 *   7. Key storage: register cards as keys (saved to NVS flash)
 *      Serial commands:
 *        'r' — register current card as key
 *        'd' — delete key by index
 *        'l' — list all stored keys
 *        'c' — clear all keys
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Preferences.h>

// ─── SPI pins ────────────────────────────────────────────────────────────────
#define SPI_SCK  12
#define SPI_MISO 13
#define SPI_MOSI 11
#define PN532_CS 10

// ─── Key storage config ───────────────────────────────────────────────────────
#define MAX_KEYS      10       // max registered keys
#define KEY_NS        "nfc_keys"  // NVS namespace

// ─── Default MIFARE key ───────────────────────────────────────────────────────
static const uint8_t MIFARE_DEFAULT_KEY[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ─── Globals ─────────────────────────────────────────────────────────────────
Adafruit_PN532 nfc(PN532_CS);
Preferences    prefs;

// Last scanned card (for 'r' command)
uint8_t g_lastUID[7] = {0};
uint8_t g_lastUIDLen = 0;

// ─── Helpers ─────────────────────────────────────────────────────────────────

void printHex(const uint8_t *buf, uint8_t len, bool spaces = true) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
        if (spaces && i < len - 1) Serial.print(' ');
    }
}

// ─── Key storage: NVS helpers ─────────────────────────────────────────────────

/** Build NVS key string like "k00", "k01", ... */
void keyName(uint8_t idx, char *out) {
    sprintf(out, "k%02u", idx);
}

/** How many keys are currently stored */
uint8_t countKeys() {
    prefs.begin(KEY_NS, true);
    uint8_t n = prefs.getUChar("count", 0);
    prefs.end();
    return n;
}

/** Save count to NVS */
void saveCount(uint8_t n) {
    prefs.begin(KEY_NS, false);
    prefs.putUChar("count", n);
    prefs.end();
}

/** Load UID of key at index. Returns actual UID length or 0 if not found. */
uint8_t loadKey(uint8_t idx, uint8_t *uid) {
    char kn[8];
    keyName(idx, kn);
    prefs.begin(KEY_NS, true);
    uint8_t len = prefs.getBytesLength(kn);
    if (len > 0 && len <= 7) prefs.getBytes(kn, uid, len);
    prefs.end();
    return len;
}

/** Save a UID as a new key. Returns assigned index or 255 on failure. */
uint8_t saveKey(const uint8_t *uid, uint8_t uidLen) {
    uint8_t n = countKeys();
    if (n >= MAX_KEYS) return 255;
    char kn[8];
    keyName(n, kn);
    prefs.begin(KEY_NS, false);
    prefs.putBytes(kn, uid, uidLen);
    prefs.putUChar("count", n + 1);
    prefs.end();
    return n;
}

/** Delete key at index, shift remaining keys down */
bool deleteKey(uint8_t idx) {
    uint8_t n = countKeys();
    if (idx >= n) return false;

    prefs.begin(KEY_NS, false);
    // Shift keys idx+1..n-1 down by one
    for (uint8_t i = idx; i < n - 1; i++) {
        char src[8], dst[8];
        keyName(i + 1, src);
        keyName(i,     dst);
        uint8_t tmp[7];
        uint8_t len = prefs.getBytesLength(src);
        prefs.getBytes(src, tmp, len);
        prefs.putBytes(dst, tmp, len);
        prefs.remove(src);
    }
    // Remove last slot
    char last[8];
    keyName(n - 1, last);
    prefs.remove(last);
    prefs.putUChar("count", n - 1);
    prefs.end();
    return true;
}

/** Clear ALL keys */
void clearAllKeys() {
    uint8_t n = countKeys();
    prefs.begin(KEY_NS, false);
    for (uint8_t i = 0; i < n; i++) {
        char kn[8]; keyName(i, kn);
        prefs.remove(kn);
    }
    prefs.putUChar("count", 0);
    prefs.end();
}

/** Check if a UID matches any stored key. Returns index or 255. */
uint8_t findKey(const uint8_t *uid, uint8_t uidLen) {
    uint8_t n = countKeys();
    for (uint8_t i = 0; i < n; i++) {
        uint8_t stored[7];
        uint8_t sLen = loadKey(i, stored);
        if (sLen == uidLen && memcmp(stored, uid, uidLen) == 0)
            return i;
    }
    return 255;
}

// ─── List all keys to Serial ──────────────────────────────────────────────────
void listKeys() {
    uint8_t n = countKeys();
    Serial.printf("\n  Stored keys: %d / %d\n", n, MAX_KEYS);
    if (n == 0) { Serial.println("  (none)"); return; }
    for (uint8_t i = 0; i < n; i++) {
        uint8_t uid[7];
        uint8_t len = loadKey(i, uid);
        Serial.printf("  [%02u] ", i);
        printHex(uid, len);
        Serial.printf("  (%d bytes)\n", len);
    }
}

// ─── Handle Serial commands ───────────────────────────────────────────────────
void handleSerial() {
    if (!Serial.available()) return;
    char cmd = Serial.read();
    // Flush rest of line
    while (Serial.available()) Serial.read();

    Serial.println();
    switch (cmd) {
        case 'r': {
            if (g_lastUIDLen == 0) {
                Serial.println("  No card scanned yet — scan a card first.");
                break;
            }
            // Check if already registered
            if (findKey(g_lastUID, g_lastUIDLen) != 255) {
                Serial.print("  Already registered: ");
                printHex(g_lastUID, g_lastUIDLen);
                Serial.println();
                break;
            }
            uint8_t idx = saveKey(g_lastUID, g_lastUIDLen);
            if (idx == 255) {
                Serial.printf("  Key storage full! (%d/%d)\n", MAX_KEYS, MAX_KEYS);
            } else {
                Serial.printf("  ✓ Key saved as [%02u]: ", idx);
                printHex(g_lastUID, g_lastUIDLen);
                Serial.println();
            }
            break;
        }
        case 'd': {
            Serial.print("  Delete key index: ");
            // Wait briefly for index digit(s)
            uint32_t t = millis();
            while (!Serial.available() && millis() - t < 3000) delay(10);
            if (!Serial.available()) { Serial.println("timeout"); break; }
            uint8_t idx = (uint8_t)Serial.parseInt();
            if (deleteKey(idx)) {
                Serial.printf("  ✓ Key [%02u] deleted.\n", idx);
            } else {
                Serial.printf("  Key [%02u] not found.\n", idx);
            }
            break;
        }
        case 'l':
            listKeys();
            break;
        case 'c':
            clearAllKeys();
            Serial.println("  ✓ All keys cleared.");
            break;
        default:
            Serial.println("  Commands: r=register  d=delete  l=list  c=clear");
            break;
    }
    Serial.println();
}

// ─── MIFARE Classic demo ──────────────────────────────────────────────────────
void demoMifareClassic(uint8_t *uid, uint8_t uidLen) {
    Serial.println("  [ MIFARE Classic ]");

    if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLen, 0, 0,
                                             (uint8_t *)MIFARE_DEFAULT_KEY)) {
        Serial.println("  Auth sector 0 FAILED (non-default key?)");
        return;
    }
    Serial.println("  Auth sector 0: OK");

    uint8_t block[16] = {0};
    if (nfc.mifareclassic_ReadDataBlock(0, block)) {
        Serial.print("  Block 0 (manufacturer): ");
        printHex(block, 16);
        Serial.println();
        Serial.printf("  Manufacturer UID: %02X %02X %02X %02X\n",
                      block[0], block[1], block[2], block[3]);
        Serial.printf("  BCC (check byte):  0x%02X\n", block[4]);
        Serial.printf("  SAK from block 0:  0x%02X\n", block[5]);
    } else {
        Serial.println("  Block 0 read FAILED");
    }

    for (uint8_t blk = 1; blk <= 2; blk++) {
        if (nfc.mifareclassic_ReadDataBlock(blk, block)) {
            Serial.printf("  Block %d:           ", blk);
            printHex(block, 16);
            Serial.println();
        }
    }
    Serial.println("  Block 3: sector trailer (keys hidden by hardware)");
}

// ─── MIFARE Ultralight / NTAG demo ───────────────────────────────────────────
void demoMifareUltralight() {
    Serial.println("  [ MIFARE Ultralight / NTAG ]");
    Serial.println("  Pages 0-9:");
    for (uint8_t page = 0; page <= 9; page++) {
        uint8_t buf[4] = {0};
        if (nfc.mifareultralight_ReadPage(page, buf)) {
            Serial.printf("    Page %02u: ", page);
            printHex(buf, 4);
            if (page >= 4) {
                Serial.print("  | ");
                for (uint8_t b = 0; b < 4; b++)
                    Serial.print((buf[b] >= 0x20 && buf[b] < 0x7F) ? (char)buf[b] : '.');
            }
            Serial.println();
        } else {
            Serial.printf("    Page %02u: read failed\n", page);
        }
    }
}

// ─── SETUP ───────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("╔══════════════════════════════════════════════╗");
    Serial.println("║  LAB 2.3 — SPI  |  PN532 NFC Reader Demo    ║");
    Serial.println("╚══════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("SPI pins:  SCK=%d  MISO=%d  MOSI=%d  CS=%d\n",
                  SPI_SCK, SPI_MISO, SPI_MOSI, PN532_CS);
    Serial.println("PN532 DIP: SW1=OFF  SW2=OFF  (SPI mode)");
    Serial.println();

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, PN532_CS);
    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("ERROR: PN532 not found! Check wiring and DIP switches.");
        while (1) delay(1000);
    }
    Serial.println("PN532 found!");
    Serial.printf("  Chip:     PN5%02X\n",   (versiondata >> 24) & 0xFF);
    Serial.printf("  Firmware: v%d.%d\n",
                  (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);
    Serial.println();

    nfc.SAMConfig();

    Serial.printf("Stored keys: %d / %d\n", countKeys(), MAX_KEYS);
    listKeys();
    Serial.println();
    Serial.println("Commands: r=register last card  d<N>=delete key N  l=list  c=clear all");
    Serial.println("────────────────────────────────────────────────");
    Serial.println("Waiting for NFC card or tag...");
}

// ─── LOOP ────────────────────────────────────────────────────────────────────
void loop() {
    handleSerial();

    uint8_t uid[7] = {0};
    uint8_t uidLen = 0;

    bool found = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 500);
    if (!found) return;

    // Save for 'r' command
    memcpy(g_lastUID, uid, uidLen);
    g_lastUIDLen = uidLen;

    Serial.println();
    Serial.println("┌─── Card detected ──────────────────────────────");
    Serial.printf( "│ UID (%d bytes): ", uidLen);
    printHex(uid, uidLen);
    Serial.println();

    Serial.print("│ UID (DEC):    ");
    for (uint8_t i = 0; i < uidLen; i++) {
        Serial.print(uid[i]);
        if (i < uidLen - 1) Serial.print('-');
    }
    Serial.println();

    // ─── Key check ───────────────────────────────────────────────────────────
    uint8_t keyIdx = findKey(uid, uidLen);
    if (keyIdx != 255) {
        Serial.printf("│ 🔑 KEY MATCH  — slot [%02u]  ✅ ACCESS GRANTED\n", keyIdx);
    } else {
        Serial.println("│ ❌ Unknown card — not in key list");
    }
    Serial.println("│ Hint: press 'r' in Serial Monitor to register this card");

    Serial.println("├─── Memory ─────────────────────────────────────");

    if (uidLen == 4) {
        demoMifareClassic(uid, uidLen);
    } else if (uidLen == 7) {
        demoMifareUltralight();
    } else {
        Serial.println("  Unknown type — no memory demo.");
    }

    Serial.println("└────────────────────────────────────────────────");
    delay(2000);
}
