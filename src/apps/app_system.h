#pragma once
// apps/app_system.h — Menedżer plików, E-book, Terminal UART, Log

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <SD.h>

// ══════════════════════════════════════════════
//  MENEDŻER PLIKÓW SD
// ══════════════════════════════════════════════
#define MAX_PLIKOW 48

struct PlikInfo {
    String nazwa;
    bool katalog;
    size_t rozmiar;
};

static PlikInfo pliki[MAX_PLIKOW];
static int liczba_plikow = 0;
static String biezaca_sciezka = "/";

void skanuj_katalog(const String& sciezka) {
    liczba_plikow = 0;
    File dir = SD.open(sciezka);
    if (!dir) return;

    // Dodaj ".." jeśli nie jesteśmy w głównym
    if (sciezka != "/") {
        pliki[liczba_plikow].nazwa = "..";
        pliki[liczba_plikow].katalog = true;
        pliki[liczba_plikow].rozmiar = 0;
        liczba_plikow++;
    }

    while (liczba_plikow < MAX_PLIKOW) {
        File f = dir.openNextFile();
        if (!f) break;
        pliki[liczba_plikow].nazwa   = String(f.name());
        pliki[liczba_plikow].katalog = f.isDirectory();
        pliki[liczba_plikow].rozmiar = f.isDirectory() ? 0 : f.size();
        liczba_plikow++;
        f.close();
    }
    dir.close();
}

String formatuj_rozmiar(size_t b) {
    char buf[16];
    if (b >= 1048576) snprintf(buf, sizeof(buf), "%.1fMB", b / 1048576.0f);
    else if (b >= 1024) snprintf(buf, sizeof(buf), "%.1fKB", b / 1024.0f);
    else snprintf(buf, sizeof(buf), "%dB", (int)b);
    return String(buf);
}

