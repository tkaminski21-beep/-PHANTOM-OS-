#pragma once
// apps/app_monitor.h — Monitor RAM/CPU/Flash + Benchmark + Edytor HEX

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <SD.h>

extern "C" {
  #include "esp_system.h"
  #include "esp_heap_caps.h"
}

// ─── MONITOR SYSTEMU ────────────────────────────────────
static uint32_t _mon_ram_hist[120] = {};
static int      _mon_ram_idx = 0;

void monitor_systemu() {
    unsigned long ostatni = 0;
    bool redraw = true;

    while (true) {
        if (millis() - ostatni > 500) {
            ostatni = millis();
            uint32_t wolne    = esp_get_free_heap_size();
            uint32_t min_heap = esp_get_minimum_free_heap_size();
            uint32_t psram    = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            uint32_t wewn     = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

            _mon_ram_hist[_mon_ram_idx % 120] = wolne;
            _mon_ram_idx++;
            redraw = true;

            if (redraw) {
                M5Cardputer.Display.fillScreen(THEME_BG);
                ui_draw_header("MONITOR SYSTEMU", THEME_CYAN);

                const uint32_t TOTAL = 320UL * 1024UL;

                // Pasek RAM
                int pct_ram = (int)(100.0f - (float)wolne / TOTAL * 100.0f);
                pct_ram = constrain(pct_ram, 0, 100);
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, 24); M5Cardputer.Display.print("RAM wew:");
                M5Cardputer.Display.drawRect(60, 24, 140, 10, THEME_BORDER);
                uint32_t kol_r = pct_ram > 80 ? THEME_RED : pct_ram > 50 ? THEME_YELLOW : THEME_GREEN;
                M5Cardputer.Display.fillRect(61, 25, pct_ram * 138 / 100, 8, kol_r);
                M5Cardputer.Display.setTextColor(TFT_WHITE);
                M5Cardputer.Display.setCursor(206, 24);
                char b1[6]; snprintf(b1, sizeof(b1), "%d%%", pct_ram);
                M5Cardputer.Display.print(b1);

                // Pasek PSRAM
                const uint32_t PSRAM_TOTAL = 8UL * 1024UL * 1024UL;
                int pct_ps = (psram > 0) ? (int)((float)(PSRAM_TOTAL - psram) / PSRAM_TOTAL * 100.0f) : 0;
                pct_ps = constrain(pct_ps, 0, 100);
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, 38); M5Cardputer.Display.print("PSRAM:  ");
                M5Cardputer.Display.drawRect(60, 38, 140, 10, THEME_BORDER);
                M5Cardputer.Display.fillRect(61, 39, pct_ps * 138 / 100, 8, THEME_BLUE);
                M5Cardputer.Display.setTextColor(TFT_WHITE);
                M5Cardputer.Display.setCursor(206, 38);
                char b2[6]; snprintf(b2, sizeof(b2), "%d%%", pct_ps);
                M5Cardputer.Display.print(b2);

                // Wiersze info
                struct InfoRow { const char* lab; uint32_t val; const char* suf; };
                InfoRow rows[] = {
                    { "Wolny heap:",     wolne    / 1024, "KB" },
                    { "Min heap:",       min_heap / 1024, "KB" },
                    { "PSRAM wolny:",    psram    / 1024, "KB" },
                    { "RAM wewn.:",      wewn     / 1024, "KB" },
                };
                int y = 56;
                for (auto& r : rows) {
                    M5Cardputer.Display.setTextColor(THEME_MUTED);
                    M5Cardputer.Display.setCursor(4, y); M5Cardputer.Display.print(r.lab);
                    M5Cardputer.Display.setTextColor(TFT_WHITE);
                    M5Cardputer.Display.setCursor(130, y);
                    char vb[20]; snprintf(vb, sizeof(vb), "%lu %s", r.val, r.suf);
                    M5Cardputer.Display.print(vb);
                    y += 13;
                }

                // Uptime
                unsigned long up = millis() / 1000;
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, y); M5Cardputer.Display.print("Uptime:");
                M5Cardputer.Display.setTextColor(TFT_WHITE);
                M5Cardputer.Display.setCursor(130, y);
                char ub[20]; snprintf(ub, sizeof(ub), "%02lu:%02lu:%02lu",
                                      up / 3600, (up % 3600) / 60, up % 60);
                M5Cardputer.Display.print(ub);
                y += 13;

                // Bateria
                int bat = M5Cardputer.Power.getBatteryLevel();
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, y); M5Cardputer.Display.print("Bateria:");
                M5Cardputer.Display.setTextColor(bat < 20 ? THEME_RED : THEME_GREEN);
                M5Cardputer.Display.setCursor(130, y);
                char bb[8]; snprintf(bb, sizeof(bb), "%d%%", bat);
                M5Cardputer.Display.print(bb);
                y += 14;

                // Wykres RAM
                M5Cardputer.Display.drawRect(4, y, 232, 36, THEME_BORDER);
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(6, y + 1); M5Cardputer.Display.print("RAM");
                for (int i = 1; i < 120; i++) {
                    int j  = (_mon_ram_idx - 120 + i + 120) % 120;
                    int jp = (_mon_ram_idx - 120 + i - 1 + 120) % 120;
                    if (_mon_ram_hist[jp] == 0 || _mon_ram_hist[j] == 0) continue;
                    int y1 = y + 34 - (int)((float)_mon_ram_hist[jp] / TOTAL * 32.0f);
                    int y2 = y + 34 - (int)((float)_mon_ram_hist[j]  / TOTAL * 32.0f);
                    M5Cardputer.Display.drawLine(4 + i * 2 - 2, y1, 4 + i * 2, y2, THEME_GREEN);
                }

                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, 212);
                M5Cardputer.Display.print("Auto-odswiezanie 0.5s  Q=wyjscie");
                redraw = false;
            }
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (char c : st.word) if (c == 'q' || c == 'Q') return;
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(50);
    }
}

