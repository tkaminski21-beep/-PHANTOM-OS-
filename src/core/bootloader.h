#pragma once
// core/bootloader.h

#include "M5Cardputer.h"
#include "theme.h"

void bootloader_show() {
  M5Cardputer.Display.fillScreen(THEME_BG);

  const char* lines[] = {
    "CardPuter Advanced OS",
    "v1.0.0-PL",
    "",
    "> Inicjalizacja ESP32-S3...",
    "> Montowanie karty SD...",
    "> Uruchamianie WiFi...",
    "> Uruchamianie Bluetooth...",
    "> Ladowanie interfejsu...",
    "",
    "System gotowy.",
  };
  int n = 10;

  M5Cardputer.Display.setTextSize(1);

  // Tytuł
  M5Cardputer.Display.setTextColor(THEME_CYAN);
  M5Cardputer.Display.setCursor(30, 10);
  M5Cardputer.Display.println("CARDPUTER ADVANCED OS");
  M5Cardputer.Display.drawLine(0, 22, 240, 22, THEME_BORDER);

  int y = 30;
  for (int i = 0; i < n; i++) {
    delay(180);
    if (strlen(lines[i]) == 0) { y += 6; continue; }
    bool isTitle = (i < 2);
    bool isDone  = (i == n - 1);
    M5Cardputer.Display.setTextColor(
      isDone ? THEME_GREEN : isTitle ? TFT_WHITE : THEME_MUTED
    );
    M5Cardputer.Display.setCursor(6, y);
    M5Cardputer.Display.println(lines[i]);
    y += 14;

    // Pasek postępu
    int prog = (int)((float)(i + 1) / n * 230);
    M5Cardputer.Display.fillRect(4, 210, prog, 4, THEME_CYAN);
    M5Cardputer.Display.drawRect(4, 210, 232, 4, THEME_BORDER);
  }

  delay(600);
  M5Cardputer.Display.fillScreen(THEME_BG);
}