void menedzer_plikow() {
    biezaca_sciezka = "/";
    skanuj_katalog(biezaca_sciezka);

    while (true) {
        // Buduj etykiety
        const char* etykiety[MAX_PLIKOW];
        char bufory[MAX_PLIKOW][40];
        for (int i = 0; i < liczba_plikow; i++) {
            if (pliki[i].katalog) {
                snprintf(bufory[i], 40, "[DIR] %s", pliki[i].nazwa.c_str());
            } else {
                snprintf(bufory[i], 40, "%-22s %s",
                         pliki[i].nazwa.c_str(),
                         formatuj_rozmiar(pliki[i].rozmiar).c_str());
            }
            etykiety[i] = bufory[i];
        }

        // Pokaż ścieżkę w nagłówku
        String nagl = "PLIKI: " + biezaca_sciezka;
        if (nagl.length() > 22) nagl = "..." + nagl.substring(nagl.length() - 19);
        int sel = ui_select_list(etykiety, liczba_plikow, nagl.c_str(), THEME_CYAN);

        if (sel < 0) return;

        PlikInfo& wyb = pliki[sel];

        if (wyb.katalog) {
            if (wyb.nazwa == "..") {
                // Wróć w górę
                int pos = biezaca_sciezka.lastIndexOf('/', biezaca_sciezka.length() - 2);
                biezaca_sciezka = (pos <= 0) ? "/" : biezaca_sciezka.substring(0, pos + 1);
            } else {
                biezaca_sciezka = biezaca_sciezka + wyb.nazwa + "/";
            }
            skanuj_katalog(biezaca_sciezka);
        } else {
            // Akcje na pliku
            const char* akcje[] = { "Podglad", "Kopiuj", "Przenies", "Usun", "Wstecz" };
            int akt = ui_select_list(akcje, 5, wyb.nazwa.c_str(), THEME_CYAN);

            String sciezka_pelna = biezaca_sciezka + wyb.nazwa;

            if (akt == 0) {
                // Podgląd pliku
                File f = SD.open(sciezka_pelna);
                if (!f) continue;
                M5Cardputer.Display.fillScreen(THEME_BG);
                ui_draw_header(wyb.nazwa.c_str(), THEME_CYAN);
                int y = 24; bool redraw = true;
                int offset = 0;
                String tresc = "";
                while (f.available() && tresc.length() < 2000) tresc += (char)f.read();
                f.close();

                while (true) {
                    if (redraw) {
                        M5Cardputer.Display.fillRect(0, 22, 240, 188, THEME_BG);
                        y = 24; int pos = offset;
                        while (pos < (int)tresc.length() && y < 206) {
                            M5Cardputer.Display.setTextColor(TFT_WHITE);
                            M5Cardputer.Display.setCursor(4, y);
                            M5Cardputer.Display.print(tresc.substring(pos, pos + 37));
                            pos += 37; y += 12;
                        }
                        M5Cardputer.Display.setTextColor(THEME_MUTED);
                        M5Cardputer.Display.setCursor(4, 212);
                        M5Cardputer.Display.print("W/S=przewin  Q=wyjscie");
                        redraw = false;
                    }
                    M5Cardputer.update();
                    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                        auto st = M5Cardputer.Keyboard.keysState();
                        for (auto c : st.word) {
                            if (c == 's' || c == 'S') { offset = min((int)tresc.length(), offset + 37); redraw = true; }
                            if (c == 'w' || c == 'W') { offset = max(0, offset - 37); redraw = true; }
                            if (c == 'q' || c == 'Q') goto koniec_podglad;
                        }
                    }
                    if (M5Cardputer.BtnA.wasPressed()) goto koniec_podglad;
                    delay(10);
                }
                koniec_podglad:;
            } else if (akt == 1) {
                // Kopiuj — pobierz nazwę docelową
                M5Cardputer.Display.fillScreen(THEME_BG);
                ui_draw_header("KOPIUJ", THEME_CYAN);
                String cel = ui_input_string("Nowa nazwa:", 4, 30, 30);
                if (cel.length() > 0) {
                    File zrodlo = SD.open(sciezka_pelna);
                    File dest   = SD.open(("/"+cel).c_str(), FILE_WRITE);
                    if (zrodlo && dest) {
                        uint8_t buf[512];
                        while (zrodlo.available()) {
                            int rd = zrodlo.read(buf, 512);
                            dest.write(buf, rd);
                        }
                        ui_show_info("Skopiowano!", THEME_GREEN);
                    } else ui_show_error("Blad kopiowania!");
                    zrodlo.close(); dest.close();
                    delay(1200);
                    skanuj_katalog(biezaca_sciezka);
                }
            } else if (akt == 2) {
                // Przenieś (kopiuj + usuń)
                M5Cardputer.Display.fillScreen(THEME_BG);
                ui_draw_header("PRZENIES", THEME_CYAN);
                String cel = ui_input_string("Nowa sciezka:", 4, 30, 32);
                if (cel.length() > 0) {
                    File zrodlo = SD.open(sciezka_pelna);
                    File dest   = SD.open(cel, FILE_WRITE);
                    if (zrodlo && dest) {
                        uint8_t buf[512];
                        while (zrodlo.available()) { int rd = zrodlo.read(buf, 512); dest.write(buf, rd); }
                        zrodlo.close(); dest.close();
                        SD.remove(sciezka_pelna);
                        ui_show_info("Przenieisono!", THEME_GREEN);
                    } else ui_show_error("Blad!");
                    delay(1200);
                    skanuj_katalog(biezaca_sciezka);
                }
            } else if (akt == 3) {
                // Usuń
                SD.remove(sciezka_pelna);
                ui_show_info("Usunieto!", THEME_YELLOW);
                delay(1000);
                skanuj_katalog(biezaca_sciezka);
            }
        }
    }
}

