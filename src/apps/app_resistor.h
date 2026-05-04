#pragma once
// apps/app_resistor.h — Kalkulator rezystorów 4/5 pasmowy

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"

struct BandColor {
  const char* name;
  uint32_t    color;
  int         value;
  long        multiplier;
  float       tolerance;  // 0 = nie dotyczy
};

// 10 kolorów cyfr + mnożniki
static BandColor COLORS[] = {
  { "Czarny",      TFT_BLACK,    0,  1,          0.0 },
  { "Brazowy",     0x8B22,       1,  10,         1.0 },
  { "Czerwony",    TFT_RED,      2,  100,        2.0 },
  { "Pomaranczowy",0xFD20,       3,  1000,       0.0 },
  { "Zolty",       TFT_YELLOW,   4,  10000,      0.0 },
  { "Zielony",     TFT_GREEN,    5,  100000,     0.5 },
  { "Niebieski",   TFT_BLUE,     6,  1000000,    0.25},
  { "Fioletowy",   0x780F,       7,  10000000,   0.1 },
  { "Szary",       TFT_DARKGREY, 8,  100000000,  0.05},
  { "Bialy",       TFT_WHITE,    9,  1000000000, 0.0 },
};

static BandColor TOL_COLORS[] = {
  { "Zloty",     0xD4A0, 0, 0, 5.0  },
  { "Srebrny",   0xC618, 0, 0, 10.0 },
  { "Brazowy",   0x8B22, 0, 0, 1.0  },
  { "Czerwony",  TFT_RED,0, 0, 2.0  },
  { "Zielony",   TFT_GREEN,0,0,0.5  },
  { "Niebieski", TFT_BLUE,0,0, 0.25 },
};

String resistor_format(long ohm) {
  char buf[20];
  if (ohm >= 1000000000L) snprintf(buf, sizeof(buf), "%.2f GO", ohm / 1000000000.0);
  else if (ohm >= 1000000L) snprintf(buf, sizeof(buf), "%.2f MO", ohm / 1000000.0);
  else if (ohm >= 1000L)    snprintf(buf, sizeof(buf), "%.2f kO", ohm / 1000.0);
  else                       snprintf(buf, sizeof(buf), "%ld O",   ohm);
  return String(buf);
}

// ─── Rysuj pasek rezystora ──────────────────────────────
void resistor_draw_body(int b1, int b2, int b3, int mult, int tol, bool is5band) {
  int y = 60, h = 20;
  // Korpus
  M5Cardputer.Display.fillRoundRect(30, y, 180, h, 4, 0xF5A0); // beżowy
  M5Cardputer.Display.drawRoundRect(30, y, 180, h, 4, TFT_DARKGREY);
  // Przewody
  M5Cardputer.Display.drawLine(0, y + h/2, 30, y + h/2, TFT_SILVER);
  M5Cardputer.Display.drawLine(210, y + h/2, 240, y + h/2, TFT_SILVER);

  // Paski
  int xPositions[5] = { 50, 75, 100, 130, 160 };
  int bands5[5]     = { b1, b2, b3, mult, tol };
  int bands4[4]     = { b1, b2, mult, tol };
  int bw = 12;

  int cnt = is5band ? 5 : 4;
  int* bvals = is5band ? bands5 : bands4;
  // Dla 4-pasmowego ustaw pozycje
  int xPos4[4] = { 55, 85, 130, 160 };

  for (int i = 0; i < cnt; i++) {
    int xp = is5band ? xPositions[i] : xPos4[i];
    uint32_t col;
    if (i == cnt - 1) col = TOL_COLORS[bvals[i]].color;
    else if (i == cnt - 2) col = COLORS[bvals[i]].color;
    else col = COLORS[bvals[i]].color;
    M5Cardputer.Display.fillRect(xp, y, bw, h, col);
    M5Cardputer.Display.drawRect(xp, y, bw, h, TFT_DARKGREY);
  }
}

