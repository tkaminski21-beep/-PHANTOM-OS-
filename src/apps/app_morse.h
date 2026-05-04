#pragma once
// app_morse.h — Kod Morse'a — koder i dekoder przez głośnik/IR

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include "core/sound.h"

extern uint16_t UI_BG, UI_FG, UI_PRI;

// ─── Tablica Morse'a ──────────────────────────────────
struct MorseCode { char ch; const char* code; };
static const MorseCode MORSE_TABLE[] = {
    {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
    {'E', "."},    {'F', "..-."}, {'G', "--."},  {'H', "...."},
    {'I', ".."},   {'J', ".---"}, {'K', "-.-"},  {'L', ".-.."},
    {'M', "--"},   {'N', "-."},   {'O', "---"},  {'P', ".--."},
    {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
    {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"},
    {'Y', "-.--"}, {'Z', "--.."},
    {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
    {'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."},
    {'8', "---.."}, {'9', "----."},
    {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."}, {'!', "-.-.--"},
    {' ', " "},
};
const int MORSE_COUNT = sizeof(MORSE_TABLE)/sizeof(MORSE_TABLE[0]);

static const char* _morse_lookup(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;  // upper
    for (int i = 0; i < MORSE_COUNT; i++)
        if (MORSE_TABLE[i].ch == c) return MORSE_TABLE[i].code;
    return "";
}

// ─── NADAJNIK ─────────────────────────────────────────
// Dot = 100ms, dash = 300ms, between letters = 300ms, between words = 700ms
#define MORSE_DOT 120
#define MORSE_DASH (3 * MORSE_DOT)
#define MORSE_GAP MORSE_DOT
#define MORSE_LETTER_GAP (3 * MORSE_DOT)
#define MORSE_WORD_GAP (7 * MORSE_DOT)
#define MORSE_FREQ 800

static bool _morse_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

void morse_send_string(const String& msg) {
    M5Cardputer.Display.fillScreen(UI_BG);
    M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(70, 10); M5Cardputer.Display.print("MORSE NADAWANIE");
    M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setCursor(8, 30);
    M5Cardputer.Display.print("Tekst: " + msg.substring(0, min((int)msg.length(), 28)));

    M5Cardputer.Display.setCursor(8, 124);
    M5Cardputer.Display.print("FN+Q = stop");

    int yProgress = 50;
    String currentCode = "";

    for (size_t i = 0; i < msg.length(); i++) {
        if (_morse_check_esc()) return;
        char ch = msg[i];
        const char* code = _morse_lookup(ch);
        currentCode = String(code);

        // Pokaż aktualną literę
        M5Cardputer.Display.fillRect(8, yProgress, 224, 40, UI_BG);
        M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
        M5Cardputer.Display.setTextSize(3);
        M5Cardputer.Display.setCursor(110, yProgress);
        M5Cardputer.Display.print(ch);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setCursor(8, yProgress + 35);
        M5Cardputer.Display.print(currentCode);

        if (ch == ' ') { delay(MORSE_WORD_GAP); continue; }

        for (int j = 0; code[j]; j++) {
            if (_morse_check_esc()) return;
            if (code[j] == '.') {
                if (snd::enabled()) { M5Cardputer.Speaker.tone(MORSE_FREQ, MORSE_DOT); } else { delay(MORSE_DOT); }
                delay(MORSE_DOT);
                delay(MORSE_GAP);
            } else if (code[j] == '-') {
                if (snd::enabled()) { M5Cardputer.Speaker.tone(MORSE_FREQ, MORSE_DASH); } else { delay(MORSE_DASH); }
                delay(MORSE_DASH);
                delay(MORSE_GAP);
            }
        }
        delay(MORSE_LETTER_GAP);
    }
    delay(500);
}

void app_morse_send() {
    M5Cardputer.Display.fillScreen(UI_BG);
    M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(70, 10); M5Cardputer.Display.print("MORSE - WPISZ");
    M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

    M5Cardputer.Display.setCursor(8, 30); M5Cardputer.Display.print("Wpisz tekst:");
    M5Cardputer.Display.drawRect(8, 45, 224, 16, UI_PRI);
    M5Cardputer.Display.setCursor(8, 110); M5Cardputer.Display.print("ENTER=nadaj  ESC=wyjdz");

    String input = "";
    bool redraw = true;
    while (true) {
        if (redraw) {
            M5Cardputer.Display.fillRect(9, 46, 222, 14, UI_BG);
            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setCursor(12, 49);
            M5Cardputer.Display.print(input);
            M5Cardputer.Display.print("_");
            redraw = false;
        }
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.del && input.length() > 0) { input.remove(input.length()-1); redraw = true; }
            for (char c : s.word) {
                if (c == '\n' || c == '\r') {
                    if (input.length() > 0) { morse_send_string(input); return; }
                }
                else if (c == 27) return;
                else if (c >= 32 && c < 127 && input.length() < 50) {
                    input += c;
                    redraw = true;
                }
            }
        }
        delay(20);
    }
}

// ─── TABLICA MORSE'A (referencja) ──────────────────────
void app_morse_table() {
    int offset = 0;
    bool redraw = true;
    while (true) {
        if (redraw) {
            M5Cardputer.Display.fillScreen(UI_BG);
            M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(80, 10); M5Cardputer.Display.print("TABLICA MORSE");
            M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

            // 2 kolumny, 8 wierszy = 16 znaków na ekranie
            const int ROWS = 8;
            for (int i = 0; i < ROWS*2; i++) {
                int idx = offset + i;
                if (idx >= MORSE_COUNT) break;
                int col = i / ROWS;
                int row = i % ROWS;
                int x = 12 + col * 115;
                int y = 28 + row * 11;
                M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
                M5Cardputer.Display.setCursor(x, y);
                char ch = MORSE_TABLE[idx].ch;
                if (ch == ' ') M5Cardputer.Display.print("[ ]");
                else { M5Cardputer.Display.print(ch); }
                M5Cardputer.Display.setTextColor(UI_PRI & 0xC618, UI_BG);
                M5Cardputer.Display.setCursor(x + 18, y);
                M5Cardputer.Display.print(MORSE_TABLE[idx].code);
            }

            M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
            M5Cardputer.Display.setCursor(8, 122);
            M5Cardputer.Display.print("FN+;/. przewin  ESC=wyjdz");
            redraw = false;
        }
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn) {
                if (_word_eq(s.word, ";")) { if(offset > 0) { offset = max(0, offset-16); redraw = true; } }
                if (_word_eq(s.word, ".")) { if(offset+16 < MORSE_COUNT) { offset += 16; redraw = true; } }
            } else {
                for (char c : s.word) { if (c==27) return; }
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}

// ─── MENU ─────────────────────────────────────────────
void app_morse_menu() {
    while (true) {
        const char* opts[] = {"Nadaj tekst","Tablica Morse'a"};
        int sel = ui_select_list(opts, 2, "KOD MORSE'A", UI_PRI);
        if (sel < 0) return;
        if (sel == 0) app_morse_send();
        if (sel == 1) app_morse_table();
    }
}
