#pragma once
// core/keyboard_input.h v4.0 — POPRAWNA obsługa klawiatury Cardputer ADV
//
// KLUCZOWA POPRAWKA: M5Cardputer.Keyboard.keysState() zwraca:
//   .enter — BOOL true gdy wciśnięty klawisz OK (prawy dolny róg)
//   .del   — BOOL true gdy wciśnięty BACKSPACE
//   .fn    — BOOL true gdy wciśnięty FN
//   .ctrl  — BOOL true gdy wciśnięty CTRL
//   .alt   — BOOL true gdy wciśnięty ALT
//   .word  — std::vector<char> z literami/cyframi/symbolami
//
// Dlatego "OK" NIE jest w s.word jako '\n', tylko w s.enter == true!
//
// Strzałki na CardputerADV:
//   ←  = FN+,    →  = FN+/    ↑  = FN+;    ↓  = FN+.

#include "M5Cardputer.h"
#include <Arduino.h>
#include <vector>

// ─── Globalne flagi (Bruce-style) ────────────────────
inline bool PrevPress   = false;
inline bool NextPress   = false;
inline bool SelPress    = false;   // OK / ENTER
inline bool EscPress    = false;   // ESC / BACKSPACE / FN+Q
inline bool AnyKeyPress = false;
inline char LastChar    = 0;
inline bool LastDel     = false;
inline bool LastFn      = false;
inline bool LastEnter   = false;

inline void input_clear() {
    PrevPress = NextPress = SelPress = EscPress = AnyKeyPress = false;
    LastChar = 0; LastDel = false; LastFn = false; LastEnter = false;
}

// ─── input_update: główna funkcja ────────────────────
inline void input_update() {
    M5Cardputer.update();

    PrevPress = false; NextPress = false; SelPress = false; EscPress = false;
    AnyKeyPress = false; LastChar = 0; LastDel = false; LastFn = false; LastEnter = false;

    // BtnA (G0) = SEL
    if (M5Cardputer.BtnA.wasPressed()) {
        SelPress = true; AnyKeyPress = true;
    }

    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return;

    auto s = M5Cardputer.Keyboard.keysState();
    LastFn = s.fn;
    LastDel = s.del;
    LastEnter = s.enter;

    // OK/ENTER — KLUCZOWE: użyj s.enter (NIE '\n' w s.word)
    if (s.enter) {
        SelPress = true;
        AnyKeyPress = true;
    }

    // Backspace bez FN = ESC (powrót)
    if (s.del && !s.fn) {
        EscPress = true;
        AnyKeyPress = true;
    }

    // Litery i znaki w s.word
    for (char c : s.word) {
        AnyKeyPress = true;
        LastChar = c;

        if (s.fn) {
            // FN + ; lub , = poprzedni (UP/LEFT)
            if (c == ';' || c == ',') PrevPress = true;
            // FN + . lub / = następny (DOWN/RIGHT)
            if (c == '.' || c == '/') NextPress = true;
            // FN + Q = ESC
            if (c == 'q' || c == 'Q') EscPress = true;
        } else {
            // ESC = ` (backtick) lub kod 27
            if (c == 27 || c == '`') EscPress = true;
            // ENTER alternatywne (jeśli zewnętrzna klawiatura wysyła '\n')
            if (c == '\n' || c == '\r') SelPress = true;
            // TAB → next (alternatywa)
            if (c == '\t') NextPress = true;
        }
    }
}

// ─── Pomocnicze (kompatybilność wsteczna) ────────────
template<typename KS>
inline bool _is_up(const KS& s) {
    if (s.fn) {
        for (char c : s.word) if (c == ';' || c == ',') return true;
    }
    return false;
}
template<typename KS>
inline bool _is_down(const KS& s) {
    if (s.fn) {
        for (char c : s.word) if (c == '.' || c == '/') return true;
    }
    return false;
}
template<typename KS>
inline bool _is_left(const KS& s)  { return s.fn && [&]{ for(char c:s.word)if(c==',')return true; return false; }(); }
template<typename KS>
inline bool _is_right(const KS& s) { return s.fn && [&]{ for(char c:s.word)if(c=='/')return true; return false; }(); }
template<typename KS>
inline bool _is_ok(const KS& s) {
    // OK = klawisz Enter (s.enter) lub '\n' w s.word
    if (s.enter) return true;
    for (char c : s.word) if (c == '\n' || c == '\r') return true;
    return false;
}
template<typename KS>
inline bool _is_esc(const KS& s) {
    if (s.fn) {
        for (char c : s.word) if (c == 'q' || c == 'Q') return true;
    } else {
        for (char c : s.word) if (c == 27 || c == '`') return true;
        if (s.del) return true;
    }
    return false;
}

// ─── Pomocnicze dla aplikacji ────────────────────────

// Czeka na OK lub ESC, zwraca true jeśli OK
inline bool ui_wait_ok_or_esc() {
    while (true) {
        input_update();
        if (SelPress) return true;
        if (EscPress) return false;
        delay(20);
    }
}

// Sprawdza czy ESC został wciśnięty (do długich pętli)
inline bool ui_check_esc() {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto s = M5Cardputer.Keyboard.keysState();
        if (s.fn) {
            for (char c : s.word) if (c == 'q' || c == 'Q') return true;
        } else {
            for (char c : s.word) if (c == 27 || c == '`') return true;
            if (s.del) return true;
        }
    }
    return false;
}
