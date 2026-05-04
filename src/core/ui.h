#pragma once
// core/ui.h — Wspólne elementy UI v3.8
//
// NAWIGACJA — działa zarówno strzałkami jak i bez FN:
//
//   GÓRA:    ↑ (strzałka = FN+;)  lub  klawisz W
//   DÓŁ:     ↓ (strzałka = FN+.)  lub  klawisz S
//   LEWO:    ← (strzałka = FN+,)  -- w głównym menu
//   PRAWO:   → (strzałka = FN+/)  -- w głównym menu
//   OK:      ENTER / BtnA
//   WRÓĆ:    ESC (klawisz ESC lub FN+Q)
//
// Na Cardputer ADV strzałki fizyczne = FN+;/.  /.,
// więc FN musi być wciśnięty razem ze strzałką.
// Dodajemy też W/S jako alternatywę bez FN.

#include "M5Cardputer.h"
#include "theme.h"
#include <Arduino.h>
#include <vector>

extern uint16_t UI_BG;
extern uint16_t UI_FG;
extern uint16_t UI_PRI;

// ─── Nagłówek ─────────────────────────────────────────
void ui_draw_header(const char* title, uint32_t color = 0) {
  uint32_t c = color ? color : (uint32_t)UI_PRI;
  M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
  M5Cardputer.Display.fillRect(0, 0, 240, 20, (uint32_t)UI_BG);
  M5Cardputer.Display.drawLine(0, 20, 240, 20, c);
  M5Cardputer.Display.setTextColor(c, (uint32_t)UI_BG);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(6, 6);
  M5Cardputer.Display.print("< ");
  M5Cardputer.Display.print(title);
}

void ui_show_error(const char* msg) {
  M5Cardputer.Display.fillRoundRect(10,86,220,44,6,(uint32_t)UI_BG);
  M5Cardputer.Display.drawRoundRect(10,86,220,44,6,0xF800);
  M5Cardputer.Display.drawRoundRect(11,87,218,42,5,0xF800);
  M5Cardputer.Display.setTextColor(0xF800,(uint32_t)UI_BG);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(16,102);
  M5Cardputer.Display.print("BLAD: "); M5Cardputer.Display.print(msg);
}

void ui_show_info(const char* msg, uint32_t color = 0) {
  uint32_t c = color ? color : (uint32_t)UI_PRI;
  M5Cardputer.Display.fillRoundRect(10,86,220,44,6,(uint32_t)UI_BG);
  M5Cardputer.Display.drawRoundRect(10,86,220,44,6,c);
  M5Cardputer.Display.drawRoundRect(11,87,218,42,5,c);
  M5Cardputer.Display.setTextColor(c,(uint32_t)UI_BG);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(16,102);
  M5Cardputer.Display.print(msg);
}

void ui_progress(int percent, int y = 90, uint32_t color = 0) {
  uint32_t c = color ? color : (uint32_t)UI_PRI;
  M5Cardputer.Display.drawRect(4,y,232,6,c);
  M5Cardputer.Display.fillRect(5,y+1,(int)(230.0f*percent/100),4,c);
}

// ─── Helpery ─────────────────────────────────────────
static char _word_first(const std::vector<char>& w) { return w.empty()?0:w[0]; }
static bool _word_contains(const std::vector<char>& w, char ch) {
  for (char c:w) if(c==ch) return true; return false;
}
static bool _word_eq(const std::vector<char>& w, const char* s) {
  if (w.size()!=strlen(s)) return false;
  for (size_t i=0;i<w.size();i++) if(w[i]!=s[i]) return false;
  return true;
}

// ─── Detekcja klawiszy nawigacji ─────────────────────
// Akceptuje zarówno strzałki (FN+;/.) jak i W/S bez FN
typedef decltype(M5Cardputer.Keyboard.keysState()) KeysState_t;

static bool _is_up(const KeysState_t& s) {
  // Strzałka góra = FN+;   LUB   klawisz W (bez FN)
  if (s.fn && _word_eq(s.word, ";")) return true;
  if (!s.fn) {
    for (char c : s.word) if (c=='w'||c=='W') return true;
  }
  return false;
}
static bool _is_down(const KeysState_t& s) {
  // Strzałka dół = FN+.   LUB   klawisz S (bez FN)
  if (s.fn && _word_eq(s.word, ".")) return true;
  if (!s.fn) {
    for (char c : s.word) if (c=='s'||c=='S') return true;
  }
  return false;
}
static bool _is_left(const KeysState_t& s) {
  // Strzałka lewo = FN+,
  if (s.fn && _word_eq(s.word, ",")) return true;
  return false;
}
static bool _is_right(const KeysState_t& s) {
  // Strzałka prawo = FN+/
  if (s.fn && _word_eq(s.word, "/")) return true;
  return false;
}
static bool _is_ok(const KeysState_t& s) {
  if (s.fn) return false;
  for (char c : s.word) if (c=='\n'||c=='\r') return true;
  return false;
}
static bool _is_esc(const KeysState_t& s) {
  if (s.fn) {
    for (char c : s.word) if (c=='q'||c=='Q') return true;
  } else {
    for (char c : s.word) if (c==27) return true;
  }
  return false;
}

