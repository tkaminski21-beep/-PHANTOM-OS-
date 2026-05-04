#pragma once
// core/colors.h — Edytor kolorów UI z presetami i własnym RGB
// Kolory zapisywane w Preferences i używane w całym systemie

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <Preferences.h>

extern Preferences prefs;
extern uint16_t UI_BG;
extern uint16_t UI_FG;
extern uint16_t UI_PRI;
extern void load_colors();
extern void save_colors();
extern bool _word_eq(const std::vector<char>& word, const char* str);

// ─── Konwersja RGB888 → RGB565 ────────────────────────
inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
inline void rgb888(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = ((c >> 11) & 0x1F) << 3;
    g = ((c >> 5)  & 0x3F) << 2;
    b = (c & 0x1F) << 3;
}

// ─── EDYTOR RGB ───────────────────────────────────────
// Edytuje 3 kanały: R, G, B (0-255)
// Strzałki — zmiana wartości aktywnego kanału
// TAB — następny kanał (R → G → B → R...)
// ENTER — zapisz dla wybranego elementu UI
static uint16_t _rgb_edit(const char* title, uint16_t initial, uint16_t bgcol, uint16_t pricol) {
    uint8_t r, g, b;
    rgb888(initial, r, g, b);
    int channel = 0;  // 0=R, 1=G, 2=B
    bool redraw = true;

    while (true) {
        if (redraw) {
            uint16_t cur = rgb565(r, g, b);
            M5Cardputer.Display.fillScreen(bgcol);
            // Ramka
            M5Cardputer.Display.drawRoundRect(5, 5, 230, 124, 5, pricol);
            M5Cardputer.Display.drawLine(5, 22, 235, 22, pricol);
            // Tytuł
            M5Cardputer.Display.setTextColor(pricol, bgcol);
            M5Cardputer.Display.setTextSize(1);
            int tlen = strlen(title) * 6;
            M5Cardputer.Display.setCursor((240 - tlen) / 2, 9);
            M5Cardputer.Display.print(title);

            // Podgląd koloru — duży kwadrat
            M5Cardputer.Display.fillRect(140, 30, 80, 50, cur);
            M5Cardputer.Display.drawRect(140, 30, 80, 50, pricol);

            // Wartości RGB i suwaki
            const char* labels[] = {"R", "G", "B"};
            uint8_t vals[] = {r, g, b};
            uint16_t cols[] = {rgb565(255,0,0), rgb565(0,255,0), rgb565(0,0,255)};

            for (int i = 0; i < 3; i++) {
                int y = 30 + i * 22;
                bool active = (i == channel);
                // Etykieta
                M5Cardputer.Display.setTextColor(active ? pricol : (cols[i] & 0xC618), bgcol);
                M5Cardputer.Display.setCursor(8, y);
                M5Cardputer.Display.print(labels[i]);

                // Suwak
                M5Cardputer.Display.drawRect(20, y, 110, 10, pricol);
                M5Cardputer.Display.fillRect(21, y+1, 108 * vals[i] / 255, 8, cols[i]);

                // Wartość liczbowa
                M5Cardputer.Display.setTextColor(active ? pricol : (pricol & 0xC618), bgcol);
                M5Cardputer.Display.setCursor(140, y + 14);
                if (i == channel) {
                    char buf[8]; snprintf(buf, sizeof(buf), "[%d]", vals[i]);
                    M5Cardputer.Display.print(buf);
                } else {
                    char buf[6]; snprintf(buf, sizeof(buf), "%d", vals[i]);
                    M5Cardputer.Display.print(buf);
                }
            }

            // Hex
            M5Cardputer.Display.setTextColor(pricol, bgcol);
            M5Cardputer.Display.setCursor(8, 100);
            char hex[20];
            snprintf(hex, sizeof(hex), "Hex: #%02X%02X%02X", r, g, b);
            M5Cardputer.Display.print(hex);

            M5Cardputer.Display.setCursor(8, 116);
            M5Cardputer.Display.print("FN+;/. wartosc  TAB kanal  ENTER OK");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn) {
                if (_word_eq(s.word, ";")) {
                    // zmniejsz
                    if (channel==0) r = max(0, r-5);
                    if (channel==1) g = max(0, g-5);
                    if (channel==2) b = max(0, b-5);
                    redraw = true;
                }
                if (_word_eq(s.word, ".")) {
                    if (channel==0) r = min(255, r+5);
                    if (channel==1) g = min(255, g+5);
                    if (channel==2) b = min(255, b+5);
                    redraw = true;
                }
                if (_word_eq(s.word, ",")) {
                    if (channel==0) r = max(0, r-25);
                    if (channel==1) g = max(0, g-25);
                    if (channel==2) b = max(0, b-25);
                    redraw = true;
                }
                if (_word_eq(s.word, "/")) {
                    if (channel==0) r = min(255, r+25);
                    if (channel==1) g = min(255, g+25);
                    if (channel==2) b = min(255, b+25);
                    redraw = true;
                }
            } else {
                for (char c : s.word) {
                    if (c == '\t') { channel = (channel + 1) % 3; redraw = true; }
                    if (c=='\n'||c=='\r') return rgb565(r, g, b);
                    if (c==27) return initial;  // anuluj
                }
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return rgb565(r, g, b);
        delay(20);
    }
}

// ─── MENU EDYCJI KOLORÓW ──────────────────────────────
void edit_custom_colors() {
    while (true) {
        const char* opts[] = {"Tlo (BG)", "Tekst (FG)", "Akcent/ramki (PRI)", "Zapisz i wyjdz"};
        int sel = ui_select_list(opts, 4, "EDYTOR KOLOROW RGB", UI_PRI);
        if (sel < 0) return;
        if (sel == 0) {
            uint16_t c = _rgb_edit("KOLOR TLA (BG)", UI_BG, UI_BG, UI_PRI);
            UI_BG = c;
        }
        if (sel == 1) {
            uint16_t c = _rgb_edit("KOLOR TEKSTU (FG)", UI_FG, UI_BG, UI_PRI);
            UI_FG = c;
        }
        if (sel == 2) {
            uint16_t c = _rgb_edit("KOLOR AKCENTU (PRI)", UI_PRI, UI_BG, UI_PRI);
            UI_PRI = c;
        }
        if (sel == 3) {
            save_colors();
            ui_show_info("Zapisano!", UI_PRI);
            delay(1000);
            return;
        }
    }
}
