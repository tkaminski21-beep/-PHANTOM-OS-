#pragma once
// app_screensaver.h — Wygaszacz ekranu typu Matrix
// Może być wywołany ręcznie lub automatycznie po bezczynności

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <Preferences.h>

extern Preferences prefs;

// Konfigurowalne kolory matrix
#define MATRIX_COLS 30
#define MATRIX_ROWS 17

static int   _mx_drops[MATRIX_COLS];
static int   _mx_speed[MATRIX_COLS];
static char  _mx_chars[MATRIX_COLS][MATRIX_ROWS];

static char _mx_random_char() {
    int r = random(60);
    if (r < 10) return '0' + r;
    if (r < 36) return 'A' + (r - 10);
    if (r < 46) return ':';
    if (r < 50) return '<';
    if (r < 54) return '>';
    return '*';
}

static void _mx_init() {
    for (int i = 0; i < MATRIX_COLS; i++) {
        _mx_drops[i] = random(MATRIX_ROWS);
        _mx_speed[i] = random(1, 4);
        for (int j = 0; j < MATRIX_ROWS; j++) _mx_chars[i][j] = _mx_random_char();
    }
}

void app_screensaver_matrix() {
    _mx_init();
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    bool running = true;
    while (running) {
        M5Cardputer.update();
        // Wyjście po dowolnym klawiszu
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) running = false;
        if (M5Cardputer.BtnA.wasPressed()) running = false;

        for (int col = 0; col < MATRIX_COLS; col++) {
            // Odśwież znaki
            if (random(20) < 1) {
                for (int j = 0; j < MATRIX_ROWS; j++) _mx_chars[col][j] = _mx_random_char();
            }

            int x = col * 8;
            int headRow = _mx_drops[col];

            // "Głowa" deszczu — biała, najjaśniejsza
            int headY = headRow * 8;
            if (headY < 135) {
                M5Cardputer.Display.fillRect(x, headY, 8, 8, TFT_BLACK);
                M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
                M5Cardputer.Display.setTextSize(1);
                M5Cardputer.Display.setCursor(x, headY);
                M5Cardputer.Display.print(_mx_chars[col][headRow % MATRIX_ROWS]);
            }

            // Ogon — zielony, ciemniejszy ku tyłowi
            for (int t = 1; t < 8; t++) {
                int y = (headRow - t) * 8;
                if (y < 0 || y >= 135) continue;
                uint16_t green = 0x07E0;
                if (t > 4) green = 0x03E0;  // ciemniejszy zielony
                if (t > 6) green = 0x0140;
                M5Cardputer.Display.fillRect(x, y, 8, 8, TFT_BLACK);
                M5Cardputer.Display.setTextColor(green, TFT_BLACK);
                M5Cardputer.Display.setTextSize(1);
                M5Cardputer.Display.setCursor(x, y);
                M5Cardputer.Display.print(_mx_chars[col][(headRow - t + MATRIX_ROWS) % MATRIX_ROWS]);
            }

            // Wymaż za ogonem
            int eraseY = (headRow - 8) * 8;
            if (eraseY >= 0 && eraseY < 135) {
                M5Cardputer.Display.fillRect(x, eraseY, 8, 8, TFT_BLACK);
            }

            _mx_drops[col]++;
            if (_mx_drops[col] > MATRIX_ROWS + 8) {
                _mx_drops[col] = -random(10);
                _mx_speed[col] = random(1, 4);
            }
        }

        delay(60);
    }
}

// ─── STARFIELD ────────────────────────────────────────
struct Star { float x, y, z; };
static Star _stars[40];

static void _star_reset(int i) {
    _stars[i].x = random(-1000, 1000);
    _stars[i].y = random(-1000, 1000);
    _stars[i].z = random(100, 1000);
}

void app_screensaver_starfield() {
    for (int i = 0; i < 40; i++) _star_reset(i);
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    bool running = true;
    while (running) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) running = false;
        if (M5Cardputer.BtnA.wasPressed()) running = false;

        for (int i = 0; i < 40; i++) {
            // Stara pozycja — wymaż
            int x_old = (int)(_stars[i].x / _stars[i].z * 100) + 120;
            int y_old = (int)(_stars[i].y / _stars[i].z * 100) + 67;
            if (x_old >= 0 && x_old < 240 && y_old >= 0 && y_old < 135)
                M5Cardputer.Display.drawPixel(x_old, y_old, TFT_BLACK);

            _stars[i].z -= 8;
            if (_stars[i].z < 1) { _star_reset(i); continue; }

            int x_new = (int)(_stars[i].x / _stars[i].z * 100) + 120;
            int y_new = (int)(_stars[i].y / _stars[i].z * 100) + 67;
            if (x_new < 0 || x_new >= 240 || y_new < 0 || y_new >= 135) {
                _star_reset(i); continue;
            }

            // Im bliżej tym jaśniejsze
            uint16_t col;
            if (_stars[i].z < 200)      col = TFT_WHITE;
            else if (_stars[i].z < 500) col = 0xC618;  // jasny szary
            else                         col = 0x4208; // ciemny szary
            M5Cardputer.Display.drawPixel(x_new, y_new, col);
        }

        delay(40);
    }
}

// ─── MENU SCREENSAVERA ────────────────────────────────
void app_screensaver_menu() {
    while (true) {
        const char* opts[] = {"Matrix","Starfield","Wlacz auto-uruchom"};
        int sel = ui_select_list(opts, 3, "WYGASZACZ EKRANU", THEME_GREEN);
        if (sel < 0) return;
        if (sel == 0) app_screensaver_matrix();
        if (sel == 1) app_screensaver_starfield();
        if (sel == 2) {
            bool current = prefs.getBool("ss_auto", false);
            prefs.putBool("ss_auto", !current);
            ui_show_info(!current ? "Auto-uruchom: WLACZONY" : "Auto-uruchom: WYLACZONY", THEME_CYAN);
            delay(1500);
        }
    }
}
