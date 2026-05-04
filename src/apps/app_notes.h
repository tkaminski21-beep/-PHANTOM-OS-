#pragma once
// apps/app_notes.h — Notatnik z kartą SD

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <SD.h>

#define NOTES_DIR "/notes"
#define MAX_NOTES 32

// ─── Lista notatek z karty SD ────────────────────────────
void notes_list_files(String files[], int& count) {
  count = 0;
  File dir = SD.open(NOTES_DIR);
  if (!dir) { SD.mkdir(NOTES_DIR); return; }
  while (true) {
    File f = dir.openNextFile();
    if (!f || count >= MAX_NOTES) break;
    if (!f.isDirectory()) {
      files[count++] = String(f.name());
    }
    f.close();
  }
  dir.close();
}

// ─── Wyświetl notatke na ekranie ─────────────────────────
void notes_view(const String& filename) {
  ui_draw_header(("NOTATKA: " + filename).c_str(), THEME_GREEN);

  String path = String(NOTES_DIR) + "/" + filename;
  File f = SD.open(path);
  if (!f) {
    ui_show_error("Nie mozna otworzyc!");
    delay(1500);
    return;
  }

  int y = 26, lineH = 12;
  M5Cardputer.Display.setTextColor(TFT_WHITE);
  M5Cardputer.Display.setTextSize(1);

  while (f.available() && y < 208) {
    String line = f.readStringUntil('\n');
    M5Cardputer.Display.setCursor(4, y);
    M5Cardputer.Display.print(line.substring(0, 38));
    y += lineH;
  }
  f.close();

  M5Cardputer.Display.setTextColor(THEME_MUTED);
  M5Cardputer.Display.setCursor(4, 212);
  M5Cardputer.Display.print("Dowolny klawisz = powrot");
  ui_wait_key();
}

// ─── Nowa notatka ───────────────────────────────────────
void notes_new() {
  M5Cardputer.Display.fillScreen(THEME_BG);
  ui_draw_header("NOWA NOTATKA", THEME_GREEN);

  M5Cardputer.Display.setTextColor(THEME_MUTED);
  M5Cardputer.Display.setCursor(4, 28);
  M5Cardputer.Display.print("Nazwa pliku (.txt):");
  String name = ui_input_string("", 4, 40, 20);
  if (name.length() == 0) return;
  if (!name.endsWith(".txt")) name += ".txt";

  M5Cardputer.Display.fillRect(0, 60, 240, 145, THEME_BG);
  M5Cardputer.Display.setTextColor(THEME_MUTED);
  M5Cardputer.Display.setCursor(4, 60);
  M5Cardputer.Display.print("Tresc (ENTER=nowa linia, FN+S=zapis):");

  String path = String(NOTES_DIR) + "/" + name;
  File f = SD.open(path, FILE_WRITE);
  if (!f) { ui_show_error("Blad zapisu!"); delay(1500); return; }

  String line = "";
  int y = 78;
  M5Cardputer.Display.setTextColor(TFT_WHITE);
  M5Cardputer.Display.setCursor(4, y);

  while (true) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      auto s = M5Cardputer.Keyboard.keysState();

      // FN+S = zapis i wyjście
      if (s.fn && _word_eq(s.word, "s")) {
        f.println(line);
        f.close();
        ui_show_info("Zapisano!", THEME_GREEN);
        delay(1200);
        return;
      }

      if (s.del && line.length() > 0) {
        line.remove(line.length() - 1);
      } else if (!s.word.empty()) {
        char c = s.word[0];
        if (c == '\n' || c == '\r') {
          f.println(line);
          line = "";
          y += 12;
          if (y > 200) { f.close(); ui_show_info("Zapisano!", THEME_GREEN); delay(1200); return; }
        } else {
          line.concat(c);
        }
      }

      // Przerysuj linię
      M5Cardputer.Display.fillRect(0, y, 240, 12, THEME_BG);
      M5Cardputer.Display.setTextColor(TFT_WHITE);
      M5Cardputer.Display.setCursor(4, y);
      M5Cardputer.Display.print(line.substring(0, 38));
      M5Cardputer.Display.print("_");
    }
    if (M5Cardputer.BtnA.wasPressed()) {
      f.println(line);
      f.close();
      ui_show_info("Zapisano!", THEME_GREEN);
      delay(1200);
      return;
    }
    delay(10);
  }
}

// ─── Usuń notatkę ───────────────────────────────────────
void notes_delete(const String& filename) {
  String path = String(NOTES_DIR) + "/" + filename;
  SD.remove(path);
}

// ─── Główna funkcja notatnika ────────────────────────────
void app_notes_run() {
  while (true) {
    // Opcje menu
    const char* opts[] = { "Lista notatek", "Nowa notatka", "Powrot" };
    int sel = ui_select_list(opts, 3, "NOTATNIK", THEME_GREEN);

    if (sel == 2 || sel < 0) return;

    if (sel == 1) {
      notes_new();
      continue;
    }

    // Lista notatek
    String files[MAX_NOTES];
    int count = 0;
    notes_list_files(files, count);

    if (count == 0) {
      ui_draw_header("NOTATNIK", THEME_GREEN);
      M5Cardputer.Display.setTextColor(THEME_MUTED);
      M5Cardputer.Display.setCursor(40, 110);
      M5Cardputer.Display.print("Brak notatek na SD");
      delay(1500);
      continue;
    }

    // Konwertuj na tablicę C-stringów
    const char* fnames[MAX_NOTES];
    for (int i = 0; i < count; i++) fnames[i] = files[i].c_str();

    int pick = ui_select_list(fnames, count, "WYBIERZ NOTATKE", THEME_GREEN);
    if (pick < 0) continue;

    // Akcja na notatce
    const char* actions[] = { "Czytaj", "Usun", "Wstecz" };
    int act = ui_select_list(actions, 3, files[pick].c_str(), THEME_GREEN);
    if (act == 0) notes_view(files[pick]);
    else if (act == 1) {
      notes_delete(files[pick]);
      ui_show_info("Usunieto!", THEME_ORANGE);
      delay(1000);
    }
  }
}
