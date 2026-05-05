/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║     PHANTOM OS  v1.1                                         ║
 * ║   Sterowanie w stylu Bruce firmware                          ║
 * ║   ;/. = nawigacja, ENTER = OK, ` = wstecz                    ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include "M5Cardputer.h"
#include "M5Unified.h"
#include <SD.h>
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>

#include "core/theme.h"
#include "core/ui.h"
#include "core/bootloader.h"
#include "core/sound.h"

#include "apps/app_notes.h"
#include "apps/app_resistor.h"
#include "apps/app_mp3.h"
#include "apps/app_wifi.h"
#include "apps/app_bluetooth.h"
#include "apps/app_launcher.h"
#include "apps/app_settings.h"
#include "apps/app_kalkulator.h"
#include "apps/app_konwerter.h"
#include "apps/app_narzedzia.h"
#include "apps/app_zegar.h"
#include "apps/app_siec.h"
#include "apps/app_hardware.h"
#include "apps/app_gry.h"
#include "apps/app_system.h"
#include "apps/app_partycje.h"
#include "apps/app_monitor.h"
#include "apps/app_oscyloskop.h"
#include "apps/app_zaawansowane.h"
#include "apps/app_ir.h"
#include "apps/app_rf.h"
#include "apps/app_nrf24.h"
#include "apps/app_gps.h"
#include "apps/app_fm.h"
#include "apps/app_wifi_atk.h"
#include "apps/app_akcelerometr.h"
#include "apps/app_badusb.h"
#include "apps/app_pwnagotchi.h"
#include "apps/app_ota.h"
#include "apps/app_screensaver.h"
#include "apps/app_stoper_timer.h"
#include "apps/app_morse.h"
#include "apps/app_pogoda.h"
#include "apps/app_rfid.h"
#include "apps/app_radio.h"

Preferences prefs;
bool        sd_ok = false;

// ═══════════════════════════════════════════════════════
//  KOLORY
// ═══════════════════════════════════════════════════════
uint16_t UI_BG  = 0x0000;
uint16_t UI_FG  = 0xFFFF;
uint16_t UI_PRI = 0xFFFF;

void load_colors() {
    UI_BG  = (uint16_t)prefs.getUInt("col_bg",  0x0000);
    UI_FG  = (uint16_t)prefs.getUInt("col_fg",  0xFFFF);
    UI_PRI = (uint16_t)prefs.getUInt("col_pri", 0xFFFF);
}
void save_colors() {
    prefs.putUInt("col_bg",  UI_BG);
    prefs.putUInt("col_fg",  UI_FG);
    prefs.putUInt("col_pri", UI_PRI);
}

#include "core/colors.h"

// ═══════════════════════════════════════════════════════
//  BATERIA
// ═══════════════════════════════════════════════════════
#ifndef PIN_BAT_ADC
#define PIN_BAT_ADC 10
#endif
static int _bat_cached = -1;
static unsigned long _bat_last = 0;

int get_battery() {
    if (millis() - _bat_last < 10000 && _bat_cached >= 0) return _bat_cached;
    _bat_last = millis();
    int level = M5Cardputer.Power.getBatteryLevel();
    if (level >= 0 && level <= 100) { _bat_cached = level; return level; }
    pinMode(PIN_BAT_ADC, INPUT);
    uint32_t mv = analogReadMilliVolts(PIN_BAT_ADC);
    float v = mv * 2.0f / 1000.0f;
    float pct = (v - 3.3f) / (4.15f - 3.3f) * 100.0f;
    _bat_cached = (int)constrain(pct, 1, 100);
    return _bat_cached;
}
bool is_charging() { return M5Cardputer.Power.isCharging(); }

void draw_battery(int x, int y) {
    int bat = get_battery();
    bool ch = is_charging();
    uint32_t col = (uint32_t)UI_PRI;
    uint32_t bar = (uint32_t)UI_PRI;
    if (bat < 16)       { col = bar = 0xF800; }
    else if (bat < 34)  { col = bar = 0xFFE0; }
    if (ch)               col = 0x07E0;
    M5Cardputer.Display.fillRect(x-30, y, 65, 14, (uint32_t)UI_BG);
    M5Cardputer.Display.drawRoundRect(x+1, y+1, 34, 12, 2, col);
    M5Cardputer.Display.fillRect(x+35, y+4, 3, 6, col);
    M5Cardputer.Display.fillRoundRect(x+3, y+3, 30*bat/100, 8, 1, bar);
    M5Cardputer.Display.drawLine(x+11, y+3, x+11, y+11, (uint32_t)UI_BG);
    M5Cardputer.Display.drawLine(x+21, y+3, x+21, y+11, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextColor(col, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    char buf[8]; snprintf(buf, sizeof(buf), "%d%%", bat);
    M5Cardputer.Display.setCursor(x - (int)strlen(buf)*6 - 2, y+3);
    M5Cardputer.Display.print(buf);
}

void draw_statusbar() {
    M5Cardputer.Display.fillRect(0, 0, 240, 22, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    struct tm ti;
    if (getLocalTime(&ti, 0)) {
        char buf[10]; snprintf(buf, sizeof(buf), "%02d:%02d", ti.tm_hour, ti.tm_min);
        M5Cardputer.Display.setCursor(7, 7);
        M5Cardputer.Display.print(buf);
    } else {
        M5Cardputer.Display.setCursor(7, 7);
        M5Cardputer.Display.print("PHANTOM");
    }
    if (WiFi.status() == WL_CONNECTED) {
        M5Cardputer.Display.fillCircle(156, 13, 2, (uint32_t)UI_PRI);
        M5Cardputer.Display.drawArc(156, 13, 6, 4, 210, 330, (uint32_t)UI_PRI);
        M5Cardputer.Display.drawArc(156, 13, 11, 9, 210, 330, (uint32_t)UI_PRI);
    }
    if (sd_ok) {
        M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF, (uint32_t)UI_BG);
        M5Cardputer.Display.setCursor(136, 7); M5Cardputer.Display.print("SD");
    }
    draw_battery(194, 4);
    M5Cardputer.Display.drawLine(5, 22, 235, 22, (uint32_t)UI_PRI);
}

// ═══════════════════════════════════════════════════════
//  SPLASH
// ═══════════════════════════════════════════════════════
void draw_splash() {
    M5Cardputer.Display.fillScreen(0x0000);
    for (int y = 0; y < 135; y++) {
        uint8_t b = y * 12 / 135;
        M5Cardputer.Display.drawFastHLine(0, y, 240, (uint16_t)b);
    }
    M5Cardputer.Display.drawRoundRect(4, 4, 232, 127, 8, 0xFFFF);
    M5Cardputer.Display.drawRoundRect(6, 6, 228, 123, 6, 0x7BEF);
    M5Cardputer.Display.setTextColor(0xFFFF, 0x0000);
    M5Cardputer.Display.setTextSize(3);
    M5Cardputer.Display.setCursor(22, 28); M5Cardputer.Display.print("PHANTOM");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(0x7BEF, 0x0000);
    M5Cardputer.Display.setCursor(60, 62); M5Cardputer.Display.print("OS  v1.1  for");
    M5Cardputer.Display.setTextColor(0x07FF, 0x0000);
    M5Cardputer.Display.setCursor(28, 76); M5Cardputer.Display.print("M5Stack CardputerADV");
    M5Cardputer.Display.drawLine(20, 96, 220, 96, 0x7BEF);
    M5Cardputer.Display.setTextColor(0x4208, 0x0000);
    M5Cardputer.Display.setCursor(82, 102); M5Cardputer.Display.print("loading...");
    for (int i = 0; i <= 200; i += 4) {
        M5Cardputer.Display.fillRect(20, 114, i, 6, 0xFFFF);
        if (i < 200) M5Cardputer.Display.fillRect(20+i, 114, 200-i, 6, 0x2104);
        delay(8);
    }
    delay(300);
}

// ═══════════════════════════════════════════════════════
//  PRZEGLĄDARKA SD (z paginacją)
// ═══════════════════════════════════════════════════════
void app_sd_browser(String path = "/") {
    while (true) {
        std::vector<String> entries;
        File dir = SD.open(path);
        if (!dir) { ui_show_info("Blad otwarcia!", (uint32_t)UI_PRI); delay(1500); return; }
        while (true) {
            File entry = dir.openNextFile();
            if (!entry) break;
            String name = String(entry.name());
            if (entry.isDirectory()) name = "[" + name + "]";
            entries.push_back(name);
            entry.close();
        }
        dir.close();

        if (path != "/") entries.insert(entries.begin(), "..");
        if (entries.empty()) { ui_show_info("Pusty folder", (uint32_t)UI_PRI); delay(1500); return; }

        const char** opts = new const char*[entries.size()];
        for (size_t i = 0; i < entries.size(); i++) opts[i] = entries[i].c_str();
        String title = "SD: " + path;
        int sel = ui_select_list(opts, entries.size(), title.c_str(), (uint32_t)UI_PRI);
        delete[] opts;

        if (sel < 0) return;
        if (entries[sel] == "..") {
            int last = path.lastIndexOf('/', path.length()-2);
            path = (last <= 0) ? "/" : path.substring(0, last+1);
        } else if (entries[sel].startsWith("[")) {
            String sub = entries[sel].substring(1, entries[sel].length()-1);
            path = (path.endsWith("/") ? path : path+"/") + sub + "/";
        } else {
            String fp = (path.endsWith("/") ? path : path+"/") + entries[sel];
            File f = SD.open(fp);
            if (f) {
                uint32_t sz = f.size(); f.close();
                ui_draw_header(entries[sel].c_str(), (uint32_t)UI_PRI);
                M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(4, 28); M5Cardputer.Display.print("Sciezka:");
                M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(4, 40); M5Cardputer.Display.print(fp);
                M5Cardputer.Display.setTextColor((uint32_t)UI_FG, (uint32_t)UI_BG);
                char sb[32]; snprintf(sb, sizeof(sb), "Rozmiar: %lu B", sz);
                M5Cardputer.Display.setCursor(4, 56); M5Cardputer.Display.print(sb);
                M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF, (uint32_t)UI_BG);
                M5Cardputer.Display.setCursor(4, 125); M5Cardputer.Display.print("ENTER=powrot");
                while (true) {
                    M5Cardputer.update();
                    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                        auto s = M5Cardputer.Keyboard.keysState();
                        if (_is_ok(s) || _is_esc(s)) break;
                    }
                    if (M5Cardputer.BtnA.wasPressed()) break;
                    delay(10);
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════
//  IKONY DUŻYCH KAFELKÓW
// ═══════════════════════════════════════════════════════
static void icon_wifi_big(uint32_t c) {
    M5Cardputer.Display.fillCircle(120, 90, 5, c);
    M5Cardputer.Display.drawArc(120, 90, 16, 12, 210, 330, c);
    M5Cardputer.Display.drawArc(120, 90, 28, 24, 210, 330, c);
    M5Cardputer.Display.drawArc(120, 90, 40, 36, 210, 330, c);
}
static void icon_bt_big(uint32_t c) {
    M5Cardputer.Display.drawWideLine(120,50,105,65,3,c);
    M5Cardputer.Display.drawWideLine(120,50,105,95,3,c);
    M5Cardputer.Display.drawWideLine(120,50,120,105,3,c);
    M5Cardputer.Display.drawWideLine(120,105,135,90,3,c);
    M5Cardputer.Display.drawWideLine(120,50,135,65,3,c);
    M5Cardputer.Display.drawWideLine(135,65,105,90,3,c);
    M5Cardputer.Display.drawWideLine(135,90,105,65,3,c);
}
static void icon_ir_big(uint32_t c) {
    M5Cardputer.Display.fillCircle(120, 75, 7, c);
    M5Cardputer.Display.drawArc(120, 75, 20, 15, 220, 320, c);
    M5Cardputer.Display.drawArc(120, 75, 33, 28, 220, 320, c);
    M5Cardputer.Display.drawArc(120, 75, 46, 41, 220, 320, c);
    M5Cardputer.Display.drawRoundRect(103, 92, 34, 20, 3, c);
    for (int i=0;i<4;i++) M5Cardputer.Display.fillRect(107+i*7, 98, 4, 4, c);
}
static void icon_rf_big(uint32_t c) {
    M5Cardputer.Display.drawWideLine(120, 100, 120, 55, 3, c);
    M5Cardputer.Display.fillTriangle(120, 48, 113, 58, 127, 58, c);
    M5Cardputer.Display.drawArc(100, 78, 16, 12, 300, 60, c);
    M5Cardputer.Display.drawArc(100, 78, 28, 24, 300, 60, c);
    M5Cardputer.Display.drawArc(140, 78, 16, 12, 120, 240, c);
    M5Cardputer.Display.drawArc(140, 78, 28, 24, 120, 240, c);
}
static void icon_nrf_big(uint32_t c) {
    M5Cardputer.Display.drawWideLine(120, 100, 120, 60, 3, c);
    M5Cardputer.Display.fillTriangle(120, 52, 113, 62, 127, 62, c);
    M5Cardputer.Display.drawArc(120, 82, 16, 12, 200, 340, c);
    M5Cardputer.Display.drawArc(120, 82, 30, 25, 200, 340, c);
    M5Cardputer.Display.drawArc(120, 82, 44, 39, 200, 340, c);
}
static void icon_gps_big(uint32_t c) {
    M5Cardputer.Display.fillCircle(120, 65, 20, c);
    M5Cardputer.Display.fillCircle(120, 65, 12, (uint32_t)UI_BG);
    M5Cardputer.Display.fillCircle(120, 65, 6, c);
    M5Cardputer.Display.fillTriangle(104, 72, 136, 72, 120, 100, c);
}
static void icon_fm_big(uint32_t c) {
    M5Cardputer.Display.drawRoundRect(88, 58, 64, 42, 5, c);
    M5Cardputer.Display.drawRect(96, 66, 28, 10, c);
    M5Cardputer.Display.fillCircle(136, 88, 8, c);
    M5Cardputer.Display.fillCircle(136, 88, 4, (uint32_t)UI_BG);
    M5Cardputer.Display.drawWideLine(140, 58, 152, 46, 2, c);
}
static void icon_note_big(uint32_t c) {
    M5Cardputer.Display.drawRoundRect(90, 44, 60, 66, 4, c);
    M5Cardputer.Display.drawLine(90, 54, 150, 54, c);
    for (int i=0;i<5;i++) M5Cardputer.Display.drawLine(98, 64+i*8, 142, 64+i*8, c);
}
static void icon_calc_big(uint32_t c) {
    M5Cardputer.Display.drawRoundRect(90, 44, 60, 68, 4, c);
    M5Cardputer.Display.fillRoundRect(94, 48, 52, 18, 2, c);
    for (int j=0;j<3;j++) for (int i=0;i<3;i++)
        M5Cardputer.Display.fillRoundRect(95+i*17, 72+j*12, 12, 8, 2, c);
}
static void icon_atk_big(uint32_t c) {
    M5Cardputer.Display.fillTriangle(125,44,100,80,120,80,c);
    M5Cardputer.Display.fillTriangle(115,68,140,68,115,104,c);
}
static void icon_pwn_big(uint32_t c) {
    M5Cardputer.Display.drawRoundRect(84,48,72,52,8,c);
    M5Cardputer.Display.fillRect(98,63,14,11,c);
    M5Cardputer.Display.fillRect(104,65,3,7,(uint32_t)UI_BG);
    M5Cardputer.Display.fillRect(128,63,14,11,c);
    M5Cardputer.Display.fillRect(134,65,3,7,(uint32_t)UI_BG);
    M5Cardputer.Display.fillCircle(120,88,5,c);
    M5Cardputer.Display.fillCircle(120,88,3,(uint32_t)UI_BG);
    M5Cardputer.Display.drawWideLine(120,48,120,36,2,c);
    M5Cardputer.Display.fillCircle(120,33,4,c);
}
static void icon_badusb_big(uint32_t c) {
    M5Cardputer.Display.drawRoundRect(100,44,40,66,3,c);
    M5Cardputer.Display.fillRect(109,48,6,8,c); M5Cardputer.Display.fillRect(125,48,6,8,c);
    M5Cardputer.Display.fillRect(112,68,16,16,c);
    M5Cardputer.Display.fillCircle(116,73,2,(uint32_t)UI_BG);
    M5Cardputer.Display.fillCircle(124,73,2,(uint32_t)UI_BG);
}
static void icon_ota_big(uint32_t c) {
    M5Cardputer.Display.fillRoundRect(88,44,64,28,14,c);
    M5Cardputer.Display.fillTriangle(120,108,100,84,140,84,c);
    M5Cardputer.Display.fillTriangle(120,108,104,90,136,90,(uint32_t)UI_BG);
    M5Cardputer.Display.fillRect(112,84,16,6,c);
}
static void icon_gear_big(uint32_t c) {
    M5Cardputer.Display.drawCircle(120,78,20,c);
    for (int a=0;a<360;a+=45) {
        int x1=120+(int)(20*cos(a*M_PI/180)), y1=78+(int)(20*sin(a*M_PI/180));
        int x2=120+(int)(30*cos(a*M_PI/180)), y2=78+(int)(30*sin(a*M_PI/180));
        M5Cardputer.Display.drawWideLine(x1,y1,x2,y2,5,c);
    }
    M5Cardputer.Display.fillCircle(120,78,8,c);
    M5Cardputer.Display.fillCircle(120,78,5,(uint32_t)UI_BG);
}
static void icon_imu_big(uint32_t c) {
    M5Cardputer.Display.drawWideLine(120,108,120,50,3,c);
    M5Cardputer.Display.fillTriangle(120,48,113,58,127,58,c);
    M5Cardputer.Display.drawWideLine(86,94,144,74,3,c);
    M5Cardputer.Display.fillTriangle(144,74,138,70,138,80,c);
    M5Cardputer.Display.fillCircle(120,80,5,c);
}
static void icon_game_big(uint32_t c) {
    M5Cardputer.Display.fillRoundRect(85,58,70,42,8,c);
    M5Cardputer.Display.fillRect(98,73,4,14,(uint32_t)UI_BG);
    M5Cardputer.Display.fillRect(93,78,14,4,(uint32_t)UI_BG);
    M5Cardputer.Display.fillCircle(135,73,3,(uint32_t)UI_BG);
    M5Cardputer.Display.fillCircle(145,84,3,(uint32_t)UI_BG);
    M5Cardputer.Display.fillCircle(135,95,3,(uint32_t)UI_BG);
    M5Cardputer.Display.fillCircle(125,84,3,(uint32_t)UI_BG);
}
static void icon_sys_big(uint32_t c) {
    M5Cardputer.Display.drawRoundRect(84,44,72,50,3,c);
    M5Cardputer.Display.fillRect(90,50,60,36,(uint32_t)UI_BG);
    M5Cardputer.Display.drawRect(90,50,60,36,c);
    M5Cardputer.Display.fillRect(114,94,12,8,c);
}
static void icon_screen_big(uint32_t c) {
    M5Cardputer.Display.setTextColor(c,(uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(94,48); M5Cardputer.Display.print("01");
    M5Cardputer.Display.setCursor(122,62); M5Cardputer.Display.print("10");
    M5Cardputer.Display.setCursor(100,76); M5Cardputer.Display.print("11");
    M5Cardputer.Display.setCursor(130,90); M5Cardputer.Display.print("01");
    M5Cardputer.Display.setCursor(100,104); M5Cardputer.Display.print("0X");
}
static void icon_clock_big(uint32_t c) {
    M5Cardputer.Display.drawCircle(120,78,30,c);
    M5Cardputer.Display.drawCircle(120,78,28,c);
    M5Cardputer.Display.drawWideLine(120,78,120,55,2,c);
    M5Cardputer.Display.drawWideLine(120,78,138,78,2,c);
    M5Cardputer.Display.fillCircle(120,78,3,c);
}
static void icon_morse_big(uint32_t c) {
    for (int i=0;i<4;i++) {
        int y = 55+i*15;
        M5Cardputer.Display.fillCircle(95, y, 4, c);
        M5Cardputer.Display.fillRect(106, y-3, 22, 7, c);
        M5Cardputer.Display.fillCircle(135, y, 4, c);
        if (i%2==0) M5Cardputer.Display.fillRect(146, y-3, 14, 7, c);
    }
}
static void icon_internet_big(uint32_t c) {
    M5Cardputer.Display.drawCircle(120,75,32,c);
    M5Cardputer.Display.drawLine(120,43,120,107,c);
    M5Cardputer.Display.drawLine(88,75,152,75,c);
    M5Cardputer.Display.drawArc(120,75,32,30,0,360,c);
    M5Cardputer.Display.drawArc(120,75,20,18,0,360,c);
}
static void icon_sd_big(uint32_t c) {
    M5Cardputer.Display.drawRoundRect(98,46,44,62,4,c);
    M5Cardputer.Display.fillTriangle(98,46,98,54,105,46,c);
    for (int i=0;i<3;i++) M5Cardputer.Display.fillRect(103,56+i*12,34,8,c);
}
static void icon_null_big(uint32_t c) {
    M5Cardputer.Display.drawRoundRect(96,54,48,50,5,c);
    M5Cardputer.Display.setTextColor(c,(uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(3);
    M5Cardputer.Display.setCursor(112,68); M5Cardputer.Display.print("?");
}

// ═══════════════════════════════════════════════════════
//  KAFELKI
// ═══════════════════════════════════════════════════════
struct Tile { const char* label; void (*handler)(); void (*draw_icon)(uint32_t c); };

void w_notes()      { app_notes_run(); }
void w_kalk()       { app_kalkulator_run(); }
void w_rezyst()     { app_resistor_run(); }
void w_konw()       { app_konwerter_run(); }
void w_zegar()      { app_zegar_run(); }
void w_naorz()      { app_narzedzia_run(); }
void w_wifi()       { app_wifi_run(); }
void w_bt()         { app_bluetooth_menu(); }
void w_wifi_atk()   { app_wifi_atk_menu(); }
void w_pwn()        { app_pwnagotchi_menu(); }
void w_badusb()     { app_badusb_menu(); }
void w_siec()       { app_siec_run(); }
void w_mp3()        { app_mp3_run(); }
void w_ir()         { app_ir_menu(); }
void w_rf()         { app_rf_menu(); }
void w_nrf()        { app_nrf24_menu(); }
void w_gps()        { app_gps_menu(); }
void w_fm()         { app_fm_menu(); }
void w_osc()        { oscyloskop(); }
void w_imu()        { app_akcelerometr_menu(); }
void w_launcher()   { uruchom_m5launcher(); }
void w_bin()        { uruchom_bin_z_sd(); }
void w_monitor()    { monitor_systemu(); }
void w_pliki()      { menedzer_plikow(); }
void w_gry()        { app_gry_run(); }
void w_screensaver(){ app_screensaver_menu(); }
void w_ota()        { app_ota_menu(); }
void w_stoper()     { app_stoper_timer_menu(); }
void w_morse()      { app_morse_menu(); }
void w_pogoda()     { app_internet_menu(); }
void w_rfid()       { app_rfid_menu(); }
void w_radio()      { app_radio_menu(); }
void w_sd()         { if (sd_ok) app_sd_browser(); else { ui_show_info("Brak karty SD!", (uint32_t)UI_PRI); delay(1500); } }
void w_ustawienia();

static Tile TILES[] = {
    { "WiFi",        w_wifi,       icon_wifi_big    },
    { "Bluetooth",   w_bt,         icon_bt_big      },
    { "WiFi Ataki",  w_wifi_atk,   icon_atk_big     },
    { "Pwnagotchi",  w_pwn,        icon_pwn_big     },
    { "BadUSB BLE",  w_badusb,     icon_badusb_big  },
    { "OTA Update",  w_ota,        icon_ota_big     },
    { "Siec/Ping",   w_siec,       icon_null_big    },
    { "IR",          w_ir,         icon_ir_big      },
    { "RF CC1101",   w_rf,         icon_rf_big      },
    { "NRF24",       w_nrf,        icon_nrf_big     },
    { "GPS",         w_gps,        icon_gps_big     },
    { "FM Radio",    w_fm,         icon_fm_big      },
    { "IMU/Poziom",  w_imu,        icon_imu_big     },
    { "Oscyloskop",  w_osc,        icon_null_big    },
    { "Notatnik",    w_notes,      icon_note_big    },
    { "Kalkulator",  w_kalk,       icon_calc_big    },
    { "Rezystory",   w_rezyst,     icon_null_big    },
    { "Konwerter",   w_konw,       icon_null_big    },
    { "Zegar",       w_zegar,      icon_clock_big   },
    { "Narzedzia",   w_naorz,      icon_null_big    },
    { "MP3",         w_mp3,        icon_null_big    },
    { "Launcher",    w_launcher,   icon_null_big    },
    { "Uruch.bin",   w_bin,        icon_null_big    },
    { "Monitor",     w_monitor,    icon_sys_big     },
    { "Pliki SD",    w_pliki,      icon_null_big    },
    { "Przeglad.SD", w_sd,         icon_sd_big      },
    { "Gry",         w_gry,        icon_game_big    },
    { "Stoper",      w_stoper,     icon_clock_big   },
    { "Morse",       w_morse,      icon_morse_big   },
    { "Internet",    w_pogoda,     icon_internet_big},
    { "RFID RC522",  w_rfid,       icon_null_big    },
    { "Radio Net",   w_radio,      icon_fm_big      },
    { "Wygaszacz",   w_screensaver,icon_screen_big  },
    { "Ustawienia",  w_ustawienia, icon_gear_big    },
};
const int N_TILES = sizeof(TILES) / sizeof(TILES[0]);

int  aktTile = 0;
int  prevTile = -1;  // do partial redraw
unsigned long _last_activity = 0;
unsigned long _last_sb = 0;
bool full_redraw = true;

// ═══════════════════════════════════════════════════════
//  RYSOWANIE — partial update bez migotania
//
//  full_redraw = pełne odświeżenie (statusbar, ramka)
//  Przy zmianie kafelka odświeżamy TYLKO wnętrze ramki
//  (od y=24 do y=120, x=6 do x=234) — bez fillScreen!
// ═══════════════════════════════════════════════════════
void draw_tile_area() {
    // Tylko wnętrze ramki — żeby nie było migotania
    M5Cardputer.Display.startWrite();

    // Wymaż obszar ikony i nazwy
    M5Cardputer.Display.fillRect(6, 24, 228, 96, (uint32_t)UI_BG);

    // Strzałki na bokach (wewnątrz ramki)
    if (aktTile > 0)
        M5Cardputer.Display.fillTriangle(13,72, 22,62, 22,82, (uint32_t)UI_PRI);
    if (aktTile < N_TILES - 1)
        M5Cardputer.Display.fillTriangle(227,72, 218,62, 218,82, (uint32_t)UI_PRI);

    // Ikona
    TILES[aktTile].draw_icon((uint32_t)UI_PRI);

    M5Cardputer.Display.endWrite();
}

void draw_label_area() {
    M5Cardputer.Display.startWrite();
    // Wymaż obszar etykiety
    M5Cardputer.Display.fillRect(0, 113, 240, 22, (uint32_t)UI_BG);

    // Etykieta - duża czcionka
    M5Cardputer.Display.setTextSize(2);
    const char* lbl = TILES[aktTile].label;
    int llen = strlen(lbl) * 12;
    if (llen > 220) { M5Cardputer.Display.setTextSize(1); llen = strlen(lbl)*6; }
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor((240 - llen) / 2, 116);
    M5Cardputer.Display.print(lbl);
    M5Cardputer.Display.setTextSize(1);

    // Pasek pozycji na samym dole
    int barX = 5, barY = 132, barW = 230;
    M5Cardputer.Display.drawLine(barX, barY, barX+barW, barY, (uint32_t)UI_PRI & 0x2104);
    int markX = barX + aktTile * barW / max(1, N_TILES-1);
    M5Cardputer.Display.fillRect(markX-3, barY-2, 7, 5, (uint32_t)UI_PRI);

    M5Cardputer.Display.endWrite();
}

void draw_full() {
    M5Cardputer.Display.startWrite();
    M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
    draw_statusbar();
    M5Cardputer.Display.drawRoundRect(5, 24, 230, 88, 5, (uint32_t)UI_PRI);
    M5Cardputer.Display.endWrite();
    draw_tile_area();
    draw_label_area();
}

// ═══════════════════════════════════════════════════════
//  USTAWIENIA
// ═══════════════════════════════════════════════════════
struct ColorPreset { const char* name; uint16_t bg, fg, pri; };
static const ColorPreset PRESETS[] = {
    { "Bruce / biel",     0x0000, 0xFFFF, 0xFFFF },
    { "Neon rozowy",      0x0000, 0xF81F, 0xF81F },
    { "Matrix zielony",   0x0000, 0x07E0, 0x07E0 },
    { "Cyjan neon",       0x0000, 0x07FF, 0x07FF },
    { "Zolty neon",       0x0000, 0xFFE0, 0xFFE0 },
    { "Niebieski",        0x0000, 0x001F, 0x07FF },
    { "Pomaranczowy",     0x0000, 0xFB40, 0xFB40 },
    { "Czerwony",         0x0000, 0xF800, 0xF800 },
    { "Biale tlo",        0xFFFF, 0x0000, 0x0000 },
    { "Cyber Punk",       0x000F, 0x07FF, 0xF81F },
    { "Amber (terminal)", 0x0000, 0xFCC0, 0xFCC0 },
};
const int N_PRESETS = sizeof(PRESETS)/sizeof(PRESETS[0]);

void menu_preset_kolorow() {
    const char* names[N_PRESETS];
    for (int i=0; i<N_PRESETS; i++) names[i] = PRESETS[i].name;
    int sel = ui_select_list(names, N_PRESETS, "PRESETY KOLOROW", (uint32_t)UI_PRI);
    if (sel >= 0) {
        UI_BG  = PRESETS[sel].bg;
        UI_FG  = PRESETS[sel].fg;
        UI_PRI = PRESETS[sel].pri;
        save_colors();
        snd::confirm();
        ui_show_info("Zapisano!", (uint32_t)UI_PRI);
        delay(800);
    }
}

void menu_jasnosc() {
    int j = prefs.getInt("brightness", 128);
    bool redraw = true;
    while (true) {
        if (redraw) {
            ui_draw_header("JASNOSC EKRANU", (uint32_t)UI_PRI);
            M5Cardputer.Display.setTextSize(3);
            M5Cardputer.Display.setTextColor((uint32_t)UI_PRI,(uint32_t)UI_BG);
            char buf[8]; snprintf(buf,sizeof(buf),"%d%%",j*100/255);
            M5Cardputer.Display.setCursor(85,50); M5Cardputer.Display.print(buf);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.drawRect(20,90,200,8,(uint32_t)UI_PRI);
            M5Cardputer.Display.fillRect(21,91,196*j/255,6,(uint32_t)UI_PRI);
            M5Cardputer.Display.setTextColor((uint32_t)UI_PRI>>1&0x7BEF,(uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(8,108);
            M5Cardputer.Display.print(",/. zmien   ENTER=zapisz   `=anuluj");
            redraw=false;
        }
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto s = M5Cardputer.Keyboard.keysState();
            if (_is_left(s)  || _is_up(s))    { j=max(10,j-10); M5Cardputer.Display.setBrightness(j); snd::tick(); redraw=true; }
            if (_is_right(s) || _is_down(s))  { j=min(255,j+10); M5Cardputer.Display.setBrightness(j); snd::tick(); redraw=true; }
            if (_is_ok(s))   { prefs.putInt("brightness",j); snd::confirm(); return; }
            if (_is_esc(s))  { snd::cancel(); return; }
        }
        delay(10);
    }
}

