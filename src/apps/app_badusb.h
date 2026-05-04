#pragma once
// app_badusb.h — Bad USB (BLE HID Keyboard) dla CardputerOS3
// Implementacja własna przez NimBLE — bez zewnętrznej biblioteki BLE Keyboard
//
// Skrypty .txt w /badusb/ na karcie SD:
//   STRING tekst    — wpisuje tekst
//   ENTER           — klawisz Enter
//   DELAY 500       — czeka 500ms
//   GUI r           — Win+r
//   CTRL c          — Ctrl+c
//   ALT F4          — Alt+F4
//   TAB             — Tab
//   ESC             — Escape
//   BACKSPACE       — Backspace

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>
#include <HIDTypes.h>
#include <SD.h>

// HID Report Map dla klawiatury USB (Boot Protocol)
static const uint8_t HID_REPORT_MAP[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x01, // Report ID (1)
    0x05, 0x07, // Usage Page (Key Codes)
    0x19, 0xe0, // Usage Minimum (224)
    0x29, 0xe7, // Usage Maximum (231)
    0x15, 0x00, // Logical Minimum (0)
    0x25, 0x01, // Logical Maximum (1)
    0x75, 0x01, // Report Size (1)
    0x95, 0x08, // Report Count (8)
    0x81, 0x02, // Input (Data, Variable, Absolute) - Modifier Keys
    0x95, 0x01, // Report Count (1)
    0x75, 0x08, // Report Size (8)
    0x81, 0x01, // Input (Constant) - Reserved
    0x95, 0x06, // Report Count (6)
    0x75, 0x08, // Report Size (8)
    0x15, 0x00, // Logical Minimum (0)
    0x25, 0x65, // Logical Maximum (101)
    0x05, 0x07, // Usage Page (Key Codes)
    0x19, 0x00, // Usage Minimum (0)
    0x29, 0x65, // Usage Maximum (101)
    0x81, 0x00, // Input (Data, Array)
    0xC0        // End Collection
};


extern uint16_t UI_BG, UI_FG, UI_PRI;

// ─── BLE HID Keyboard ────────────────────────────────
static NimBLEHIDDevice* _hid = nullptr;
static NimBLECharacteristic* _input = nullptr;
static bool _hid_connected = false;
static NimBLEServer* _hid_server = nullptr;

class HIDCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        _hid_connected = true;
    }
    void onDisconnect(NimBLEServer* pServer) override {
        _hid_connected = false;
        pServer->getAdvertising()->start();
    }
};

static void _hid_init() {
    NimBLEDevice::init("CardputerHID");
    NimBLEDevice::setSecurityAuth(false, false, true);
    NimBLEDevice::setPower(ESP_PWR_LVL_P21);

    _hid_server = NimBLEDevice::createServer();
    _hid_server->setCallbacks(new HIDCallbacks());

    _hid = new NimBLEHIDDevice(_hid_server);
    _input = _hid->inputReport(1);

    _hid->manufacturer()->setValue("M5Stack");
    _hid->pnp(0x02, 0x045e, 0x0750, 0x0300);
    _hid->hidInfo(0x00, 0x01);

    NimBLESecurity* pSecurity = new NimBLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_NO_BOND);

    _hid->reportMap((uint8_t*)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));
    _hid->startServices();

    NimBLEAdvertising* pAdv = _hid_server->getAdvertising();
    pAdv->setAppearance(HID_KEYBOARD);
    pAdv->addServiceUUID(_hid->hidService()->getUUID());
    pAdv->start();
}

static void _hid_deinit() {
    if (_hid_server) {
        _hid_server->getAdvertising()->stop();
        NimBLEDevice::deinit(true);
        _hid = nullptr;
        _input = nullptr;
        _hid_server = nullptr;
        _hid_connected = false;
    }
}

// HID key codes
#define HID_KEY_A         0x04
#define HID_KEY_RETURN    0x28
#define HID_KEY_ESCAPE    0x29
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_TAB       0x2B
#define HID_KEY_SPACE     0x2C
#define HID_MOD_LCTRL     0x01
#define HID_MOD_LSHIFT    0x02
#define HID_MOD_LALT      0x04
#define HID_MOD_LGUI      0x08

