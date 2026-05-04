#pragma once
// app_stoper_timer.h — Stoper i Timer z alarmem dźwiękowym

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include "core/sound.h"

extern uint16_t UI_BG, UI_FG, UI_PRI;

static bool _st_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ─── STOPER ───────────────────────────────────────────
void app_stoper() {
    bool running = false;
    unsigned long startMs = 0;
    unsigned long elapsed = 0;
    std::vector<unsigned long> laps;

    bool redraw = true;
    while (true) {
        unsigned long now = running ? (elapsed + (millis() - startMs)) : elapsed;

        if (redraw || running) {
            M5Cardputer.Display.fillScreen(UI_BG);
            M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(85, 10); M5Cardputer.Display.print("STOPER");
            M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

            // Czas duży
            unsigned long s = now / 1000;
            unsigned long ms = (now % 1000) / 10;
            char buf[16];
            snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", s/60, s%60, ms);
            M5Cardputer.Display.setTextSize(3);
            M5Cardputer.Display.setCursor(35, 35);
            M5Cardputer.Display.print(buf);

            // Stan
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setTextColor(running ? 0x07E0 : 0xF800, UI_BG);
            M5Cardputer.Display.setCursor(95, 70);
            M5Cardputer.Display.print(running ? "BIEGNIE" : "PAUZA");

            // Ostatnie 3 okrążenia
            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            int yy = 82;
            int start = max(0, (int)laps.size() - 3);
            for (size_t i = start; i < laps.size(); i++) {
                unsigned long ls = laps[i] / 1000;
                unsigned long lms = (laps[i] % 1000) / 10;
                char lb[24];
                snprintf(lb, sizeof(lb), "Lap %d: %02lu:%02lu.%02lu", (int)(i+1), ls/60, ls%60, lms);
                M5Cardputer.Display.setCursor(10, yy);
                M5Cardputer.Display.print(lb);
                yy += 10;
            }

            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setCursor(8, 118);
            M5Cardputer.Display.print("SPACE=start/stop  L=lap  R=reset  ESC");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) {
                if (c == ' ') {
                    if (running) { elapsed += millis() - startMs; running = false; snd::cancel(); }
                    else { startMs = millis(); running = true; snd::confirm(); }
                    redraw = true;
                }
                if ((c=='l'||c=='L') && running) {
                    laps.push_back(elapsed + (millis() - startMs));
                    snd::tick();
                    redraw = true;
                }
                if (c=='r'||c=='R') {
                    running = false; elapsed = 0; laps.clear();
                    snd::tick(); redraw = true;
                }
                if (c == 27) return;
            }
        }
        delay(20);
    }
}

// ─── TIMER ────────────────────────────────────────────
void app_timer() {
    int minutes = 5, seconds = 0;
    int field = 0;  // 0=min, 1=sec
    bool running = false;
    bool redraw = true;
    unsigned long startMs = 0;
    long remaining = 0;
    bool alarmed = false;

    while (true) {
        long total = running ? max(0L, (long)((minutes*60+seconds)*1000 - (long)(millis()-startMs))) : (long)(minutes*60+seconds)*1000;

        if (running && total <= 0 && !alarmed) {
            alarmed = true;
            // Alarm — 5x trzy tony
            for (int i = 0; i < 5; i++) {
                snd::beep(); delay(120);
                snd::beep(); delay(120);
                snd::beep(); delay(400);
                if (_st_check_esc()) break;
            }
            running = false;
            redraw = true;
        }

        if (redraw || running) {
            M5Cardputer.Display.fillScreen(UI_BG);
            M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(95, 10); M5Cardputer.Display.print("TIMER");
            M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

            // Wyświetl czas (od cofany)
            int m = total / 60000;
            int s = (total / 1000) % 60;
            char buf[10]; snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
            M5Cardputer.Display.setTextSize(4);
            M5Cardputer.Display.setTextColor(alarmed ? 0xF800 : UI_PRI, UI_BG);
            M5Cardputer.Display.setCursor(60, 38);
            M5Cardputer.Display.print(buf);

            // Pasek postępu
            if (!running && !alarmed) {
                int totalMs = (minutes*60+seconds)*1000;
                if (totalMs > 0) {
                    M5Cardputer.Display.drawRect(20, 90, 200, 8, UI_PRI);
                }
            } else if (running) {
                int totalMs = (minutes*60+seconds)*1000;
                int filled = totalMs > 0 ? 196 * total / totalMs : 0;
                M5Cardputer.Display.drawRect(20, 90, 200, 8, UI_PRI);
                M5Cardputer.Display.fillRect(21, 91, filled, 6, UI_PRI);
            }

            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setCursor(8, 105);
            if (running) M5Cardputer.Display.print("BIEGNIE - SPACE=pauza ESC=stop");
            else if (alarmed) M5Cardputer.Display.print("KONIEC! Dowolny klawisz...");
            else {
                M5Cardputer.Display.print(field==0 ? "MIN  sec  " : "min  SEC  ");
                M5Cardputer.Display.print("FN+;/. zmiana TAB pole");
            }
            M5Cardputer.Display.setCursor(8, 118);
            M5Cardputer.Display.print("SPACE=start  R=reset  ESC=wyjdz");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn && !running) {
                if (_word_eq(s.word, ";")) {
                    if (field == 0) minutes = (minutes+1) % 60;
                    else seconds = (seconds+1) % 60;
                    snd::tick(); redraw = true;
                }
                if (_word_eq(s.word, ".")) {
                    if (field == 0) minutes = (minutes-1+60) % 60;
                    else seconds = (seconds-1+60) % 60;
                    snd::tick(); redraw = true;
                }
            }
            for (char c : s.word) {
                if (c == ' ') {
                    if (alarmed) { alarmed = false; redraw = true; continue; }
                    if (!running && (minutes > 0 || seconds > 0)) {
                        startMs = millis(); running = true; snd::confirm();
                    } else if (running) {
                        running = false; snd::cancel();
                    }
                    redraw = true;
                }
                if (c == '\t') { field = 1 - field; snd::tick(); redraw = true; }
                if (c=='r'||c=='R') { running = false; alarmed = false; redraw = true; }
                if (c == 27) return;
            }
        }
        delay(20);
    }
}

