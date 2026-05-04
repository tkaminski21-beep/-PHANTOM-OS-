#pragma once
// apps/app_wifi.h — WiFi Manager

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>

#define MAX_NETWORKS 16
#define WIFI_PREF_NS "wifi"

struct WifiNetwork {
  String ssid;
  int    rssi;
  bool   encrypted;
};

static WifiNetwork wifiNets[MAX_NETWORKS];
static int         wifiNetCount = 0;

extern Preferences prefs;

// ─── Sygnał WiFi → słupki ──────────────────────────────
const char* wifi_signal_str(int rssi) {
  if (rssi >= -50) return "████";
  if (rssi >= -65) return "███ ";
  if (rssi >= -75) return "██  ";
  if (rssi >= -85) return "█   ";
  return "    ";
}

// ─── Skanuj sieci ──────────────────────────────────────
void wifi_scan() {
  M5Cardputer.Display.fillScreen(THEME_BG);
  ui_draw_header("WI-FI", THEME_CYAN);
  M5Cardputer.Display.setTextColor(THEME_MUTED);
  M5Cardputer.Display.setCursor(60, 110);
  M5Cardputer.Display.print("Skanowanie...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();
  wifiNetCount = 0;
  for (int i = 0; i < n && i < MAX_NETWORKS; i++) {
    wifiNets[i].ssid      = WiFi.SSID(i);
    wifiNets[i].rssi      = WiFi.RSSI(i);
    wifiNets[i].encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    wifiNetCount++;
  }
}

// ─── Połącz z siecią ──────────────────────────────────
bool wifi_connect(const String& ssid, const String& password) {
  WiFi.begin(ssid.c_str(), password.c_str());

  M5Cardputer.Display.fillRect(0, 80, 240, 40, THEME_BG);
  M5Cardputer.Display.setTextColor(THEME_YELLOW);
  M5Cardputer.Display.setCursor(4, 85);
  M5Cardputer.Display.print("Laczenie z: ");
  M5Cardputer.Display.print(ssid);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    tries++;
    M5Cardputer.Display.fillRect(4 + tries * 10, 98, 8, 6, THEME_CYAN);
  }

  if (WiFi.status() == WL_CONNECTED) {
    // Zapis danych do Preferences
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);

    // Sync NTP
    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");

    ui_show_info("Polaczono!", THEME_GREEN);
    delay(1200);
    return true;
  }

  ui_show_error("Blad polaczenia!");
  delay(1500);
  return false;
}

// ─── Główna aplikacja WiFi ─────────────────────────────
void app_wifi_run() {
  while (true) {
    const char* opts[] = {
      "Skanuj sieci",
      "Zapisana siec",
      "Rozlacz",
      "Status",
      "Powrot"
    };
    int sel = ui_select_list(opts, 5, "WI-FI MANAGER", THEME_CYAN);
    if (sel < 0 || sel == 4) return;

    // ── Skanuj ──────────────────────────────────────────
    if (sel == 0) {
      wifi_scan();

      if (wifiNetCount == 0) {
        ui_show_error("Brak sieci!");
        delay(1500);
        continue;
      }

      // Pokaż listę sieci
      const char* names[MAX_NETWORKS];
      char labels[MAX_NETWORKS][48];
      for (int i = 0; i < wifiNetCount; i++) {
        snprintf(labels[i], sizeof(labels[i]), "%s%s %ddBm",
                 wifiNets[i].encrypted ? "[*] " : "[ ] ",
                 wifiNets[i].ssid.c_str(),
                 wifiNets[i].rssi);
        names[i] = labels[i];
      }

      int pick = ui_select_list(names, wifiNetCount, "WYBIERZ SIEC", THEME_CYAN);
      if (pick < 0) continue;

      String password = "";
      if (wifiNets[pick].encrypted) {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("HASLO WI-FI", THEME_CYAN);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 28);
        M5Cardputer.Display.print("Siec: ");
        M5Cardputer.Display.print(wifiNets[pick].ssid);
        password = ui_input_string("Haslo:", 4, 50, 32);
        if (password.length() == 0) continue;
      }

      wifi_connect(wifiNets[pick].ssid, password);
    }

    // ── Zapisana sieć ────────────────────────────────────
    else if (sel == 1) {
      String ssid = prefs.getString("ssid", "");
      String pass = prefs.getString("pass", "");
      if (ssid.length() == 0) {
        ui_show_error("Brak zapisanej sieci");
        delay(1500);
        continue;
      }
      M5Cardputer.Display.fillScreen(THEME_BG);
      ui_draw_header("ZAPISANA SIEC", THEME_CYAN);
      M5Cardputer.Display.setTextColor(TFT_WHITE);
      M5Cardputer.Display.setCursor(4, 40);
      M5Cardputer.Display.print("SSID: "); M5Cardputer.Display.print(ssid);
      M5Cardputer.Display.setCursor(4, 56);
      M5Cardputer.Display.print("Haslo: ****");
      delay(800);
      wifi_connect(ssid, pass);
    }

    // ── Rozłącz ──────────────────────────────────────────
    else if (sel == 2) {
      WiFi.disconnect();
      ui_show_info("Rozlaczono.", THEME_YELLOW);
      delay(1200);
    }

    // ── Status ───────────────────────────────────────────
    else if (sel == 3) {
      M5Cardputer.Display.fillScreen(THEME_BG);
      ui_draw_header("STATUS WI-FI", THEME_CYAN);

      bool conn = (WiFi.status() == WL_CONNECTED);
      M5Cardputer.Display.setTextColor(conn ? THEME_GREEN : THEME_RED);
      M5Cardputer.Display.setCursor(4, 30);
      M5Cardputer.Display.print(conn ? "Status: POLACZONY" : "Status: ROZLACZONY");

      if (conn) {
        char buf[64];
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(4, 46);
        snprintf(buf, sizeof(buf), "SSID:  %s", WiFi.SSID().c_str());
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setCursor(4, 58);
        snprintf(buf, sizeof(buf), "IP:    %s", WiFi.localIP().toString().c_str());
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setCursor(4, 70);
        snprintf(buf, sizeof(buf), "GW:    %s", WiFi.gatewayIP().toString().c_str());
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setCursor(4, 82);
        snprintf(buf, sizeof(buf), "DNS:   %s", WiFi.dnsIP().toString().c_str());
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setCursor(4, 94);
        snprintf(buf, sizeof(buf), "RSSI:  %d dBm", WiFi.RSSI());
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setCursor(4, 106);
        snprintf(buf, sizeof(buf), "MAC:   %s", WiFi.macAddress().c_str());
        M5Cardputer.Display.print(buf);
      }

      M5Cardputer.Display.setTextColor(THEME_MUTED);
      M5Cardputer.Display.setCursor(4, 212);
      M5Cardputer.Display.print("Dowolny klawisz = powrot");
      ui_wait_key();
    }
  }
}
