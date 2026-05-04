#pragma once
// apps/app_oscyloskop.h — Oscyloskop, Analizator logiczny, Generator PWM, Miernik freq.

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <math.h>
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"

#define OSC_PIN     ADC1_CHANNEL_0  // GPIO1
#define LOGIC_PINS  4
static const int LOGIC_GPIO[LOGIC_PINS] = { 1, 2, 3, 4 }; // zmień wg potrzeb
#define PWM_PIN     GPIO_NUM_10
#define FREQ_PIN    GPIO_NUM_5

// ══════════════════════════════════════════════
//  OSCYLOSKOP
// ══════════════════════════════════════════════
#define OSC_SAMPLES 200
#define OSC_H       100
#define OSC_Y0      120

void oscyloskop() {
    // Konfiguracja ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(OSC_PIN, ADC_ATTEN_DB_12);

    uint16_t probki[OSC_SAMPLES];
    int       skala_czas  = 1;  // 1=szybko, 10=wolno
    float     skala_v     = 1.0f;
    bool      wyzwalanie  = false;
    int       prog_wyz    = 2048;
    bool      redraw_info = true;
    char      tryb_sprzeg = 'D'; // DC/AC

    while (true) {
        // Zbierz próbki
        if (wyzwalanie) {
            // Czekaj na zbocze narastające
            unsigned long timeout = millis() + 200;
            int prev = adc1_get_raw(OSC_PIN);
            while (millis() < timeout) {
                int curr = adc1_get_raw(OSC_PIN);
                if (prev < prog_wyz && curr >= prog_wyz) break;
                prev = curr;
            }
        }
        for (int i = 0; i < OSC_SAMPLES; i++) {
            probki[i] = adc1_get_raw(OSC_PIN);
            if (skala_czas > 1) delayMicroseconds(skala_czas * 10);
        }

        // Rysuj
        M5Cardputer.Display.fillScreen(THEME_BG);

        // Siatka
        for (int gx = 0; gx < 240; gx += 40)
            M5Cardputer.Display.drawFastVLine(gx, 20, OSC_H, 0x2104);
        for (int gy = OSC_Y0 - OSC_H/2; gy <= OSC_Y0 + OSC_H/2; gy += 25)
            M5Cardputer.Display.drawFastHLine(0, gy, 240, 0x2104);
        M5Cardputer.Display.drawFastHLine(0, OSC_Y0, 240, THEME_BORDER); // linia 0V

        // Rysuj sygnał
        uint16_t min_v = 4095, max_v = 0;
        for (int i = 0; i < OSC_SAMPLES; i++) {
            if (probki[i] < min_v) min_v = probki[i];
            if (probki[i] > max_v) max_v = probki[i];
        }
        for (int i = 1; i < OSC_SAMPLES; i++) {
            int x1 = (i - 1) * 240 / OSC_SAMPLES;
            int x2 = i * 240 / OSC_SAMPLES;
            int y1 = OSC_Y0 + OSC_H/2 - (int)((float)probki[i-1] / 4095 * OSC_H * skala_v);
            int y2 = OSC_Y0 + OSC_H/2 - (int)((float)probki[i]   / 4095 * OSC_H * skala_v);
            y1 = constrain(y1, OSC_Y0 - OSC_H/2, OSC_Y0 + OSC_H/2);
            y2 = constrain(y2, OSC_Y0 - OSC_H/2, OSC_Y0 + OSC_H/2);
            M5Cardputer.Display.drawLine(x1, y1, x2, y2, THEME_GREEN);
        }

        // Pomiary
        float vpp  = (max_v - min_v) * 3.3f / 4095;
        float vavg = 0;
        for (int i = 0; i < OSC_SAMPLES; i++) vavg += probki[i];
        vavg = vavg / OSC_SAMPLES * 3.3f / 4095;

        // Estymacja częstotliwości przez zero-crossings
        int zero = (max_v + min_v) / 2;
        int przejscia = 0;
        for (int i = 1; i < OSC_SAMPLES; i++)
            if (probki[i-1] < zero && probki[i] >= zero) przejscia++;
        float freq_est = (przejscia > 0) ? (float)przejscia * 1000000.0f / (OSC_SAMPLES * skala_czas * 10 + OSC_SAMPLES) : 0;

        // Nagłówek info
        M5Cardputer.Display.fillRect(0, 0, 240, 18, THEME_PANEL);
        M5Cardputer.Display.setTextColor(THEME_GREEN);
        M5Cardputer.Display.setCursor(2, 4);
        char ib[60];
        snprintf(ib, sizeof(ib), "Vpp:%.2fV Vavg:%.2fV ~%.0fHz T:%d %c",
                 vpp, vavg, freq_est, skala_czas, tryb_sprzeg);
        M5Cardputer.Display.print(ib);

        // Wyzwalanie marker
        if (wyzwalanie) {
            int wy = OSC_Y0 + OSC_H/2 - (int)((float)prog_wyz / 4095 * OSC_H * skala_v);
            M5Cardputer.Display.drawFastHLine(0, wy, 10, THEME_YELLOW);
            M5Cardputer.Display.drawTriangle(10, wy, 16, wy-4, 16, wy+4, THEME_YELLOW);
        }

        M5Cardputer.Display.fillRect(0, 124, 240, 11, THEME_PANEL);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(2, 126);
        M5Cardputer.Display.print("+/-=skala T=czas W=wyzw. C=sprz. Q=wr");

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                if (c == '+') skala_v = min(4.0f, skala_v * 2);
                if (c == '-') skala_v = max(0.25f, skala_v / 2);
                if (c == 't' || c == 'T') skala_czas = (skala_czas == 1) ? 5 : (skala_czas == 5) ? 20 : 1;
                if (c == 'w' || c == 'W') { wyzwalanie = !wyzwalanie; }
                if (c == 'c' || c == 'C') tryb_sprzeg = (tryb_sprzeg == 'D') ? 'A' : 'D';
                if (c == 'q' || c == 'Q') return;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
    }
}

