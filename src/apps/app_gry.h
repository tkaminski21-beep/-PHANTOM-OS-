#pragma once
// apps/app_gry.h — Snake, Tetris, Pong

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <math.h>

// ══════════════════════════════════════════════
//  SNAKE
// ══════════════════════════════════════════════
#define SN_W 30
#define SN_H 16
#define SN_SZ 7

void gra_snake() {
    int wx[200], wy[200], dlugosc = 4;
    int dx = 1, dy = 0, jx = 0, jy = 0;
    int wynik = 0;
    bool koniec = false;

    // Inicjalizacja węża
    for (int i = 0; i < dlugosc; i++) { wx[i] = 10 - i; wy[i] = 8; }

    // Losuj jedzenie
    auto losuj_jedzenie = [&]() {
        bool zajete;
        do {
            zajete = false;
            jx = random(0, SN_W); jy = random(0, SN_H);
            for (int i = 0; i < dlugosc; i++)
                if (wx[i] == jx && wy[i] == jy) { zajete = true; break; }
        } while (zajete);
    };
    losuj_jedzenie();

    auto rysuj = [&]() {
        M5Cardputer.Display.fillScreen(THEME_BG);
        // Ramka
        M5Cardputer.Display.drawRect(0, 14, SN_W * SN_SZ + 2, SN_H * SN_SZ + 2, THEME_BORDER);
        // Wynik
        M5Cardputer.Display.setTextColor(THEME_GREEN);
        M5Cardputer.Display.setCursor(160, 3);
        char sc[12]; snprintf(sc, sizeof(sc), "Wynik: %d", wynik);
        M5Cardputer.Display.print(sc);
        // Wąż
        for (int i = 0; i < dlugosc; i++) {
            uint32_t kol = (i == 0) ? THEME_GREEN : (i % 2 == 0 ? 0x07C0 : 0x0600);
            M5Cardputer.Display.fillRect(1 + wx[i] * SN_SZ, 15 + wy[i] * SN_SZ, SN_SZ - 1, SN_SZ - 1, kol);
        }
        // Jedzenie
        M5Cardputer.Display.fillCircle(1 + jx * SN_SZ + SN_SZ / 2, 15 + jy * SN_SZ + SN_SZ / 2, SN_SZ / 2 - 1, THEME_RED);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(2, 3);
        M5Cardputer.Display.print("WASD=kierunek  Q=wyjscie");
    };

    int predkosc = 120;
    unsigned long ostatni_ruch = 0;

    while (!koniec) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if ((c == 'w' || c == 'W') && dy == 0) { dx = 0; dy = -1; }
                if ((c == 's' || c == 'S') && dy == 0) { dx = 0; dy =  1; }
                if ((c == 'a' || c == 'A') && dx == 0) { dx = -1; dy = 0; }
                if ((c == 'd' || c == 'D') && dx == 0) { dx =  1; dy = 0; }
                if (c == 'q' || c == 'Q') return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;

        if (millis() - ostatni_ruch < (unsigned long)predkosc) { delay(5); continue; }
        ostatni_ruch = millis();

        // Przesuń węża
        int nx = wx[0] + dx, ny = wy[0] + dy;

        // Kolizja ze ścianą
        if (nx < 0 || nx >= SN_W || ny < 0 || ny >= SN_H) koniec = true;
        // Kolizja z sobą
        for (int i = 1; i < dlugosc; i++)
            if (wx[i] == nx && wy[i] == ny) { koniec = true; break; }

        if (!koniec) {
            // Czy zjadł?
            bool zjadl = (nx == jx && ny == jy);
            int nowa_dl = dlugosc + (zjadl ? 1 : 0);

            // Przesuń ogon
            for (int i = nowa_dl - 1; i > 0; i--) { wx[i] = wx[i-1]; wy[i] = wy[i-1]; }
            wx[0] = nx; wy[0] = ny;
            dlugosc = nowa_dl;

            if (zjadl) { wynik += 10; losuj_jedzenie(); if (predkosc > 50) predkosc -= 3; }
        }
        rysuj();
    }

    // Koniec gry
    M5Cardputer.Display.fillRoundRect(50, 90, 140, 40, 6, THEME_PANEL);
    M5Cardputer.Display.drawRoundRect(50, 90, 140, 40, 6, THEME_RED);
    M5Cardputer.Display.setTextColor(THEME_RED);
    M5Cardputer.Display.setCursor(70, 95);
    M5Cardputer.Display.print("KONIEC GRY!");
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(70, 110);
    char sc[20]; snprintf(sc, sizeof(sc), "Wynik: %d", wynik);
    M5Cardputer.Display.print(sc);
    delay(500);
    ui_wait_key();
}

