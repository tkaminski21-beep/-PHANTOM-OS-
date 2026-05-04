#pragma once
// apps/app_zaawansowane.h

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <math.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <SD.h>

// ══════════════════════════════════════════════
//  KALKULATOR NAUKOWY — funkcje pomocnicze (static)
// ══════════════════════════════════════════════
static bool _kn_tryb_rad = true;

static void _kn_zastap(String& s, const char* fcn, double (*fn)(double)) {
    while (true) {
        int pos = s.indexOf(fcn);
        if (pos < 0) break;
        int ks = s.indexOf('(', pos);
        if (ks < 0) break;
        int level = 1, ke = ks + 1;
        while (ke < (int)s.length() && level > 0) {
            if (s[ke] == '(') level++;
            if (s[ke] == ')') level--;
            ke++;
        }
        double arg = s.substring(ks + 1, ke - 1).toDouble();
        if (!_kn_tryb_rad) {
            if (!strcmp(fcn,"sin") || !strcmp(fcn,"cos") || !strcmp(fcn,"tan"))
                arg = arg * M_PI / 180.0;
        }
        char buf[24]; snprintf(buf, sizeof(buf), "%.10g", fn(arg));
        s = s.substring(0, pos) + String(buf) + s.substring(ke);
    }
}

static double _kn_oblicz(const String& w) {
    String s = w;
    s.replace("pi", "3.14159265358979");
    // Zastąp samodzielne 'e'
    for (int i = (int)s.length() - 1; i >= 0; i--) {
        if ((s[i]=='e'||s[i]=='E') &&
            (i==0||!isalpha(s[i-1])) &&
            (i+1>=(int)s.length()||!isalpha(s[i+1]))) {
            s = s.substring(0,i) + "2.71828182845905" + s.substring(i+1);
        }
    }
    // Funkcje — dłuższe przed krótszymi
    _kn_zastap(s, "asin", asin);
    _kn_zastap(s, "acos", acos);
    _kn_zastap(s, "atan", atan);
    _kn_zastap(s, "sin",  sin);
    _kn_zastap(s, "cos",  cos);
    _kn_zastap(s, "tan",  tan);
    _kn_zastap(s, "sqrt", sqrt);
    _kn_zastap(s, "log",  log10);
    _kn_zastap(s, "ln",   log);
    _kn_zastap(s, "abs",  fabs);
    // Potęgowanie
    while (s.indexOf('^') >= 0) {
        int op = s.indexOf('^');
        int lp = op - 1;
        while (lp > 0 && (isdigit(s[lp-1])||s[lp-1]=='.'||s[lp-1]=='-')) lp--;
        int pp = op + 1;
        while (pp < (int)s.length()-1 &&
               (isdigit(s[pp])||s[pp]=='.'||(s[pp]=='-'&&pp==op+1))) pp++;
        double baza = s.substring(lp, op).toDouble();
        double wyk  = s.substring(op+1, pp+1).toDouble();
        char buf[24]; snprintf(buf, sizeof(buf), "%.10g", pow(baza, wyk));
        s = s.substring(0, lp) + String(buf) + s.substring(pp+1);
    }
    return s.toDouble();
}