// ══════════════════════════════════════════════
//  ANALIZATOR LOGICZNY
// ══════════════════════════════════════════════
#define LA_SAMPLES  200

void analizator_logiczny() {
    for (int i = 0; i < LOGIC_PINS; i++) {
        gpio_config_t cfg = {};
        cfg.pin_bit_mask = (1ULL << LOGIC_GPIO[i]);
        cfg.mode = GPIO_MODE_INPUT;
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&cfg);
    }

    uint8_t dane[LOGIC_PINS][LA_SAMPLES] = {};
    int predkosc = 1; // ms między próbkami

    while (true) {
        // Zbierz próbki
        for (int s = 0; s < LA_SAMPLES; s++) {
            for (int p = 0; p < LOGIC_PINS; p++)
                dane[p][s] = gpio_get_level((gpio_num_t)LOGIC_GPIO[p]);
            if (predkosc > 0) delay(predkosc);
        }

        // Rysuj
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("ANALIZATOR LOGICZNY", THEME_CYAN);

        static const uint32_t KOLORY_LA[] = { THEME_GREEN, THEME_CYAN, THEME_YELLOW, THEME_ORANGE };
        int kanalH = 28;

        for (int p = 0; p < LOGIC_PINS; p++) {
            int y_base = 24 + p * kanalH;

            // Etykieta
            M5Cardputer.Display.setTextColor(KOLORY_LA[p]);
            M5Cardputer.Display.setCursor(2, y_base + 8);
            char lab[8]; snprintf(lab, sizeof(lab), "G%d", LOGIC_GPIO[p]);
            M5Cardputer.Display.print(lab);

            // Sygnał
            int prev_x = 20, prev_y = y_base + (dane[p][0] ? 4 : kanalH - 4);
            for (int s = 1; s < LA_SAMPLES; s++) {
                int x = 20 + s * 220 / LA_SAMPLES;
                int y = y_base + (dane[p][s] ? 4 : kanalH - 8);
                if (dane[p][s] != dane[p][s-1]) {
                    // Zbocze pionowe
                    M5Cardputer.Display.drawFastVLine(x, y_base + 4, kanalH - 12, KOLORY_LA[p]);
                }
                M5Cardputer.Display.drawLine(prev_x, prev_y, x, prev_y, KOLORY_LA[p]);
                prev_x = x; prev_y = y;
            }
            M5Cardputer.Display.drawLine(prev_x, prev_y, 240, prev_y, KOLORY_LA[p]);

            // Linia podziału
            M5Cardputer.Display.drawFastHLine(20, y_base + kanalH - 1, 220, THEME_BORDER);
        }

        // Zlicz zbocza
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 140);
        M5Cardputer.Display.print("Zbocza: ");
        for (int p = 0; p < LOGIC_PINS; p++) {
            int zbocza = 0;
            for (int s = 1; s < LA_SAMPLES; s++)
                if (dane[p][s] != dane[p][s-1]) zbocza++;
            M5Cardputer.Display.setTextColor(KOLORY_LA[p]);
            char bb[8]; snprintf(bb, sizeof(bb), "G%d:%d ", LOGIC_GPIO[p], zbocza);
            M5Cardputer.Display.print(bb);
        }

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 154);
        char sp[24]; snprintf(sp, sizeof(sp), "Predkosc: %dms/probe  +/-=zm", predkosc);
        M5Cardputer.Display.print(sp);
        M5Cardputer.Display.setCursor(4, 166);
        M5Cardputer.Display.print("ENTER/SPACJA=ponow  Q=wyjscie");

        bool wyjdz = false, ponow = false;
        while (!ponow && !wyjdz) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                auto st = M5Cardputer.Keyboard.keysState();
                for (auto c : st.word) {
                    if (c == ' ' || c == '\n') ponow = true;
                    if (c == '+') predkosc = min(100, predkosc * 2);
                    if (c == '-') predkosc = max(0, predkosc / 2);
                    if (c == 'q' || c == 'Q') wyjdz = true;
                }
            }
            if (M5Cardputer.BtnA.wasPressed()) wyjdz = true;
            delay(10);
        }
        if (wyjdz) return;
    }
}

