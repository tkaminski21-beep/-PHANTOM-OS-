#pragma once
// apps/app_narzedzia.h — Generator haseł, Base64, HEX

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"

// ─── Generator haseł ────────────────────────────────────
static const char MALE[]   = "abcdefghijklmnopqrstuvwxyz";
static const char DUZE[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char CYFRY[]  = "0123456789";
static const char ZNAKI[]  = "!@#$%^&*()-_=+[]{}|;:,.<>?";

String generuj_haslo(int dlugosc, bool male, bool duze, bool cyfry, bool znaki_spec) {
    String pula = "";
    if (male)       pula += String(MALE);
    if (duze)       pula += String(DUZE);
    if (cyfry)      pula += String(CYFRY);
    if (znaki_spec) pula += String(ZNAKI);
    if (pula.length() == 0) pula = String(MALE);

    String haslo = "";
    for (int i = 0; i < dlugosc; i++)
        haslo += pula[esp_random() % pula.length()];
    return haslo;
}

void app_generator_hasel() {
    int dlugosc = 16;
    bool male_l = true, duze_l = true, cyfry_l = true, znaki_l = false;
    String haslo = generuj_haslo(dlugosc, male_l, duze_l, cyfry_l, znaki_l);
    bool redraw = true;

    while (true) {
        if (redraw) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("GENERATOR HASEL", THEME_GREEN);

            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("Wygenerowane haslo:");
            M5Cardputer.Display.fillRoundRect(4, 40, 232, 24, 4, THEME_PANEL);
            M5Cardputer.Display.drawRoundRect(4, 40, 232, 24, 4, THEME_GREEN);
            M5Cardputer.Display.setTextColor(TFT_WHITE);
            M5Cardputer.Display.setCursor(8, 48);
            // Podziel na dwie linie jeśli za długie
            if (haslo.length() > 20) {
                M5Cardputer.Display.setCursor(8, 42);
                M5Cardputer.Display.print(haslo.substring(0, 20));
                M5Cardputer.Display.setCursor(8, 54);
                M5Cardputer.Display.print(haslo.substring(20));
            } else {
                M5Cardputer.Display.print(haslo);
            }

            // Opcje
            auto opcja = [&](const char* lab, bool wl, int y) {
                M5Cardputer.Display.fillRoundRect(4, y, 112, 18, 3, wl ? THEME_GREEN>>1 : THEME_PANEL);
                M5Cardputer.Display.drawRoundRect(4, y, 112, 18, 3, wl ? THEME_GREEN : THEME_BORDER);
                M5Cardputer.Display.setTextColor(wl ? THEME_GREEN : THEME_MUTED);
                M5Cardputer.Display.setCursor(10, y + 5);
                M5Cardputer.Display.print(wl ? "[X] " : "[ ] ");
                M5Cardputer.Display.print(lab);
            };
            opcja("Male litery",  male_l,  80);
            opcja("Duze litery",  duze_l,  102);
            opcja("Cyfry",        cyfry_l, 124);
            opcja("Znaki spec.",  znaki_l, 146);

            // Długość
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(125, 80);
            M5Cardputer.Display.print("Dlugosc:");
            M5Cardputer.Display.setTextColor(TFT_WHITE);
            M5Cardputer.Display.setTextSize(2);
            M5Cardputer.Display.setCursor(125, 94);
            M5Cardputer.Display.print(dlugosc);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(125, 120);
            M5Cardputer.Display.print("+/- zmien");
            M5Cardputer.Display.setCursor(125, 134);
            M5Cardputer.Display.print("A/D opt.");

            // Przyciski dolne
            M5Cardputer.Display.fillRoundRect(4, 168, 110, 20, 4, THEME_GREEN);
            M5Cardputer.Display.setCursor(18, 174); M5Cardputer.Display.setTextColor(THEME_BG);
            M5Cardputer.Display.print("G = Generuj nowe");
            M5Cardputer.Display.fillRoundRect(126, 168, 110, 20, 4, THEME_GRAY);
            M5Cardputer.Display.setCursor(140, 174); M5Cardputer.Display.setTextColor(TFT_WHITE);
            M5Cardputer.Display.print("Q = Wyjscie");

            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 196);
            M5Cardputer.Display.print("1=male 2=duze 3=cyfry 4=znaki");

            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if (c == 'g' || c == 'G') { haslo = generuj_haslo(dlugosc, male_l, duze_l, cyfry_l, znaki_l); redraw = true; }
                if (c == 'q' || c == 'Q') return;
                if (c == '1') { male_l  = !male_l;  redraw = true; }
                if (c == '2') { duze_l  = !duze_l;  redraw = true; }
                if (c == '3') { cyfry_l = !cyfry_l; redraw = true; }
                if (c == '4') { znaki_l = !znaki_l; redraw = true; }
                if (c == '+') { if (dlugosc < 32) dlugosc++; haslo = generuj_haslo(dlugosc, male_l, duze_l, cyfry_l, znaki_l); redraw = true; }
                if (c == '-') { if (dlugosc > 4)  dlugosc--; haslo = generuj_haslo(dlugosc, male_l, duze_l, cyfry_l, znaki_l); redraw = true; }
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}

// ─── Base64 encode ───────────────────────────────────────
static const char B64CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64_encode(const String& in) {
    String out = "";
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out += B64CHARS[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }
    if (valb > -6) out += B64CHARS[((val << 8) >> (valb + 8)) & 0x3F];
    while (out.length() % 4) out += '=';
    return out;
}

String base64_decode(const String& in) {
    String out = "";
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (c == '=') break;
        const char* p = strchr(B64CHARS, c);
        if (!p) continue;
        val = (val << 6) + (p - B64CHARS);
        valb += 6;
        if (valb >= 0) {
            out += (char)((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return out;
}

String do_hex(const String& in) {
    String out = "";
    for (unsigned char c : in) {
        char buf[3]; snprintf(buf, sizeof(buf), "%02X", c);
        out += buf;
    }
    return out;
}

String z_hex(const String& in) {
    String out = "";
    for (int i = 0; i + 1 < (int)in.length(); i += 2) {
        char buf[3] = { in[i], in[i+1], 0 };
        out += (char)strtol(buf, nullptr, 16);
    }
    return out;
}

void app_enkoder_run() {
    while (true) {
        const char* tryby[] = { "Base64 Koduj", "Base64 Dekoduj", "Tekst -> HEX", "HEX -> Tekst", "Powrot" };
        int sel = ui_select_list(tryby, 5, "ENKODER / DEKODER", THEME_CYAN);
        if (sel < 0 || sel == 4) return;

        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("WPISZ TEKST", THEME_CYAN);
        String wejscie = ui_input_string("Tekst:", 4, 30, 40);
        if (wejscie.length() == 0) continue;

        String wynik;
        switch (sel) {
            case 0: wynik = base64_encode(wejscie); break;
            case 1: wynik = base64_decode(wejscie); break;
            case 2: wynik = do_hex(wejscie); break;
            case 3: wynik = z_hex(wejscie); break;
        }

        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("WYNIK", THEME_GREEN);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 30);
        M5Cardputer.Display.print("Wejscie:"); M5Cardputer.Display.print(wejscie.substring(0, 26));
        M5Cardputer.Display.setCursor(4, 44);
        M5Cardputer.Display.print("Wynik:");
        M5Cardputer.Display.setTextColor(TFT_WHITE);

        // Wynik w kilku liniach
        int y = 58;
        for (int i = 0; i < (int)wynik.length(); i += 26) {
            M5Cardputer.Display.setCursor(4, y);
            M5Cardputer.Display.print(wynik.substring(i, i + 26));
            y += 13;
            if (y > 190) break;
        }

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("Dowolny klawisz = powrot");
        ui_wait_key();
    }
}

void app_narzedzia_run() {
    while (true) {
        const char* opts[] = { "Generator hasel", "Enkoder/Dekoder", "Powrot" };
        int sel = ui_select_list(opts, 3, "NARZEDZIA", THEME_CYAN);
        if (sel < 0 || sel == 2) return;
        if (sel == 0) app_generator_hasel();
        if (sel == 1) app_enkoder_run();
    }
}
