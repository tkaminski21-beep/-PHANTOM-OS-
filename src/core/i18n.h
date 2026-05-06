#pragma once
// core/i18n.h — System tłumaczeń PL/EN (System internationalization)
//
// Użycie:
//   T("settings")  → zwraca "Ustawienia" lub "Settings" zależnie od języka
//
// Język zapisany w Preferences pod kluczem "lang" (0=PL, 1=EN)

#include <Arduino.h>
#include <Preferences.h>

extern Preferences prefs;

inline int g_lang = 0;  // 0=PL, 1=EN

inline void i18n_load() {
    g_lang = prefs.getInt("lang", 0);
}
inline void i18n_set(int l) {
    g_lang = (l == 1) ? 1 : 0;
    prefs.putInt("lang", g_lang);
}

// ─── Słownik ─────────────────────────────────────────
struct Translation { const char* key; const char* pl; const char* en; };

static const Translation _T[] = {
    // Menu główne
    {"wifi",          "WiFi",                  "WiFi"},
    {"bluetooth",     "Bluetooth",             "Bluetooth"},
    {"wifi_atk",      "Ataki WiFi",            "WiFi Attacks"},
    {"pwnagotchi",    "Pwnagotchi",            "Pwnagotchi"},
    {"badusb",        "BadUSB BLE",            "BadUSB BLE"},
    {"ota",           "Aktualizacja OTA",      "OTA Update"},
    {"network",       "Siec/Ping",             "Network/Ping"},
    {"ir",            "Pilot IR",              "IR Remote"},
    {"rf",            "RF CC1101",             "RF CC1101"},
    {"nrf24",         "NRF24",                 "NRF24"},
    {"gps",           "GPS",                   "GPS"},
    {"fm",            "Radio FM",              "FM Radio"},
    {"imu",           "Czujnik / Poziomica",   "IMU / Level"},
    {"oscope",        "Oscyloskop",            "Oscilloscope"},
    {"notes",         "Notatnik",              "Notes"},
    {"calc",          "Kalkulator",            "Calculator"},
    {"resistor",      "Rezystory",             "Resistors"},
    {"converter",     "Konwerter",             "Converter"},
    {"clock",         "Zegar",                 "Clock"},
    {"tools",         "Narzedzia",             "Tools"},
    {"mp3",           "Odtwarzacz MP3",        "MP3 Player"},
    {"launcher",      "Launcher",              "Launcher"},
    {"run_bin",       "Uruchom .bin",          "Run .bin"},
    {"monitor",       "Monitor systemu",       "System Monitor"},
    {"sd_files",      "Pliki SD",              "SD Files"},
    {"sd_browser",    "Przegladarka SD",       "SD Browser"},
    {"games",         "Gry",                   "Games"},
    {"stopwatch",     "Stoper",                "Stopwatch"},
    {"morse",         "Kod Morse'a",           "Morse Code"},
    {"internet",      "Pogoda i krypto",       "Weather & Crypto"},
    {"radio_net",     "Radio internetowe",     "Internet Radio"},
    {"meshtastic",    "Meshtastic",            "Meshtastic"},
    {"rfid",          "RFID RC522",            "RFID RC522"},
    {"screensaver",   "Wygaszacz",             "Screensaver"},
    {"settings",      "Ustawienia",            "Settings"},

    // Ustawienia
    {"color_presets",       "Presety kolorow",        "Color presets"},
    {"rgb_editor",          "Edytor RGB",             "RGB editor"},
    {"brightness",          "Jasnosc ekranu",         "Screen brightness"},
    {"sound_toggle",        "Dzwiek (wlacz/wylacz)",  "Sound (on/off)"},
    {"screensaver_time",    "Wygaszacz - czas",       "Screensaver - time"},
    {"language",            "Jezyk / Language",       "Language / Jezyk"},
    {"system_info",         "Informacje o systemie",  "System info"},

    // Komunikaty
    {"loading",       "Ladowanie...",          "Loading..."},
    {"connecting",    "Laczenie...",           "Connecting..."},
    {"saved",         "Zapisano!",             "Saved!"},
    {"error",         "Blad",                  "Error"},
    {"yes",           "Tak",                   "Yes"},
    {"no",            "Nie",                   "No"},
    {"on",            "Wlaczone",              "Enabled"},
    {"off",           "Wylaczone",             "Disabled"},
    {"ok",            "OK",                    "OK"},
    {"cancel",        "Anuluj",                "Cancel"},
    {"back",          "Powrot",                "Back"},
    {"select",        "Wybierz",               "Select"},
    {"running",       "Dziala",                "Running"},
    {"stopped",       "Zatrzymano",            "Stopped"},

    // WiFi/BT
    {"scanning",      "Skanowanie...",         "Scanning..."},
    {"no_networks",   "Brak sieci",            "No networks"},
    {"target",        "Cel",                   "Target"},
    {"channel",       "Kanal",                 "Channel"},
    {"signal",        "Sygnal",                "Signal"},

    // Stopka
    {"footer_nav",    "Strzalki=nawigacja",    "Arrows=navigate"},
    {"footer_ok",     "OK=wybierz",            "OK=select"},
    {"footer_esc",    "ESC=powrot",            "ESC=back"},

    // Aplikacje
    {"weather",       "Pogoda",                "Weather"},
    {"crypto",        "Kryptowaluty",          "Cryptocurrency"},
    {"timer",         "Minutnik",              "Timer"},
    {"alarm",         "Alarm",                 "Alarm"},
};
static const int _T_COUNT = sizeof(_T) / sizeof(_T[0]);

// ─── T() — pobierz tłumaczenie ───────────────────────
inline const char* T(const char* key) {
    for (int i = 0; i < _T_COUNT; i++) {
        if (strcmp(_T[i].key, key) == 0) {
            return (g_lang == 1) ? _T[i].en : _T[i].pl;
        }
    }
    return key;  // fallback - zwróć klucz
}

inline const char* lang_name() {
    return (g_lang == 1) ? "English" : "Polski";
}