// ══════════════════════════════════════════════
//  GENERATOR PWM
// ══════════════════════════════════════════════
void generator_pwm() {
    int freq      = 1000;  // Hz
    int wypelnienie = 50;  // %
    bool aktywny  = false;

    // Konfiguracja LEDC
    ledc_timer_config_t timer = {};
    timer.speed_mode      = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_10_BIT;
    timer.timer_num       = LEDC_TIMER_0;
    timer.freq_hz         = freq;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
    timer.clk_cfg         = LEDC_AUTO_CLK;
#endif

    ledc_channel_config_t channel = {};
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel    = LEDC_CHANNEL_0;
    channel.timer_sel  = LEDC_TIMER_0;
    channel.gpio_num   = PWM_PIN;
    channel.duty       = 0;
    channel.hpoint     = 0;

    bool redraw = true;

    while (true) {
        if (redraw) {
            M5Cardputer.Display.fillScreen(THEME_BG);
            ui_draw_header("GENERATOR PWM", THEME_ORANGE);

            // Wizualizacja przebiegu PWM
            int y0 = 80, h = 30, szerok = 200;
            int wys_wys = (int)(szerok * wypelnienie / 100);
            M5Cardputer.Display.drawRect(20, y0, szerok, h + 10, THEME_BORDER);

            // Wysokie
            M5Cardputer.Display.fillRect(20, y0, wys_wys, h / 2, aktywny ? THEME_ORANGE : THEME_MUTED);
            // Niskie
            M5Cardputer.Display.fillRect(20 + wys_wys, y0 + h / 2, szerok - wys_wys, h / 2,
                                          aktywny ? 0x2104 : THEME_BORDER);
            // Linie
            M5Cardputer.Display.drawFastVLine(20 + wys_wys, y0, h, aktywny ? THEME_ORANGE : THEME_MUTED);

            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 28);
            M5Cardputer.Display.print("Pin wyjsciowy: GPIO");
            M5Cardputer.Display.print(PWM_PIN);

            // Parametry
            auto param = [&](const char* lab, String val, uint32_t kol, int y) {
                M5Cardputer.Display.setTextColor(THEME_MUTED);
                M5Cardputer.Display.setCursor(4, y);
                M5Cardputer.Display.print(lab);
                M5Cardputer.Display.setTextColor(kol);
                M5Cardputer.Display.setCursor(130, y);
                M5Cardputer.Display.print(val);
            };

            param("Czestotliwosc:", String(freq) + " Hz", THEME_ORANGE, 120);
            param("Wypelnienie:", String(wypelnienie) + "%", THEME_YELLOW, 134);
            param("Okres:", String(1000000 / freq) + " us", THEME_CYAN, 148);
            float czas_wys = (float)wypelnienie / freq / 100 * 1000;
            char tb[16]; snprintf(tb, sizeof(tb), "%.2f ms", czas_wys);
            param("Czas wysoki:", tb, THEME_GREEN, 162);

            M5Cardputer.Display.setTextColor(aktywny ? THEME_GREEN : THEME_RED);
            M5Cardputer.Display.setCursor(4, 176);
            M5Cardputer.Display.print(aktywny ? "STATUS: AKTYWNY" : "STATUS: STOP");

            M5Cardputer.Display.setTextColor(THEME_MUTED);
            M5Cardputer.Display.setCursor(4, 200);
            M5Cardputer.Display.print("F/V=freq +/-=wypeln ENTER=start Q=wr");
            redraw = false;
        }

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) {
                bool zmiana = false;
                if (c == 'f' || c == 'F') { freq = min(100000, freq * 2); zmiana = true; }
                if (c == 'v' || c == 'V') { freq = max(1, freq / 2); zmiana = true; }
                if (c == '+') { wypelnienie = min(99, wypelnienie + 5); zmiana = true; }
                if (c == '-') { wypelnienie = max(1,  wypelnienie - 5); zmiana = true; }
                if (c == '\n' || c == ' ') {
                    aktywny = !aktywny;
                    if (aktywny) {
                        timer.freq_hz = freq;
                        ledc_timer_config(&timer);
                        ledc_channel_config(&channel);
                        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                                      (1023 * wypelnienie / 100));
                        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    } else {
                        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    }
                    zmiana = true;
                }
                if (zmiana && aktywny) {
                    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1023 * wypelnienie / 100);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                }
                if (c == 'q' || c == 'Q') {
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                    return;
                }
                if (zmiana) redraw = true;
            }
        }
        if (M5Cardputer.BtnA.wasPressed()) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            return;
        }
        delay(10);
    }
}