// ─── ALARM (czas zegarowy) ─────────────────────────────
void app_alarm() {
    int alarmH = 7, alarmM = 0;
    int field = 0;
    bool armed = false;
    bool redraw = true;

    while (true) {
        if (redraw || armed) {
            M5Cardputer.Display.fillScreen(UI_BG);
            M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(95, 10); M5Cardputer.Display.print("ALARM");
            M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

            // Ustaw godzinę
            char buf[10]; snprintf(buf, sizeof(buf), "%02d:%02d", alarmH, alarmM);
            M5Cardputer.Display.setTextSize(4);
            M5Cardputer.Display.setCursor(70, 38);
            M5Cardputer.Display.print(buf);

            // Aktualny czas
            struct tm ti;
            if (getLocalTime(&ti, 0)) {
                M5Cardputer.Display.setTextSize(1);
                M5Cardputer.Display.setTextColor(UI_PRI & 0xC618, UI_BG);
                M5Cardputer.Display.setCursor(8, 80);
                char now[20]; snprintf(now, sizeof(now), "Teraz: %02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
                M5Cardputer.Display.print(now);

                // Sprawdź alarm
                if (armed && ti.tm_hour == alarmH && ti.tm_min == alarmM) {
                    M5Cardputer.Display.fillScreen(0xF800);
                    M5Cardputer.Display.setTextColor(TFT_WHITE, 0xF800);
                    M5Cardputer.Display.setTextSize(3);
                    M5Cardputer.Display.setCursor(50, 50); M5Cardputer.Display.print("ALARM!");
                    for (int i = 0; i < 30; i++) {
                        snd::beep(); delay(150);
                        if (_st_check_esc()) break;
                    }
                    armed = false;
                    redraw = true;
                    continue;
                }
            }

            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setTextColor(armed ? 0x07E0 : 0xF800, UI_BG);
            M5Cardputer.Display.setCursor(8, 95);
            M5Cardputer.Display.print(armed ? "URUCHOMIONY" : "WYLACZONY");

            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setCursor(8, 110);
            M5Cardputer.Display.print(field==0 ? "[H] M    " : " H [M]   ");
            M5Cardputer.Display.print("FN+;/. TAB pole");
            M5Cardputer.Display.setCursor(8, 120);
            M5Cardputer.Display.print("SPACE=arm/disarm  ESC=wyjdz");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn && !armed) {
                if (_word_eq(s.word, ";")) {
                    if (field == 0) alarmH = (alarmH+1) % 24;
                    else alarmM = (alarmM+1) % 60;
                    snd::tick(); redraw = true;
                }
                if (_word_eq(s.word, ".")) {
                    if (field == 0) alarmH = (alarmH-1+24) % 24;
                    else alarmM = (alarmM-1+60) % 60;
                    snd::tick(); redraw = true;
                }
            }
            for (char c : s.word) {
                if (c == '\t') { field = 1 - field; snd::tick(); redraw = true; }
                if (c == ' ') { armed = !armed; snd::confirm(); redraw = true; }
                if (c == 27) return;
            }
        }
        delay(500);
    }
}

// ─── MENU ─────────────────────────────────────────────
void app_stoper_timer_menu() {
    while (true) {
        const char* opts[] = {"Stoper","Timer (countdown)","Alarm (godzina)"};
        int sel = ui_select_list(opts, 3, "STOPER / TIMER", UI_PRI);
        if (sel < 0) return;
        if (sel == 0) app_stoper();
        if (sel == 1) app_timer();
        if (sel == 2) app_alarm();
    }
}
