#pragma once
// apps/app_settings.h — Ustawienia systemu

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <Preferences.h>
#include <WiFi.h>

extern Preferences prefs;

void app_settings_run() {
  while (true) {
    const char* opts[] = {
      "Jasnosc ekranu",
      "Dzwiek (beeper)",
      "Obrot ekranu",
      "Informacje o systemie",
      "Resetuj ustawienia",
      "Restart urzadzenia",
      "Powrot",
    };
    int sel = ui_select_list(opts, 7, "USTAWIENIA", THEME_GRAY);
    if (sel < 0 || sel == 6) return;

    // ── Jasność ──────────────────────────────────────
    if (sel == 0) {
      int br = prefs.getInt("brightness", 128);
      M5Cardputer.Display.fillScreen(THEME_BG);
      ui_draw_header("JASNOSC EKRANU", THEME_GRAY);
      M5Cardputer.Display.setTextColor(THEME_MUTED);
      M5Cardputer.Display.setCursor(4, 35);
      M5Cardputer.Display.print("A/D = zmniejsz/zwieksz");
      M5Cardputer.Display.setCursor(4, 48);
      M5Cardputer.Display.print("ENTER = zapisz");

      while (true) {
        // Rysuj suwak
        M5Cardputer.Display.fillRect(4, 70, 232, 20, THEME_BG);
        M5Cardputer.Display.drawRect(4, 76, 232, 8, THEME_BORDER);
        M5Cardputer.Display.fillRect(5, 77, (int)(230.0 * br / 255), 6, THEME_GRAY);
        char buf[10]; snprintf(buf, sizeof(buf), "%d%%", br * 100 / 255);
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(100, 92);
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setBrightness(br);

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
          auto st = M5Cardputer.Keyboard.keysState();
          for (auto c : st.word) {
            if (c == 'd' || c == 'D') br = min(255, br + 10);
            if (c == 'a' || c == 'A') br = max(10, br - 10);
            if (c == '\n') goto save_br;
          }
        }
        if (M5Cardputer.BtnA.wasPressed()) goto save_br;
        delay(10);
      }
      save_br:
      prefs.putInt("brightness", br);
      ui_show_info("Zapisano!", THEME_GREEN);
      delay(800);
    }

    // ── Dźwięk ──────────────────────────────────────
    else if (sel == 1) {
      bool beep = prefs.getBool("beep", true);
      beep = !beep;
      prefs.putBool("beep", beep);
      ui_show_info(beep ? "Dzwiek: WLACZONY" : "Dzwiek: WYLACZONY", THEME_YELLOW);
      if (beep) M5Cardputer.Speaker.tone(1000, 100);
      delay(1000);
    }

    // ── Obrót ───────────────────────────────────────
    else if (sel == 2) {
      int rot = prefs.getInt("rotation", 1);
      rot = (rot + 1) % 4;
      prefs.putInt("rotation", rot);
      M5Cardputer.Display.setRotation(rot);
      ui_show_info("Obrot ustawiony", THEME_CYAN);
      delay(1000);
    }

    // ── Info ─────────────────────────────────────────
    else if (sel == 3) {
      M5Cardputer.Display.fillScreen(THEME_BG);
      ui_draw_header("O SYSTEMIE", THEME_GRAY);
      M5Cardputer.Display.setTextSize(1);

      struct { const char* label; String value; } rows[] = {
        { "System:",   "CardPuter OS v1.0.0-PL" },
        { "Chipset:",  "ESP32-S3" },
        { "Taktowanie:", "240 MHz" },
        { "RAM:",      "8MB PSRAM" },
        { "Flash:",    "16MB" },
        { "WiFi IP:",  WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "Brak" },
        { "MAC:",      WiFi.macAddress() },
        { "Autor:",    "CardPuter OS PL" },
      };

      int y = 28;
      for (auto& r : rows) {
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, y);
        M5Cardputer.Display.print(r.label);
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(90, y);
        M5Cardputer.Display.print(r.value);
        y += 14;
      }
      M5Cardputer.Display.setTextColor(THEME_MUTED);
      M5Cardputer.Display.setCursor(4, 212);
      M5Cardputer.Display.print("Dowolny klawisz = powrot");
      ui_wait_key();
    }

    // ── Reset ustawień ───────────────────────────────
    else if (sel == 4) {
      prefs.clear();
      M5Cardputer.Display.setBrightness(128);
      M5Cardputer.Display.setRotation(1);
      ui_show_info("Ustawienia zresetowane!", THEME_YELLOW);
      delay(1500);
    }

    // ── Restart ──────────────────────────────────────
    else if (sel == 5) {
      M5Cardputer.Display.fillScreen(THEME_BG);
      M5Cardputer.Display.setTextColor(THEME_RED);
      M5Cardputer.Display.setCursor(60, 110);
      M5Cardputer.Display.print("Restartowanie...");
      delay(1500);
      esp_restart();
    }
  }
}
