#pragma once
// core/sound.h — Dźwięki nawigacji
// Używane w głównym menu i aplikacjach

#include "M5Cardputer.h"
#include <Preferences.h>

extern Preferences prefs;

namespace snd {

inline bool enabled() {
    return prefs.getBool("snd_on", true);
}

inline void set_enabled(bool e) {
    prefs.putBool("snd_on", e);
}

// ─── Krótki "tick" przy poruszaniu po menu ────────────
inline void tick() {
    if (!enabled()) return;
    M5Cardputer.Speaker.tone(2200, 15);
}

// ─── Wyższy ton przy potwierdzeniu/wyborze ────────────
inline void confirm() {
    if (!enabled()) return;
    M5Cardputer.Speaker.tone(1800, 30);
    delay(40);
    M5Cardputer.Speaker.tone(2400, 50);
}

// ─── Niski ton przy anulowaniu/błędzie ────────────────
inline void cancel() {
    if (!enabled()) return;
    M5Cardputer.Speaker.tone(800, 30);
    delay(40);
    M5Cardputer.Speaker.tone(500, 80);
}

// ─── Trzy tony — start systemu ────────────────────────
inline void boot() {
    if (!enabled()) return;
    M5Cardputer.Speaker.tone(1000, 80); delay(90);
    M5Cardputer.Speaker.tone(1500, 80); delay(90);
    M5Cardputer.Speaker.tone(2200, 120);
}

// ─── Krótki sygnał błędu ──────────────────────────────
inline void error() {
    if (!enabled()) return;
    M5Cardputer.Speaker.tone(400, 100);
    delay(110);
    M5Cardputer.Speaker.tone(400, 100);
}

// ─── Sukces — wzrastające tony ────────────────────────
inline void success() {
    if (!enabled()) return;
    M5Cardputer.Speaker.tone(1200, 50); delay(60);
    M5Cardputer.Speaker.tone(1800, 50); delay(60);
    M5Cardputer.Speaker.tone(2400, 80);
}

// ─── Pojedynczy beep — ostrzeżenie ────────────────────
inline void beep() {
    if (!enabled()) return;
    M5Cardputer.Speaker.tone(2000, 50);
}

}  // namespace snd
