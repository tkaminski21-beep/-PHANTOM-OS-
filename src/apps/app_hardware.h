#pragma once
// apps/app_hardware.h — ADC, IR, I2C, Wygaszacz

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <Wire.h>
#include <math.h>

extern "C" {
  #include "driver/adc.h"
  #include "driver/gpio.h"
  #include "driver/ledc.h"
}

#define PIN_ADC    ADC1_CHANNEL_0   // GPIO1
#define IR_TX_PIN  GPIO_NUM_44
#define IR_RX_PIN  GPIO_NUM_0

// ─── MIERNIK ADC ────────────────────────────────────────
void miernik_adc() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(PIN_ADC, ADC_ATTEN_DB_12);

    float historia[120] = {};
    int   indeks = 0;
    const float MAX_V = 3.3f;

    while (true) {
        int   raw     = adc1_get_raw(PIN_ADC);
        float napiecie = (float)raw * MAX_V / 4095.0f;
        historia[indeks % 120] = napiecie;
        indeks++;

        // Cyfry
        M5Cardputer.Display.fillRect(0, 38, 240, 50, THEME_BG);
        uint32_t kol = napiecie > 2.5f ? THEME_RED : napiecie > 1.5f ? THEME_YELLOW : THEME_GREEN;
        M5Cardputer.Display.setTextColor(kol);
        M5Cardputer.Display.setTextSize(3);
        char vb[10]; snprintf(vb, sizeof(vb), "%.3fV", napiecie);
        M5Cardputer.Display.setCursor(30, 44);
        M5Cardputer.Display.print(vb);
        M5Cardputer.Display.setTextSize(1);

        // RAW
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 80);
        char rb[24]; snprintf(rb, sizeof(rb), "RAW: %d  (12-bit)", raw);
        M5Cardputer.Display.print(rb);

        // Wykres
        M5Cardputer.Display.fillRect(0, 94, 240, 80, THEME_PANEL);
        M5Cardputer.Display.drawRect(0, 94, 240, 80, THEME_BORDER);
        for (int i = 1; i < 120; i++) {
            int xi = i * 2;
            int j  = (indeks - 120 + i + 120) % 120;
            int jp = (j - 1 + 120) % 120;
            int y1 = 174 - (int)(historia[jp] * 75.0f / MAX_V);
            int y2 = 174 - (int)(historia[j]  * 75.0f / MAX_V);
            y1 = constrain(y1, 95, 173);
            y2 = constrain(y2, 95, 173);
            M5Cardputer.Display.drawLine(xi - 2, y1, xi, y2, THEME_GREEN);
        }

        // Pasek woltomierza
        M5Cardputer.Display.drawRect(4, 178, 232, 12, THEME_BORDER);
        M5Cardputer.Display.fillRect(5, 179, (int)(230.0f * napiecie / MAX_V), 10, kol);

        if (indeks == 1) {
            M5Cardputer.Display.fillRect(0, 0, 240, 20, THEME_PANEL);
            M5Cardputer.Display.drawLine(0, 20, 240, 20, THEME_ORANGE);
            M5Cardputer.Display.setTextColor(THEME_ORANGE);
            M5Cardputer.Display.setCursor(6, 6);
            M5Cardputer.Display.print("< MIERNIK ADC  GPIO1  0-3.3V");
        }

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 196);
        M5Cardputer.Display.print("Auto-pomiar  Q = wyjscie");

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (char c : st.word) if (c == 'q' || c == 'Q') return;
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
        delay(100);
    }
}

// ─── TESTER IR ──────────────────────────────────────────
// Ręczne nadawanie NEC przez bitbang (bez zewnętrznych bibliotek IR)
static void _ir_burst(int us) {
    uint32_t t = (uint32_t)micros();
    while ((uint32_t)micros() - t < (uint32_t)us) {
        gpio_set_level(IR_TX_PIN, 1); delayMicroseconds(13);
        gpio_set_level(IR_TX_PIN, 0); delayMicroseconds(13);
    }
}
static void _ir_space(int us) {
    gpio_set_level(IR_TX_PIN, 0);
    delayMicroseconds(us);
}

