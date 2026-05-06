#pragma once
// core/ui.h v6.0 — Bruce-style submenu with larger fonts
//
// Zmiany:
//   • Tytuł podmenu w setTextSize(2) (12px wysokość) jak w Bruce
//   • Karty wyższe (24px) z większym tekstem
//   • Ikona po lewej + tekst w setTextSize(1) ale z większymi odstępami
//   • Stopka z aktualnymi nazwami klawiszy ("OK", "ESC")
//   • Integracja z keyboard_input.h — używa SelPress/EscPress (s.enter)

#include "M5Cardputer.h"
#include "theme.h"
#include "i18n.h"
#include <Arduino.h>
#include <vector>

extern uint16_t UI_BG;
extern uint16_t UI_FG;
extern uint16_t UI_PRI;

// ─── Helpery klawiatury (przed include keyboard_input.h!) ──
inline char _word_first(const std::vector<char>& w) { return w.empty()?0:w[0]; }
inline bool _word_contains(const std::vector<char>& w, char ch) { for(char c:w)if(c==ch)return true;return false; }
inline bool _word_eq(const std::vector<char>& w, const char* s) {
  if (w.size()!=strlen(s)) return false;
  for (size_t i=0;i<w.size();i++) if(w[i]!=s[i])return false;
  return true;
}

#include "keyboard_input.h"

// ─── Pomocnicze: ciemniejsza wersja koloru ──────────
inline uint16_t _dim(uint16_t c, int factor = 2) {
    uint8_t r = ((c >> 11) & 0x1F);
    uint8_t g = ((c >> 5) & 0x3F);
    uint8_t b = (c & 0x1F);
    r >>= factor; g >>= factor; b >>= factor;
    return (r << 11) | (g << 5) | b;
}

// ─── Header okna z DUŻYM tytułem ──────────────────────
inline void ui_draw_header(const char* title, uint32_t color = 0) {
    uint32_t c = color ? color : (uint32_t)UI_PRI;
    uint32_t cdim = _dim((uint16_t)c, 2);

    M5Cardputer.Display.fillScreen((uint32_t)UI_BG);

    // Pasek tytułu - większy (26px)
    M5Cardputer.Display.fillRect(0, 0, 240, 26, _dim((uint16_t)c, 4));
    M5Cardputer.Display.drawLine(0, 26, 240, 26, c);
    M5Cardputer.Display.drawLine(0, 27, 240, 27, cdim);

    // Strzałka "back" po lewej
    M5Cardputer.Display.fillTriangle(8, 13, 14, 7, 14, 19, c);
    M5Cardputer.Display.fillRect(14, 11, 6, 4, c);

    // Tytuł - DUŻY (size 2)
    M5Cardputer.Display.setTextColor(c, _dim((uint16_t)c, 4));
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(26, 6);
    M5Cardputer.Display.print(title);
    M5Cardputer.Display.setTextSize(1);
}

// ─── Info / Error ─────────────────────────────────────
inline void ui_show_error(const char* msg) {
    uint32_t red = 0xF800;
    M5Cardputer.Display.fillRoundRect(10, 80, 220, 50, 8, _dim(0xF800, 3));
    M5Cardputer.Display.drawRoundRect(10, 80, 220, 50, 8, red);
    M5Cardputer.Display.drawRoundRect(11, 81, 218, 48, 7, red);
    M5Cardputer.Display.setTextColor(0xFFFF, _dim(0xF800, 3));
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(20, 88);
    M5Cardputer.Display.print("! ");
    M5Cardputer.Display.print(T("error"));
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(20, 110);
    M5Cardputer.Display.print(msg);
}