// ─── BENCHMARK ──────────────────────────────────────────
void benchmark() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("BENCHMARK", THEME_YELLOW);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("Uruchamianie testow...");

    struct TestResult { const char* nazwa; unsigned long wynik; };
    TestResult wyniki[6];
    int nt = 0;

    // Test 1: Integer
    {
        volatile uint32_t x = 1;
        unsigned long t = millis();
        for (uint32_t i = 0; i < 1000000UL; i++) x = x * 1664525UL + 1013904223UL;
        wyniki[nt++] = { "Integer 1M iter:", millis() - t };
        ui_progress(17, 200, THEME_YELLOW);
    }
    // Test 2: Float
    {
        volatile float f = 1.0f;
        unsigned long t = millis();
        for (int i = 0; i < 100000; i++) f = f * 1.000001f + 0.000001f;
        wyniki[nt++] = { "Float 100K iter:", millis() - t };
        ui_progress(33, 200, THEME_YELLOW);
    }
    // Test 3: sinf
    {
        volatile float f = 0;
        unsigned long t = millis();
        for (int i = 0; i < 10000; i++) f += sinf((float)i) + cosf((float)i);
        wyniki[nt++] = { "Sin+Cos 10K:", millis() - t };
        ui_progress(50, 200, THEME_YELLOW);
    }
    // Test 4: RAM
    {
        uint8_t* buf = (uint8_t*)malloc(32768);
        unsigned long ms = 9999;
        if (buf) {
            unsigned long t = millis();
            memset(buf, 0xAA, 32768);
            ms = millis() - t;
            free(buf);
        }
        wyniki[nt++] = { "Zapis RAM 32KB:", ms };
        ui_progress(67, 200, THEME_YELLOW);
    }
    // Test 5: SD
    {
        unsigned long ms = 9999;
        // Próbuj otworzyć cokolwiek
        const char* probe[] = { "/system.log", "/apps", "/" };
        for (auto& p : probe) {
            File f = SD.open(p);
            if (f) {
                uint8_t tmp[512]; int rd = 0;
                unsigned long t = millis();
                while (f.available() && rd < 51200) { f.read(tmp, 512); rd += 512; }
                ms = millis() - t;
                f.close(); break;
            }
        }
        wyniki[nt++] = { "Odczyt SD 50KB:", ms };
        ui_progress(84, 200, THEME_YELLOW);
    }
    // Test 6: Ekran
    {
        unsigned long t = millis();
        for (int i = 0; i < 10; i++) M5Cardputer.Display.fillScreen(THEME_BG);
        wyniki[nt++] = { "Ekran fillScr x10:", millis() - t };
        ui_progress(100, 200, THEME_YELLOW);
    }

    // Wyniki
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("WYNIKI BENCHMARK", THEME_YELLOW);
    int y = 26;
    for (int i = 0; i < nt; i++) {
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, y); M5Cardputer.Display.print(wyniki[i].nazwa);
        uint32_t kol = wyniki[i].wynik < 50 ? THEME_GREEN
                     : wyniki[i].wynik < 200 ? THEME_YELLOW : THEME_RED;
        M5Cardputer.Display.setTextColor(kol);
        M5Cardputer.Display.setCursor(172, y);
        char buf[12]; snprintf(buf, sizeof(buf), "%lu ms", wyniki[i].wynik);
        M5Cardputer.Display.print(buf);
        y += 14;
    }
    M5Cardputer.Display.drawLine(0, y + 4, 240, y + 4, THEME_BORDER);
    M5Cardputer.Display.setTextColor(THEME_CYAN);
    M5Cardputer.Display.setCursor(4, y + 8); M5Cardputer.Display.print("ESP32-S3 @ 240MHz  8MB PSRAM");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 212); M5Cardputer.Display.print("Dowolny klawisz = powrot");
    ui_wait_key();
}

