#pragma once
// apps/app_launcher.h — M5Launcher-compatible App Manager

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>

extern Preferences prefs;

#define APPS_DIR "/apps"
#define MAX_APPS 32

// ─── Repozytoria aplikacji (M5Launcher format) ──────────
struct AppRepo {
  const char* name;
  const char* url;  // JSON manifest
};

// Możesz dodać własne repozytorium!
static AppRepo REPOS[] = {
  { "M5Cardputer Apps", "https://raw.githubusercontent.com/m5stack/M5Launcher/main/manifest.json" },
  { "Community Store",  "https://your-own-repo.example.com/apps.json" },
};
static int REPO_COUNT = 1; // zmień na 2 gdy dodasz własne

struct AppEntry {
  String name;
  String filename;
  String url;
  String size;
  bool   local;   // true = na SD, false = do pobrania
};

static AppEntry apps[MAX_APPS];
static int      appCount = 0;

// ─── Skanuj aplikacje na SD ─────────────────────────────
void launcher_scan_sd() {
  // Skanuj APPS_DIR i /
  const char* dirs[] = { APPS_DIR, "/" };
  for (auto d : dirs) {
    File dir = SD.open(d);
    if (!dir) continue;
    while (appCount < MAX_APPS) {
      File f = dir.openNextFile();
      if (!f) break;
      String fname = f.name();
      if (fname.endsWith(".bin") || fname.endsWith(".BIN")) {
        apps[appCount].name     = fname.substring(0, fname.lastIndexOf('.'));
        apps[appCount].filename = fname;
        apps[appCount].url      = "";
        apps[appCount].size     = String((int)(f.size() / 1024)) + "KB";
        apps[appCount].local    = true;
        appCount++;
      }
      f.close();
    }
    dir.close();
  }
}

// ─── Pobierz manifest JSON ze sklepu ───────────────────
bool launcher_fetch_store(const char* url, int repoIdx) {
#ifndef SKIP_WIFI_FEATURES
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(url);
  http.setTimeout(8000);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();

  // Prosty parser JSON (szuka "name" i "url")
  int pos = 0;
  while (appCount < MAX_APPS) {
    int nameIdx = payload.indexOf("\"name\":", pos);
    if (nameIdx < 0) break;
    int urlIdx  = payload.indexOf("\"url\":", nameIdx);
    int sizeIdx = payload.indexOf("\"size\":", nameIdx);
    if (urlIdx < 0) break;

    int ns = payload.indexOf("\"", nameIdx + 7) + 1;
    int ne = payload.indexOf("\"", ns);
    int us = payload.indexOf("\"", urlIdx + 6) + 1;
    int ue = payload.indexOf("\"", us);

    apps[appCount].name     = payload.substring(ns, ne);
    apps[appCount].filename = apps[appCount].name + ".bin";
    apps[appCount].url      = payload.substring(us, ue);
    if (sizeIdx > 0) {
      int ss = payload.indexOf("\"", sizeIdx + 7) + 1;
      int se = payload.indexOf("\"", ss);
      apps[appCount].size = payload.substring(ss, se);
    }
    apps[appCount].local    = false;
    appCount++;
    pos = ue;
  }
  return true;
#else
  return false;
#endif
}

