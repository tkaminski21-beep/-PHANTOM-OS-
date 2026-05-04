#pragma once
// apps/app_zegar.h — Zegar, Stoper, Minutnik, Kalendarz

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <time.h>
#include <math.h>

// ─── ZEGAR CYFROWY ──────────────────────────────────────
void zegar_cyfrowy() {
    while (true) {
        struct tm ti;
        bool ntpOK = getLocalTime(&ti);

        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("ZEGAR", THEME_CYAN);

        if (ntpOK) {
            char godz[9];
            strftime(godz, sizeof(godz), "%H:%M:%S", &ti);

            // Duży czas
            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setTextSize(3);
            M5Cardputer.Display.setCursor(20, 55);
            M5Cardputer.Display.print(godz);
            M5Cardputer.Display.setTextSize(1);

            // Data
            char data[32];
            // Polskie nazwy dni
            const char* dni[] = {"Niedziela","Poniedzialek","Wtorek","Sroda","Czwartek","Piatek","Sobota"};
            const char* mies[] = {"Stycznia","Lutego","Marca","Kwietnia","Maja","Czerwca",
                                  "Lipca","Sierpnia","Wrzesnia","Pazdziernika","Listopada","Grudnia"};
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(30, 100);
            snprintf(data, sizeof(data), "%s, %d %s %d",
                     dni[ti.tm_wday], ti.tm_mday, mies[ti.tm_mon], ti.tm_year + 1900);
            M5Cardputer.Display.print(data);

            // Tydzień roku
            char tw[20]; strftime(tw, sizeof(tw), "Tydzien: %W", &ti);
            M5Cardputer.Display.setCursor(60, 116);
            M5Cardputer.Display.print(tw);
        } else {
            M5Cardputer.Display.setTextColor(THEME_RED);
            M5Cardputer.Display.setCursor(30, 80);
            M5Cardputer.Display.print("Brak synchronizacji NTP");
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(10, 96);
            M5Cardputer.Display.print("Polacz z WiFi aby pobrac czas");
        }

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("Q = wyjscie");

        // Czekaj 1 sekundę, sprawdzając klawiaturę
        for (int i = 0; i < 50; i++) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                auto st = M5Cardputer.Keyboard.keysState();
                for (auto c : st.word) if (c == 'q' || c == 'Q') return;
            }
            if (M5Cardputer.BtnA.wasPressed()) return;
            delay(20);
        }
    }
}

// ─── STOPER ─────────────────────────────────────────────
void stoper() {
    unsigned long start = 0, elapsed = 0, lap = 0;
    bool dziala = false;
    int okrazenia = 0;
    unsigned long czasy_okrazen[10] = {};

    while (true) {
        unsigned long teraz = dziala ? millis() - start + elapsed : elapsed;
        unsigned long ms    = teraz % 1000;
        unsigned long s     = (teraz / 1000) % 60;
        unsigned long m     = (teraz / 60000) % 60;
        unsigned long h     = teraz / 3600000;

        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("STOPER", THEME_YELLOW);

        M5Cardputer.Display.setTextColor(dziala ? THEME_GREEN : THEME_YELLOW);
        M5Cardputer.Display.setTextSize(3);
        char buf[16];
        snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h * 60 + m, s, ms / 10);
        M5Cardputer.Display.setCursor(8, 50);
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setTextSize(1);

        // Okrążenia
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 95);
        M5Cardputer.Display.print("Okrazenia:");
        for (int i = 0; i < okrazenia && i < 6; i++) {
            unsigned long lt = czasy_okrazen[i];
            char lb[20]; snprintf(lb, sizeof(lb), "#%d: %02lu:%02lu.%02lu",
                i + 1, (lt/60000)%60, (lt/1000)%60, (lt%1000)/10);
            M5Cardputer.Display.setTextColor(i % 2 ? THEME_CYAN : TFT_WHITE);
            M5Cardputer.Display.setCursor(4, 107 + i * 13);
            M5Cardputer.Display.print(lb);
        }

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 200);
        M5Cardputer.Display.print("SPACJA=start/stop  L=okrazenie  R=reset  Q=wr");

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if (c == ' ') {
                    if (dziala) { elapsed += millis() - start; dziala = false; }
                    else { start = millis(); dziala = true; }
                }
                if ((c == 'l' || c == 'L') && dziala && okrazenia < 10) {
                    czasy_okrazen[okrazenia++] = teraz;
                }
                if (c == 'r' || c == 'R') {
                    elapsed = 0; dziala = false; okrazenia = 0;
                }
                if (c == 'q' || c == 'Q') return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(50);
    }
}

