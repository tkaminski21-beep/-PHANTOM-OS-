#pragma once
// apps/app_kalkulator.h — Kalkulator ogólny

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <math.h>

void app_kalkulator_run() {
    String wyswietlacz = "0";
    String bufor = "";
    double poprzednia = 0;
    char operacja = 0;
    bool nowa_liczba = true;
    bool redraw = true;

    auto rysuj = [&]() {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("KALKULATOR", THEME_CYAN);

        // Wyświetlacz
        M5Cardputer.Display.fillRoundRect(4, 24, 232, 28, 4, THEME_PANEL);
        M5Cardputer.Display.drawRoundRect(4, 24, 232, 28, 4, THEME_CYAN);
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setTextSize(2);
        String disp = wyswietlacz;
        if (disp.length() > 13) disp = disp.substring(disp.length() - 13);
        M5Cardputer.Display.setCursor(8, 30);
        M5Cardputer.Display.print(disp);
        M5Cardputer.Display.setTextSize(1);

        // Aktywna operacja
        if (operacja) {
            M5Cardputer.Display.setTextColor(THEME_YELLOW);
            M5Cardputer.Display.setCursor(210, 28);
            M5Cardputer.Display.print(String(operacja));
        }

        // Przyciski
        struct { const char* label; int x, y; uint32_t kolor; } przyciski[] = {
            {"7",4,58,THEME_PANEL}, {"8",62,58,THEME_PANEL}, {"9",120,58,THEME_PANEL}, {"/",178,58,THEME_ORANGE},
            {"4",4,84,THEME_PANEL}, {"5",62,84,THEME_PANEL}, {"6",120,84,THEME_PANEL}, {"*",178,84,THEME_ORANGE},
            {"1",4,110,THEME_PANEL}, {"2",62,110,THEME_PANEL}, {"3",120,110,THEME_PANEL},{"-",178,110,THEME_ORANGE},
            {"0",4,136,THEME_PANEL}, {".",62,136,THEME_PANEL},{"=",120,136,THEME_GREEN},{"+",178,136,THEME_ORANGE},
            {"C",4,162,THEME_RED},  {"sqrt",62,162,THEME_BLUE},{"^2",120,162,THEME_BLUE},{"Q",178,162,THEME_GRAY},
        };

        for (auto& p : przyciski) {
            M5Cardputer.Display.fillRoundRect(p.x, p.y, 54, 22, 3, p.kolor);
            M5Cardputer.Display.drawRoundRect(p.x, p.y, 54, 22, 3, THEME_BORDER);
            M5Cardputer.Display.setTextColor(TFT_WHITE);
            M5Cardputer.Display.setCursor(p.x + 18, p.y + 7);
            M5Cardputer.Display.print(p.label);
        }

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 190);
        M5Cardputer.Display.print("Klawiatura: cyfry +-*/= C Q=wyjscie");
    };

    auto oblicz = [&]() {
        double biezaca = wyswietlacz.toDouble();
        double wynik = poprzednia;
        switch (operacja) {
            case '+': wynik = poprzednia + biezaca; break;
            case '-': wynik = poprzednia - biezaca; break;
            case '*': wynik = poprzednia * biezaca; break;
            case '/': wynik = (biezaca != 0) ? poprzednia / biezaca : 0; break;
        }
        // Formatuj wynik
        if (wynik == (long long)wynik)
            wyswietlacz = String((long long)wynik);
        else {
            char buf[20]; snprintf(buf, sizeof(buf), "%.6g", wynik);
            wyswietlacz = String(buf);
        }
        operacja = 0;
        nowa_liczba = true;
    };

    while (true) {
        if (redraw) { rysuj(); redraw = false; }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if (c >= '0' && c <= '9') {
                    if (nowa_liczba) { wyswietlacz = String(c); nowa_liczba = false; }
                    else if (wyswietlacz == "0") wyswietlacz = String(c);
                    else wyswietlacz.concat(c);
                    redraw = true;
                } else if (c == '.') {
                    if (nowa_liczba) { wyswietlacz = "0."; nowa_liczba = false; }
                    else if (wyswietlacz.indexOf('.') < 0) wyswietlacz += '.';
                    redraw = true;
                } else if (c == '+' || c == '-' || c == '*' || c == '/') {
                    if (operacja && !nowa_liczba) oblicz();
                    poprzednia = wyswietlacz.toDouble();
                    operacja = c; nowa_liczba = true; redraw = true;
                } else if (c == '=' || c == '\n') {
                    if (operacja) oblicz();
                    redraw = true;
                } else if (c == 'c' || c == 'C') {
                    wyswietlacz = "0"; poprzednia = 0; operacja = 0; nowa_liczba = true; redraw = true;
                } else if (c == 'q' || c == 'Q') return;
            }
            // Backspace
            if (st.del && wyswietlacz.length() > 1) {
                wyswietlacz.remove(wyswietlacz.length() - 1); redraw = true;
            } else if (st.del) { wyswietlacz = "0"; redraw = true; }
            // sqrt
            if (st.fn && _word_eq(st.word, "s")) {
                double v = wyswietlacz.toDouble();
                char buf[20]; snprintf(buf, sizeof(buf), "%.6g", sqrt(v));
                wyswietlacz = String(buf); nowa_liczba = true; redraw = true;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}
