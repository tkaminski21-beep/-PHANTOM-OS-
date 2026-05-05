#pragma once
// app_radio.h — Radio internetowe (streaming MP3/AAC przez WiFi)
// Używa ESP8266Audio do dekodowania
//
// Domyślne stacje: różne polskie i międzynarodowe radia
// Można dodać własne na karcie SD: /radio/stations.txt
//   Format: NAZWA|URL  (jedna stacja na linię)

#include "M5Cardputer.h"
#include "core/ui.h"
#include <WiFi.h>
#include <SD.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

extern uint16_t UI_BG, UI_FG, UI_PRI;

// ─── Domyślne stacje ──────────────────────────────────
struct RadioStation { const char* name; const char* url; };
static const RadioStation _default_stations[] = {
    {"RMF FM",       "http://rs101-krk.rmfstream.pl/RMFFM48"},
    {"RMF Maxx",     "http://rs101-krk.rmfstream.pl/rmf_maxxx"},
    {"Radio ZET",    "http://n-4-13.dcs.redcdn.pl/sc/o2/Eurozet/live/audio.livx?audio=5"},
    {"Antyradio",    "https://an.cdn.eurozet.pl/ant-waw.mp3"},
    {"Eska Hits",    "http://waw02-03.ic.smcdn.pl/3290-1.mp3"},
    {"Trojka PR",    "http://stream4.nadaje.com:11104/trojka-aac"},
    {"BBC R1",       "http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one"},
    {"NRK P1",       "http://lyd.nrk.no/nrk_radio_p1_mp3_h"},
    {"SomaFM Groove","http://ice1.somafm.com/groovesalad-128-mp3"},
    {"SomaFM Drone", "http://ice1.somafm.com/dronezone-128-mp3"},
    {"BoxRadio Lofi","http://stream.zeno.fm/0r0xa792kwzuv"},
};
static const int N_DEFAULT_STATIONS = sizeof(_default_stations)/sizeof(_default_stations[0]);

// ─── Globalne obiekty audio ───────────────────────────
static AudioGeneratorMP3*       _mp3 = nullptr;
static AudioFileSourceICYStream* _stream = nullptr;
static AudioFileSourceBuffer*   _buffer = nullptr;
static AudioOutputI2S*          _out = nullptr;

static void _audio_stop() {
    if (_mp3) {
        if (_mp3->isRunning()) _mp3->stop();
        delete _mp3; _mp3 = nullptr;
    }
    if (_buffer) { delete _buffer; _buffer = nullptr; }
    if (_stream) { delete _stream; _stream = nullptr; }
    if (_out)    { delete _out;    _out = nullptr; }
}

static void _meta_cb(void* cbData, const char* type, bool isUnicode, const char* str) {
    // Można obsłużyć metadane (Title, StreamTitle), na razie pusto
}

static bool _audio_start(const char* url) {
    _audio_stop();

    _out = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
    _out->SetGain(0.5f);

    _stream = new AudioFileSourceICYStream(url);
    _stream->RegisterMetadataCB(_meta_cb, nullptr);
    _buffer = new AudioFileSourceBuffer(_stream, 8192);

    _mp3 = new AudioGeneratorMP3();
    return _mp3->begin(_buffer, _out);
}

// ─── Załaduj stacje (domyślne + z SD) ─────────────────
static std::vector<String> _names;
static std::vector<String> _urls;

static void _load_stations() {
    _names.clear();
    _urls.clear();
    for (int i = 0; i < N_DEFAULT_STATIONS; i++) {
        _names.push_back(String(_default_stations[i].name));
        _urls.push_back(String(_default_stations[i].url));
    }
    // Dodaj z SD (jeśli istnieje)
    if (SD.exists("/radio/stations.txt")) {
        File f = SD.open("/radio/stations.txt", FILE_READ);
        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            int sep = line.indexOf('|');
            if (sep > 0) {
                _names.push_back(line.substring(0, sep));
                _urls.push_back(line.substring(sep+1));
            }
        }
        f.close();
    }
}