// ══════════════════════════════════════════════
//  CZYTNIK E-BOOKÓW (.txt z SD)
// ══════════════════════════════════════════════
void czytnik_ebookow() {
    // Szukaj plików .txt
    String txt_pliki[MAX_PLIKOW];
    int txt_liczba = 0;
    File dir = SD.open("/books");
    if (!dir) { SD.mkdir("/books"); }
    // Szukaj też w głównym
    File root = SD.open("/");
    while (txt_liczba < MAX_PLIKOW) {
        File f = root.openNextFile();
        if (!f) break;
        String n = f.name();
        if (n.endsWith(".txt") || n.endsWith(".TXT")) txt_pliki[txt_liczba++] = n;
        f.close();
    }
    root.close();

    if (txt_liczba == 0) {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("E-BOOK", THEME_YELLOW);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 50);
        M5Cardputer.Display.print("Brak plikow .txt na SD.");
        M5Cardputer.Display.setCursor(4, 65);
        M5Cardputer.Display.print("Wrzuc ksiazki do /books/");
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("Dowolny klawisz = powrot");
        ui_wait_key(); return;
    }

    const char* nbuf[MAX_PLIKOW];
    for (int i = 0; i < txt_liczba; i++) nbuf[i] = txt_pliki[i].c_str();
    int sel = ui_select_list(nbuf, txt_liczba, "WYBIERZ KSIAZKE", THEME_YELLOW);
    if (sel < 0) return;

    File f = SD.open(("/"+txt_pliki[sel]).c_str());
    if (!f) { ui_show_error("Nie mozna otworzyc!"); delay(1500); return; }

    long rozmiar = f.size();
    long pozycja = 0;
    const int LINIA_SZ = 38;
    const int WIERSZY = 15;
    bool redraw = true;

    while (true) {
        if (redraw) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            // Nagłówek z nazwą pliku
            String nagl = txt_pliki[sel];
            if (nagl.length() > 24) nagl = nagl.substring(0, 21) + "...";
            ui_draw_header(nagl.c_str(), THEME_YELLOW);

            f.seek(pozycja);
            int y = 22;
            for (int w = 0; w < WIERSZY && f.available(); w++) {
                String linia = "";
                while (f.available() && (int)linia.length() < LINIA_SZ) {
                    char c = f.read();
                    if (c == '\n') break;
                    if (c >= 32) linia.concat(c);
                }
                M5Cardputer.Display.setTextColor(TFT_WHITE);
                M5Cardputer.Display.setCursor(4, y);
                M5Cardputer.Display.print(linia);
                y += 12;
            }

            // Pasek postępu
            int pct = rozmiar > 0 ? (int)((long)pozycja * 100 / rozmiar) : 0;
            M5Cardputer.Display.drawRect(4, 205, 200, 6, THEME_BORDER);
            M5Cardputer.Display.fillRect(5, 206, pct * 198 / 100, 4, THEME_YELLOW);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(208, 205);
            char pb[5]; snprintf(pb, sizeof(pb), "%d%%", pct);
            M5Cardputer.Display.print(pb);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 214);
            M5Cardputer.Display.print("S=dalej  W=wstecz  Q=wyjscie");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if (c == 's' || c == 'S') {
                    long npos = f.position();
                    if (npos < rozmiar) { pozycja = npos; redraw = true; }
                }
                if (c == 'w' || c == 'W') {
                    pozycja = max(0L, pozycja - LINIA_SZ * WIERSZY * 2);
                    redraw = true;
                }
                if (c == 'q' || c == 'Q') goto koniec_ebook;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) goto koniec_ebook;
        delay(10);
    }
    koniec_ebook:
    f.close();
}

// ══════════════════════════════════════════════
//  TERMINAL UART
// ══════════════════════════════════════════════
void terminal_uart() {
    // Wybór prędkości
    const char* baudrate[] = { "9600", "19200", "38400", "57600", "115200", "230400", "Powrot" };
    int sel = ui_select_list(baudrate, 7, "TERMINAL UART", THEME_GREEN);
    if (sel < 0 || sel == 6) return;

    long baudy[] = { 9600, 19200, 38400, 57600, 115200, 230400 };
    Serial2.begin(baudy[sel], SERIAL_8N1, GPIO_NUM_18, GPIO_NUM_17); // TX=17, RX=18

    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header(("TERMINAL UART " + String(baudy[sel])).c_str(), THEME_GREEN);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 22);
    M5Cardputer.Display.print("TX=GPIO17 RX=GPIO18  FN+Q=wyjscie");

    int y = 34;
    String linia_wej = "";

    while (true) {
        // Odbiór z UART
        while (Serial2.available()) {
            char c = Serial2.read();
            if (c == '\n' || c == '\r') {
                if (y > 200) {
                    M5Cardputer.Display.fillRect(0, 34, 240, 170, THEME_BG);
                    y = 34;
                }
            } else {
                if (y > 34 && linia_wej.length() >= 37) {
                    y += 11;
                    linia_wej = "";
                }
                linia_wej.concat(c);
                M5Cardputer.Display.setTextColor(THEME_CYAN);
                M5Cardputer.Display.setCursor(4, y);
                M5Cardputer.Display.print(linia_wej);
            }
        }

        // Klawiatura → wyślij przez UART
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            if (st.fn && _word_eq(st.word, "q")) break;
            for (auto c : st.word) {
                Serial2.print(c);
                // Echo lokalne
                M5Cardputer.Display.setTextColor(THEME_GREEN);
                M5Cardputer.Display.setCursor(4, 205);
                M5Cardputer.Display.fillRect(0, 204, 240, 12, THEME_PANEL);
                M5Cardputer.Display.print("> ");
                static String bufor_wyj = "";
                if (c == '\n') { Serial2.println(); bufor_wyj = ""; }
                else bufor_wyj.concat(c);
                M5Cardputer.Display.print(bufor_wyj);
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) break;
        delay(5);
    }
    Serial2.end();
}