// ══════════════════════════════════════════════
//  MIERNIK CZĘSTOTLIWOŚCI
// ══════════════════════════════════════════════
void miernik_czestotliwosci() {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << FREQ_PIN);
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&cfg);

    static float historia_freq[100] = {};
    static int idx_f = 0;

    while (true) {
        // Zlicz zbocza przez 100ms
        int zbocza = 0;
        int prev = gpio_get_level(FREQ_PIN);
        unsigned long koniec = millis() + 100;
        while (millis() < koniec) {
            int curr = gpio_get_level(FREQ_PIN);
            if (curr == 1 && prev == 0) zbocza++;
            prev = curr;
        }
        float freq = zbocza * 10.0f; // zbocza/100ms -> Hz

        historia_freq[idx_f % 100] = freq;
        idx_f++;

        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("MIERNIK CZESTOTLIWOSCI", THEME_CYAN);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 24);
        M5Cardputer.Display.print("Pin: GPIO"); M5Cardputer.Display.print(FREQ_PIN);

        // Duży wynik
        M5Cardputer.Display.setTextColor(THEME_CYAN);
        M5Cardputer.Display.setTextSize(2);
        char fb[20];
        if (freq >= 1000) snprintf(fb, sizeof(fb), "%.2f kHz", freq / 1000);
        else snprintf(fb, sizeof(fb), "%.1f Hz", freq);
        M5Cardputer.Display.setCursor(20, 50);
        M5Cardputer.Display.print(fb);
        M5Cardputer.Display.setTextSize(1);

        // Okres
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 80);
        if (freq > 0) {
            char pb[24]; snprintf(pb, sizeof(pb), "Okres: %.2f us", 1000000.0f / freq);
            M5Cardputer.Display.print(pb);
        }

        // Wykres historii
        M5Cardputer.Display.drawRect(4, 96, 232, 60, THEME_BORDER);
        float maks = 1.0f;
        for (int i = 0; i < 100; i++) if (historia_freq[i] > maks) maks = historia_freq[i];
        for (int i = 1; i < 100; i++) {
            int j  = (idx_f - 100 + i + 100) % 100;
            int jp = (j - 1 + 100) % 100;
            int y1 = 155 - (int)(historia_freq[jp] / maks * 56);
            int y2 = 155 - (int)(historia_freq[j]  / maks * 56);
            M5Cardputer.Display.drawLine(4 + i * 2 - 2, y1, 4 + i * 2, y2, THEME_CYAN);
        }

        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 168);
        M5Cardputer.Display.print("Zbocza/100ms: "); M5Cardputer.Display.print(zbocza);
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("Auto-pomiar co 100ms  Q=wyjscie");

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            for (auto c : st.word) if (c == 'q' || c == 'Q') return;
        }
        if (M5Cardputer.BtnA.wasPressed()) return;
    }
}

void app_oscyloskop_run() {
    while (true) {
        const char* opts[] = {
            "Oscyloskop ADC",
            "Analizator logiczny",
            "Generator PWM",
            "Miernik czestotliwosci",
            "Powrot"
        };
        int sel = ui_select_list(opts, 5, "NARZEDZIA ELEKTRONICZNE", THEME_ORANGE);
        if (sel < 0 || sel == 4) return;
        switch (sel) {
            case 0: oscyloskop();               break;
            case 1: analizator_logiczny();       break;
            case 2: generator_pwm();             break;
            case 3: miernik_czestotliwosci();    break;
        }
    }
}