static void _hid_send(uint8_t mod, uint8_t key) {
    if (!_input || !_hid_connected) return;
    uint8_t msg[8] = {mod, 0, key, 0, 0, 0, 0, 0};
    _input->setValue(msg, sizeof(msg));
    _input->notify();
    delay(10);
    memset(msg, 0, 8);
    _input->setValue(msg, sizeof(msg));
    _input->notify();
    delay(5);
}

// ASCII → HID keycode (uproszczone, US layout)
static void _type_char(char ch) {
    uint8_t mod = 0, key = 0;
    if (ch >= 'a' && ch <= 'z') { key = HID_KEY_A + (ch - 'a'); }
    else if (ch >= 'A' && ch <= 'Z') { key = HID_KEY_A + (ch - 'A'); mod = HID_MOD_LSHIFT; }
    else if (ch >= '1' && ch <= '9') { key = 0x1E + (ch - '1'); }
    else if (ch == '0') { key = 0x27; }
    else if (ch == ' ') { key = HID_KEY_SPACE; }
    else if (ch == '\n') { key = HID_KEY_RETURN; }
    else if (ch == '\t') { key = HID_KEY_TAB; }
    else if (ch == '-') { key = 0x2D; }
    else if (ch == '=') { key = 0x2E; }
    else if (ch == '[') { key = 0x2F; }
    else if (ch == ']') { key = 0x30; }
    else if (ch == '\\') { key = 0x31; }
    else if (ch == ';') { key = 0x33; }
    else if (ch == '\'') { key = 0x34; }
    else if (ch == '`') { key = 0x35; }
    else if (ch == ',') { key = 0x36; }
    else if (ch == '.') { key = 0x37; }
    else if (ch == '/') { key = 0x38; }
    else if (ch == '!') { key = 0x1E; mod = HID_MOD_LSHIFT; }
    else if (ch == '@') { key = 0x1F; mod = HID_MOD_LSHIFT; }
    else if (ch == '#') { key = 0x20; mod = HID_MOD_LSHIFT; }
    else if (ch == '$') { key = 0x21; mod = HID_MOD_LSHIFT; }
    else if (ch == '%') { key = 0x22; mod = HID_MOD_LSHIFT; }
    else if (ch == '^') { key = 0x23; mod = HID_MOD_LSHIFT; }
    else if (ch == '&') { key = 0x24; mod = HID_MOD_LSHIFT; }
    else if (ch == '*') { key = 0x25; mod = HID_MOD_LSHIFT; }
    else if (ch == '(') { key = 0x26; mod = HID_MOD_LSHIFT; }
    else if (ch == ')') { key = 0x27; mod = HID_MOD_LSHIFT; }
    else if (ch == '_') { key = 0x2D; mod = HID_MOD_LSHIFT; }
    else if (ch == '+') { key = 0x2E; mod = HID_MOD_LSHIFT; }
    else if (ch == ':') { key = 0x33; mod = HID_MOD_LSHIFT; }
    else if (ch == '"') { key = 0x34; mod = HID_MOD_LSHIFT; }
    else if (ch == '<') { key = 0x36; mod = HID_MOD_LSHIFT; }
    else if (ch == '>') { key = 0x37; mod = HID_MOD_LSHIFT; }
    else if (ch == '?') { key = 0x38; mod = HID_MOD_LSHIFT; }
    if (key) _hid_send(mod, key);
}

static uint8_t _key_from_char(char c) {
    if (c >= 'a' && c <= 'z') return HID_KEY_A + (c - 'a');
    if (c >= 'A' && c <= 'Z') return HID_KEY_A + (c - 'A');
    if (c >= '1' && c <= '9') return 0x1E + (c - '1');
    if (c == '0') return 0x27;
    return 0;
}

