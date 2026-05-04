#pragma once
// apps/app_siec.h — Ping, Skaner portów, Przeglądarka HTTP

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

// ─── PING ───────────────────────────────────────────────
void narzedzie_ping() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("PING", THEME_CYAN);
    String host = ui_input_string("Adres hosta:", 4, 30, 32);
    if (host.length() == 0) return;

    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header(("PING: " + host).c_str(), THEME_CYAN);

    IPAddress ip;
    bool resolved = WiFi.hostByName(host.c_str(), ip);
    if (!resolved) {
        ui_show_error("Nie mozna rozwiazac DNS!");
        delay(2000); return;
    }

    char ipbuf[20];
    snprintf(ipbuf, sizeof(ipbuf), "IP: %s", ip.toString().c_str());
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 28);
    M5Cardputer.Display.print(ipbuf);

    int y = 44;
    long sumaMs = 0; int udane = 0;
    for (int i = 0; i < 5; i++) {
        WiFiClient klient;
        unsigned long t = millis();
        bool ok = klient.connect(ip, 80);
        unsigned long ms = millis() - t;
        klient.stop();

        M5Cardputer.Display.setCursor(4, y + i * 14);
        if (ok) {
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            char buf[40]; snprintf(buf, sizeof(buf), "Od %s: czas=%lums TTL=64", ip.toString().c_str(), ms);
            M5Cardputer.Display.print(buf);
            sumaMs += ms; udane++;
        } else {
            M5Cardputer.Display.setTextColor(THEME_RED);
            M5Cardputer.Display.print("Limit czasu zadania.");
        }
        delay(300);
    }

    M5Cardputer.Display.setTextColor(THEME_YELLOW);
    M5Cardputer.Display.setCursor(4, y + 5 * 14 + 6);
    M5Cardputer.Display.print("Statystyki: 5 wys., ");
    M5Cardputer.Display.print(udane); M5Cardputer.Display.print(" odb.");
    if (udane > 0) {
        M5Cardputer.Display.setCursor(4, y + 6 * 14 + 6);
        M5Cardputer.Display.print("Sredni czas: "); M5Cardputer.Display.print(sumaMs / udane); M5Cardputer.Display.print(" ms");
    }

    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 212);
    M5Cardputer.Display.print("Dowolny klawisz = powrot");
    ui_wait_key();
}

// ─── SKANER PORTÓW ──────────────────────────────────────
void skaner_portow() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("SKANER PORTOW", THEME_ORANGE);
    String host = ui_input_string("Adres IP/host:", 4, 28, 24);
    if (host.length() == 0) return;

    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header(("SKANUJE: " + host).c_str(), THEME_ORANGE);

    // Typowe porty
    int porty[] = {21,22,23,25,53,80,110,143,443,445,3306,3389,8080,8443,8888};
    const char* uslugi[] = {"FTP","SSH","Telnet","SMTP","DNS","HTTP","POP3","IMAP",
                             "HTTPS","SMB","MySQL","RDP","HTTP-alt","HTTPS-alt","Dev"};
    int n = 15;

    IPAddress ip;
    WiFi.hostByName(host.c_str(), ip);

    int y = 24; int otwarte = 0;
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, y); y += 12;
    M5Cardputer.Display.print("Skanuje 15 portow...");

    for (int i = 0; i < n; i++) {
        WiFiClient k;
        k.setTimeout(400);
        bool otw = k.connect(ip, porty[i]);
        k.stop();

        if (y > 198) break;
        if (otw) {
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            char buf[30]; snprintf(buf, sizeof(buf), "%-5d %-8s OTWARTY", porty[i], uslugi[i]);
            M5Cardputer.Display.setCursor(4, y);
            M5Cardputer.Display.print(buf);
            y += 13; otwarte++;
        }
        // Pasek postępu
        ui_progress((i + 1) * 100 / n, 210, THEME_ORANGE);
    }

    if (otwarte == 0) {
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 80);
        M5Cardputer.Display.print("Brak otwartych portow.");
    }

    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 212);
    M5Cardputer.Display.print("Dowolny klawisz = powrot");
    ui_wait_key();
}