// ─── Główna aplikacja rezystorów ────────────────────────
void app_resistor_run() {
  int bands[5] = { 1, 0, 0, 2, 0 }; // domyślnie: braz,czar,czar,x100,zloty = 1kΩ ±5%
  bool is5 = false;
  int cursor = 0;  // aktywne pasmo
  bool redraw = true;

  while (true) {
    if (redraw) {
      M5Cardputer.Display.fillScreen(THEME_BG);
      ui_draw_header(is5 ? "REZYSTORY 5-PASM." : "REZYSTORY 4-PASM.", THEME_ORANGE);

      // Rezystor graficzny
      int b3  = is5 ? bands[2] : 0;
      int mult = is5 ? bands[3] : bands[2];
      int tol  = is5 ? bands[4] : bands[3];
      resistor_draw_body(bands[0], bands[1], b3, mult, tol, is5);

      // Oblicz wartość
      long val;
      int cnt = is5 ? 5 : 4;
      if (is5) {
        val = (long)(COLORS[bands[0]].value * 100 +
                     COLORS[bands[1]].value * 10 +
                     COLORS[bands[2]].value) * COLORS[bands[3]].multiplier;
      } else {
        val = (long)(COLORS[bands[0]].value * 10 +
                     COLORS[bands[1]].value) * COLORS[bands[2]].multiplier;
      }
      float tolPct = TOL_COLORS[tol].tolerance;

      // Wynik
      M5Cardputer.Display.setTextColor(THEME_ORANGE);
      M5Cardputer.Display.setTextSize(2);
      M5Cardputer.Display.setCursor(30, 90);
      M5Cardputer.Display.print(resistor_format(val));
      M5Cardputer.Display.setTextSize(1);
      M5Cardputer.Display.setTextColor(THEME_MUTED);
      M5Cardputer.Display.setCursor(30, 110);
      char tbuf[40];
      snprintf(tbuf, sizeof(tbuf), "+/-%.2f%%  (%.0f - %.0f)",
               tolPct, val * (1 - tolPct/100.0), val * (1 + tolPct/100.0));
      M5Cardputer.Display.print(tbuf);

      // Pasma - wybór koloru
      int bandCnt = is5 ? 5 : 4;
      const char* labels[] = { "P1","P2","P3","MULT","TOL" };
      const char* labels4[] = { "P1","P2","MULT","TOL" };

      for (int i = 0; i < bandCnt; i++) {
        int xb = 4 + i * 47;
        bool sel = (i == cursor);
        if (sel) M5Cardputer.Display.drawRoundRect(xb - 2, 126, 44, 60, 3, THEME_ORANGE);

        M5Cardputer.Display.setTextColor(sel ? THEME_ORANGE : THEME_MUTED);
        M5Cardputer.Display.setCursor(xb + 2, 128);
        M5Cardputer.Display.print(is5 ? labels[i] : labels4[i]);

        // Kolor paska
        uint32_t bc = (i == bandCnt - 1) ? TOL_COLORS[bands[i]].color : COLORS[bands[i]].color;
        M5Cardputer.Display.fillRect(xb, 140, 40, 16, bc);
        M5Cardputer.Display.drawRect(xb, 140, 40, 16, TFT_DARKGREY);

        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(xb + 2, 160);
        const char* nm = (i == bandCnt - 1) ? TOL_COLORS[bands[i]].name : COLORS[bands[i]].name;
        // Tylko pierwsze 5 znaków nazwy
        char s[6]; strncpy(s, nm, 5); s[5] = 0;
        M5Cardputer.Display.print(s);
      }

      // Sterowanie
      M5Cardputer.Display.setTextColor(THEME_MUTED);
      M5Cardputer.Display.setCursor(4, 200);
      M5Cardputer.Display.print("A/D=pasmo  W/S=kolor  FN+M=tryb");
      redraw = false;
    }

    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      auto st = M5Cardputer.Keyboard.keysState();
      int bandCnt = is5 ? 5 : 4;

      // FN+M = przełącz 4/5 pasmowy
      if (st.fn && _word_eq(st.word, "m")) {
        is5 = !is5;
        cursor = 0;
        for (int i = 0; i < 5; i++) bands[i] = 0;
        redraw = true;
        continue;
      }
      // ESC/FN+Q = wyjście
      if (st.fn && _word_eq(st.word, "q")) return;

      for (auto c : st.word) {
        if (c == 27) return;
        int maxColors = (cursor == bandCnt - 1) ? 6 : 10;
        if (c == 'd' || c == 'D') { cursor = (cursor + 1) % bandCnt; redraw = true; }
        if (c == 'a' || c == 'A') { cursor = (cursor - 1 + bandCnt) % bandCnt; redraw = true; }
        if (c == 'w' || c == 'W') { bands[cursor] = (bands[cursor] + 1) % maxColors; redraw = true; }
        if (c == 's' || c == 'S') { bands[cursor] = (bands[cursor] - 1 + maxColors) % maxColors; redraw = true; }
      }
    }
    if (M5Cardputer.BtnA.wasPressed()) return;
    delay(10);
  }
}
