#pragma once
// app_akcelerometr.h — BMI270 / IMU dla CardputerOS
// Poziomnica, gra kulka, dane raw
// Naprawka: M5Cardputer.Imu nie istnieje na StampS3 — używamy M5.Imu

#include "M5Cardputer.h"
#include "core/theme.h"
#include "core/ui.h"
#include <math.h>

extern uint16_t UI_BG, UI_FG, UI_PRI;

// ─── Odczyt IMU ───────────────────────────────────────
// Na CardputerADV ze StampS3 IMU = BMI270 przez M5.Imu
static float _ax = 0, _ay = 0, _az = 1.0f;
static float _gx = 0, _gy = 0, _gz = 0;
static bool  _imu_ok = false;

static void _imu_update() {
    // M5Unified na S3: getAccelData / getGyroData
    float ax, ay, az;
    _imu_ok = M5.Imu.getAccelData(&ax, &ay, &az);
    if (_imu_ok) {
        // Łagodne filtrowanie (low-pass)
        _ax = _ax * 0.7f + ax * 0.3f;
        _ay = _ay * 0.7f + ay * 0.3f;
        _az = _az * 0.7f + az * 0.3f;
        float gx, gy, gz;
        M5.Imu.getGyroData(&gx, &gy, &gz);
        _gx = gx; _gy = gy; _gz = gz;
    }
}

static bool _imu_esc() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
    auto s = M5Cardputer.Keyboard.keysState();
    for (char c : s.word) { if (c==27||c=='q'||c=='Q') return true; }
    return false;
}