// ══════════════════════════════════════════════
//  TETRIS
// ══════════════════════════════════════════════
#define TT_W 10
#define TT_H 20
#define TT_SZ 10

static uint8_t plansza[TT_H][TT_W] = {};
static uint32_t KOLORY_TETRIS[] = {
    0x0000, 0x07FF, 0xF800, 0x001F, 0xFFE0, 0xF81F, 0x07E0, 0xFD20
};

// Klocki Tetris (4 obroty x 4 komórki)
static int8_t KLOCKI[7][4][4][2] = {
    {{{0,0},{1,0},{2,0},{3,0}}, {{1,0},{1,1},{1,2},{1,3}}, {{0,1},{1,1},{2,1},{3,1}}, {{0,0},{0,1},{0,2},{0,3}}}, // I
    {{{0,0},{1,0},{1,1},{2,1}}, {{1,0},{1,1},{0,1},{0,2}}, {{0,0},{1,0},{1,1},{2,1}}, {{1,0},{1,1},{0,1},{0,2}}}, // S
    {{{0,1},{1,1},{1,0},{2,0}}, {{0,0},{0,1},{1,1},{1,2}}, {{0,1},{1,1},{1,0},{2,0}}, {{0,0},{0,1},{1,1},{1,2}}}, // Z
    {{{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}}}, // O
    {{{1,0},{1,1},{1,2},{0,2}}, {{0,0},{1,0},{2,0},{2,1}}, {{0,0},{0,1},{0,2},{1,0}}, {{0,0},{0,1},{1,1},{2,1}}}, // L
    {{{0,0},{0,1},{0,2},{1,2}}, {{0,0},{1,0},{2,0},{0,1}}, {{0,0},{1,0},{1,1},{1,2}}, {{2,0},{0,1},{1,1},{2,1}}}, // J
    {{{1,0},{0,1},{1,1},{2,1}}, {{0,0},{0,1},{1,1},{0,2}}, {{0,0},{1,0},{2,0},{1,1}}, {{0,1},{1,0},{1,1},{1,2}}}, // T
};

void rysuj_plansze_tt(int kx, int ky, int ktyp, int kobrot) {
    M5Cardputer.Display.fillRect(0, 0, TT_W * TT_SZ + 2, TT_H * TT_SZ + 2, THEME_BG);
    M5Cardputer.Display.drawRect(0, 0, TT_W * TT_SZ + 2, TT_H * TT_SZ + 2, THEME_BORDER);

    for (int r = 0; r < TT_H; r++)
        for (int c = 0; c < TT_W; c++)
            if (plansza[r][c])
                M5Cardputer.Display.fillRect(1 + c * TT_SZ, 1 + r * TT_SZ, TT_SZ - 1, TT_SZ - 1, KOLORY_TETRIS[plansza[r][c]]);

    // Aktywny klocek
    for (int i = 0; i < 4; i++) {
        int bx = kx + KLOCKI[ktyp][kobrot][i][0];
        int by = ky + KLOCKI[ktyp][kobrot][i][1];
        if (by >= 0 && by < TT_H && bx >= 0 && bx < TT_W)
            M5Cardputer.Display.fillRect(1 + bx * TT_SZ, 1 + by * TT_SZ, TT_SZ - 1, TT_SZ - 1, KOLORY_TETRIS[ktyp + 1]);
    }
}