static void _ir_send_nec(uint8_t adres, uint8_t komenda) {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << IR_TX_PIN);
    cfg.mode = GPIO_MODE_OUTPUT;
    gpio_config(&cfg);
    gpio_set_level(IR_TX_PIN, 0);

    // Start
    _ir_burst(9000); _ir_space(4500);
    // 32 bity: adres, ~adres, komenda, ~komenda
    uint32_t data = ((uint32_t)adres)        |
                    ((uint32_t)(~adres) << 8) |
                    ((uint32_t)komenda  << 16)|
                    ((uint32_t)(~komenda)<< 24);
    for (int i = 0; i < 32; i++) {
        _ir_burst(562);
        _ir_space((data >> i) & 1 ? 1687 : 562);
    }
    _ir_burst(562);
}

void tester_ir() {
    while (true) {
        const char* opts[] = {
            "Nadaj kod NEC", "Odbierz kod IR",
            "Pilot TV (demo)", "Powrot"
        };
        int sel = ui_select_list(opts, 4, "TESTER IR", THEME_YELLOW);
        if (sel < 0 || sel == 3) return;

        if (sel == 0) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("NADAJ IR NEC", THEME_YELLOW);
            String addr_s = ui_input_string("Adres hex (np FF):", 4, 30, 4);
            String cmd_s  = ui_input_string("Komenda hex (np 45):", 4, 64, 4);
            if (addr_s.length() == 0) continue;
            uint8_t adr = (uint8_t)strtol(addr_s.c_str(), nullptr, 16);
            uint8_t cmd = (uint8_t)strtol(cmd_s.c_str(),  nullptr, 16);
            _ir_send_nec(adr, cmd);
            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setCursor(4, 100);
            char buf[40]; snprintf(buf, sizeof(buf), "Wyslano: ADDR=0x%02X CMD=0x%02X", adr, cmd);
            M5Cardputer.Display.print(buf);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 212); M5Cardputer.Display.print("Dowolny klawisz = powrot");
            ui_wait_key();

        } else if (sel == 1) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("ODBIERZ IR", THEME_YELLOW);
            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 30); M5Cardputer.Display.print("Oczekiwanie na sygnal...");
            M5Cardputer.Display.setCursor(4, 212); M5Cardputer.Display.print("Q = wyjscie");

            gpio_config_t cfg = {};
            cfg.pin_bit_mask = (1ULL << IR_RX_PIN);
            cfg.mode = GPIO_MODE_INPUT;
            cfg.pull_up_en = GPIO_PULLUP_ENABLE;
            gpio_config(&cfg);

            uint32_t kod = 0; int bity = 0;
            unsigned long timeout = millis() + 15000;
            int prev = gpio_get_level(IR_RX_PIN);

            while (millis() < timeout) {
                M5Cardputer.update();
                if (M5Cardputer.BtnA.wasPressed()) break;
                if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                    auto st = M5Cardputer.Keyboard.keysState();
                    bool q = false;
                    for (char c : st.word) if (c == 'q' || c == 'Q') q = true;
                    if (q) break;
                }
                int curr = gpio_get_level(IR_RX_PIN);
                if (prev == 1 && curr == 0) {
                    unsigned long t = micros();
                    while (gpio_get_level(IR_RX_PIN) == 0);
                    unsigned long dur = micros() - t;
                    if (dur > 8000) { kod = 0; bity = 0; }
                    else if (dur > 1000) { kod |= (1UL << bity); bity++; }
                    else bity++;
                    if (bity == 32) {
                        M5Cardputer.Display.fillRect(0, 48, 240, 50, THEME_BG);
                        M5Cardputer.Display.setTextColor(THEME_GREEN);
                        M5Cardputer.Display.setCursor(4, 50);
                        char rb[40]; snprintf(rb, sizeof(rb), "Kod NEC: 0x%08lX", (unsigned long)kod);
                        M5Cardputer.Display.print(rb);
                        M5Cardputer.Display.setCursor(4, 66);
                        snprintf(rb, sizeof(rb), "ADDR: 0x%02X  CMD: 0x%02X",
                                 (uint8_t)kod, (uint8_t)(kod >> 16));
                        M5Cardputer.Display.print(rb);
                        timeout = millis() + 5000;
                        kod = 0; bity = 0;
                    }
                }
                prev = curr;
            }

        } else if (sel == 2) {
            struct { const char* naz; uint8_t cmd; } p[] = {
                {"Wlacz/Wylacz", 0x45}, {"Glosniej",  0x46},
                {"Ciszej",       0x47}, {"Kanal+",    0x44},
                {"Kanal-",       0x43}, {"Mute",      0x0D},
            };
            const char* pn[6]; for (int i=0;i<6;i++) pn[i]=p[i].naz;
            int pick = ui_select_list(pn, 6, "PILOT TV", THEME_YELLOW);
            if (pick >= 0) {
                _ir_send_nec(0x00, p[pick].cmd);
                M5Cardputer.Display.fillScreen(THEME_BG);
                ui_draw_header("WYSLANO IR", THEME_GREEN);
                M5Cardputer.Display.setTextColor(THEME_GREEN);
                M5Cardputer.Display.setCursor(4, 50); M5Cardputer.Display.print("Wyslano: ");
                M5Cardputer.Display.print(p[pick].naz);
                delay(1000);
            }
        }
    }
}