static bool _badusb_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ─── PARSER KOMEND ────────────────────────────────────
static void _parse_line(String line) {
    line.trim();
    if (line.length() == 0 || line.startsWith("//") || line.startsWith("REM")) return;

    if (line.startsWith("STRING ")) {
        String txt = line.substring(7);
        for (size_t i = 0; i < txt.length(); i++) _type_char(txt[i]);
    }
    else if (line == "ENTER")     { _hid_send(0, HID_KEY_RETURN); }
    else if (line == "TAB")       { _hid_send(0, HID_KEY_TAB); }
    else if (line == "ESC")       { _hid_send(0, HID_KEY_ESCAPE); }
    else if (line == "BACKSPACE") { _hid_send(0, HID_KEY_BACKSPACE); }
    else if (line == "SPACE")     { _hid_send(0, HID_KEY_SPACE); }
    else if (line.startsWith("DELAY ")) { delay(line.substring(6).toInt()); }
    else if (line.startsWith("GUI ") && line.length() >= 5) {
        uint8_t k = _key_from_char(line.charAt(4));
        if (k) _hid_send(HID_MOD_LGUI, k);
    }
    else if (line.startsWith("CTRL ALT ") && line.length() >= 10) {
        uint8_t k = _key_from_char(line.charAt(9));
        if (k) _hid_send(HID_MOD_LCTRL | HID_MOD_LALT, k);
        else if (line == "CTRL ALT DELETE") _hid_send(HID_MOD_LCTRL|HID_MOD_LALT, 0x4C);
    }
    else if (line.startsWith("CTRL ") && line.length() >= 6) {
        uint8_t k = _key_from_char(line.charAt(5));
        if (k) _hid_send(HID_MOD_LCTRL, k);
    }
    else if (line.startsWith("ALT ") && line.length() >= 5) {
        uint8_t k = _key_from_char(line.charAt(4));
        if (k) _hid_send(HID_MOD_LALT, k);
        else if (line == "ALT F4") _hid_send(HID_MOD_LALT, 0x3D);
    }
    else if (line.startsWith("SHIFT ") && line.length() >= 7) {
        uint8_t k = _key_from_char(line.charAt(6));
        if (k) _hid_send(HID_MOD_LSHIFT, k);
    }
}

// ─── URUCHOM SKRYPT ───────────────────────────────────
static void _run_script(const String& path) {
    ui_draw_header("BadUSB - CZEKAM", UI_PRI);
    M5Cardputer.Display.setTextColor(UI_FG, UI_BG);
    M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("Paruj klawiature BLE:");
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setCursor(4, 40); M5Cardputer.Display.print("CardputerHID");
    M5Cardputer.Display.setTextColor(UI_FG, UI_BG);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = anuluj");

    _hid_init();
    unsigned long t0 = millis();
    while (!_hid_connected) {
        if (_badusb_check_esc()) { _hid_deinit(); return; }
        if (millis()-t0 > 60000) { ui_show_info("Timeout!", UI_PRI); delay(2000); _hid_deinit(); return; }
        M5Cardputer.Display.fillRect(200, 26, 30, 12, UI_BG);
        M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
        M5Cardputer.Display.setCursor(200, 28);
        M5Cardputer.Display.print((millis()-t0)/1000);
        delay(500);
    }

    ui_draw_header("BadUSB - WYKONUJE", UI_PRI);
    M5Cardputer.Display.setTextColor(0x07E0, UI_BG);
    M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("Polaczono!");
    M5Cardputer.Display.setTextColor(UI_FG, UI_BG);
    M5Cardputer.Display.setCursor(4, 40); M5Cardputer.Display.print(path);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop");

    delay(2000);  // chwila dla OS na inicjalizację
    File f = SD.open(path, FILE_READ);
    if (!f) { ui_show_info("Blad pliku SD!", UI_PRI); delay(2000); _hid_deinit(); return; }

    int lineNum = 0;
    while (f.available() && !_badusb_check_esc()) {
        String line = f.readStringUntil('\n');
        _parse_line(line);
        lineNum++;
        M5Cardputer.Display.fillRect(4, 56, 220, 12, UI_BG);
        M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
        M5Cardputer.Display.setCursor(4, 58);
        char buf[24]; snprintf(buf, sizeof(buf), "Linia: %d", lineNum);
        M5Cardputer.Display.print(buf);
    }
    f.close();
    ui_show_info("Skrypt zakonczony!", 0x07E0);
    delay(1500);
    _hid_deinit();
}

