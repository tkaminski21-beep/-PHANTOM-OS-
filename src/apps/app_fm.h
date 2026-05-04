#pragma once
// app_fm.h — Radio FM (Si4713) dla CardputerOS3
// I2C: SDA=21 SCL=22

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <Wire.h>
#include <Adafruit_Si4713.h>

#ifndef FM_SDA
#define FM_SDA 21
#define FM_SCL 22
#endif

static Adafruit_Si4713 _fm(-1);
static bool  _fm_ok   = false;
static float _fm_freq = 100.0f;
static uint8_t _fm_power = 115;

static bool _fm_setup() {
    Wire.begin(FM_SDA, FM_SCL);
    _fm_ok = _fm.begin(0x11);
    if (_fm_ok) _fm.setProperty(SI4713_PROP_TX_PREEMPHASIS, 0);
    return _fm_ok;
}

static bool _fm_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ── NADAWANIE FM ─────────────────────────────────────
void app_fm_transmit() {
    if (!_fm_setup()) { ui_show_info("Brak Si4713!", THEME_RED); delay(2000); return; }

    _fm.setTXpower(_fm_power);
    _fm.tuneFM((uint16_t)(_fm_freq * 100));

    bool redraw = true;
    while (true) {
        if (redraw) {
            ui_draw_header("FM NADAWANIE", THEME_YELLOW);
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("NADAWANIE AKTYWNE");
            M5Cardputer.Display.setTextColor(THEME_TEXT);
            char buf[32];
            snprintf(buf, sizeof(buf), "Czestotliwosc: %.1f MHz", _fm_freq);
            M5Cardputer.Display.setCursor(4, 42); M5Cardputer.Display.print(buf);
            snprintf(buf, sizeof(buf), "Moc: %d dBuV", _fm_power);
            M5Cardputer.Display.setCursor(4, 56); M5Cardputer.Display.print(buf);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 76); M5Cardputer.Display.print("A/D = czest -/+0.1MHz");
            M5Cardputer.Display.setCursor(4, 90); M5Cardputer.Display.print("W/S = moc +/-");
            M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop nadawanie");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn) {
                for (char c : s.word) { if (c=='q'||c=='Q') goto fm_done; }
            } else {
                for (char c : s.word) {
                    if (c=='d'||c=='D') { _fm_freq += 0.1f; if (_fm_freq>108.0f) _fm_freq=87.5f; redraw=true; }
                    if (c=='a'||c=='A') { _fm_freq -= 0.1f; if (_fm_freq<87.5f)  _fm_freq=108.0f; redraw=true; }
                    if (c=='w'||c=='W') { if (_fm_power<120) _fm_power++; redraw=true; }
                    if (c=='s'||c=='S') { if (_fm_power>88)  _fm_power--; redraw=true; }
                    if (c==27) goto fm_done;
                }
            }
        }
        if (redraw) {
            _fm.setTXpower(_fm_power);
            _fm.tuneFM((uint16_t)(_fm_freq * 100));
        }
        delay(50);
    }
    fm_done:
    _fm.setTXpower(0);
}

// ── SPEKTRUM FM ───────────────────────────────────────
void app_fm_spectrum() {
    if (!_fm_setup()) { ui_show_info("Brak Si4713!", THEME_RED); delay(2000); return; }
    ui_draw_header("FM SPEKTRUM 87-108MHz", THEME_YELLOW);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop");

    const float FSTART = 87.5f, FSTEP = 0.2f;
    const int N = 103;
    const int BAR_W = max(1, 220/N);

    while (!_fm_check_esc()) {
        M5Cardputer.Display.fillRect(10, 20, 220, 100, THEME_BG);
        M5Cardputer.Display.drawLine(10, 120, 230, 120, THEME_BORDER);
        for (int i = 0; i < N; i++) {
            _fm.tuneFM((uint16_t)((FSTART + i*FSTEP)*100));
            delay(2);
            int noise = _fm.currNoiseLevel;
            int h = map(constrain(noise, 0, 255), 0, 255, 0, 90);
            uint32_t col = h>60 ? THEME_RED : (h>30 ? THEME_YELLOW : THEME_GREEN);
            M5Cardputer.Display.fillRect(10+i*BAR_W, 120-h, BAR_W-1, h, col);
        }
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(10, 122); M5Cardputer.Display.print("87.5");
        M5Cardputer.Display.setCursor(195, 122); M5Cardputer.Display.print("108MHz");
    }
    _fm.setTXpower(0);
}

// ── GŁÓWNE MENU ───────────────────────────────────────
void app_fm_menu() {
    while (true) {
        const char* opts[] = {"Nadawaj FM","Spektrum FM"};
        int sel = ui_select_list(opts, 2, "RADIO FM (Si4713)", THEME_YELLOW);
        if (sel < 0) return;
        if (sel == 0) app_fm_transmit();
        if (sel == 1) app_fm_spectrum();
    }
}
