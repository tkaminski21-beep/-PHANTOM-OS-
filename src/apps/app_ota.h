#pragma once
// app_ota.h — OTA Update firmware przez WiFi
// Dwa tryby:
//   1) Web Update — Cardputer hostuje stronę gdzie można wgrać .bin
//   2) HTTP Update — pobiera firmware z URL

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <HTTPClient.h>
#include <Preferences.h>

extern Preferences prefs;

static bool _ota_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ─── WEB UPDATE — strona z formularzem upload ─────────
void app_ota_web() {
    if (WiFi.status() != WL_CONNECTED) {
        ui_show_info("Polacz najpierw WiFi!", THEME_RED);
        delay(2000); return;
    }

    WebServer server(80);
    static bool _ota_uploading = false;
    static size_t _ota_progress = 0;
    static size_t _ota_total = 0;

    String htmlPage = R"raw(
<!DOCTYPE html><html><head><title>AOS OTA Update</title>
<style>
body{font-family:Arial;background:#1a1a2e;color:#fff;text-align:center;padding:30px;}
.box{background:#16213e;padding:30px;border-radius:10px;max-width:400px;margin:auto;}
input[type=file]{width:100%;padding:10px;margin:10px 0;background:#0f3460;color:#fff;border:none;border-radius:5px;}
button{background:#e94560;color:#fff;border:none;padding:12px 30px;border-radius:5px;cursor:pointer;font-size:16px;}
.bar{background:#0f3460;border-radius:5px;height:20px;margin-top:20px;}
.fill{background:#e94560;height:20px;border-radius:5px;width:0%;}
</style></head><body><div class="box">
<h2>AOS OTA Update</h2>
<p>Wybierz plik firmware.bin do wgrania:</p>
<form method="POST" action="/update" enctype="multipart/form-data" id="uf">
<input type="file" name="firmware" accept=".bin" required>
<button type="submit">Wgraj firmware</button></form>
<div class="bar"><div class="fill" id="p"></div></div>
<div id="s"></div></div></body></html>
)raw";

    server.on("/", [&]() { server.send(200, "text/html", htmlPage); });
    server.on("/update", HTTP_POST, [&]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", Update.hasError() ? "FAIL" : "OK - restart...");
        delay(1000);
        ESP.restart();
    }, [&]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            _ota_uploading = true;
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                Update.printError(Serial);
            _ota_progress = upload.totalSize;
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) _ota_total = upload.totalSize;
            else Update.printError(Serial);
            _ota_uploading = false;
        }
    });

    server.begin();

    bool redraw = true;
    while (!_ota_check_esc()) {
        server.handleClient();

        if (redraw || _ota_uploading) {
            ui_draw_header("OTA - WEB UPDATE", THEME_GREEN);
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setCursor(4, 24); M5Cardputer.Display.print("Serwer aktywny:");
            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setCursor(4, 40);
            M5Cardputer.Display.print("http://" + WiFi.localIP().toString());
            M5Cardputer.Display.setTextColor(THEME_TEXT);
            M5Cardputer.Display.setCursor(4, 60);
            M5Cardputer.Display.print("Otworz w przegladarce");
            M5Cardputer.Display.setCursor(4, 72);
            M5Cardputer.Display.print("i wgraj firmware.bin");

            if (_ota_uploading) {
                M5Cardputer.Display.fillRect(4, 90, 232, 18, THEME_BG);
                M5Cardputer.Display.drawRect(4, 90, 232, 16, THEME_GREEN);
                int pct = _ota_total > 0 ? (_ota_progress*100/_ota_total) : 0;
                M5Cardputer.Display.fillRect(6, 92, 228 * _ota_progress / max((size_t)1, _ota_total), 12, THEME_GREEN);
                M5Cardputer.Display.setTextColor(THEME_CYAN);
                M5Cardputer.Display.setCursor(100, 110);
                char buf[16]; snprintf(buf, sizeof(buf), "%d KB", (int)(_ota_progress/1024));
                M5Cardputer.Display.print(buf);
            }

            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = anuluj");
            redraw = false;
        }
        delay(20);
    }
    server.stop();
}

// ─── HTTP UPDATE — pobierz z URL ──────────────────────
void app_ota_http() {
    if (WiFi.status() != WL_CONNECTED) {
        ui_show_info("Polacz najpierw WiFi!", THEME_RED);
        delay(2000); return;
    }

    String url = prefs.getString("ota_url", "https://example.com/firmware.bin");

    ui_draw_header("OTA - HTTP UPDATE", THEME_GREEN);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 24); M5Cardputer.Display.print("URL:");
    M5Cardputer.Display.setTextColor(THEME_CYAN);
    M5Cardputer.Display.setCursor(4, 36);
    M5Cardputer.Display.print(url.length() > 38 ? url.substring(0,38)+"~" : url);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 60);
    M5Cardputer.Display.print("ENTER = pobierz i flashuj");
    M5Cardputer.Display.setCursor(4, 76);
    M5Cardputer.Display.print("ESC   = anuluj");

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            for (char c : s.word) {
                if (c=='\n'||c=='\r') goto start_dl;
                if (c==27) return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) goto start_dl;
        delay(10);
    }
    start_dl:

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code != 200) {
        char msg[40]; snprintf(msg, sizeof(msg), "HTTP %d - blad!", code);
        ui_show_info(msg, THEME_RED); delay(2000); return;
    }
    int total = http.getSize();
    if (total <= 0) { ui_show_info("Niepoprawny rozmiar", THEME_RED); delay(2000); return; }

    if (!Update.begin(total)) {
        ui_show_info("Brak miejsca!", THEME_RED); delay(2000); return;
    }

    WiFiClient* stream = http.getStreamPtr();
    int written = 0;
    uint8_t buf[1024];

    while (http.connected() && written < total && !_ota_check_esc()) {
        size_t avail = stream->available();
        if (avail) {
            int rd = stream->readBytes(buf, min(avail, sizeof(buf)));
            Update.write(buf, rd);
            written += rd;

            // Progress bar
            int pct = written * 100 / total;
            ui_draw_header("OTA - POBIERANIE", THEME_GREEN);
            M5Cardputer.Display.setTextColor(THEME_TEXT);
            M5Cardputer.Display.setCursor(4, 30);
            char pb[40]; snprintf(pb, sizeof(pb), "%d / %d KB (%d%%)", written/1024, total/1024, pct);
            M5Cardputer.Display.print(pb);
            M5Cardputer.Display.fillRect(4, 50, 232, 18, THEME_BG);
            M5Cardputer.Display.drawRect(4, 50, 232, 16, THEME_GREEN);
            M5Cardputer.Display.fillRect(6, 52, 228*pct/100, 12, THEME_GREEN);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 125);
            M5Cardputer.Display.print("FN+Q = anuluj");
        }
        delay(1);
    }
    http.end();

    if (Update.end(true)) {
        ui_show_info("OK! Restart...", THEME_GREEN);
        delay(1500);
        ESP.restart();
    } else {
        ui_show_info("Update FAILED!", THEME_RED);
        delay(2000);
    }
}

// ─── GŁÓWNE MENU OTA ──────────────────────────────────
void app_ota_menu() {
    while (true) {
        const char* opts[] = {"Web Update (przegladarka)","HTTP Update (URL)"};
        int sel = ui_select_list(opts, 2, "OTA UPDATE", THEME_GREEN);
        if (sel < 0) return;
        if (sel == 0) app_ota_web();
        if (sel == 1) app_ota_http();
    }
}