// ─── WYBÓR SKRYPTU ────────────────────────────────────
void app_badusb_select_script() {
    SD.mkdir("/badusb");
    std::vector<String> files;
    File dir = SD.open("/badusb");
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) {
            String n = String(entry.name());
            if (n.endsWith(".txt") || n.endsWith(".TXT")) files.push_back(n);
        }
        entry.close();
    }
    dir.close();

    if (files.empty()) {
        ui_draw_header("BadUSB - BRAK SKRYPTOW", UI_PRI);
        M5Cardputer.Display.setTextColor(UI_FG, UI_BG);
        M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("Wgraj .txt do /badusb/");
        M5Cardputer.Display.setCursor(4, 44); M5Cardputer.Display.print("Przyklad (hello.txt):");
        M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
        M5Cardputer.Display.setCursor(4, 58); M5Cardputer.Display.print("DELAY 1000");
        M5Cardputer.Display.setCursor(4, 70); M5Cardputer.Display.print("STRING Hello!");
        M5Cardputer.Display.setCursor(4, 82); M5Cardputer.Display.print("ENTER");
        M5Cardputer.Display.setTextColor(UI_FG, UI_BG);
        M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ENTER=powrot");
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

    const char** opts = new const char*[files.size()];
    for (size_t i = 0; i < files.size(); i++) opts[i] = files[i].c_str();
    int sel = ui_select_list(opts, files.size(), "BadUSB", UI_PRI);
    delete[] opts;
    if (sel >= 0) _run_script("/badusb/" + files[sel]);
}

// ─── PRZYKŁADY ────────────────────────────────────────
void app_badusb_create_examples() {
    SD.mkdir("/badusb");
    if (!SD.exists("/badusb/hello.txt")) {
        File f = SD.open("/badusb/hello.txt", FILE_WRITE);
        if (f) { f.println("DELAY 1000"); f.println("STRING Hello from CardputerOS BadUSB!"); f.println("ENTER"); f.close(); }
    }
    if (!SD.exists("/badusb/winr.txt")) {
        File f = SD.open("/badusb/winr.txt", FILE_WRITE);
        if (f) { f.println("DELAY 500"); f.println("GUI r"); f.println("DELAY 500"); f.println("STRING notepad"); f.println("ENTER"); f.println("DELAY 1500"); f.println("STRING Hacked by CardputerOS!"); f.close(); }
    }
    ui_show_info("Utworzono /badusb/*.txt", 0x07E0);
    delay(1500);
}

// ─── MENU ─────────────────────────────────────────────
void app_badusb_menu() {
    while (true) {
        const char* opts[] = {"Wybierz skrypt","Utworz przyklady","Info"};
        int sel = ui_select_list(opts, 3, "BadUSB (BLE HID)", UI_PRI);
        if (sel < 0) return;
        if (sel == 0) app_badusb_select_script();
        if (sel == 1) app_badusb_create_examples();
        if (sel == 2) {
            ui_draw_header("BadUSB INFO", UI_PRI);
            M5Cardputer.Display.setTextColor(UI_FG, UI_BG);
            M5Cardputer.Display.setCursor(4,26); M5Cardputer.Display.print("Cardputer jako BLE HID");
            M5Cardputer.Display.setCursor(4,38); M5Cardputer.Display.print("klawiatura. Komendy:");
            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setCursor(4,54); M5Cardputer.Display.print("STRING txt  ENTER  TAB  ESC");
            M5Cardputer.Display.setCursor(4,66); M5Cardputer.Display.print("DELAY ms   GUI r   CTRL c");
            M5Cardputer.Display.setCursor(4,78); M5Cardputer.Display.print("ALT F4     SHIFT a  REM...");
            M5Cardputer.Display.setTextColor(UI_FG, UI_BG);
            M5Cardputer.Display.setCursor(4,94); M5Cardputer.Display.print("Skrypty: /badusb/*.txt");
            M5Cardputer.Display.setCursor(4,125); M5Cardputer.Display.print("ENTER=powrot");
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
    }
}
