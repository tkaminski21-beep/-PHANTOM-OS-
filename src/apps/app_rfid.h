#pragma once
// app_rfid.h — Czytnik/zapisywacz RFID przez moduł RC522 (Grove/SPI)
//
// PODŁĄCZENIE RC522 do Cardputer ADV:
//   RC522 SDA  → CS pin (np. 1 - Grove SCL)
//   RC522 SCK  → SCK (40 - na bus SD)
//   RC522 MOSI → MOSI (14 - na bus SD)
//   RC522 MISO → MISO (39 - na bus SD)
//   RC522 RST  → IRQ pin (np. 2)
//   RC522 GND  → GND
//   RC522 3V3  → 3V3
//
// UWAGA: Współdzieli SPI z kartą SD - operacje sekwencyjne.

#include "M5Cardputer.h"
#include "core/ui.h"
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>

extern uint16_t UI_BG, UI_FG, UI_PRI;

#ifndef RFID_CS
#define RFID_CS  1
#endif
#ifndef RFID_RST
#define RFID_RST 2
#endif

static MFRC522 _mfrc(RFID_CS, RFID_RST);
static bool _rfid_initialized = false;

static void _rfid_init() {
    if (_rfid_initialized) return;
    SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, RFID_CS);
    pinMode(RFID_CS, OUTPUT);
    digitalWrite(RFID_CS, HIGH);
    _mfrc.PCD_Init();
    delay(50);
    _rfid_initialized = true;
}

static String _uid_to_hex(MFRC522::Uid uid) {
    String s = "";
    for (byte i = 0; i < uid.size; i++) {
        if (uid.uidByte[i] < 0x10) s += "0";
        s += String(uid.uidByte[i], HEX);
        if (i < uid.size - 1) s += ":";
    }
    s.toUpperCase();
    return s;
}

