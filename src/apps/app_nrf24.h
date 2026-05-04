#pragma once
// app_nrf24.h — NRF24L01 dla CardputerOS3
// Piny: CE=16 CSN=5 SCK=18 MOSI=23 MISO=19

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <RF24.h>
#include <SPI.h>

#ifndef NRF24_CE_PIN
#define NRF24_CE_PIN 16
#define NRF24_CS_PIN 5
#define NRF24_SCK    18
#define NRF24_MOSI   23
#define NRF24_MISO   19
#endif

static RF24 _nrf(NRF24_CE_PIN, NRF24_CS_PIN);
static bool _nrf_ok = false;

static bool _nrf_setup() {
    SPI.begin(NRF24_SCK, NRF24_MISO, NRF24_MOSI, NRF24_CS_PIN);
    _nrf_ok = _nrf.begin();
    return _nrf_ok;
}

static bool _nrf_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ── SPEKTRUM 2.4GHz ───────────────────────────────────
void app_nrf24_spectrum() {
    if (!_nrf_setup()) { ui_show_info("Brak NRF24L01!", THEME_RED); delay(2000); return; }
    ui_draw_header("NRF24 SPEKTRUM 2.4GHz", THEME_CYAN);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop");

    _nrf.setAutoAck(false);
    _nrf.startListening(); _nrf.stopListening();

    const int CH = 126;
    uint8_t vals[CH] = {0};

    while (!_nrf_check_esc()) {
        for (int ch = 0; ch < CH; ch++) {
            _nrf.setChannel(ch);
            _nrf.startListening(); delayMicroseconds(128); _nrf.stopListening();
            if (_nrf.testCarrier()) vals[ch] = min(vals[ch]+4, 100);
            else vals[ch] = vals[ch] > 2 ? vals[ch]-2 : 0;
        }
        M5Cardputer.Display.fillRect(0, 20, 240, 100, THEME_BG);
        M5Cardputer.Display.drawLine(0, 120, 240, 120, THEME_BORDER);
        for (int i = 0; i < CH; i++) {
            if (vals[i] == 0) continue;
            int x = i * 240 / CH;
            int w = max(1, 240/CH);
            int h = vals[i];
            uint32_t col = h>70 ? THEME_RED : (h>30 ? THEME_ORANGE : THEME_GREEN);
            M5Cardputer.Display.fillRect(x, 120-h, w, h, col);
        }
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 122); M5Cardputer.Display.print("2.4GHz  CH1:14  CH6:46  CH11:61");
    }
    _nrf.powerDown();
}

// ── SCANNER ───────────────────────────────────────────
void app_nrf24_scan() {
    if (!_nrf_setup()) { ui_show_info("Brak NRF24L01!", THEME_RED); delay(2000); return; }
    ui_draw_header("NRF24 SCANNER", THEME_CYAN);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("Szukam urzadzen...");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop");

    _nrf.setAutoAck(false);
    _nrf.setPayloadSize(32);
    _nrf.setAddressWidth(3);
    _nrf.openReadingPipe(1, 0xAA0000AA);
    _nrf.setDataRate(RF24_2MBPS);

    int found = 0, yPos = 42;
    for (int ch = 2; ch <= 84 && !_nrf_check_esc(); ch++) {
        _nrf.setChannel(ch);
        _nrf.startListening(); delay(3);
        if (_nrf.available()) {
            uint8_t buf[32]; _nrf.read(buf, 32);
            found++;
            if (yPos < 115) {
                M5Cardputer.Display.setTextColor(THEME_CYAN);
                M5Cardputer.Display.setCursor(4, yPos);
                char line[40];
                snprintf(line, sizeof(line), "CH%d: %02X%02X%02X%02X%02X", ch, buf[0],buf[1],buf[2],buf[3],buf[4]);
                M5Cardputer.Display.print(line);
                yPos += 12;
            }
        }
        _nrf.stopListening();
        M5Cardputer.Display.fillRect(100, 26, 140, 12, THEME_BG);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(100, 26);
        char prog[28]; snprintf(prog, sizeof(prog), "CH:%d znaleziono:%d", ch, found);
        M5Cardputer.Display.print(prog);
    }
    if (found == 0) { M5Cardputer.Display.setTextColor(THEME_MUTED); M5Cardputer.Display.setCursor(4,42); M5Cardputer.Display.print("Nic nie znaleziono"); }
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ENTER=wyjdz");
    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) { if (c=='\n'||c=='\r'||c==27) { _nrf.powerDown(); return; } }
            if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') { _nrf.powerDown(); return; } } }
        }
        if (M5Cardputer.BtnA.wasPressed()) { _nrf.powerDown(); return; }
        delay(10);
    }
}