// ─── EDYTOR HEX ─────────────────────────────────────────
void edytor_hex() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("EDYTOR HEX", THEME_ORANGE);
    String sciezka = ui_input_string("Plik (np /notes/n.txt):", 4, 28, 32);
    if (sciezka.length() == 0) return;
    if (!sciezka.startsWith("/")) sciezka = "/" + sciezka;

    File f = SD.open(sciezka.c_str());
    if (!f) { ui_show_error("Nie znaleziono pliku!"); delay(1500); return; }

    uint32_t rozmiar = f.size();
    uint32_t offset  = 0;
    const int ROWS   = 12;
    const int BPR    = 8; // bytes per row

    while (true) {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header(sciezka.c_str(), THEME_ORANGE);

        f.seek(offset);
        int y = 22;
        uint8_t buf[BPR];

        for (int w = 0; w < ROWS && (offset + (uint32_t)(w * BPR)) < rozmiar; w++) {
            int rd = f.read(buf, BPR);
            uint32_t adres = offset + (uint32_t)(w * BPR);
            // Adres
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, y);
            char ab[9]; snprintf(ab, sizeof(ab), "%05lX:", (unsigned long)adres);
            M5Cardputer.Display.print(ab);
            // Bajty HEX
            M5Cardputer.Display.setTextColor(TFT_WHITE);
            for (int b = 0; b < rd; b++) {
                M5Cardputer.Display.setCursor(42 + b * 20, y);
                char hb[3]; snprintf(hb, sizeof(hb), "%02X", buf[b]);
                M5Cardputer.Display.print(hb);
            }
            // ASCII
            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setCursor(204, y);
            for (int b = 0; b < rd; b++) {
                char ch = (buf[b] >= 32 && buf[b] < 127) ? (char)buf[b] : '.';
                M5Cardputer.Display.print(ch);
            }
            y += 13;
        }

        int pct = rozmiar > 0 ? (int)((float)offset / rozmiar * 100.0f) : 0;
        ui_progress(pct, 202, THEME_ORANGE);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 212);
        char pb[40]; snprintf(pb, sizeof(pb), "0x%05lX/0x%05lX  W/S=scroll  Q=wr",
                               (unsigned long)offset, (unsigned long)rozmiar);
        M5Cardputer.Display.print(pb);

        bool wyjdz = false;
        while (true) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                auto st = M5Cardputer.Keyboard.keysState();
                for (char c : st.word) {
                    uint32_t krok = (uint32_t)(BPR * ROWS);
                    if (c == 's' || c == 'S') {
                        if (offset + krok < rozmiar) offset += krok;
                    }
                    if (c == 'w' || c == 'W') {
                        if (offset >= krok) offset -= krok; else offset = 0;
                    }
                    if (c == 'q' || c == 'Q') { wyjdz = true; }
                }
                break;
            }
            if (M5Cardputer.BtnA.wasPressed()) { wyjdz = true; break; }
            delay(10);
        }
        if (wyjdz) break;
    }
    f.close();
}

// ─── MENU MONITORA ──────────────────────────────────────
void app_monitor_run() {
    while (true) {
        const char* opts[] = {
            "Monitor RAM / CPU",
            "Benchmark",
            "Podglad HEX pliku",
            "Powrot"
        };
        int sel = ui_select_list(opts, 4, "MONITOR SYSTEMU", THEME_CYAN);
        if (sel < 0 || sel == 3) return;
        switch (sel) {
            case 0: monitor_systemu(); break;
            case 1: benchmark();       break;
            case 2: edytor_hex();      break;
        }
    }
}