// ══════════════════════════════════════════════
//  LOG SYSTEMOWY
// ══════════════════════════════════════════════
#define LOG_PLIK "/system.log"
#define MAX_LOG_SIZE 50000

void dodaj_log(const String& wpis) {
    // Sprawdź rozmiar logu
    if (SD.exists(LOG_PLIK)) {
        File f = SD.open(LOG_PLIK);
        if (f.size() > MAX_LOG_SIZE) { f.close(); SD.remove(LOG_PLIK); }
        else f.close();
    }
    File f = SD.open(LOG_PLIK, FILE_APPEND);
    if (!f) return;
    struct tm ti; getLocalTime(&ti);
    char ts[24]; strftime(ts, sizeof(ts), "[%d.%m.%Y %H:%M:%S] ", &ti);
    f.print(ts); f.println(wpis);
    f.close();
}

void pogladaj_log() {
    if (!SD.exists(LOG_PLIK)) {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("LOG SYSTEMU", THEME_MUTED);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 50);
        M5Cardputer.Display.print("Log jest pusty.");
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("Dowolny klawisz = powrot");
        ui_wait_key(); return;
    }

    File f = SD.open(LOG_PLIK);
    String tresc = "";
    while (f.available() && tresc.length() < 3000) tresc += (char)f.read();
    f.close();

    int offset = max(0, (int)tresc.length() - 37 * 15); // Pokaż od końca
    bool redraw = true;

    while (true) {
        if (redraw) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("LOG SYSTEMU", THEME_MUTED);
            int y = 22;
            int pos = offset;
            while (pos < (int)tresc.length() && y < 205) {
                M5Cardputer.Display.setTextColor(tresc[pos] == '[' ? THEME_MUTED : TFT_WHITE);
                M5Cardputer.Display.setCursor(4, y);
                M5Cardputer.Display.print(tresc.substring(pos, pos + 37));
                pos += 37; y += 11;
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 212);
            M5Cardputer.Display.print("W/S=scroll  C=czysc  Q=wyjscie");
            redraw = false;
        }
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if (c == 's' || c == 'S') { offset = min((int)tresc.length(), offset + 37); redraw = true; }
                if (c == 'w' || c == 'W') { offset = max(0, offset - 37); redraw = true; }
                if (c == 'c' || c == 'C') { SD.remove(LOG_PLIK); ui_show_info("Log wyczyszczony!", THEME_YELLOW); delay(1000); return; }
                if (c == 'q' || c == 'Q') return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}

void app_system_run() {
    while (true) {
        const char* opts[] = {
            "Menedzer plikow SD",
            "Czytnik e-bookow (.txt)",
            "Terminal UART",
            "Log systemowy",
            "Powrot"
        };
        int sel = ui_select_list(opts, 5, "SYSTEM", THEME_GRAY);
        if (sel < 0 || sel == 4) return;
        switch (sel) {
            case 0: menedzer_plikow();    break;
            case 1: czytnik_ebookow();   break;
            case 2: terminal_uart();     break;
            case 3: pogladaj_log();      break;
        }
    }
}