void kalkulator_naukowy() {
    String wyraz = "", wyswietlacz = "0";
    bool blad = false, redraw = true;
    _kn_tryb_rad = true;

    struct BtnInfo { const char* lab; uint32_t kol; };
    static const BtnInfo BTN[16] = {
        {"sin",THEME_BLUE}, {"cos",THEME_BLUE}, {"tan",THEME_BLUE}, {"asin",THEME_BLUE},
        {"sqrt",THEME_CYAN},{"log",THEME_CYAN},  {"ln",THEME_CYAN},  {"abs",THEME_CYAN},
        {"pi",THEME_GREEN}, {"e",THEME_GREEN},   {"^",THEME_ORANGE},  {"!",THEME_ORANGE},
        {"(",THEME_GRAY},   {")",THEME_GRAY},    {"C",THEME_RED},     {"=",THEME_GREEN},
    };

    while (true) {
        if (redraw) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("KALKULATOR NAUKOWY", THEME_CYAN);
            // Wyrażenie
            M5Cardputer.Display.fillRoundRect(4,22,232,14,3,THEME_PANEL);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(6,24);
            String sk = wyraz.length()>37 ? "..."+wyraz.substring(wyraz.length()-34) : wyraz;
            M5Cardputer.Display.print(sk);
            // Wynik
            M5Cardputer.Display.fillRoundRect(4,38,232,20,3,THEME_PANEL);
            M5Cardputer.Display.drawRoundRect(4,38,232,20,3,blad?THEME_RED:THEME_CYAN);
            M5Cardputer.Display.setTextColor(blad?THEME_RED:TFT_WHITE);
            M5Cardputer.Display.setTextSize(2);
            M5Cardputer.Display.setCursor(8,41);
            String ds = wyswietlacz; if(ds.length()>13) ds=ds.substring(0,13);
            M5Cardputer.Display.print(ds);
            M5Cardputer.Display.setTextSize(1);
            // RAD/DEG
            M5Cardputer.Display.setTextColor(_kn_tryb_rad?THEME_YELLOW:THEME_CYAN);
            M5Cardputer.Display.setCursor(198,42);
            M5Cardputer.Display.print(_kn_tryb_rad?"RAD":"DEG");
            // Przyciski
            for (int fk=0; fk<16; fk++) {
                int x = 4+(fk%4)*58, y = 64+(fk/4)*22;
                M5Cardputer.Display.fillRoundRect(x,y,56,20,3,BTN[fk].kol>>2);
                M5Cardputer.Display.drawRoundRect(x,y,56,20,3,BTN[fk].kol);
                M5Cardputer.Display.setTextColor(TFT_WHITE);
                int tx = x+(56-(int)strlen(BTN[fk].lab)*6)/2;
                M5Cardputer.Display.setCursor(tx,y+6);
                M5Cardputer.Display.print(BTN[fk].lab);
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4,157);
            M5Cardputer.Display.print("sin(x) cos(x) sqrt(x) log(x) x^y");
            M5Cardputer.Display.setCursor(4,169);
            M5Cardputer.Display.print("ENTER=oblicz C=czysc R=RAD/DEG Q=wr");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            if (st.del) {
                if (wyraz.length()>0) wyraz.remove(wyraz.length()-1);
                blad=false; redraw=true;
            }
            for (auto c : st.word) {
                if (c=='\n'||c=='=') {
                    blad=false;
                    if (wyraz.length()>0) {
                        double wynik = _kn_oblicz(wyraz);
                        if (isnan(wynik)||isinf(wynik)) { wyswietlacz="BLAD"; blad=true; }
                        else {
                            char rb[24];
                            if (wynik==(long long)wynik) snprintf(rb,sizeof(rb),"%lld",(long long)wynik);
                            else snprintf(rb,sizeof(rb),"%.8g",wynik);
                            wyswietlacz=String(rb); wyraz=wyswietlacz;
                        }
                    }
                    redraw=true;
                } else if (c=='c'||c=='C') { wyraz=""; wyswietlacz="0"; blad=false; redraw=true;
                } else if (c=='r'||c=='R') { _kn_tryb_rad=!_kn_tryb_rad; redraw=true;
                } else if (c=='q'||c=='Q') { return;
                } else { wyraz+=c; redraw=true; }
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(10);
    }
}

// ══════════════════════════════════════════════
//  NOTATNIK SZYFROWANY (XOR + HEX)
// ══════════════════════════════════════════════
#define NOTATKI_ENC_DIR "/enc_notes"

static String _enc_szyfruj(const String& tekst, const String& haslo) {
    uint8_t k[32]={};
    for (int i=0;i<(int)haslo.length()&&i<32;i++) k[i]=(uint8_t)haslo[i];
    String out="";
    for (int i=0;i<(int)tekst.length();i++) {
        char buf[3]; snprintf(buf,sizeof(buf),"%02X",(uint8_t)((uint8_t)tekst[i]^k[i%32]));
        out+=buf;
    }
    return out;
}

static String _enc_odszyfruj(const String& hex, const String& haslo) {
    uint8_t k[32]={};
    for (int i=0;i<(int)haslo.length()&&i<32;i++) k[i]=(uint8_t)haslo[i];
    String out="";
    for (int i=0;i+1<(int)hex.length();i+=2) {
        char hb[3]={hex[i],hex[i+1],0};
        out+=(char)((uint8_t)strtol(hb,nullptr,16)^k[(i/2)%32]);
    }
    return out;
}

void notatnik_szyfrowany() {
    SD.mkdir(NOTATKI_ENC_DIR);
    while (true) {
        const char* opts[]={"Nowa zaszyfrowana notatka","Otworz notatke","Powrot"};
        int sel=ui_select_list(opts,3,"NOTATNIK SZYFROWANY",THEME_GREEN);
        if (sel<0||sel==2) return;

        if (sel==0) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("NOWA NOTATKA (ZASZYFROWANA)",THEME_GREEN);
            String nazwa=ui_input_string("Nazwa:",4,28,20);
            if (nazwa.length()==0) continue;
            String h1=ui_input_string("Haslo:",4,58,24);
            String h2=ui_input_string("Powtorz haslo:",4,88,24);
            if (h1!=h2) { ui_show_error("Hasla nie sa zgodne!"); delay(1500); continue; }
            String tresc=ui_input_string("Tresc:",4,118,60);
            String enc=_enc_szyfruj(tresc,h1);
            String path=String(NOTATKI_ENC_DIR)+"/"+nazwa+".enc";
            File f=SD.open(path.c_str(),FILE_WRITE);
            if (f) { f.print(enc); f.close(); ui_show_info("Zapisano!",THEME_GREEN); }
            else     ui_show_error("Blad zapisu SD!");
            delay(1500);
        } else {
            String pliki[16]; int lp=0;
            File dir=SD.open(NOTATKI_ENC_DIR);
            if (dir) {
                while(lp<16){File f=dir.openNextFile();if(!f)break;pliki[lp++]=String(f.name());f.close();}
                dir.close();
            }
            if (lp==0) { ui_show_info("Brak notatek.",THEME_YELLOW); delay(1200); continue; }
            const char* np[16]; for(int i=0;i<lp;i++) np[i]=pliki[i].c_str();
            int pick=ui_select_list(np,lp,"WYBIERZ NOTATKE",THEME_GREEN);
            if (pick<0) continue;
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("PODAJ HASLO",THEME_GREEN);
            String haslo=ui_input_string("Haslo:",4,30,24);
            String path=String(NOTATKI_ENC_DIR)+"/"+pliki[pick];
            File f=SD.open(path.c_str());
            if (!f) { ui_show_error("Nie mozna otworzyc!"); delay(1500); continue; }
            String hex=f.readString(); f.close();
            String odsz=_enc_odszyfruj(hex,haslo);
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header(pliki[pick].c_str(),THEME_GREEN);
            M5Cardputer.Display.setTextColor(TFT_WHITE);
            int y=24;
            for (int i=0;i<(int)odsz.length()&&y<206;i+=36) {
                M5Cardputer.Display.setCursor(4,y);
                M5Cardputer.Display.print(odsz.substring(i,i+36));
                y+=12;
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4,212);
            M5Cardputer.Display.print("Dowolny klawisz = powrot");
            ui_wait_key();
        }
    }
}