// ─── SKAN UID ──────────────────────────────────────────
void app_rfid_skan() {
    _rfid_init();

    M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(70, 5); M5Cardputer.Display.print("RFID - SKAN UID");
    M5Cardputer.Display.drawLine(0, 18, 240, 18, (uint32_t)UI_PRI);

    M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(8, 26); M5Cardputer.Display.print("Przyloz karte do RC522...");
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ESC=wyjdz  ENTER=zapisz");

    String last_uid = "";
    int found_count = 0;

    while (true) {
        input_update();
        if (EscPress) return;

        if (_mfrc.PICC_IsNewCardPresent() && _mfrc.PICC_ReadCardSerial()) {
            String uid = _uid_to_hex(_mfrc.uid);
            byte type = _mfrc.PICC_GetType(_mfrc.uid.sak);
            const char* type_str = (const char*)_mfrc.PICC_GetTypeName((MFRC522::PICC_Type)type);

            if (uid != last_uid) {
                found_count++;
                last_uid = uid;

                M5Cardputer.Display.fillRect(0, 22, 240, 100, (uint32_t)UI_BG);

                M5Cardputer.Display.setTextSize(1);
                M5Cardputer.Display.setTextColor(0x07E0, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(8, 26); M5Cardputer.Display.print("KARTA #");
                M5Cardputer.Display.print(found_count);

                M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(8, 42); M5Cardputer.Display.print("UID:");
                M5Cardputer.Display.setTextSize(2);
                M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(8, 56); M5Cardputer.Display.print(uid);
                M5Cardputer.Display.setTextSize(1);

                M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(8, 80);
                M5Cardputer.Display.print("Typ: "); M5Cardputer.Display.print(type_str);

                M5Cardputer.Display.setCursor(8, 94);
                M5Cardputer.Display.print("SAK: 0x");
                M5Cardputer.Display.print(_mfrc.uid.sak, HEX);

                M5Cardputer.Display.setCursor(8, 108);
                M5Cardputer.Display.print("Bajtow UID: ");
                M5Cardputer.Display.print(_mfrc.uid.size);
            }
            _mfrc.PICC_HaltA();
        }

        if (SelPress && last_uid.length() > 0) {
            // Zapisz na SD
            SD.mkdir("/rfid");
            char path[40];
            snprintf(path, sizeof(path), "/rfid/uid_%lu.txt", millis()/1000);
            File f = SD.open(path, FILE_WRITE);
            if (f) {
                f.println("# Karta RFID");
                f.print("UID: "); f.println(last_uid);
                f.print("SAK: 0x"); f.println(_mfrc.uid.sak, HEX);
                f.print("Czas: "); f.println(millis());
                f.close();
                ui_show_info("Zapisano na SD!", 0x07E0);
                delay(1500);
                M5Cardputer.Display.fillRect(0, 22, 240, 100, (uint32_t)UI_BG);
                M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(8, 26);
                M5Cardputer.Display.print("Przyloz karte...");
                last_uid = "";
            } else {
                ui_show_info("Blad zapisu SD!", 0xF800);
                delay(1500);
            }
        }

        delay(80);
    }
}

// ─── INFO O KARCIE — czytaj zawartość bloku 0 ─────────
void app_rfid_info() {
    _rfid_init();

    M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(70, 5); M5Cardputer.Display.print("RFID - INFO");
    M5Cardputer.Display.drawLine(0, 18, 240, 18, (uint32_t)UI_PRI);

    M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(8, 26); M5Cardputer.Display.print("Przyloz karte...");
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ESC=wyjdz");

    while (true) {
        input_update();
        if (EscPress) return;

        if (_mfrc.PICC_IsNewCardPresent() && _mfrc.PICC_ReadCardSerial()) {
            String uid = _uid_to_hex(_mfrc.uid);
            byte type = _mfrc.PICC_GetType(_mfrc.uid.sak);

            M5Cardputer.Display.fillRect(0, 22, 240, 100, (uint32_t)UI_BG);
            M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("UID:");
            M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(4, 38); M5Cardputer.Display.print(uid);

            // Próba odczytu pierwszego bloku (block 0 jest manufacturer data)
            MFRC522::MIFARE_Key key;
            for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

            byte buf[18];
            byte size = sizeof(buf);
            byte block = 4;  // Block 4 - pierwszy w sektorze 1

            MFRC522::StatusCode status = _mfrc.PCD_Authenticate(
                MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(_mfrc.uid));

            M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(4, 56); M5Cardputer.Display.print("Block 4:");
            if (status == MFRC522::STATUS_OK) {
                status = _mfrc.MIFARE_Read(block, buf, &size);
                M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
                if (status == MFRC522::STATUS_OK) {
                    char hex[60] = {0};
                    for (byte i = 0; i < 16; i++)
                        snprintf(hex+i*3, 4, "%02X ", buf[i]);
                    M5Cardputer.Display.setCursor(4, 70);
                    M5Cardputer.Display.print(hex);
                    M5Cardputer.Display.setCursor(4, 84);
                    String ascii = "";
                    for (byte i = 0; i < 16; i++) {
                        if (buf[i] >= 32 && buf[i] < 127) ascii += (char)buf[i];
                        else ascii += '.';
                    }
                    M5Cardputer.Display.print("ASCII: " + ascii);
                } else {
                    M5Cardputer.Display.setCursor(4, 70);
                    M5Cardputer.Display.print("Read FAILED");
                }
            } else {
                M5Cardputer.Display.setTextColor(0xF800, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(4, 70);
                M5Cardputer.Display.print("AUTH FAILED (default key)");
            }

            _mfrc.PICC_HaltA();
            _mfrc.PCD_StopCrypto1();
        }
        delay(150);
    }
}

// ─── MENU ─────────────────────────────────────────────
void app_rfid_menu() {
    while (true) {
        const char* opts[] = {"Skan UID", "Czytaj blok", "Info / podlaczenie"};
        int sel = ui_select_list(opts, 3, "RFID (RC522)", (uint32_t)UI_PRI);
        if (sel < 0) return;
        if (sel == 0) app_rfid_skan();
        if (sel == 1) app_rfid_info();
        if (sel == 2) {
            ui_draw_header("RFID - PODLACZENIE", (uint32_t)UI_PRI);
            M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("Modul RC522:");
            M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(4, 40); M5Cardputer.Display.print("SDA  -> GPIO1 (Grove)");
            M5Cardputer.Display.setCursor(4, 52); M5Cardputer.Display.print("SCK  -> GPIO40");
            M5Cardputer.Display.setCursor(4, 64); M5Cardputer.Display.print("MOSI -> GPIO14");
            M5Cardputer.Display.setCursor(4, 76); M5Cardputer.Display.print("MISO -> GPIO39");
            M5Cardputer.Display.setCursor(4, 88); M5Cardputer.Display.print("RST  -> GPIO2");
            M5Cardputer.Display.setCursor(4, 100); M5Cardputer.Display.print("3V3 + GND");
            M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF, (uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ENTER=powrot");
            while (true) {
                input_update();
                if (SelPress || EscPress) break;
                delay(20);
            }
        }
    }
}