// ─── PRZEGLĄDARKA HTTP ──────────────────────────────────
void przegladarka_http() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("PRZEGLADARKA HTTP", THEME_CYAN);
    String url = ui_input_string("URL (http://...):", 4, 30, 60);
    if (url.length() == 0 || !url.startsWith("http")) return;

    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("POBIERANIE...", THEME_CYAN);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 28);
    M5Cardputer.Display.print(url.substring(0, 36));

    HTTPClient http;
    http.begin(url.c_str());
    http.setTimeout(8000);
    int kod = http.GET();

    M5Cardputer.Display.fillScreen(THEME_BG);
    char hdr[24]; snprintf(hdr, sizeof(hdr), "HTTP %d", kod);
    ui_draw_header(hdr, kod == 200 ? THEME_GREEN : THEME_RED);

    if (kod == 200) {
        String tresc = http.getString();
        // Usuń tagi HTML (bardzo prosta metoda)
        String czysty = "";
        bool wTag = false;
        for (char c : tresc) {
            if (c == '<') wTag = true;
            else if (c == '>') wTag = false;
            else if (!wTag && c >= 32) czysty.concat(c);
            if (czysty.length() > 800) break;
        }

        // Wyświetl tekst w liniach
        int y = 24, linia = 0;
        int offset = 0;
        bool redraw2 = true;

        while (true) {
            if (redraw2) {
                M5Cardputer.Display.fillRect(0, 22, 240, 188, THEME_BG);
                y = 24;
                int pos = offset;
                while (pos < (int)czysty.length() && y < 205) {
                    String ln = czysty.substring(pos, pos + 37);
                    M5Cardputer.Display.setTextColor(TFT_WHITE);
                    M5Cardputer.Display.setCursor(4, y);
                    M5Cardputer.Display.print(ln);
                    pos += 37; y += 12; linia++;
                }
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, 212);
                M5Cardputer.Display.print("W/S=przewin  Q=wyjscie");
                redraw2 = false;
            }
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                auto st = M5Cardputer.Keyboard.keysState();
                for (auto c : st.word) {
                    if (c == 's' || c == 'S') { offset = min((int)czysty.length() - 37, offset + 37); redraw2 = true; }
                    if (c == 'w' || c == 'W') { offset = max(0, offset - 37); redraw2 = true; }
                    if (c == 'q' || c == 'Q') goto koniec_http;
                }
            }
            if (M5Cardputer.BtnA.wasPressed()) goto koniec_http;
            delay(10);
        }
    } else {
        M5Cardputer.Display.setTextColor(THEME_RED);
        M5Cardputer.Display.setCursor(4, 50);
        M5Cardputer.Display.print("Blad HTTP: "); M5Cardputer.Display.print(kod);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("Dowolny klawisz = powrot");
        ui_wait_key();
    }
    koniec_http:
    http.end();
}

// ─── MQTT KLIENT ────────────────────────────────────────
void mqtt_klient() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("MQTT KLIENT", THEME_CYAN);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 28);
    M5Cardputer.Display.print("Broker MQTT:");
    String broker = ui_input_string("", 4, 42, 32);
    if (broker.length() == 0) return;

    String temat = ui_input_string("Temat:", 4, 72, 32);
    if (temat.length() == 0) return;

    String wiadom = ui_input_string("Wiadomosc:", 4, 102, 40);

    // Prosta implementacja MQTT publish przez TCP (port 1883)
    WiFiClient klient;
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("MQTT", THEME_CYAN);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 30);
    M5Cardputer.Display.print("Laczenie z: "); M5Cardputer.Display.print(broker);

    if (!klient.connect(broker.c_str(), 1883)) {
        ui_show_error("Blad polaczenia MQTT!");
        delay(1500); return;
    }

    // MQTT CONNECT packet (protocol: MQTT v4, clean session, keepalive 60s)
    static const uint8_t MQTT_CONNECT[] = {
        0x10, 0x16,
        0x00, 0x04, 'M','Q','T','T',
        0x04, 0x02, 0x00, 0x3C,
        0x00, 0x0C,
        'C','a','r','d','P','u','t','e','r','O','S','1'
    };
    klient.write(MQTT_CONNECT, sizeof(MQTT_CONNECT));
    delay(200);

    // MQTT PUBLISH
    String pld = wiadom;
    int total = 2 + temat.length() + pld.length();
    uint8_t pub[128];
    int pi = 0;
    pub[pi++] = 0x30; // PUBLISH
    pub[pi++] = total;
    pub[pi++] = 0; pub[pi++] = temat.length();
    for (char c : temat) pub[pi++] = c;
    for (char c : pld)    pub[pi++] = c;
    klient.write(pub, pi);
    delay(200);
    klient.stop();

    ui_show_info("Wyslano MQTT!", THEME_GREEN);
    delay(1500);
}

void app_siec_run() {
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            ui_draw_header("NARZEDZIA SIECIOWE", THEME_CYAN);
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("SIEC", THEME_CYAN);
            M5Cardputer.Display.setTextColor(THEME_RED);
            M5Cardputer.Display.setCursor(20, 100);
            M5Cardputer.Display.print("Brak polaczenia WiFi!");
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(20, 116);
            M5Cardputer.Display.print("Najpierw polacz WiFi.");
            M5Cardputer.Display.setCursor(4, 212);
            M5Cardputer.Display.print("Dowolny klawisz = powrot");
            ui_wait_key(); return;
        }
        const char* opts[] = { "Ping", "Skaner portow", "Przegladarka HTTP", "Klient MQTT", "Powrot" };
        int sel = ui_select_list(opts, 5, "NARZEDZIA SIECIOWE", THEME_CYAN);
        if (sel < 0 || sel == 4) return;
        switch (sel) {
            case 0: narzedzie_ping(); break;
            case 1: skaner_portow(); break;
            case 2: przegladarka_http(); break;
            case 3: mqtt_klient(); break;
        }
    }
}