// ─── Pobierz plik .bin i zapisz na SD ──────────────────
bool launcher_download_app(const AppEntry& app, int idx) {
  if (WiFi.status() != WL_CONNECTED) {
    ui_show_error("Brak WiFi!"); delay(1500); return false;
  }

  M5Cardputer.Display.fillScreen(THEME_BG);
  ui_draw_header("POBIERANIE", THEME_GREEN);
  M5Cardputer.Display.setTextColor(TFT_WHITE);
  M5Cardputer.Display.setCursor(4, 35);
  M5Cardputer.Display.print("Pobieram: ");
  M5Cardputer.Display.print(app.name);

  SD.mkdir(APPS_DIR);
  String savePath = String(APPS_DIR) + "/" + app.filename;

  HTTPClient http;
  http.begin(app.url.c_str());
  http.setTimeout(30000);
  int code = http.GET();

  if (code != 200) {
    http.end();
    ui_show_error("HTTP Error");
    delay(1500);
    return false;
  }

  int total = http.getSize();
  WiFiClient* stream = http.getStreamPtr();
  File f = SD.open(savePath, FILE_WRITE);
  if (!f) {
    http.end();
    ui_show_error("Blad zapisu SD");
    delay(1500);
    return false;
  }

  uint8_t buf[1024];
  int downloaded = 0;
  while (http.connected() && (downloaded < total || total == -1)) {
    int avail = stream->available();
    if (avail > 0) {
      int rd = stream->readBytes(buf, min(avail, (int)sizeof(buf)));
      f.write(buf, rd);
      downloaded += rd;
      int pct = total > 0 ? (downloaded * 100 / total) : 0;
      ui_progress(pct, 60, THEME_GREEN);
      M5Cardputer.Display.setCursor(100, 50);
      M5Cardputer.Display.setTextColor(THEME_MUTED);
      char prog[20]; snprintf(prog, sizeof(prog), "%d/%dKB", downloaded/1024, total/1024);
      M5Cardputer.Display.print(prog);
    }
    delay(1);
  }

  f.close();
  http.end();
  apps[idx].local = true;
  ui_show_info("Pobrano! Zapisano na SD.", THEME_GREEN);
  delay(1500);
  return true;
}

// ─── URUCHOM aplikację przez OTA load ──────────────────
// M5Launcher uruchamia .bin przez Arduino Update API
void launcher_boot_app(const String& path) {
  M5Cardputer.Display.fillScreen(THEME_BG);
  ui_draw_header("URUCHAMIANIE", THEME_GREEN);
  M5Cardputer.Display.setTextColor(TFT_WHITE);
  M5Cardputer.Display.setCursor(4, 40);
  M5Cardputer.Display.print("Ladowanie: "); M5Cardputer.Display.print(path);

  File f = SD.open(path);
  if (!f) { ui_show_error("Nie znaleziono pliku!"); delay(1500); return; }

  size_t fileSize = f.size();
  M5Cardputer.Display.setCursor(4, 56);
  M5Cardputer.Display.print("Rozmiar: "); M5Cardputer.Display.print(fileSize / 1024); M5Cardputer.Display.print("KB");

  // Wgrywanie przez Arduino Update API (nie wymaga esp_ota_ops / app_update)
  if (!Update.begin(fileSize, U_FLASH)) {
    ui_show_error("Update.begin failed!"); f.close(); delay(1500); return;
  }

  uint8_t buf[1024];
  int written = 0;
  while (f.available()) {
    int rd = f.read(buf, sizeof(buf));
    Update.write(buf, rd);
    written += rd;
    ui_progress(written * 100 / fileSize, 70, THEME_GREEN);
  }
  f.close();

  if (!Update.end(true)) {
    char eb[48]; snprintf(eb, sizeof(eb), "OTA blad: %s", Update.errorString());
    ui_show_error(eb); delay(1500); return;
  }

  M5Cardputer.Display.setTextColor(THEME_GREEN);
  M5Cardputer.Display.setCursor(4, 90);
  M5Cardputer.Display.print("Uruchamianie... restart!");
  delay(1500);
  esp_restart();
}

