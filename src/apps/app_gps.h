#pragma once
// app_gps.h — GPS Tracker dla CardputerOS3
// Piny UART: RX=15 TX=13 Baud=9600

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SD.h>
#include <time.h>

#ifndef GPS_RX_PIN
#define GPS_RX_PIN 15
#endif
#ifndef GPS_TX_PIN
#define GPS_TX_PIN 13
#endif
#ifndef GPS_BAUD
#define GPS_BAUD 9600
#endif

static TinyGPSPlus    _gps;
static HardwareSerial _gpsSerial(2);
static bool           _gps_started = false;

static void _gps_start() {
    if (!_gps_started) {
        _gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        _gps_started = true;
    }
}

static bool _gps_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ── TRACKER NA ŻYWO ───────────────────────────────────
void app_gps_tracker() {
    _gps_start();
    ui_draw_header("GPS TRACKER", THEME_GREEN);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("Szukam sygnalu GPS...");
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = wyjdz");

    unsigned long lastUpdate = 0, startTime = millis();
    while (!_gps_check_esc()) {
        while (_gpsSerial.available()) _gps.encode(_gpsSerial.read());
        if (millis() - lastUpdate > 1000) {
            lastUpdate = millis();
            ui_draw_header("GPS TRACKER", THEME_GREEN);
            bool fix = _gps.location.isValid() && _gps.location.age() < 2000;
            M5Cardputer.Display.setTextColor(fix ? THEME_GREEN : THEME_ORANGE);
            M5Cardputer.Display.setCursor(4, 22);
            M5Cardputer.Display.print(fix ? "FIX OK" : "Szukam...");
            char sat[20]; snprintf(sat, sizeof(sat), "Sat: %d", (int)_gps.satellites.value());
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(100, 22); M5Cardputer.Display.print(sat);
            if (fix) {
                char buf[40];
                M5Cardputer.Display.setTextColor(THEME_TEXT);
                snprintf(buf, sizeof(buf), "Lat:  %.6f", _gps.location.lat());
                M5Cardputer.Display.setCursor(4, 36); M5Cardputer.Display.print(buf);
                snprintf(buf, sizeof(buf), "Lng:  %.6f", _gps.location.lng());
                M5Cardputer.Display.setCursor(4, 48); M5Cardputer.Display.print(buf);
                snprintf(buf, sizeof(buf), "Alt:  %.1f m", _gps.altitude.meters());
                M5Cardputer.Display.setCursor(4, 60); M5Cardputer.Display.print(buf);
                snprintf(buf, sizeof(buf), "Speed:%.1f km/h", _gps.speed.kmph());
                M5Cardputer.Display.setCursor(4, 72); M5Cardputer.Display.print(buf);
                if (_gps.time.isValid()) {
                    M5Cardputer.Display.setTextColor(THEME_CYAN);
                    snprintf(buf, sizeof(buf), "UTC:  %02d:%02d:%02d", _gps.time.hour(), _gps.time.minute(), _gps.time.second());
                    M5Cardputer.Display.setCursor(4, 86); M5Cardputer.Display.print(buf);
                }
            } else {
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, 44);
                char buf[32]; snprintf(buf, sizeof(buf), "Oczekuje: %lus", (millis()-startTime)/1000);
                M5Cardputer.Display.print(buf);
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = wyjdz");
        }
        delay(10);
    }
}

// ── LOG TRASY ─────────────────────────────────────────
void app_gps_log() {
    _gps_start();
    ui_draw_header("GPS LOG TRASY", THEME_GREEN);
    if (!SD.exists("/")) { ui_show_info("Brak karty SD!", THEME_RED); delay(2000); return; }
    char fname[32]; snprintf(fname, sizeof(fname), "/gps_%lu.csv", millis()/1000);
    File f = SD.open(fname, FILE_WRITE);
    if (!f) { ui_show_info("Blad pliku SD!", THEME_RED); delay(2000); return; }
    f.println("lat,lng,alt,speed,time,date,sats");

    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("Zapisuje trase...");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 40); M5Cardputer.Display.print(fname);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop i zapisz");

    int points = 0;
    unsigned long lastLog = 0;
    while (!_gps_check_esc()) {
        while (_gpsSerial.available()) _gps.encode(_gpsSerial.read());
        if (millis()-lastLog > 2000 && _gps.location.isValid()) {
            lastLog = millis();
            char row[128];
            snprintf(row, sizeof(row), "%.6f,%.6f,%.1f,%.1f,%02d:%02d:%02d,%04d-%02d-%02d,%d",
                     _gps.location.lat(), _gps.location.lng(),
                     _gps.altitude.meters(), _gps.speed.kmph(),
                     _gps.time.hour(), _gps.time.minute(), _gps.time.second(),
                     _gps.date.year(), _gps.date.month(), _gps.date.day(),
                     (int)_gps.satellites.value());
            f.println(row);
            points++;
            M5Cardputer.Display.fillRect(0, 56, 240, 50, THEME_BG);
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setCursor(4, 58);
            char buf[32]; snprintf(buf, sizeof(buf), "Punktow: %d", points);
            M5Cardputer.Display.print(buf);
        }
        delay(10);
    }
    f.close();
    char msg[40]; snprintf(msg, sizeof(msg), "Zapisano %d pkt", points);
    ui_show_info(msg, THEME_GREEN);
    delay(2000);
}

// ── SYNC CZASU Z GPS ──────────────────────────────────
void app_gps_sync_time() {
    _gps_start();
    ui_draw_header("GPS SYNC CZASU", THEME_GREEN);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 26); M5Cardputer.Display.print("Szukam sygnalu GPS...");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = anuluj");

    unsigned long startWait = millis();
    while (!_gps_check_esc()) {
        while (_gpsSerial.available()) _gps.encode(_gpsSerial.read());
        if (_gps.time.isValid() && _gps.date.isValid() && _gps.time.age() < 2000) {
            struct tm t = {0};
            t.tm_year = _gps.date.year()-1900; t.tm_mon = _gps.date.month()-1;
            t.tm_mday = _gps.date.day(); t.tm_hour = _gps.time.hour();
            t.tm_min  = _gps.time.minute(); t.tm_sec = _gps.time.second();
            time_t epoch = mktime(&t);
            timeval tv = {epoch, 0}; settimeofday(&tv, nullptr);
            char buf[40];
            snprintf(buf, sizeof(buf), "Data: %04d-%02d-%02d", _gps.date.year(), _gps.date.month(), _gps.date.day());
            ui_show_info("Zsynchronizowano!", THEME_GREEN);
            delay(2000); return;
        }
        if (millis()-startWait > 60000) { ui_show_info("Timeout - brak GPS", THEME_RED); delay(2000); return; }
        M5Cardputer.Display.fillRect(0, 40, 240, 16, THEME_BG);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 42);
        char buf[32]; snprintf(buf, sizeof(buf), "Czekam: %lus", (millis()-startWait)/1000);
        M5Cardputer.Display.print(buf);
        delay(500);
    }
}

// ── GŁÓWNE MENU ───────────────────────────────────────
void app_gps_menu() {
    while (true) {
        const char* opts[] = {"Tracker na zywo","Log trasy na SD","Sync czasu z GPS"};
        int sel = ui_select_list(opts, 3, "GPS TRACKER", THEME_GREEN);
        if (sel < 0) return;
        if (sel == 0) app_gps_tracker();
        if (sel == 1) app_gps_log();
        if (sel == 2) app_gps_sync_time();
    }
}