// ─── SKANER I2C ─────────────────────────────────────────
void skaner_i2c() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("SKANER I2C", THEME_CYAN);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 24); M5Cardputer.Display.print("Skanuje 0x01-0x77...");

    Wire.begin();

    struct KnownDev { const char* addr; const char* name; };
    static const KnownDev znane[] = {
        {"0x3C","SSD1306 OLED"}, {"0x68","MPU6050/DS3231"},
        {"0x76","BME280"},       {"0x77","BMP280/BME680"},
        {"0x57","MAX30102"},     {"0x48","ADS1115"},
        {"0x40","INA219"},       {"0x70","TCA9548A"},
    };

    int y = 38; int znaleziono = 0;
    for (uint8_t adres = 1; adres < 0x78; adres++) {
        Wire.beginTransmission(adres);
        uint8_t blad = Wire.endTransmission();
        if (adres % 16 == 0) ui_progress(adres * 100 / 0x77, 210, THEME_CYAN);

        if (blad == 0) {
            znaleziono++;
            char hex[7]; snprintf(hex, sizeof(hex), "0x%02X", adres);
            const char* nazwa = nullptr;
            for (auto& u : znane) if (strcmp(hex, u.addr) == 0) { nazwa = u.name; break; }

            M5Cardputer.Display.setTextColor(THEME_GREEN);
            M5Cardputer.Display.setCursor(4, y); M5Cardputer.Display.print(hex);
            if (nazwa) {
                M5Cardputer.Display.setTextColor(THEME_CYAN);
                M5Cardputer.Display.setCursor(42, y); M5Cardputer.Display.print(nazwa);
            }
            y += 13;
            if (y > 198) break;
        }
    }

    if (znaleziono == 0) {
        M5Cardputer.Display.setTextColor(THEME_RED);
        M5Cardputer.Display.setCursor(4, 60); M5Cardputer.Display.print("Brak urzadzen I2C!");
    } else {
        M5Cardputer.Display.setTextColor(THEME_YELLOW);
        M5Cardputer.Display.setCursor(4, y + 4);
        M5Cardputer.Display.print("Znaleziono: "); M5Cardputer.Display.print(znaleziono);
    }
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 212); M5Cardputer.Display.print("Dowolny klawisz = powrot");
    ui_wait_key();
}

// ─── SKANER BLE (uproszczony — bez BLEDevice) ───────────
void skaner_ble() {
    // BLE i BT Classic nie mogą działać jednocześnie w tym samym firmware
    // bez specjalnej konfiguracji menuconfig. Pokazujemy info.
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("SKANER BLE", THEME_BLUE);
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 30);
    M5Cardputer.Display.print("BLE scan wymaga wylaczenia");
    M5Cardputer.Display.setCursor(4, 44);
    M5Cardputer.Display.print("BT Classic (SerialBT).");
    M5Cardputer.Display.setCursor(4, 62);
    M5Cardputer.Display.setTextColor(THEME_CYAN);
    M5Cardputer.Display.print("Uzyj osobnego firmware BLE");
    M5Cardputer.Display.setCursor(4, 76);
    M5Cardputer.Display.print("lub Bruce (ma BLE scan).");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 100);
    M5Cardputer.Display.print("Bluetooth Classic (SPP) dziala.");
    M5Cardputer.Display.setCursor(4, 212); M5Cardputer.Display.print("Dowolny klawisz = powrot");
    ui_wait_key();
}