// ─── Główna aplikacja launcher ──────────────────────────
void app_launcher_run() {
  while (true) {
    const char* opts[] = {
      "Aplikacje z karty SD",
      "Sklep (WiFi)",
      "Powrot do OS",
    };
    int sel = ui_select_list(opts, 3, "LAUNCHER", THEME_GREEN);
    if (sel < 0 || sel == 2) return;

    // ── Karta SD ──────────────────────────────────────
    if (sel == 0) {
      appCount = 0;
      launcher_scan_sd();

      if (appCount == 0) {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("APLIKACJE SD", THEME_GREEN);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 50);
        M5Cardputer.Display.print("Brak .bin na karcie SD.");
        M5Cardputer.Display.setCursor(4, 64);
        M5Cardputer.Display.print("Skopiuj pliki do:");
        M5Cardputer.Display.setCursor(4, 78);
        M5Cardputer.Display.setTextColor(THEME_CYAN);
        M5Cardputer.Display.print("/apps/*.bin");
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("Dowolny klawisz = powrot");
        ui_wait_key();
        continue;
      }

      const char* anames[MAX_APPS];
      char alabels[MAX_APPS][40];
      for (int i = 0; i < appCount; i++) {
        snprintf(alabels[i], sizeof(alabels[i]), "%s  [%s]",
                 apps[i].name.c_str(), apps[i].size.c_str());
        anames[i] = alabels[i];
      }

      int pick = ui_select_list(anames, appCount, "WYBIERZ APP", THEME_GREEN);
      if (pick < 0) continue;

      const char* actions[] = { "Uruchom", "Usun z SD", "Wstecz" };
      int act = ui_select_list(actions, 3, apps[pick].name.c_str(), THEME_GREEN);
      if (act == 0) {
        String path = String(APPS_DIR) + "/" + apps[pick].filename;
        launcher_boot_app(path);
      } else if (act == 1) {
        String path = String(APPS_DIR) + "/" + apps[pick].filename;
        SD.remove(path);
        ui_show_info("Usunieto!", THEME_YELLOW);
        delay(1000);
      }
    }

    // ── Sklep WiFi ──────────────────────────────────────
    else if (sel == 1) {
      if (WiFi.status() != WL_CONNECTED) {
        ui_show_error("Najpierw polacz WiFi!");
        delay(1500);
        continue;
      }

      // Wybierz repozytorium
      const char* rnames[4];
      for (int i = 0; i < REPO_COUNT; i++) rnames[i] = REPOS[i].name;
      int repoSel = ui_select_list(rnames, REPO_COUNT, "REPOZYTORIUM", THEME_GREEN);
      if (repoSel < 0) continue;

      // Pobierz manifest
      appCount = 0;
      M5Cardputer.Display.fillScreen(THEME_BG);
      ui_draw_header("POBIERANIE LISTY", THEME_GREEN);
      M5Cardputer.Display.setTextColor(THEME_MUTED);
      M5Cardputer.Display.setCursor(40, 110);
      M5Cardputer.Display.print("Lacze z repozytorium...");

      bool ok = launcher_fetch_store(REPOS[repoSel].url, repoSel);
      if (!ok || appCount == 0) {
        ui_show_error("Blad pobierania listy");
        delay(1500);
        continue;
      }

      const char* snames[MAX_APPS];
      char slabels[MAX_APPS][48];
      for (int i = 0; i < appCount; i++) {
        snprintf(slabels[i], sizeof(slabels[i]), "%s %s %s",
                 apps[i].name.c_str(),
                 apps[i].size.c_str(),
                 apps[i].local ? "[SD]" : "[GET]");
        snames[i] = slabels[i];
      }

      int pick = ui_select_list(snames, appCount, "SKLEP", THEME_GREEN);
      if (pick < 0) continue;

      if (apps[pick].local) {
        const char* acts[] = { "Uruchom", "Wstecz" };
        int act = ui_select_list(acts, 2, apps[pick].name.c_str(), THEME_GREEN);
        if (act == 0) {
          String path = String(APPS_DIR) + "/" + apps[pick].filename;
          launcher_boot_app(path);
        }
      } else {
        const char* acts[] = { "Pobierz na SD", "Wstecz" };
        int act = ui_select_list(acts, 2, apps[pick].name.c_str(), THEME_GREEN);
        if (act == 0) launcher_download_app(apps[pick], pick);
      }
    }
  }
}