// ─── MINUTNIK ───────────────────────────────────────────
void minutnik() {
    // Ustaw czas
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("MINUTNIK", THEME_ORANGE);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 30);
    M5Cardputer.Display.print("Podaj minuty:");
    String wej = ui_input_string("", 4, 46, 4);
    int minuty = wej.toInt();
    if (minuty <= 0 || minuty > 999) return;

    unsigned long koniec = millis() + (unsigned long)minuty * 60000UL;

    while (true) {
        long pozostalo = (long)(koniec - millis());
        if (pozostalo <= 0) {
            // Alarm!
            for (int i = 0; i < 5; i++) {
                M5Cardputer.Display.fillScreen(THEME_RED);
                M5Cardputer.Display.setTextColor(TFT_WHITE);
                M5Cardputer.Display.setTextSize(2);
                M5Cardputer.Display.setCursor(40, 100);
                M5Cardputer.Display.print("CZAS MINUL!");
                M5Cardputer.Speaker.tone(2000, 300);
                delay(400);
                M5Cardputer.Display.fillScreen(THEME_BG);
                delay(200);
            }
            M5Cardputer.Display.setTextSize(1);
            return;
        }

        long m = (pozostalo / 60000) % 60;
        long s = (pozostalo / 1000) % 60;
        int  pct = 100 - (int)(pozostalo * 100 / ((long)minuty * 60000L));

        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("MINUTNIK", THEME_ORANGE);

        // Okrąg odliczania (uproszczony)
        M5Cardputer.Display.drawCircle(120, 100, 55, THEME_BORDER);
        // Łuk postępu — rysujemy segmenty
        for (int deg = 0; deg < pct * 360 / 100; deg += 3) {
            float rad = (deg - 90) * 3.14159 / 180.0;
            int px = 120 + (int)(52 * cos(rad));
            int py = 100 + (int)(52 * sin(rad));
            M5Cardputer.Display.fillCircle(px, py, 2, THEME_ORANGE);
        }

        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setTextSize(2);
        char buf[8]; snprintf(buf, sizeof(buf), "%02ld:%02ld", m, s);
        M5Cardputer.Display.setCursor(82, 92);
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 170);
        M5Cardputer.Display.print("Q = anuluj");

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) if (c == 'q' || c == 'Q') return;
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(500);
    }
}

// ─── KALENDARZ ──────────────────────────────────────────
void kalendarz() {
    struct tm ti; getLocalTime(&ti);
    int rok = ti.tm_year + 1900;
    int mies = ti.tm_mon + 1;

    const char* MIESIAC[] = {"","Styczen","Luty","Marzec","Kwiecien","Maj","Czerwiec",
                              "Lipiec","Sierpien","Wrzesien","Pazdziernik","Listopad","Grudzien"};
    int dni_w_mies[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

    auto czy_przestepny = [](int r) { return (r % 4 == 0 && r % 100 != 0) || (r % 400 == 0); };

    while (true) {
        int dni = dni_w_mies[mies];
        if (mies == 2 && czy_przestepny(rok)) dni = 29;

        // Pierwszy dzień tygodnia (0=Nie, 1=Pon, ..., 6=Sob)
        struct tm tmp = {}; tmp.tm_year = rok - 1900; tmp.tm_mon = mies - 1; tmp.tm_mday = 1;
        mktime(&tmp);
        int pierw = (tmp.tm_wday + 6) % 7; // Poniedziałek = 0

        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("KALENDARZ", THEME_CYAN);

        // Nagłówek miesiąca
        M5Cardputer.Display.setTextColor(THEME_CYAN);
        M5Cardputer.Display.setCursor(50, 24);
        char hdr[20]; snprintf(hdr, sizeof(hdr), "%s %d", MIESIAC[mies], rok);
        M5Cardputer.Display.print(hdr);

        // Nagłówki dni
        const char* dn[] = {"Pn","Wt","Sr","Cz","Pt","So","Nd"};
        for (int i = 0; i < 7; i++) {
            M5Cardputer.Display.setTextColor(i == 6 ? THEME_RED : THEME_MUTED);
            M5Cardputer.Display.setCursor(4 + i * 33, 36);
            M5Cardputer.Display.print(dn[i]);
        }
        M5Cardputer.Display.drawLine(0, 47, 240, 47, THEME_BORDER);

        // Komórki kalendarza
        int col = pierw, row = 0;
        for (int d = 1; d <= dni; d++) {
            int x = 4 + col * 33;
            int y = 52 + row * 24;

            bool dzisiaj = (d == ti.tm_mday && mies == ti.tm_mon + 1 && rok == ti.tm_year + 1900);
            bool nd = (col == 6);

            if (dzisiaj) {
                M5Cardputer.Display.fillCircle(x + 8, y + 7, 9, THEME_CYAN);
                M5Cardputer.Display.setTextColor(THEME_BG);
            } else {
                M5Cardputer.Display.setTextColor(nd ? THEME_RED : TFT_WHITE);
            }
            M5Cardputer.Display.setCursor(d < 10 ? x + 4 : x, y + 3);
            M5Cardputer.Display.print(d);

            col++;
            if (col == 7) { col = 0; row++; }
        }

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("A/D=mies  W/S=rok  Q=wyjscie");

        M5Cardputer.update();
        bool q = false;
        while (!M5Cardputer.Keyboard.isChange()) {
            M5Cardputer.update();
            if (M5Cardputer.BtnA.wasPressed()) { q = true; break; }
            delay(10);
        }
        if (q) return;
        if (M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if (c == 'd' || c == 'D') { if (++mies > 12) { mies = 1; rok++; } }
                if (c == 'a' || c == 'A') { if (--mies < 1)  { mies = 12; rok--; } }
                if (c == 'w' || c == 'W') rok++;
                if (c == 's' || c == 'S') rok--;
                if (c == 'q' || c == 'Q') return;
            }
        }
    }
}

void app_zegar_run() {
    while (true) {
        const char* opts[] = { "Zegar cyfrowy", "Stoper", "Minutnik", "Kalendarz", "Powrot" };
        int sel = ui_select_list(opts, 5, "ZEGAR / CZAS", THEME_CYAN);
        if (sel < 0 || sel == 4) return;
        switch (sel) {
            case 0: zegar_cyfrowy(); break;
            case 1: stoper(); break;
            case 2: minutnik(); break;
            case 3: kalendarz(); break;
        }
    }
}