bool kolizja_tt(int kx, int ky, int ktyp, int kobrot) {
    for (int i = 0; i < 4; i++) {
        int bx = kx + KLOCKI[ktyp][kobrot][i][0];
        int by = ky + KLOCKI[ktyp][kobrot][i][1];
        if (bx < 0 || bx >= TT_W || by >= TT_H) return true;
        if (by >= 0 && plansza[by][bx]) return true;
    }
    return false;
}

void gra_tetris() {
    memset(plansza, 0, sizeof(plansza));
    int wynik = 0, poziom = 1;
    int ktyp = random(7), kobrot = 0, kx = 4, ky = 0;
    int ntyp = random(7);
    unsigned long ostatni = millis();
    int predkosc = 700;
    bool koniec = false;

    while (!koniec) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if ((c == 'a' || c == 'A') && !kolizja_tt(kx - 1, ky, ktyp, kobrot)) kx--;
                if ((c == 'd' || c == 'D') && !kolizja_tt(kx + 1, ky, ktyp, kobrot)) kx++;
                if ((c == 's' || c == 'S') && !kolizja_tt(kx, ky + 1, ktyp, kobrot)) { ky++; wynik++; }
                if ((c == 'w' || c == 'W')) {
                    int nr = (kobrot + 1) % 4;
                    if (!kolizja_tt(kx, ky, ktyp, nr)) kobrot = nr;
                }
                if (c == ' ') { while (!kolizja_tt(kx, ky + 1, ktyp, kobrot)) { ky++; wynik += 2; } }
                if (c == 'q' || c == 'Q') return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;

        // Grawitacja
        if (millis() - ostatni > (unsigned long)predkosc) {
            ostatni = millis();
            if (!kolizja_tt(kx, ky + 1, ktyp, kobrot)) {
                ky++;
            } else {
                // Umieść klocek
                for (int i = 0; i < 4; i++) {
                    int bx = kx + KLOCKI[ktyp][kobrot][i][0];
                    int by = ky + KLOCKI[ktyp][kobrot][i][1];
                    if (by >= 0) plansza[by][bx] = ktyp + 1;
                }
                // Usuń pełne linie
                int usun = 0;
                for (int r = TT_H - 1; r >= 0; r--) {
                    bool pelna = true;
                    for (int c = 0; c < TT_W; c++) if (!plansza[r][c]) { pelna = false; break; }
                    if (pelna) {
                        usun++;
                        for (int rr = r; rr > 0; rr--)
                            memcpy(plansza[rr], plansza[rr - 1], TT_W);
                        memset(plansza[0], 0, TT_W);
                        r++;
                    }
                }
                if (usun > 0) {
                    static const int pkt[] = {0, 40, 100, 300, 1200};
                    wynik += pkt[min(usun, 4)] * poziom;
                }
                // Nowy klocek
                ktyp = ntyp; ntyp = random(7); kobrot = 0; kx = 4; ky = 0;
                if (kolizja_tt(kx, ky, ktyp, kobrot)) koniec = true;
                poziom = wynik / 500 + 1;
                predkosc = max(100, 700 - poziom * 50);
            }
        }

        rysuj_plansze_tt(kx, ky, ktyp, kobrot);
        // Panel boczny
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(TT_W * TT_SZ + 6, 4);
        M5Cardputer.Display.print("Wynik:");
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(TT_W * TT_SZ + 6, 16);
        M5Cardputer.Display.print(wynik);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(TT_W * TT_SZ + 6, 32);
        M5Cardputer.Display.print("Poz:"); M5Cardputer.Display.print(poziom);
        M5Cardputer.Display.setCursor(TT_W * TT_SZ + 6, 80);
        M5Cardputer.Display.print("WASD");
        M5Cardputer.Display.setCursor(TT_W * TT_SZ + 6, 92);
        M5Cardputer.Display.print("SP=drop");

        delay(20);
    }

    M5Cardputer.Display.fillRoundRect(30, 90, 180, 40, 6, THEME_PANEL);
    M5Cardputer.Display.setTextColor(THEME_RED);
    M5Cardputer.Display.setCursor(70, 96); M5Cardputer.Display.print("GAME OVER!");
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(70, 110);
    char sc[20]; snprintf(sc, sizeof(sc), "Wynik: %d", wynik);
    M5Cardputer.Display.print(sc);
    delay(500); ui_wait_key();
}