// ─── POZIOMNICA ───────────────────────────────────────
void app_poziomnica() {
    // Upewnij się że IMU jest włączone
    M5.Imu.begin();

    M5Cardputer.Display.fillScreen((uint32_t)UI_BG);

    // Sprawdź czy IMU w ogóle odpowiada
    float tx, ty, tz;
    bool imuTest = M5.Imu.getAccelData(&tx, &ty, &tz);
    if (!imuTest) {
        M5Cardputer.Display.setTextColor(0xF800, (uint32_t)UI_BG);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(20, 50);
        M5Cardputer.Display.print("Brak IMU / BMI270!");
        M5Cardputer.Display.setCursor(20, 65);
        M5Cardputer.Display.print("Sprawdz I2C na plytce");
        delay(3000); return;
    }

    // Układ ekranu
    const int CX = 120, CY = 72;
    const int R  = 42;   // promień okręgu
    const int BRA = 10;   // promień kulki

    // Nagłówek
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(86, 5);
    M5Cardputer.Display.print("POZIOMNICA");
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF, (uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(4, 127);
    M5Cardputer.Display.print("ESC=wyjdz  Q=wyjdz");

    // Stały okrąg
    M5Cardputer.Display.drawCircle(CX, CY, R, (uint32_t)UI_PRI);
    M5Cardputer.Display.drawCircle(CX, CY, R/3, (uint32_t)UI_PRI & 0x7BEF);
    M5Cardputer.Display.drawLine(CX-R, CY, CX+R, CY, (uint32_t)UI_PRI & 0x4208);
    M5Cardputer.Display.drawLine(CX, CY-R, CX, CY+R, (uint32_t)UI_PRI & 0x4208);

    int last_bx = -999, last_by = -999;

    while (!_imu_esc()) {
        M5Cardputer.update();
        _imu_update();

        // Pozycja kulki — ax = przechylenie lewo/prawo, ay = przód/tył
        // Skala: przy 1g przechyleniu kulka jest na krawędzi
        int dx = (int)(_ax * R * 0.9f);
        int dy = (int)(_ay * R * 0.9f);

        // Ogranicz do promienia - BR żeby kulka nie wychodziła
        float len = sqrtf((float)(dx*dx + dy*dy));
        if (len > (float)(R - BR)) {
            float sc = (float)(R - BR) / len;
            dx = (int)(dx * sc);
            dy = (int)(dy * sc);
        }

        int bx = CX + dx;
        int by = CY + dy;

        if (bx != last_bx || by != last_by) {
            // Wymaż starą kulkę (narysuj tło)
            if (last_bx > 0) {
                M5Cardputer.Display.fillCircle(last_bx, last_by, BR+1, (uint32_t)UI_BG);
                // Odśwież okrąg jeśli kulka go zachodziła
                M5Cardputer.Display.drawCircle(CX, CY, R, (uint32_t)UI_PRI);
                M5Cardputer.Display.drawCircle(CX, CY, R/3, (uint32_t)UI_PRI & 0x7BEF);
                M5Cardputer.Display.drawLine(CX-R, CY, CX+R, CY, (uint32_t)UI_PRI & 0x4208);
                M5Cardputer.Display.drawLine(CX, CY-R, CX, CY+R, (uint32_t)UI_PRI & 0x4208);
            }

            // Kolor kulki: zielony w środku, accent poza
            bool centered = (len < (float)(R/4));
            uint32_t ballColor = centered ? 0x07E0 : (uint32_t)UI_PRI;
            M5Cardputer.Display.fillCircle(bx, by, BR, ballColor);

            // Kąty
            float pitch = atan2f(_ay, _az) * 180.0f / (float)M_PI;
            float roll  = atan2f(_ax, _az) * 180.0f / (float)M_PI;

            M5Cardputer.Display.fillRect(5, 110, 230, 14, (uint32_t)UI_BG);
            M5Cardputer.Display.setTextColor(centered ? 0x07E0 : (uint32_t)UI_FG, (uint32_t)UI_BG);
            M5Cardputer.Display.setTextSize(1);
            char buf[48];
            snprintf(buf, sizeof(buf), "P:%+5.1f  R:%+5.1f  %s",
                     pitch, roll, centered ? "POZIOMO!" : "");
            M5Cardputer.Display.setCursor(8, 113);
            M5Cardputer.Display.print(buf);

            last_bx = bx; last_by = by;
        }

        delay(40);
    }
}

// ─── GRA KULKA ────────────────────────────────────────
void app_kulka_gra() {
    M5.Imu.begin();

    M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI, (uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(84, 5);
    M5Cardputer.Display.print("GRA: KULKA");

    struct Wall { int x, y, w, h; };
    const Wall walls[] = {
        {0,18,240,4},{0,131,240,4},{0,18,4,116},{236,18,4,116},
        {40,38,80,4},{120,58,80,4},{40,78,100,4},{80,98,80,4},
    };
    const int NW = 8;
    const int META_X=210, META_Y=108, META_R=7;

    float bx=20, by=28, vx=0, vy=0;
    const int BRA=5;
    unsigned long tStart = millis();
    bool won = false;

    while (!_imu_esc()) {
        M5Cardputer.update();
        _imu_update();

        vx += _ax * 0.9f; vy += _ay * 0.9f;
        vx *= 0.84f; vy *= 0.84f;
        bx += vx; by += vy;

        // Kolizje
        for (int i = 0; i < NW; i++) {
            const Wall& w = walls[i];
            if (bx+BR>w.x && bx-BR<w.x+w.w && by+BR>w.y && by-BR<w.y+w.h) {
                if (fabsf(vx) > fabsf(vy)) { vx = -vx*0.5f; bx += vx; }
                else                        { vy = -vy*0.5f; by += vy; }
            }
        }

        M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
        for (int i=0;i<NW;i++)
            M5Cardputer.Display.fillRect(walls[i].x,walls[i].y,walls[i].w,walls[i].h,(uint32_t)UI_PRI);
        M5Cardputer.Display.fillCircle(META_X,META_Y,META_R,0x07E0);
        M5Cardputer.Display.fillCircle((int)bx,(int)by,BR,(uint32_t)UI_PRI);

        unsigned long el = (millis()-tStart)/1000;
        M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF,(uint32_t)UI_BG);
        M5Cardputer.Display.setCursor(4,128);
        char buf[24]; snprintf(buf,sizeof(buf),"Czas: %lus  ESC=stop",el);
        M5Cardputer.Display.print(buf);

        float dx = bx-META_X, dy = by-META_Y;
        if (sqrtf(dx*dx+dy*dy) < BR+META_R) { won=true; break; }
        delay(20);
    }

    if (won) {
        M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
        M5Cardputer.Display.setTextColor(0x07E0,(uint32_t)UI_BG);
        M5Cardputer.Display.setTextSize(3);
        M5Cardputer.Display.setCursor(52,40); M5Cardputer.Display.print("BRAWO!");
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setTextColor((uint32_t)UI_FG,(uint32_t)UI_BG);
        M5Cardputer.Display.setCursor(60,80);
        char buf[32]; snprintf(buf,sizeof(buf),"Czas: %lus",(millis()-tStart)/1000);
        M5Cardputer.Display.print(buf);
        delay(3000);
    }
}

// ─── DANE IMU RAW ─────────────────────────────────────
void app_imu_dane() {
    M5.Imu.begin();

    M5Cardputer.Display.fillScreen((uint32_t)UI_BG);
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI,(uint32_t)UI_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(62,5); M5Cardputer.Display.print("DANE IMU (BMI270)");
    M5Cardputer.Display.drawLine(0,18,240,18,(uint32_t)UI_PRI);
    M5Cardputer.Display.setTextColor((uint32_t)UI_PRI & 0x7BEF,(uint32_t)UI_BG);
    M5Cardputer.Display.setCursor(4,127); M5Cardputer.Display.print("ESC=wyjdz");

    while (!_imu_esc()) {
        M5Cardputer.update();
        _imu_update();

        M5Cardputer.Display.fillRect(0,22,240,100,(uint32_t)UI_BG);
        M5Cardputer.Display.setTextSize(1);

        M5Cardputer.Display.setTextColor((uint32_t)UI_PRI,(uint32_t)UI_BG);
        M5Cardputer.Display.setCursor(4,24); M5Cardputer.Display.print("AKCELEROMETR (g):");
        M5Cardputer.Display.setTextColor((uint32_t)UI_FG,(uint32_t)UI_BG);
        char buf[40];
        snprintf(buf,sizeof(buf),"  X: %+.4f",_ax); M5Cardputer.Display.setCursor(4,36); M5Cardputer.Display.print(buf);
        snprintf(buf,sizeof(buf),"  Y: %+.4f",_ay); M5Cardputer.Display.setCursor(4,48); M5Cardputer.Display.print(buf);
        snprintf(buf,sizeof(buf),"  Z: %+.4f",_az); M5Cardputer.Display.setCursor(4,60); M5Cardputer.Display.print(buf);

        M5Cardputer.Display.setTextColor((uint32_t)UI_PRI,(uint32_t)UI_BG);
        M5Cardputer.Display.setCursor(4,74); M5Cardputer.Display.print("ZYROSKOP (deg/s):");
        M5Cardputer.Display.setTextColor((uint32_t)UI_FG,(uint32_t)UI_BG);
        snprintf(buf,sizeof(buf),"  X: %+.2f",_gx); M5Cardputer.Display.setCursor(4,86); M5Cardputer.Display.print(buf);
        snprintf(buf,sizeof(buf),"  Y: %+.2f",_gy); M5Cardputer.Display.setCursor(4,98); M5Cardputer.Display.print(buf);
        snprintf(buf,sizeof(buf),"  Z: %+.2f",_gz); M5Cardputer.Display.setCursor(4,110); M5Cardputer.Display.print(buf);

        delay(80);
    }
}

// ─── MENU ─────────────────────────────────────────────
void app_akcelerometr_menu() {
    while (true) {
        const char* opts[] = {"Poziomnica","Gra: Kulka","Dane IMU raw"};
        int sel = ui_select_list(opts, 3, "AKCELEROMETR / IMU", (uint32_t)UI_PRI);
        if (sel < 0) return;
        if (sel == 0) app_poziomnica();
        if (sel == 1) app_kulka_gra();
        if (sel == 2) app_imu_dane();
    }
}
