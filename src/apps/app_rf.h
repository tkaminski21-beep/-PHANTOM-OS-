#pragma once
// app_rf.h — RF Sub-GHz CC1101 dla CardputerOS3
// Piny CC1101: SCK=5 MISO=19 MOSI=23 CS=18 GDO0=4

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#ifndef CC1101_SCK
#define CC1101_SCK  5
#define CC1101_MISO 19
#define CC1101_MOSI 23
#define CC1101_CS   18
#define CC1101_GDO0 4
#endif

static float   _rf_freq    = 433.92f;
static uint8_t _rf_buf[64];
static int     _rf_buf_len = 0;

static bool _rf_init(float freq) {
    ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);
    ELECHOUSE_cc1101.Init();
    if (!ELECHOUSE_cc1101.getCC1101()) return false;
    ELECHOUSE_cc1101.setMHZ(freq);
    _rf_freq = freq;
    return true;
}

static bool _rf_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

static void _rf_no_module() {
    ui_show_info("Brak modulu CC1101!", THEME_RED);
    delay(2000);
}

// ── ODBIERANIE ────────────────────────────────────────
void app_rf_receive() {
    if (!_rf_init(_rf_freq)) { _rf_no_module(); return; }
    ui_draw_header("RF - ODBIOR", THEME_ORANGE);
    char fb[24]; snprintf(fb, sizeof(fb), "%.2f MHz", _rf_freq);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(160, 6); M5Cardputer.Display.print(fb);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("Czekam na sygnal...");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = wyjdz");

    ELECHOUSE_cc1101.SetRx();
    int count = 0;

    while (!_rf_check_esc()) {
        if (ELECHOUSE_cc1101.CheckReceiveFlag()) {
            _rf_buf_len = ELECHOUSE_cc1101.ReceiveData(_rf_buf);
            if (_rf_buf_len > 0) {
                count++;
                M5Cardputer.Display.fillRect(0, 20, 240, 100, THEME_BG);
                M5Cardputer.Display.setTextColor(THEME_GREEN);
                M5Cardputer.Display.setCursor(4, 26);
                char hdr[32]; snprintf(hdr, sizeof(hdr), "Odebrano #%d (%d B)", count, _rf_buf_len);
                M5Cardputer.Display.print(hdr);
                M5Cardputer.Display.setTextColor(THEME_CYAN);
                M5Cardputer.Display.setCursor(4, 42);
                String hex = "";
                for (int i = 0; i < min(_rf_buf_len, 16); i++) {
                    char b[4]; snprintf(b, sizeof(b), "%02X ", _rf_buf[i]); hex += b;
                }
                M5Cardputer.Display.print(hex);
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, 58);
                char rssi[24]; snprintf(rssi, sizeof(rssi), "RSSI: %d dBm", ELECHOUSE_cc1101.getRssi());
                M5Cardputer.Display.print(rssi);
                M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = wyjdz");
                ELECHOUSE_cc1101.SetRx();
            }
        }
        delay(10);
    }
    ELECHOUSE_cc1101.goSleep();
}

// ── REPLAY ────────────────────────────────────────────
void app_rf_replay() {
    if (_rf_buf_len == 0) {
        ui_show_info("Brak nagranego sygnalu!", THEME_RED);
        delay(2000); return;
    }
    if (!_rf_init(_rf_freq)) { _rf_no_module(); return; }
    ui_draw_header("RF - REPLAY", THEME_ORANGE);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 28);
    char buf[32]; snprintf(buf, sizeof(buf), "Wysylam %d bajtow...", _rf_buf_len);
    M5Cardputer.Display.print(buf);
    ELECHOUSE_cc1101.SetTx();
    for (int i = 0; i < 5; i++) {
        ELECHOUSE_cc1101.SendData(_rf_buf, _rf_buf_len);
        delay(100);
    }
    ELECHOUSE_cc1101.goSleep();
    ui_show_info("Wyslano 5x!", THEME_GREEN);
    delay(1500);
}

// ── SPEKTRUM ──────────────────────────────────────────
void app_rf_spectrum() {
    if (!_rf_init(433.92f)) { _rf_no_module(); return; }
    ui_draw_header("RF SPEKTRUM 430-440MHz", THEME_ORANGE);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop");

    const float FSTART = 430.0f, FSTEP = 0.5f;
    const int N = 21;
    const int BAR_W = 220 / N;

    while (!_rf_check_esc()) {
        M5Cardputer.Display.fillRect(10, 20, 220, 100, THEME_BG);
        M5Cardputer.Display.drawLine(10, 120, 230, 120, THEME_BORDER);
        for (int i = 0; i < N; i++) {
            ELECHOUSE_cc1101.setMHZ(FSTART + i * FSTEP);
            ELECHOUSE_cc1101.SetRx(); delay(2);
            int rssi = ELECHOUSE_cc1101.getRssi();
            int h = map(constrain(rssi, -100, -20), -100, -20, 0, 90);
            uint32_t col = h > 60 ? THEME_RED : (h > 30 ? THEME_ORANGE : THEME_GREEN);
            M5Cardputer.Display.fillRect(10+i*BAR_W, 120-h, BAR_W-1, h, col);
        }
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(10, 122); M5Cardputer.Display.print("430");
        M5Cardputer.Display.setCursor(195, 122); M5Cardputer.Display.print("440MHz");
    }
    ELECHOUSE_cc1101.goSleep();
    _rf_freq = 433.92f;
}

// ── ZMIANA CZĘSTOTLIWOŚCI ─────────────────────────────
static void _rf_select_freq() {
    static const float FREQS[] = {315.00f, 433.92f, 434.42f, 868.35f, 915.00f};
    const char* names[] = {"315.00 MHz","433.92 MHz","434.42 MHz","868.35 MHz","915.00 MHz"};
    int sel = ui_select_list(names, 5, "RF CZESTOTLIWOSC", THEME_ORANGE);
    if (sel >= 0) _rf_freq = FREQS[sel];
}

// ── GŁÓWNE MENU ───────────────────────────────────────
void app_rf_menu() {
    while (true) {
        ui_draw_header("RF Sub-GHz (CC1101)", THEME_ORANGE);
        char fb[24]; snprintf(fb, sizeof(fb), "%.2f MHz", _rf_freq);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(155, 6); M5Cardputer.Display.print(fb);

        const char* opts[] = {"Odbior/Nagrywanie","Replay (wyslij)","Spektrum 430-440","Czestotliwosc"};
        int sel = ui_select_list(opts, 4, "RF Sub-GHz (CC1101)", THEME_ORANGE);
        if (sel < 0) return;
        if (sel == 0) app_rf_receive();
        if (sel == 1) app_rf_replay();
        if (sel == 2) app_rf_spectrum();
        if (sel == 3) _rf_select_freq();
    }
}