// ─── WYGASZACZ EKRANU ───────────────────────────────────
void wygaszacz_ekranu() {
    const char* tryby[] = { "Matryca (Matrix)", "Gwiezdziste niebo", "Zegar bounce", "Powrot" };
    int sel = ui_select_list(tryby, 4, "WYGASZACZ EKRANU", THEME_GREEN);
    if (sel < 0 || sel == 3) return;

    auto sprawdz_wyjscie = []() -> bool {
        M5Cardputer.update();
        if (M5Cardputer.BtnA.wasPressed()) return true;
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) return true;
        return false;
    };

    if (sel == 0) {
        // Matrix
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        int kol_y[20] = {};
        for (int i = 0; i < 20; i++) kol_y[i] = random(0, 220);
        while (!sprawdz_wyjscie()) {
            for (int i = 0; i < 20; i++) {
                int x = i * 12;
                M5Cardputer.Display.setTextColor(TFT_BLACK);
                M5Cardputer.Display.setCursor(x, max(0, kol_y[i] - 28));
                M5Cardputer.Display.print((char)(random(33, 126)));
                M5Cardputer.Display.setTextColor(0x0320); // ciemnozielony
                M5Cardputer.Display.setCursor(x, kol_y[i]);
                M5Cardputer.Display.print((char)(random(33, 126)));
                M5Cardputer.Display.setTextColor(TFT_WHITE);
                if (kol_y[i] >= 14) {
                    M5Cardputer.Display.setCursor(x, kol_y[i] - 14);
                    M5Cardputer.Display.print((char)(random(33, 126)));
                }
                kol_y[i] = (kol_y[i] + 14) % 240;
            }
            delay(60);
        }
    } else if (sel == 1) {
        // Gwiazdki
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        struct Star { int x, y; uint8_t j; };
        Star gw[80];
        for (auto& g : gw) { g.x = random(240); g.y = random(220); g.j = random(80, 255); }
        while (!sprawdz_wyjscie()) {
            for (auto& g : gw) {
                uint16_t k = M5Cardputer.Display.color565(g.j, g.j, g.j);
                M5Cardputer.Display.drawPixel(g.x, g.y, k);
                g.j = (g.j < 100) ? 255 : g.j - 4;
                if (g.j == 255) { g.x = random(240); g.y = random(220); }
            }
            delay(30);
        }
    } else if (sel == 2) {
        // Zegar bounce
        int px = 40, py = 60, dx = 2, dy = 1;
        while (!sprawdz_wyjscie()) {
            M5Cardputer.Display.fillScreen(TFT_BLACK);
            struct tm ti; getLocalTime(&ti);
            char buf[9]; strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
            M5Cardputer.Display.setTextColor(THEME_CYAN);
            M5Cardputer.Display.setTextSize(2);
            M5Cardputer.Display.setCursor(px, py);
            M5Cardputer.Display.print(buf);
            M5Cardputer.Display.setTextSize(1);
            px += dx; py += dy;
            if (px < 2 || px > 128) dx = -dx;
            if (py < 2 || py > 200) dy = -dy;
            delay(1000);
        }
        M5Cardputer.Display.setTextSize(1);
    }
}

// ─── MENU HARDWARE ──────────────────────────────────────
void app_hardware_run() {
    while (true) {
        const char* opts[] = {
            "Miernik ADC / Napiecie",
            "Tester IR (NEC)",
            "Skaner I2C",
            "Skaner BLE (info)",
            "Wygaszacz ekranu",
            "Powrot"
        };
        int sel = ui_select_list(opts, 6, "HARDWARE", THEME_ORANGE);
        if (sel < 0 || sel == 5) return;
        switch (sel) {
            case 0: miernik_adc();      break;
            case 1: tester_ir();        break;
            case 2: skaner_i2c();       break;
            case 3: skaner_ble();       break;
            case 4: wygaszacz_ekranu(); break;
        }
    }
}