// ── JAMMER ────────────────────────────────────────────
void app_nrf24_jammer() {
    if (!_nrf_setup()) { ui_show_info("Brak NRF24L01!", THEME_RED); delay(2000); return; }
    ui_draw_header("NRF24 JAMMER", THEME_RED);
    M5Cardputer.Display.setTextColor(THEME_RED);
    M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("UWAGA: Zakloca 2.4GHz WiFi/BT!");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 40); M5Cardputer.Display.print("ENTER=START  ESC=anuluj");

    bool confirmed = false;
    while (!confirmed) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) {
                if (c=='\n'||c=='\r') confirmed = true;
                if (c==27) { _nrf.powerDown(); return; }
            }
            if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') { _nrf.powerDown(); return; } } }
        }
        if (M5Cardputer.BtnA.wasPressed()) confirmed = true;
        delay(10);
    }

    _nrf.setAutoAck(false);
    _nrf.stopListening();
    _nrf.setDataRate(RF24_2MBPS);
    _nrf.setPALevel(RF24_PA_MAX);
    uint8_t payload[32];
    memset(payload, 0xFF, 32);

    M5Cardputer.Display.fillRect(0, 56, 240, 60, THEME_BG);
    M5Cardputer.Display.setTextColor(THEME_RED);
    M5Cardputer.Display.setCursor(4, 58); M5Cardputer.Display.print("JAMMING... FN+Q=STOP");

    int count = 0, ch = 2;
    while (!_nrf_check_esc()) {
        _nrf.setChannel(ch);
        _nrf.openWritingPipe(0xAA0000AA);
        _nrf.write(payload, 32);
        ch = (ch >= 84) ? 2 : ch+1;
        count++;
        if (count % 100 == 0) {
            M5Cardputer.Display.fillRect(4, 74, 220, 12, THEME_BG);
            M5Cardputer.Display.setTextColor(THEME_ORANGE);
            M5Cardputer.Display.setCursor(4, 74);
            char buf[32]; snprintf(buf, sizeof(buf), "Wyslano: %d  CH: %d", count, ch);
            M5Cardputer.Display.print(buf);
        }
    }
    _nrf.powerDown();
}

// ── INFO ──────────────────────────────────────────────
void app_nrf24_info() {
    if (!_nrf_setup()) { ui_show_info("Brak NRF24L01!", THEME_RED); delay(2000); return; }
    ui_draw_header("NRF24 INFO", THEME_CYAN);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print(_nrf_ok ? "Modul: OK" : "Modul: BRAK");
    char buf[40];
    snprintf(buf, sizeof(buf), "CE=%d  CSN=%d", NRF24_CE_PIN, NRF24_CS_PIN);
    M5Cardputer.Display.setCursor(4, 42); M5Cardputer.Display.print(buf);
    snprintf(buf, sizeof(buf), "SCK=%d MOSI=%d MISO=%d", NRF24_SCK, NRF24_MOSI, NRF24_MISO);
    M5Cardputer.Display.setCursor(4, 58); M5Cardputer.Display.print(buf);
    M5Cardputer.Display.setCursor(4, 74); M5Cardputer.Display.print("Pasmo: 2.4GHz (2400-2525 MHz)");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ENTER=wyjdz");
    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) { if (c=='\n'||c=='\r'||c==27) { _nrf.powerDown(); return; } }
        }
        if (M5Cardputer.BtnA.wasPressed()) { _nrf.powerDown(); return; }
        delay(10);
    }
}

// ── GŁÓWNE MENU ───────────────────────────────────────
void app_nrf24_menu() {
    while (true) {
        const char* opts[] = {"Spektrum 2.4GHz","Scanner urzadzen","Jammer 2.4GHz","Info/Piny"};
        int sel = ui_select_list(opts, 4, "NRF24L01 2.4GHz", THEME_CYAN);
        if (sel < 0) return;
        if (sel == 0) app_nrf24_spectrum();
        if (sel == 1) app_nrf24_scan();
        if (sel == 2) app_nrf24_jammer();
        if (sel == 3) app_nrf24_info();
    }
}
