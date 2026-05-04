#pragma once
// core/statusbar.h

#include "M5Cardputer.h"
#include "theme.h"
#include <WiFi.h>
#include <time.h>

static unsigned long _sb_last = 0;

void statusbar_draw() {
  // Użyj startWrite/endWrite żeby wszystko poszło w jednej transakcji SPI
  M5Cardputer.Display.startWrite();

  M5Cardputer.Display.fillRect(0, 0, 240, 20, THEME_PANEL);
  M5Cardputer.Display.drawLine(0, 20, 240, 20, THEME_BORDER);
  M5Cardputer.Display.setTextSize(1);

  // Czas (NTP lub RTC)
  struct tm ti;
  if (getLocalTime(&ti, 0)) {  // timeout=0 — nie blokuj jeśli brak NTP
    char buf[10];
    strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(4, 6);
    M5Cardputer.Display.print(buf);
  } else {
    // Pokaż uptime jeśli brak NTP
    unsigned long s = millis() / 1000;
    char buf[12];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", s/3600, (s%3600)/60, s%60);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 6);
    M5Cardputer.Display.print(buf);
  }

  // WiFi
  bool wifiOK = (WiFi.status() == WL_CONNECTED);
  M5Cardputer.Display.setTextColor(wifiOK ? THEME_GREEN : THEME_MUTED);
  M5Cardputer.Display.setCursor(170, 6);
  M5Cardputer.Display.print(wifiOK ? "W" : "w");

  // BT
  M5Cardputer.Display.setTextColor(THEME_CYAN);
  M5Cardputer.Display.setCursor(182, 6);
  M5Cardputer.Display.print("B");

  // Bateria — Cardputer ADV używa AXP/innego IC; getBatteryLevel() może zwracać -1
  int bat = M5Cardputer.Power.getBatteryLevel();
  if (bat >= 0 && bat <= 100) {
    uint32_t batColor = bat > 20 ? THEME_GREEN : THEME_RED;
    M5Cardputer.Display.setTextColor(batColor);
    M5Cardputer.Display.setCursor(195, 6);
    char batBuf[6];
    snprintf(batBuf, sizeof(batBuf), "%d%%", bat);
    M5Cardputer.Display.print(batBuf);
  } else {
    // Brak danych o baterii — pokaż "--"
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(195, 6);
    M5Cardputer.Display.print("--");
  }

  M5Cardputer.Display.endWrite();
}

void statusbar_update() {
  if (millis() - _sb_last > 1000) {
    _sb_last = millis();
    statusbar_draw();
  }
}