// ══════════════════════════════════════════════
//  ANALIZATOR SIECI WiFi
// ══════════════════════════════════════════════
void analizator_wifi() {
    while (true) {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("ANALIZATOR WIFI",THEME_CYAN);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(50,100);
        M5Cardputer.Display.print("Skanowanie sieci...");
        WiFi.mode(WIFI_STA); WiFi.disconnect(); delay(100);
        int n=WiFi.scanNetworks(false,true);

        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("ANALIZATOR WIFI",THEME_CYAN);

        if (n<=0) {
            M5Cardputer.Display.setTextColor(THEME_RED);
            M5Cardputer.Display.setCursor(4,50);
            M5Cardputer.Display.print("Brak sieci WiFi.");
        } else {
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4,22);
            M5Cardputer.Display.print("SSID             Ch  RSSI  Zabezp.");
            M5Cardputer.Display.drawLine(0,31,240,31,THEME_BORDER);

            int kanaly[14]={};
            for (int i=0;i<n&&i<13;i++) { int ch=WiFi.channel(i); if(ch>=1&&ch<=13) kanaly[ch]++; }

            int y=34;
            for (int i=0;i<n&&y<170;i++) {
                int rssi=WiFi.RSSI(i);
                uint32_t kol=(rssi>=-50)?THEME_GREEN:(rssi>=-70)?THEME_YELLOW:THEME_RED;
                M5Cardputer.Display.setTextColor(kol);
                M5Cardputer.Display.setCursor(4,y);
                String ssid=WiFi.SSID(i);
                if(ssid.length()==0) ssid="(ukryta)";
                if(ssid.length()>15) ssid=ssid.substring(0,13)+"..";
                char buf[40];
                snprintf(buf,sizeof(buf),"%-16s %2d %4d  %s",
                    ssid.c_str(), WiFi.channel(i), rssi,
                    WiFi.encryptionType(i)!=WIFI_AUTH_OPEN?"WPA":"OPEN");
                M5Cardputer.Display.print(buf);
                y+=12;
            }

            // Wykres kanałów
            M5Cardputer.Display.drawLine(0,172,240,172,THEME_BORDER);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4,174);
            M5Cardputer.Display.print("Kanaly 1-13:");
            for (int ch=1;ch<=13;ch++) {
                int bh=kanaly[ch]*14;
                if (bh>0) {
                    uint32_t bk=(kanaly[ch]>2)?THEME_RED:(kanaly[ch]>1)?THEME_YELLOW:THEME_GREEN;
                    M5Cardputer.Display.fillRect(36+(ch-1)*15,208-bh,12,bh,bk);
                }
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(37+(ch-1)*15,210);
                if (ch<10) M5Cardputer.Display.print(ch); else M5Cardputer.Display.print("*");
            }
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4,214);
            M5Cardputer.Display.print("Sieci:"); M5Cardputer.Display.print(n);
            M5Cardputer.Display.print("  ENTER=ponow  Q=wyjscie");
        }
        char k=ui_wait_key();
        if (k=='q'||k=='Q') return;
    }
}