void menu_screensaver_time() {
    const char* opts[] = {"30 sekund","1 minuta","90 sekund","2 minuty","5 minut","Wylaczony"};
    int times[] = {30, 60, 90, 120, 300, 0};
    int sel = ui_select_list(opts, 6, "CZAS WYGASZACZA", (uint32_t)UI_PRI);
    if (sel >= 0) {
        prefs.putInt("ss_time", times[sel]);
        prefs.putBool("ss_auto", times[sel] > 0);
        ui_show_info(times[sel]>0 ? "Zapisano!" : "Wylaczono", (uint32_t)UI_PRI);
        delay(1000);
    }
}

void w_ustawienia() {
    while (true) {
        const char* opts[] = {
            "Presety kolorow",
            "Edytor RGB",
            "Jasnosc ekranu",
            "Dzwiek (on/off)",
            "Wygaszacz - czas",
            "Wygaszacz - test",
            "Info systemu",
        };
        int sel = ui_select_list(opts, 7, "USTAWIENIA", (uint32_t)UI_PRI);
        if (sel < 0) return;
        if (sel==0) menu_preset_kolorow();
        if (sel==1) edit_custom_colors();
        if (sel==2) menu_jasnosc();
        if (sel==3) {
            bool e = snd::enabled();
            snd::set_enabled(!e);
            if (!e) snd::success();
            ui_show_info(!e ? "Dzwiek: ON" : "Dzwiek: OFF", (uint32_t)UI_PRI);
            delay(1200);
        }
        if (sel==4) menu_screensaver_time();
        if (sel==5) app_screensaver_menu();
        if (sel==6) {
            ui_draw_header("INFO SYSTEMU", (uint32_t)UI_PRI);
            M5Cardputer.Display.setTextColor((uint32_t)UI_FG,(uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(8,26); M5Cardputer.Display.print("PHANTOM OS v1.1");
            M5Cardputer.Display.setCursor(8,40); M5Cardputer.Display.print("ESP32-S3 StampS3");
            M5Cardputer.Display.setCursor(8,54); M5Cardputer.Display.print(sd_ok?"SD: OK":"SD: brak");
            M5Cardputer.Display.setCursor(8,68); M5Cardputer.Display.print(WiFi.status()==WL_CONNECTED?"WiFi: OK":"WiFi: brak");
            char bat[24]; snprintf(bat,sizeof(bat),"Bateria: %d%%%s",get_battery(),is_charging()?" CHG":"");
            M5Cardputer.Display.setCursor(8,82); M5Cardputer.Display.print(bat);
            char mem[24]; snprintf(mem,sizeof(mem),"RAM: %d KB free",ESP.getFreeHeap()/1024);
            M5Cardputer.Display.setCursor(8,96); M5Cardputer.Display.print(mem);
            M5Cardputer.Display.setTextColor((uint32_t)UI_PRI>>1&0x7BEF,(uint32_t)UI_BG);
            M5Cardputer.Display.setCursor(8,118); M5Cardputer.Display.print("ENTER lub ` = powrot");
            while (true) {
                M5Cardputer.update();
                if (M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
                    auto s = M5Cardputer.Keyboard.keysState();
                    if (_is_ok(s) || _is_esc(s)) break;
                }
                if (M5Cardputer.BtnA.wasPressed()) break;
                delay(10);
            }
        }
    }
}

// Linie 682 i wyżej
bool EscPress = false;
bool SelPress = false;
bool AnyKeyPress = false;
bool PrevPress = false;
bool NextPress = false;
char LastChar = 0;
int LastFn = 0;

void input_update() {
    M5Cardputer.update();
}


void setup() {
    auto cfg = M5.config();
    cfg.output_power = true;
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setSwapBytes(true);

    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);

    prefs.begin("cardputer_os", false);
    load_colors();

    int jasnosc = prefs.getInt("brightness", 128);
    if (jasnosc < 10) jasnosc = 128;
    M5Cardputer.Display.setRotation(prefs.getInt("rotation", 1));
    M5Cardputer.Display.setBrightness(jasnosc);
    M5Cardputer.Speaker.setVolume(64);

    draw_splash();
    snd::boot();

    // ─── INICJALIZACJA KARTY SD (Bruce-style, multi-attempt) ───
    // Cardputer/CardputerADV: SCK=40, MISO=39, MOSI=14, CS=12
    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);
    delay(50);

    // SPI z explicite podanymi pinami (kolejność Arduino: SCK, MISO, MOSI, SS)
    SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    delay(20);

    // Próba 1: standardowa szybkość 25MHz
    sd_ok = SD.begin(PIN_SD_CS, SPI, 25000000);
    // Próba 2: niższa szybkość (4MHz) jeśli pierwszej nie udało się
    if (!sd_ok) {
        SD.end();
        delay(50);
        sd_ok = SD.begin(PIN_SD_CS, SPI, 4000000);
    }
    // Próba 3: domyślne ustawienia
    if (!sd_ok) {
        SD.end();
        delay(50);
        sd_ok = SD.begin(PIN_SD_CS);
    }
    // Próba 4: re-init SPI
    if (!sd_ok) {
        SPI.end();
        delay(100);
        SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
        delay(50);
        sd_ok = SD.begin(PIN_SD_CS, SPI, 1000000);
    }

    if (sd_ok) {
        uint8_t ct = SD.cardType();
        Serial.printf("[SD] OK type=%d size=%lluMB\n", ct, SD.cardSize()/(1024*1024));
    } else {
        Serial.println("[SD] FAIL na wszystkich probach");
    }
    if (sd_ok) {
        SD.mkdir("/apps"); SD.mkdir("/music"); SD.mkdir("/notes");
        SD.mkdir("/gps"); SD.mkdir("/badusb"); SD.mkdir("/pwn");
    }

    String ssid = prefs.getString("ssid", "");
    if (ssid.length() > 0) {
        WiFi.begin(ssid.c_str(), prefs.getString("pass","").c_str());
        unsigned long t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis()-t0 < 6000) delay(100);
        if (WiFi.status() == WL_CONNECTED)
            configTime(3600,3600,"pool.ntp.org","time.google.com");
    }

    if (sd_ok) dodaj_log("PHANTOM OS v1.1");
    full_redraw = true;
    _last_activity = millis();
}