inline void ui_show_info(const char* msg, uint32_t color = 0) {
    uint32_t c = color ? color : (uint32_t)UI_PRI;
    M5Cardputer.Display.fillRoundRect(10, 86, 220, 44, 8, (uint32_t)UI_BG);
    M5Cardputer.Display.drawRoundRect(10, 86, 220, 44, 8, c);
    M5Cardputer.Display.drawRoundRect(11, 87, 218, 42, 7, c);
    M5Cardputer.Display.setTextColor(c, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(20, 102);
    M5Cardputer.Display.print(msg);
}

inline void ui_progress(int percent, int y = 90, uint32_t color = 0) {
    uint32_t c = color ? color : (uint32_t)UI_PRI;
    M5Cardputer.Display.drawRoundRect(4, y, 232, 8, 3, c);
    M5Cardputer.Display.fillRoundRect(6, y+2, (int)(228.0f*percent/100), 4, 2, c);
}

inline char ui_wait_key() {
    while (true) {
        input_update();
        if (LastChar) return LastChar;
        if (SelPress) return '\n';
        if (EscPress) return 27;
        if (LastDel)  return 8;
        delay(10);
    }
}

inline String ui_input_string(const char* prompt, int x, int y, int maxLen=24) {
    String result="";
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI,(uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(x,y);
    M5Cardputer.Display.print(prompt);
    bool redraw = true;
    while (true) {
        input_update();
        if (LastDel && result.length()>0) { result.remove(result.length()-1); redraw=true; }
        else if (LastChar && LastChar!='\n' && LastChar!='\r' && LastChar!=27 && LastChar!='`') {
            if ((int)result.length()<maxLen) { result.concat(LastChar); redraw=true; }
        }
        if (SelPress) break;
        if (EscPress) break;
        if (redraw) {
            M5Cardputer.Display.fillRect(x,y+14,220,16,(uint32_t)UI_BG);
            M5Cardputer.Display.setTextColor((uint32_t)UI_FG,(uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(x,y+14);
            M5Cardputer.Display.print(result);
            M5Cardputer.Display.print("_");
            redraw = false;
        }
        delay(10);
    }
    return result;
}

// ═══════════════════════════════════════════════════════
//  ui_select_list v6.0 — Bruce-style z DUŻYMI nazwami
//
//  Layout 240×135:
//    ┌──────────────────────────┐
//    │ ◀  TYTUL              ←  │  header 26px (size 2)
//    ├──────────────────────────┤
//    │ ┃ ▶  Pierwsza opcja    ┃ │  karta 24px (size 1.5 effect)
//    │   │  Druga opcja          │
//    │   │  Trzecia opcja        │
//    │   │  Czwarta opcja      v │
//    ├──────────────────────────┤
//    │ OK=wybierz  ESC=powrot 4/9│ stopka 13px
//    └──────────────────────────┘
//
//  Karty 24px wysokości (24*4=96px na 4 widoczne)
//  Tekst opcji: setTextSize(1) ale duże piksele 8x16
//  Aktywna karta: pełny prostokąt UI_PRI + > marker
// ═══════════════════════════════════════════════════════
inline int ui_select_list(const char** items, int count, const char* title, uint32_t color=0) {
    uint32_t c = color ? color : (uint32_t)UI_PRI;
    uint32_t cdim = _dim((uint16_t)c, 2);
    int sel = 0, prev_sel = -1, offset = 0, prev_offset = -1;
    bool full = true;

    const int ROWS = 4;       // 4 widoczne karty
    const int ITEM_H = 22;    // wysokość karty
    const int Y0 = 30;        // start kart (poniżej dużego header'a)
    const int FOOTER_Y = 122;

    auto draw_card = [&](int i) {
        int idx = i + offset;
        if (idx >= count) {
            int y = Y0 + i * ITEM_H;
            M5Cardputer.Display.fillRect(0, y, 240, ITEM_H, (uint32_t)UI_BG);
            return;
        }
        bool active = (idx == sel);
        int y = Y0 + i * ITEM_H;

        if (active) {
            // Aktywna karta - pełny kolor
            M5Cardputer.Display.fillRoundRect(2, y, 236, ITEM_H-1, 4, (uint32_t)UI_PRI);
            // Lewy pasek (ciemniejszy) - wskaźnik
            M5Cardputer.Display.fillRect(2, y, 5, ITEM_H-1, _dim((uint16_t)UI_PRI, 1));
            // Tekst opcji - DUŻY (size 1.5 = setTextSize(1) + bold-like)
            M5Cardputer.Display.setTextColor((uint32_t)UI_BG, (uint32_t)UI_PRI);
            M5Cardputer.Display.setTextSize(1);
            // Marker > po lewej
            M5Cardputer.Display.setCursor(12, y + 4);
            M5Cardputer.Display.print(">");
            // Tekst opcji - 14px Y (mocno wyśrodkowany)
            M5Cardputer.Display.setCursor(24, y + 4);
            M5Cardputer.Display.setTextSize(1);
            // Skróć jeśli za długie (przy size 1 mieści się ~32 znaki)
            const char* itm = items[idx];
            int maxlen = 30;
            if ((int)strlen(itm) > maxlen) {
                char tmp[32];
                strncpy(tmp, itm, maxlen);
                tmp[maxlen] = 0;
                M5Cardputer.Display.print(tmp);
                M5Cardputer.Display.print(">");
            } else {
                M5Cardputer.Display.print(itm);
            }

            // Numer w prawym rogu (mały)
            M5Cardputer.Display.setCursor(220, y + 4);
            char num[6]; snprintf(num, sizeof(num), "%02d", idx + 1);
            M5Cardputer.Display.print(num);

            // Druga linijka - opis (przyciemniony) - placeholder
            // Można dodać opisy w przyszłości

        } else {
            // Nieaktywna - czyste tło, tylko tekst
            M5Cardputer.Display.fillRect(0, y, 240, ITEM_H, (uint32_t)UI_BG);
            // Cienki separator
            M5Cardputer.Display.drawLine(8, y + ITEM_H - 1, 232, y + ITEM_H - 1, _dim((uint16_t)UI_PRI, 4));
            // Tekst (UI_FG) duży
            M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(24, y + 4);
            const char* itm = items[idx];
            int maxlen = 30;
            if ((int)strlen(itm) > maxlen) {
                char tmp[32];
                strncpy(tmp, itm, maxlen);
                tmp[maxlen] = 0;
                M5Cardputer.Display.print(tmp);
                M5Cardputer.Display.print(">");
            } else {
                M5Cardputer.Display.print(itm);
            }

            // Numer (mały, przyciemniony)
            M5Cardputer.Display.setTextColor(cdim, (uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(220, y + 4);
            char num[6]; snprintf(num, sizeof(num), "%02d", idx + 1);
            M5Cardputer.Display.print(num);
        }
        M5Cardputer.Display.setTextSize(1);
    };

    auto draw_footer = [&]() {
        M5Cardputer.Display.fillRect(0, FOOTER_Y, 240, 13, (uint32_t)UI_BG);
        M5Cardputer.Display.drawLine(0, FOOTER_Y, 240, FOOTER_Y, c);

        M5Cardputer.Display.setTextColor(c, (uint32_t)UI_BG);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(4, FOOTER_Y + 3);
        // Pokaż klawisze - krótko: "OK | ESC | X/Y"
        if (g_lang == 1) {
            M5Cardputer.Display.print("OK=select  ESC=back");
        } else {
            M5Cardputer.Display.print("OK=wybierz  ESC=powrot");
        }

        // Pozycja X/Y po prawej
        char pg[12]; snprintf(pg, sizeof(pg), "%d/%d", sel + 1, count);
        int len = strlen(pg) * 6;
        M5Cardputer.Display.setCursor(238 - len, FOOTER_Y + 3);
        M5Cardputer.Display.print(pg);
    };

    while (true) {
        if (sel < offset) offset = sel;
        if (sel >= offset + ROWS) offset = sel - ROWS + 1;
        if (offset < 0) offset = 0;

        bool offset_changed = (offset != prev_offset);
        bool sel_changed = (sel != prev_sel);

        if (full || offset_changed) {
            ui_draw_header(title, c);
            for (int i = 0; i < ROWS; i++) draw_card(i);
            draw_footer();
            // Scroll arrows
            if (offset > 0)
                M5Cardputer.Display.fillTriangle(232, Y0+3, 228, Y0+9, 236, Y0+9, c);
            if (offset + ROWS < count) {
                int yy = Y0 + ROWS*ITEM_H - 9;
                M5Cardputer.Display.fillTriangle(232, yy+5, 228, yy, 236, yy, c);
            }
            full = false;
        }
        else if (sel_changed && prev_sel >= 0) {
            int prev_i = prev_sel - offset;
            int new_i  = sel - offset;
            if (prev_i >= 0 && prev_i < ROWS) draw_card(prev_i);
            if (new_i  >= 0 && new_i  < ROWS) draw_card(new_i);
            draw_footer();
        }

        prev_sel = sel;
        prev_offset = offset;

        input_update();
        if (PrevPress) { if (sel > 0) sel--; }
        if (NextPress) { if (sel < count - 1) sel++; }
        if (SelPress)  return sel;
        if (EscPress)  return -1;

        delay(20);
    }
}