// ══════════════════════════════════════════════
//  SERWER WWW
// ══════════════════════════════════════════════
static WebServer* _serwer=nullptr;

void serwer_www_run() {
    if (WiFi.status()!=WL_CONNECTED) { ui_show_error("Najpierw polacz WiFi!"); delay(1500); return; }
    _serwer=new WebServer(80);

    _serwer->on("/", HTTP_GET, []() {
        String html="<html><head><meta charset='utf-8'>"
            "<style>body{font-family:monospace;background:#0a0f1a;color:#e0f0ff;padding:20px}"
            "h1{color:#00d4ff}table{border-collapse:collapse;width:100%}"
            "td,th{border:1px solid #1e3048;padding:8px}th{color:#00d4ff}</style></head>"
            "<body><h1>CardPuter OS v3.0</h1><table>"
            "<tr><th>Parametr</th><th>Wartosc</th></tr>";
        char row[80];
        snprintf(row,sizeof(row),"<tr><td>Heap</td><td>%d KB</td></tr>",(int)(esp_get_free_heap_size()/1024));
        html+=row;
        snprintf(row,sizeof(row),"<tr><td>Uptime</td><td>%lu s</td></tr>",millis()/1000);
        html+=row;
        snprintf(row,sizeof(row),"<tr><td>IP</td><td>%s</td></tr>",WiFi.localIP().toString().c_str());
        html+=row;
        snprintf(row,sizeof(row),"<tr><td>RSSI</td><td>%d dBm</td></tr>",WiFi.RSSI());
        html+=row;
        snprintf(row,sizeof(row),"<tr><td>Bateria</td><td>%d%%</td></tr>",M5Cardputer.Power.getBatteryLevel());
        html+=row;
        html+="</table></body></html>";
        _serwer->send(200,"text/html",html);
    });

    _serwer->on("/api/status", HTTP_GET, []() {
        char json[200];
        snprintf(json,sizeof(json),
            "{\"heap\":%d,\"uptime\":%lu,\"ip\":\"%s\",\"bat\":%d}",
            (int)esp_get_free_heap_size(), millis()/1000,
            WiFi.localIP().toString().c_str(),
            M5Cardputer.Power.getBatteryLevel());
        _serwer->send(200,"application/json",json);
    });

    _serwer->begin();

    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("SERWER WWW",THEME_GREEN);
    M5Cardputer.Display.setTextColor(THEME_GREEN);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(4,40);
    M5Cardputer.Display.print(WiFi.localIP().toString());
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4,64); M5Cardputer.Display.print("Otworz: http://");
    M5Cardputer.Display.print(WiFi.localIP().toString());
    M5Cardputer.Display.setCursor(4,78); M5Cardputer.Display.print("API: /api/status (JSON)");
    M5Cardputer.Display.setTextColor(THEME_YELLOW);
    M5Cardputer.Display.setCursor(4,96); M5Cardputer.Display.print("Serwer aktywny na porcie 80");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4,212); M5Cardputer.Display.print("Q lub BTN = zatrzymaj");

    while (true) {
        _serwer->handleClient();
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
            auto st=M5Cardputer.Keyboard.keysState();
            bool stop=false;
            for (auto c:st.word) if(c=='q'||c=='Q'){stop=true;break;}
            if(stop) break;
        }
        if (M5Cardputer.BtnA.wasPressed()) break;
        delay(5);
    }
    _serwer->stop(); delete _serwer; _serwer=nullptr;
}