// ═══════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════
void loop() {
    // Pełne przerysowanie tylko gdy potrzeba
    if (full_redraw) {
        draw_full();
        full_redraw = false;
        prevTile = aktTile;
    }
    // Częściowe — gdy zmienił się kafelek
    else if (prevTile != aktTile) {
        draw_tile_area();
        draw_label_area();
        prevTile = aktTile;
    }

    // Statusbar co 30s
    if (millis() - _last_sb > 30000) {
        _last_sb = millis();
        _bat_cached = -1;
        draw_statusbar();
    }

    // Auto-screensaver
    int ss_time = prefs.getInt("ss_time", 90);
    if (ss_time > 0 && millis()-_last_activity > (unsigned long)ss_time*1000) {
        app_screensaver_matrix();
        _last_activity = millis();
        full_redraw = true;
    }

    // ─── KLAWIATURA — styl Bruce (globalne flagi) ─────
    input_update();
    if (AnyKeyPress) _last_activity = millis();

    // Nawigacja po menu (PrevPress = strzałki ↑← lub WA, NextPress = ↓→ lub SD)
    if (PrevPress) {
        snd::tick();
        aktTile = (aktTile - 1 + N_TILES) % N_TILES;
    }
    if (NextPress) {
        snd::tick();
        aktTile = (aktTile + 1) % N_TILES;
    }
    // SEL = ENTER lub BtnA
    if (SelPress) {
        snd::confirm();
        if (sd_ok) dodaj_log(String("START: ")+TILES[aktTile].label);
        TILES[aktTile].handler();
        full_redraw = true;
    }
    // ESC w głównym menu = nic (zostajemy w menu)

    // Skróty FN
    if (LastFn && LastChar) {
        if (LastChar=='b' || LastChar=='B') {
            int b = M5Cardputer.Display.getBrightness();
            M5Cardputer.Display.setBrightness(b > 0 ? 0 : prefs.getInt("brightness", 128));
        }
        if (LastChar=='m' || LastChar=='M') { app_screensaver_matrix(); full_redraw = true; }
        if (LastChar=='u' || LastChar=='U') { w_ustawienia(); full_redraw = true; }
    }

    delay(10);

}