// ─── Player ───────────────────────────────────────────
static void _player(int idx) {
    int volume = 50;  // 0-100

    M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(60, 5); M5Cardputer.Display.print("RADIO INTERNETOWE");
    M5Cardputer.Display.drawLine(0, 18, 240, 18, (uint32_t)UI_PRI);

    M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(8, 26); M5Cardputer.Display.print("Laczenie z:");
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(8, 40); M5Cardputer.Display.print(_names[idx]);
    M5Cardputer.Display.setTextSize(1);

    // Setup głośnika Cardputer (PWM tone wyłącz, użyjemy I2S DAC)
    M5Cardputer.Speaker.end();
    delay(100);

    bool ok = _audio_start(_urls[idx].c_str());

    if (!ok) {
        M5Cardputer.Display.setTextColor(0xF800, (uint32_t)UI_BG);
        M5Cardputer.Display.setCursor(8, 80); M5Cardputer.Display.print("Blad polaczenia!");
        delay(2500);
        _audio_stop();
        M5Cardputer.Speaker.begin();
        return;
    }

    M5Cardputer.Display.fillRect(0, 22, 240, 100, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextColor(0x07E0, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(8, 26); M5Cardputer.Display.print("> GRA");
    M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(8, 42); M5Cardputer.Display.print(_names[idx]);
    M5Cardputer.Display.setTextSize(1);

    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("A/D=glos  W/S=stacja  ESC=stop");

    bool needRedrawVol = true;

    while (true) {
        // Kluczowe — utrzymuj strumień
        if (_mp3 && _mp3->isRunning()) {
            if (!_mp3->loop()) {
                _mp3->stop();
                M5Cardputer.Display.setTextColor(0xF800, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(8, 78); M5Cardputer.Display.print("Stream przerwany");
                delay(2000);
                break;
            }
        } else { break; }

        // Odśwież głośność
        if (needRedrawVol) {
            M5Cardputer.Display.fillRect(0, 90, 240, 22, (uint32_t)UI_BG);
            M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(8, 92); M5Cardputer.Display.print("Glosnosc:");
            M5Cardputer.Display.drawRect(78, 92, 150, 8, (uint32_t)UI_PRI);
            M5Cardputer.Display.fillRect(80, 94, 146*volume/100, 4, (uint32_t)UI_PRI);
            char vb[8]; snprintf(vb, sizeof(vb), "%d%%", volume);
            M5Cardputer.Display.setCursor(8, 105); M5Cardputer.Display.print(vb);
            needRedrawVol = false;
        }

        // Klawiatura - bez delay'a! Tylko sprawdź zmianę
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) {
                if (c == 27) {  // ESC = stop
                    _audio_stop();
                    M5Cardputer.Speaker.begin();
                    return;
                }
                if (c == 'a' || c == 'A') {
                    volume = max(0, volume - 10);
                    if (_out) _out->SetGain(volume / 100.0f);
                    needRedrawVol = true;
                }
                if (c == 'd' || c == 'D') {
                    volume = min(100, volume + 10);
                    if (_out) _out->SetGain(volume / 100.0f);
                    needRedrawVol = true;
                }
                if (c == 'w' || c == 'W') {  // poprzednia stacja
                    int newIdx = (idx - 1 + (int)_names.size()) % (int)_names.size();
                    _audio_stop();
                    _player(newIdx);
                    return;
                }
                if (c == 's' || c == 'S') {  // następna stacja
                    int newIdx = (idx + 1) % (int)_names.size();
                    _audio_stop();
                    _player(newIdx);
                    return;
                }
            }
        }
        // BEZ delay() — strumień musi być na bieżąco czytany
    }

    _audio_stop();
    M5Cardputer.Speaker.begin();
}

// ─── MENU ──────────────────────────────────────────────
void app_radio_menu() {
    if (WiFi.status() != WL_CONNECTED) {
        ui_show_info("Polacz najpierw WiFi!", 0xF800);
        delay(2500);
        return;
    }

    _load_stations();
    if (_names.empty()) {
        ui_show_info("Brak stacji radiowych!", 0xF800);
        delay(2000);
        return;
    }

    const char** opts = new const char*[_names.size()];
    for (size_t i = 0; i < _names.size(); i++) opts[i] = _names[i].c_str();
    int sel = ui_select_list(opts, _names.size(), "RADIO INTERNETOWE", (uint32_t)UI_PRI);
    delete[] opts;

    if (sel >= 0) _player(sel);
}