// ══════════════════════════════════════════════
//  WAKE-ON-LAN
// ══════════════════════════════════════════════
void wake_on_lan() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("WAKE-ON-LAN",THEME_CYAN);
    if (WiFi.status()!=WL_CONNECTED) { ui_show_error("Brak WiFi!"); delay(1500); return; }

    String mac_str=ui_input_string("MAC (XX:XX:XX:XX:XX:XX):",4,30,17);
    if (mac_str.length()<17) { ui_show_error("Zly format MAC!"); delay(1500); return; }

    uint8_t mac[6];
    if (sscanf(mac_str.c_str(),"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5])!=6) {
        ui_show_error("Zly format MAC!"); delay(1500); return;
    }

    uint8_t pakiet[102];
    memset(pakiet,0xFF,6);
    for (int i=0;i<16;i++) memcpy(pakiet+6+i*6,mac,6);

    WiFiUDP udp; udp.begin(9);
    IPAddress broadcast(255,255,255,255);
    udp.beginPacket(broadcast,9);
    udp.write(pakiet,102);
    int ret=udp.endPacket();
    udp.stop();

    M5Cardputer.Display.fillScreen(THEME_BG);
    if (ret) {
        ui_draw_header("WoL WYSLANY",THEME_GREEN);
        M5Cardputer.Display.setTextColor(THEME_GREEN);
        M5Cardputer.Display.setCursor(4,40); M5Cardputer.Display.print("Magic Packet wyslany!");
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(4,56); M5Cardputer.Display.print(mac_str);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4,72); M5Cardputer.Display.print("Broadcast 255.255.255.255:9");
    } else {
        ui_draw_header("BLAD UDP",THEME_RED);
        ui_show_error("Blad wysylania pakietu!");
    }
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4,212); M5Cardputer.Display.print("Dowolny klawisz = powrot");
    ui_wait_key();
}

// ══════════════════════════════════════════════
//  MENU ZAAWANSOWANE
// ══════════════════════════════════════════════
void app_zaawansowane_run() {
    while (true) {
        const char* opts[]={
            "Kalkulator naukowy","Notatnik szyfrowany",
            "Analizator sieci WiFi","Serwer WWW na CardPuter",
            "Wake-on-LAN","Powrot"
        };
        int sel=ui_select_list(opts,6,"ZAAWANSOWANE",THEME_CYAN);
        if (sel<0||sel==5) return;
        switch(sel) {
            case 0: kalkulator_naukowy();  break;
            case 1: notatnik_szyfrowany(); break;
            case 2: analizator_wifi();     break;
            case 3: serwer_www_run();      break;
            case 4: wake_on_lan();         break;
        }
    }
}
