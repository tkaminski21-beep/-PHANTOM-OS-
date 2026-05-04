#pragma once
// apps/app_mp3.h — Odtwarzacz MP3 z karty SD

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <SD.h>
#include <vector>

// ESP8266Audio — włącz tylko gdy biblioteka jest zainstalowana
#ifdef USE_AUDIO
  #include <AudioFileSourceSD.h>
  #include <AudioGeneratorMP3.h>
  #include <AudioOutputI2S.h>
  static AudioFileSourceSD* _audioSrc = nullptr;
  static AudioGeneratorMP3* _audioGen = nullptr;
  static AudioOutputI2S*    _audioOut = nullptr;
#endif

#define I2S_BCLK   41
#define I2S_LRCK   43
#define I2S_DOUT   42
#define MUSIC_DIR  "/music"
#define MAX_TRACKS 64

static String _mp3_tracks[MAX_TRACKS];
static int    _mp3_count   = 0;
static int    _mp3_current = 0;
static bool   _mp3_playing = false;
static int    _mp3_volume  = 70;

static void _mp3_scan() {
    _mp3_count = 0;
    File dir = SD.open(MUSIC_DIR);
    if (!dir) { SD.mkdir(MUSIC_DIR); return; }
    while (true) {
        File f = dir.openNextFile();
        if (!f || _mp3_count >= MAX_TRACKS) break;
        String name = String(f.name());
        name.toUpperCase();
        if (name.endsWith(".MP3")) {
            String orig = String(f.name());
            _mp3_tracks[_mp3_count++] = orig;
        }
        f.close();
    }
    dir.close();
}

static void _mp3_stop() {
#ifdef USE_AUDIO
    if (_audioGen && _audioGen->isRunning()) _audioGen->stop();
    if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }
#endif
    _mp3_playing = false;
}

static void _mp3_play(int idx) {
    _mp3_stop();
    if (idx < 0 || idx >= _mp3_count) return;
    _mp3_current = idx;
#ifdef USE_AUDIO
    String path = String(MUSIC_DIR) + "/" + _mp3_tracks[idx];
    _audioSrc = new AudioFileSourceSD(path.c_str());
    if (!_audioOut) {
        _audioOut = new AudioOutputI2S();
        _audioOut->SetPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
        _audioOut->SetGain((float)_mp3_volume / 100.0f);
    }
    if (!_audioGen) _audioGen = new AudioGeneratorMP3();
    if (_audioGen->begin(_audioSrc, _audioOut)) _mp3_playing = true;
#else
    _mp3_playing = true; // symulacja bez biblioteki
#endif
}

static void _mp3_loop_audio() {
#ifdef USE_AUDIO
    if (_audioGen && _audioGen->isRunning()) {
        if (!_audioGen->loop()) {
            _audioGen->stop();
            _mp3_playing = false;
            _mp3_play((_mp3_current + 1) % _mp3_count);
        }
    }
#endif
}

static unsigned long _mp3_anim_t = 0;
static int _mp3_angle = 0;

static void _mp3_draw(bool full) {
    if (full) {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("ODTWARZACZ MP3", THEME_YELLOW);

        if (_mp3_count == 0) {
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(10, 60);
            M5Cardputer.Display.print("Brak plikow MP3 na SD.");
            M5Cardputer.Display.setCursor(10, 74);
            M5Cardputer.Display.print("Umies pliki w: /music/*.mp3");
            M5Cardputer.Display.setCursor(4, 212);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.print("BTN/ENTER = powrot");
            return;
        }
    }

    // Animacja winyla (tylko obracający się element)
    int cx = 120, cy = 80, r = 45;
    if (_mp3_playing && millis() - _mp3_anim_t > 60) {
        _mp3_angle = (_mp3_angle + 6) % 360;
        _mp3_anim_t = millis();
        // Wymaż poprzedni punkt
        M5Cardputer.Display.fillCircle(cx, cy, r - 1, THEME_PANEL);
        M5Cardputer.Display.fillCircle(cx, cy, 10, THEME_BG);
        float rad = (float)_mp3_angle * 3.14159f / 180.0f;
        int ex = cx + (int)((r - 8) * cosf(rad));
        int ey = cy + (int)((r - 8) * sinf(rad));
        M5Cardputer.Display.fillCircle(ex, ey, 3, THEME_YELLOW);
    }

    if (full && _mp3_count > 0) {
        // Ramka winyla
        M5Cardputer.Display.drawCircle(cx, cy, r, THEME_BORDER);
        M5Cardputer.Display.fillCircle(cx, cy, 10, THEME_BG);

        // Nazwa
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(4, 132);
        String tn = _mp3_tracks[_mp3_current];
        if (tn.length() > 32) tn = tn.substring(0, 29) + "...";
        M5Cardputer.Display.print(tn);

        // Pozycja
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 146);
        char pos[20]; snprintf(pos, sizeof(pos), "%d / %d", _mp3_current + 1, _mp3_count);
        M5Cardputer.Display.print(pos);

        // Głośność
        M5Cardputer.Display.setCursor(4, 160);
        M5Cardputer.Display.print("Glosnosc: ");
        M5Cardputer.Display.drawRect(70, 160, 100, 8, THEME_BORDER);
        M5Cardputer.Display.fillRect(71, 161, _mp3_volume, 6, THEME_YELLOW);
        char vb[6]; snprintf(vb, sizeof(vb), " %d%%", _mp3_volume);
        M5Cardputer.Display.print(vb);

        // Status
        M5Cardputer.Display.setTextColor(_mp3_playing ? THEME_GREEN : THEME_RED);
        M5Cardputer.Display.setCursor(4, 176);
        M5Cardputer.Display.print(_mp3_playing ? ">> ODTWARZAM" : "|| ZATRZYMANO");

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 200);
        M5Cardputer.Display.print("SP=play/stop A/D=utw +/-=glos Q=wr");
    }
}

void app_mp3_run() {
    _mp3_scan();
    if (_mp3_count > 0) _mp3_play(0);

    bool needFull = true;

    while (true) {
        _mp3_loop_audio();
        _mp3_draw(needFull);
        needFull = false;

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (char c : st.word) {
                bool redraw = true;
                switch (c) {
                    case ' ':
                        if (_mp3_playing) _mp3_stop();
                        else _mp3_play(_mp3_current);
                        break;
                    case 'd': case 'D':
                        _mp3_play((_mp3_current + 1) % _mp3_count);
                        break;
                    case 'a': case 'A':
                        _mp3_play((_mp3_current - 1 + _mp3_count) % _mp3_count);
                        break;
                    case '+':
                        _mp3_volume = min(100, _mp3_volume + 5);
#ifdef USE_AUDIO
                        if (_audioOut) _audioOut->SetGain((float)_mp3_volume / 100.0f);
#endif
                        break;
                    case '-':
                        _mp3_volume = max(0, _mp3_volume - 5);
#ifdef USE_AUDIO
                        if (_audioOut) _audioOut->SetGain((float)_mp3_volume / 100.0f);
#endif
                        break;
                    case 'q': case 'Q':
                        _mp3_stop(); return;
                    default:
                        redraw = false; break;
                }
                if (redraw) needFull = true;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) { _mp3_stop(); return; }
        delay(10);
    }
}