// ══════════════════════════════════════════════
//  PONG
// ══════════════════════════════════════════════
void gra_pong() {
    float bx = 120, by = 110, bvx = 2.5f, bvy = 2.0f;
    int p1y = 100, p2y = 100, wy1 = 0, wy2 = 0;
    const int PH = 30, PW = 5, SZEROK = 240, WYSOK = 220;
    const int P1X = 6, P2X = SZEROK - 11;

    auto rysuj = [&]() {
        M5Cardputer.Display.fillScreen(THEME_BG);
        // Linia środkowa
        for (int y = 0; y < WYSOK; y += 12)
            M5Cardputer.Display.drawFastVLine(SZEROK / 2, y, 8, THEME_BORDER);
        // Paletki
        M5Cardputer.Display.fillRect(P1X, p1y, PW, PH, THEME_CYAN);
        M5Cardputer.Display.fillRect(P2X, p2y, PW, PH, THEME_ORANGE);
        // Piłka
        M5Cardputer.Display.fillCircle((int)bx, (int)by, 4, TFT_WHITE);
        // Wynik
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setCursor(90, 2); M5Cardputer.Display.print(wy1);
        M5Cardputer.Display.setCursor(140, 2); M5Cardputer.Display.print(wy2);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 210);
        M5Cardputer.Display.print("W/S=pal.L  I/K=pal.P  Q=wyjscie");
    };

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if (c == 'w' || c == 'W') p1y = max(0, p1y - 6);
                if (c == 's' || c == 'S') p1y = min(WYSOK - PH, p1y + 6);
                if (c == 'i' || c == 'I') p2y = max(0, p2y - 6);
                if (c == 'k' || c == 'K') p2y = min(WYSOK - PH, p2y + 6);
                if (c == 'q' || c == 'Q') return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;

        // Fizyka piłki
        bx += bvx; by += bvy;
        if (by < 4 || by > WYSOK - 4) bvy = -bvy;

        // Uderzenie w paletkę 1
        if (bx < P1X + PW + 4 && by > p1y && by < p1y + PH) { bvx = fabs(bvx); bvy += (by - p1y - PH/2) * 0.1f; }
        // Uderzenie w paletkę 2
        if (bx > P2X - 4 && by > p2y && by < p2y + PH) { bvx = -fabs(bvx); bvy += (by - p2y - PH/2) * 0.1f; }

        // Punktacja
        if (bx < 0) { wy2++; bx = 120; by = 110; bvx = 2.5f; bvy = 2.0f; delay(600); }
        if (bx > SZEROK) { wy1++; bx = 120; by = 110; bvx = -2.5f; bvy = 2.0f; delay(600); }

        // Limit prędkości
        if (fabs(bvx) > 6) bvx = (bvx > 0) ? 6 : -6;
        if (fabs(bvy) > 5) bvy = (bvy > 0) ? 5 : -5;

        if (wy1 >= 10 || wy2 >= 10) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setTextSize(2);
            M5Cardputer.Display.setCursor(40, 100);
            M5Cardputer.Display.print(wy1 >= 10 ? "GRACZ 1 WYGRAL!" : "GRACZ 2 WYGRAL!");
            M5Cardputer.Display.setTextSize(1);
            delay(500); ui_wait_key(); return;
        }

        rysuj();
        delay(16);
    }
}

void app_gry_run() {
    while (true) {
        const char* opts[] = { "Snake", "Tetris", "Pong", "Powrot" };
        int sel = ui_select_list(opts, 4, "GRY", THEME_YELLOW);
        if (sel < 0 || sel == 3) return;
        switch (sel) {
            case 0: gra_snake();  break;
            case 1: gra_tetris(); break;
            case 2: gra_pong();   break;
        }
    }
}