char ui_wait_key() {
  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
      auto s=M5Cardputer.Keyboard.keysState();
      if (!s.word.empty()) return s.word[0];
      if (s.del) return 8;
    }
    if (M5Cardputer.BtnA.wasPressed()) return '\n';
    delay(10);
  }
}

String ui_input_string(const char* prompt, int x, int y, int maxLen=24) {
  String result="";
  M5Cardputer.Display.setTextColor((uint32_t)UI_PRI,(uint32_t)UI_BG);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(x,y);
  M5Cardputer.Display.print(prompt);
  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
      auto s=M5Cardputer.Keyboard.keysState();
      if (s.del&&result.length()>0) result.remove(result.length()-1);
      else if (!s.word.empty()) {
        char c=s.word[0];
        if (c=='\n'||c=='\r') break;
        if (c==27) break;
        if ((int)result.length()<maxLen) result.concat(c);
      }
      M5Cardputer.Display.fillRect(x,y+14,220,16,(uint32_t)UI_BG);
      M5Cardputer.Display.setTextColor((uint32_t)UI_FG,(uint32_t)UI_BG);
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.setCursor(x,y+14);
      M5Cardputer.Display.print(result);
      M5Cardputer.Display.print("_");
    }
    if (M5Cardputer.BtnA.wasPressed()) break;
    delay(10);
  }
  return result;
}

// ═══════════════════════════════════════════════════════
//  ui_select_list v3.8
//
//  Nawigacja:
//    ↑ (FN+;)  lub  W  = poprzedni
//    ↓ (FN+.)  lub  S  = następny
//    ENTER / BtnA      = wybierz
//    ESC               = wróć
//
//  Podświetlenie:
//    Aktywna: tło UI_PRI + tekst UI_BG (zawsze czytelne)
//    Nieaktywna: tekst UI_FG na tle UI_BG
// ═══════════════════════════════════════════════════════
int ui_select_list(const char** items, int count, const char* title, uint32_t color=0) {
  uint32_t c = color ? color : (uint32_t)UI_PRI;
  int sel=0, offset=0;
  bool redraw=true;
  const int ROWS=5, ITEM_H=20, Y0=23;

  while (true) {
    if (redraw) {
      M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
      M5Cardputer.Display.drawLine(0,20,240,20,c);
      M5Cardputer.Display.setTextColor(c,(uint32_t)UI_BG);
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.setCursor(6,7);
      M5Cardputer.Display.print("< "); M5Cardputer.Display.print(title);

      if (sel<offset) offset=sel;
      if (sel>=offset+ROWS) offset=sel-ROWS+1;
      if (offset<0) offset=0;

      for (int i=0;i<ROWS&&(i+offset)<count;i++) {
        int idx=i+offset;
        bool active=(idx==sel);
        int y=Y0+i*ITEM_H;
        if (active) {
          M5Cardputer.Display.fillRect(0,y,240,ITEM_H,(uint32_t)UI_PRI);
          M5Cardputer.Display.setTextColor((uint32_t)UI_BG,(uint32_t)UI_PRI);
        } else {
          M5Cardputer.Display.setTextColor((uint32_t)UI_FG,(uint32_t)UI_BG);
        }
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(10,y+6);
        M5Cardputer.Display.print(active?">  ":"   ");
        M5Cardputer.Display.print(items[idx]);
      }

      // Scroll
      if (offset>0) {
        M5Cardputer.Display.setTextColor(c,(uint32_t)UI_BG);
        M5Cardputer.Display.setCursor(228,Y0+2); M5Cardputer.Display.print("^");
      }
      if (offset+ROWS<count) {
        M5Cardputer.Display.setTextColor(c,(uint32_t)UI_BG);
        M5Cardputer.Display.setCursor(228,Y0+(ROWS-1)*ITEM_H+6); M5Cardputer.Display.print("v");
      }

      // Stopka — pokaż właściwe skróty
      M5Cardputer.Display.fillRect(0,122,240,13,(uint32_t)UI_BG);
      M5Cardputer.Display.drawLine(0,122,240,122,c);
      M5Cardputer.Display.setTextColor(c,(uint32_t)UI_BG);
      M5Cardputer.Display.setCursor(4,125);
      char pg[36]; snprintf(pg,sizeof(pg),"Str/W/S=nav  ENTER=ok  ESC=wr %d/%d",sel+1,count);
      M5Cardputer.Display.print(pg);
      redraw=false;
    }

    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
      auto s = M5Cardputer.Keyboard.keysState();
      if (_is_up(s))   { if(sel>0){sel--;redraw=true;} }
      if (_is_down(s)) { if(sel<count-1){sel++;redraw=true;} }
      if (_is_ok(s))   return sel;
      if (_is_esc(s))  return -1;
    }
    if (M5Cardputer.BtnA.wasPressed()) return sel;
    delay(10);
  }
}
