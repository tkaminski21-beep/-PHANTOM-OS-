#pragma once
// app_ir.h — Podczerwień IR dla CardputerOS3
// IR TX: GPIO 44 (wbudowany), IR RX: GPIO 2 (zewnętrzny TSOP)

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

#ifndef IR_TX_PIN
#define IR_TX_PIN 44
#endif
#ifndef IR_RX_PIN
#define IR_RX_PIN 2
#endif

static IRsend _ir_send(IR_TX_PIN);
static IRrecv _ir_recv(IR_RX_PIN, 1024, 50, true);
static bool   _ir_init_done = false;

static void _ir_init() {
    if (!_ir_init_done) { _ir_send.begin(); _ir_init_done = true; }
}

static bool _ir_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ── ODBIERANIE ────────────────────────────────────────
void app_ir_receive() {
    ui_draw_header("IR - ODBIOR", THEME_RED);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 28);
    M5Cardputer.Display.print("Czekam na sygnal IR...");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125);
    M5Cardputer.Display.print("FN+Q = wyjdz");

    _ir_recv.enableIRIn();
    decode_results results;

    while (true) {
        if (_ir_check_esc()) { _ir_recv.disableIRIn(); return; }
        if (_ir_recv.decode(&results)) {
            _ir_recv.disableIRIn();
            ui_draw_header("IR - ODEBRANO", THEME_RED);
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("Odebrano!");
            M5Cardputer.Display.setTextColor(THEME_TEXT);
            char buf[48];
            snprintf(buf, sizeof(buf), "Protokol: %s", typeToString(results.decode_type).c_str());
            M5Cardputer.Display.setCursor(4, 42); M5Cardputer.Display.print(buf);
            snprintf(buf, sizeof(buf), "Wartosc:  0x%llX", results.value);
            M5Cardputer.Display.setCursor(4, 56); M5Cardputer.Display.print(buf);
            snprintf(buf, sizeof(buf), "Bity:     %d", results.bits);
            M5Cardputer.Display.setCursor(4, 70); M5Cardputer.Display.print(buf);
            snprintf(buf, sizeof(buf), "RAW len:  %d", results.rawlen);
            M5Cardputer.Display.setCursor(4, 84); M5Cardputer.Display.print(buf);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
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
        delay(10);
    }
}

// ── WYSYŁANIE PRESETÓW ────────────────────────────────
struct IRPreset { const char* nazwa; uint8_t proto; uint64_t address; uint64_t command; };
static const IRPreset IR_PRESETS[] = {
    {"TV Power (NEC)",    1, 0x00, 0x45},
    {"TV Vol+  (NEC)",    1, 0x00, 0x46},
    {"TV Vol-  (NEC)",    1, 0x00, 0x15},
    {"TV CH+   (NEC)",    1, 0x00, 0x09},
    {"TV CH-   (NEC)",    1, 0x00, 0x19},
    {"TV Mute  (NEC)",    1, 0x00, 0x0D},
    {"Samsung Power",     2, 0x07, 0x02},
    {"Samsung Vol+",      2, 0x07, 0x07},
    {"Samsung Vol-",      2, 0x07, 0x0B},
};
const int IR_PRESET_COUNT = 9;

void app_ir_send_preset() {
    _ir_init();
    const char* names[IR_PRESET_COUNT];
    for (int i = 0; i < IR_PRESET_COUNT; i++) names[i] = IR_PRESETS[i].nazwa;
    while (true) {
        int sel = ui_select_list(names, IR_PRESET_COUNT, "IR WYSYLANIE", THEME_RED);
        if (sel < 0) return;
        const IRPreset& p = IR_PRESETS[sel];
        switch (p.proto) {
            case 1: _ir_send.sendNEC(p.address<<8|p.command, 32); break;
            case 2: _ir_send.sendSAMSUNG(p.address<<8|p.command); break;
        }
        ui_show_info(("Wyslano: " + String(p.nazwa)).c_str(), THEME_GREEN);
        delay(800);
    }
}

// ── TV-B-GONE ─────────────────────────────────────────
void app_ir_tvbgone() {
    _ir_init();
    static const uint32_t codes[] = {
        0x20DF10EF,0xE0E040BF,0x00FF02FD,0x00FF38C7,
        0x10AF8877,0xC1AA09F6,0x0000480B,0xE0E019E6,
        0x00FF48B7,0x04FB48B7,
    };
    const int N = 10;
    ui_draw_header("TV-B-GONE", THEME_RED);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("Wylaczam wszystkie TV...");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop");

    int count = 0;
    while (true) {
        for (int i = 0; i < N; i++) {
            if (_ir_check_esc()) return;
            _ir_send.sendNEC(codes[i], 32); delay(100);
            _ir_send.sendNEC(codes[i], 32); delay(200);
            count++;
            M5Cardputer.Display.fillRect(4, 42, 220, 14, THEME_BG);
            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setCursor(4, 44);
            char buf[32]; snprintf(buf, sizeof(buf), "Kod %d/%d  Wyslano: %d", i+1, N, count);
            M5Cardputer.Display.print(buf);
        }
    }
}

// ── GŁÓWNE MENU ───────────────────────────────────────
void app_ir_menu() {
    while (true) {
        const char* opts[] = {"Odbior IR","Wyslij preset","TV-B-Gone"};
        int sel = ui_select_list(opts, 3, "PODRCZERWIEN IR", THEME_RED);
        if (sel < 0) return;
        if (sel == 0) app_ir_receive();
        if (sel == 1) app_ir_send_preset();
        if (sel == 2) app_ir_tvbgone();
    }
}

