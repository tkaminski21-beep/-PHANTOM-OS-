#pragma once
// app_pogoda.h — Pogoda (Open-Meteo) i Kryptowaluty (CoinGecko)
// Nie wymaga klucza API

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

extern uint16_t UI_BG, UI_FG, UI_PRI;
extern Preferences prefs;

// ─── POGODA — Open-Meteo (bez klucza) ─────────────────
void app_pogoda() {
    if (WiFi.status() != WL_CONNECTED) {
        ui_show_info("Polacz najpierw WiFi!", 0xF800);
        delay(2000); return;
    }

    // Domyślne współrzędne (Warszawa) — można zmienić w Preferences
    float lat = prefs.getFloat("loc_lat", 52.23f);
    float lon = prefs.getFloat("loc_lon", 21.01f);

    M5Cardputer.Display.fillScreen(UI_BG);
    M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(85, 10); M5Cardputer.Display.print("POGODA");
    M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

    M5Cardputer.Display.setCursor(8, 30); M5Cardputer.Display.print("Pobieranie...");

    HTTPClient http;
    char url[200];
    snprintf(url, sizeof(url),
        "http://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f"
        "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code&timezone=auto",
        lat, lon);
    http.begin(url);
    int code = http.GET();
    if (code != 200) {
        char msg[40]; snprintf(msg, sizeof(msg), "Blad HTTP %d", code);
        ui_show_info(msg, 0xF800); delay(2000); http.end(); return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getString());
    http.end();
    if (err) { ui_show_info("Blad JSON!", 0xF800); delay(2000); return; }

    float temp     = doc["current"]["temperature_2m"];
    int   humidity = doc["current"]["relative_humidity_2m"];
    float wind     = doc["current"]["wind_speed_10m"];
    int   wcode    = doc["current"]["weather_code"];

    const char* wdesc = "Pogoda";
    if (wcode == 0) wdesc = "Slonecznie";
    else if (wcode <= 3) wdesc = "Czesciowe zachmurzenie";
    else if (wcode <= 48) wdesc = "Mgla";
    else if (wcode <= 67) wdesc = "Deszcz";
    else if (wcode <= 77) wdesc = "Snieg";
    else if (wcode <= 82) wdesc = "Ulewny deszcz";
    else if (wcode <= 86) wdesc = "Burza sniegowa";
    else if (wcode <= 99) wdesc = "Burza";

    M5Cardputer.Display.fillScreen(UI_BG);
    M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(85, 10); M5Cardputer.Display.print("POGODA");
    M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

    char loc[24]; snprintf(loc, sizeof(loc), "%.2f, %.2f", lat, lon);
    M5Cardputer.Display.setTextColor(UI_PRI & 0xC618, UI_BG);
    M5Cardputer.Display.setCursor(8, 28); M5Cardputer.Display.print(loc);

    // Duża temperatura
    char tb[16]; snprintf(tb, sizeof(tb), "%.1f C", temp);
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setTextSize(4);
    M5Cardputer.Display.setCursor(60, 42);
    M5Cardputer.Display.print(tb);

    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(8, 80); M5Cardputer.Display.print(wdesc);
    char info[30];
    snprintf(info, sizeof(info), "Wilg: %d%%   Wiatr: %.1f km/h", humidity, wind);
    M5Cardputer.Display.setCursor(8, 95); M5Cardputer.Display.print(info);

    M5Cardputer.Display.setTextColor(UI_PRI & 0xC618, UI_BG);
    M5Cardputer.Display.setCursor(8, 122); M5Cardputer.Display.print("ENTER=odswiez  ESC=wyjdz");

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) {
                if (c=='\n'||c=='\r') { app_pogoda(); return; }
                if (c==27) return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}

// ─── KRYPTOWALUTY — CoinGecko (bez klucza) ────────────
void app_krypto() {
    if (WiFi.status() != WL_CONNECTED) {
        ui_show_info("Polacz najpierw WiFi!", 0xF800);
        delay(2000); return;
    }

    M5Cardputer.Display.fillScreen(UI_BG);
    M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(70, 10); M5Cardputer.Display.print("KRYPTOWALUTY");
    M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);
    M5Cardputer.Display.setCursor(8, 30); M5Cardputer.Display.print("Pobieranie...");

    HTTPClient http;
    http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,solana,dogecoin,cardano&vs_currencies=usd&include_24hr_change=true");
    int code = http.GET();
    if (code != 200) {
        char msg[32]; snprintf(msg, sizeof(msg), "HTTP %d", code);
        ui_show_info(msg, 0xF800); delay(2000); http.end(); return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getString());
    http.end();
    if (err) { ui_show_info("Blad JSON!", 0xF800); delay(2000); return; }

    struct Coin { const char* sym; const char* key; };
    Coin coins[] = {
        {"BTC", "bitcoin"},
        {"ETH", "ethereum"},
        {"SOL", "solana"},
        {"DOGE","dogecoin"},
        {"ADA", "cardano"},
    };

    M5Cardputer.Display.fillScreen(UI_BG);
    M5Cardputer.Display.drawRoundRect(5,5,230,124,5,UI_PRI);
    M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(70, 10); M5Cardputer.Display.print("KRYPTOWALUTY USD");
    M5Cardputer.Display.drawLine(5, 22, 235, 22, UI_PRI);

    for (int i = 0; i < 5; i++) {
        float price  = doc[coins[i].key]["usd"];
        float change = doc[coins[i].key]["usd_24h_change"];

        int y = 28 + i * 17;
        M5Cardputer.Display.setTextColor(UI_PRI, UI_BG);
        M5Cardputer.Display.setCursor(10, y);
        M5Cardputer.Display.print(coins[i].sym);

        char pb[16];
        if (price >= 1000) snprintf(pb, sizeof(pb), "$%.0f", price);
        else if (price >= 1) snprintf(pb, sizeof(pb), "$%.2f", price);
        else snprintf(pb, sizeof(pb), "$%.4f", price);
        M5Cardputer.Display.setCursor(50, y);
        M5Cardputer.Display.print(pb);

        // Zmiana 24h
        M5Cardputer.Display.setTextColor(change >= 0 ? 0x07E0 : 0xF800, UI_BG);
        M5Cardputer.Display.setCursor(160, y);
        char cb[12]; snprintf(cb, sizeof(cb), "%+.2f%%", change);
        M5Cardputer.Display.print(cb);
    }

    M5Cardputer.Display.setTextColor(UI_PRI & 0xC618, UI_BG);
    M5Cardputer.Display.setCursor(8, 122); M5Cardputer.Display.print("ENTER=odswiez  ESC=wyjdz");

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) {
                if (c=='\n'||c=='\r') { app_krypto(); return; }
                if (c==27) return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}

// ─── MENU ─────────────────────────────────────────────
void app_internet_menu() {
    while (true) {
        const char* opts[] = {"Pogoda","Kryptowaluty"};
        int sel = ui_select_list(opts, 2, "INTERNET", UI_PRI);
        if (sel < 0) return;
        if (sel == 0) app_pogoda();
        if (sel == 1) app_krypto();
    }
}
