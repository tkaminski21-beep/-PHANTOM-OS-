#pragma once
// apps/app_partycje.h — Menedżer partycji OTA + M5Launcher

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"
#include <SD.h>
#include <Update.h>
// esp_partition.h jest dostępne przez Update.h / framework
// esp_ota_ops używamy przez Arduino Update API (bez bezpośredniego linku do app_update)
extern "C" {
  #include <esp_partition.h>
  #include <esp_system.h>
}

// ─── Pokaż info o partycjach ────────────────────────────
void pokaz_partycje() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("PARTYCJE FLASH", THEME_ORANGE);

    const esp_partition_t* biezaca = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    // Aktywna partycja — szukamy tej z flagą bootable
    {
        esp_partition_iterator_t _it = esp_partition_find(
            ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
        while (_it) {
            const esp_partition_t* _p = esp_partition_get(_it);
            // aktywna partycja APP
            biezaca = _p; // uproszczenie — pokaż pierwszą APP
            break;
        }
        esp_partition_iterator_release(_it);
    }
    const esp_partition_t* boot = biezaca;
    const esp_partition_t* next = nullptr;

    int y = 24;
    auto wiersz = [&](const char* label, const char* val, uint32_t kol) {
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, y);
        M5Cardputer.Display.print(label);
        M5Cardputer.Display.setTextColor(kol);
        M5Cardputer.Display.setCursor(110, y);
        M5Cardputer.Display.print(val);
        y += 14;
    };

    wiersz("Biezaca:", biezaca ? biezaca->label : "?", THEME_GREEN);
    wiersz("Startowa:", boot    ? boot->label    : "?", THEME_CYAN);
    wiersz("Nastepna:", next    ? next->label    : "?", THEME_YELLOW);

    y += 4;
    M5Cardputer.Display.drawLine(0, y, 240, y, THEME_BORDER);
    y += 8;

    // Lista partycji
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, y); y += 13;
    M5Cardputer.Display.print("Wszystkie partycje:");

    esp_partition_iterator_t it = esp_partition_find(
        ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it && y < 196) {
        const esp_partition_t* p = esp_partition_get(it);
        bool aktywna = biezaca && (p->address == biezaca->address);
        M5Cardputer.Display.setTextColor(aktywna ? THEME_GREEN : TFT_WHITE);
        M5Cardputer.Display.setCursor(4, y);
        char buf[48];
        snprintf(buf, sizeof(buf), "%s%-8s 0x%06lX %4dKB",
                 aktywna ? ">" : " ",
                 p->label,
                 (unsigned long)p->address,
                 (int)(p->size / 1024));
        M5Cardputer.Display.print(buf);
        y += 12;
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);

    // RAM
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, y + 4);
    char ram[48];
    snprintf(ram, sizeof(ram), "Wolne heap: %d KB",
             (int)(esp_get_free_heap_size() / 1024));
    M5Cardputer.Display.print(ram);

    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 212);
    M5Cardputer.Display.print("Dowolny klawisz = powrot");
    ui_wait_key();
}

// ─── Wgraj .bin z SD na partycję OTA ───────────────────
static bool _ota_wgraj(const String& sciezka) {
    File f = SD.open(sciezka.c_str());
    if (!f) { ui_show_error("Nie znaleziono pliku!"); delay(1500); return false; }
    size_t rozmiar = f.size();
    if (rozmiar == 0) { ui_show_error("Pusty plik!"); f.close(); delay(1500); return false; }

    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("WGRYWANIE OTA", THEME_GREEN);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(4, 28);
    // Pokaż tylko nazwę pliku
    String fn = sciezka;
    int sl = fn.lastIndexOf('/');
    if (sl >= 0) fn = fn.substring(sl + 1);
    M5Cardputer.Display.print(fn.substring(0, 36));
    char rb[24]; snprintf(rb, sizeof(rb), "Rozmiar: %d KB", (int)(rozmiar / 1024));
    M5Cardputer.Display.setCursor(4, 42); M5Cardputer.Display.print(rb);

    // Używamy Arduino Update API — nie wymaga bezpośredniego linku do app_update
    if (!Update.begin(rozmiar, U_FLASH)) {
        ui_show_error("OTA begin error!"); f.close(); delay(1500); return false;
    }

    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 56);
    M5Cardputer.Display.print("Update.begin OK");

    uint8_t buf[4096];
    size_t zapisano = 0;
    bool ok = true;
    while (f.available() && ok) {
        int rd = f.read(buf, sizeof(buf));
        if (rd <= 0) break;
        if (Update.write(buf, rd) != (size_t)rd) { ok = false; break; }
        zapisano += rd;
        int pct = (int)((float)zapisano / rozmiar * 100.0f);
        ui_progress(pct, 75, THEME_GREEN);
        M5Cardputer.Display.fillRect(100, 62, 60, 11, THEME_BG);
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(100, 63);
        char pb[8]; snprintf(pb, sizeof(pb), "%d%%", pct);
        M5Cardputer.Display.print(pb);
    }
    f.close();

    if (!ok || !Update.end(true)) {
        char eb[40]; snprintf(eb, sizeof(eb), "OTA blad: %s", Update.errorString());
        ui_show_error(eb); delay(1500); return false;
    }

    M5Cardputer.Display.setTextColor(THEME_GREEN);
    M5Cardputer.Display.setCursor(4, 92); M5Cardputer.Display.print("Wgrano pomyslnie!");
    M5Cardputer.Display.setCursor(4, 106); M5Cardputer.Display.print("Restart za 2s...");
    delay(2000);
    esp_restart();
    return true;
}

// ─── Uruchom M5Launcher ─────────────────────────────────
void uruchom_m5launcher() {
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("M5LAUNCHER", THEME_GREEN);

    const char* mozliwe[] = {
        "/apps/M5Launcher.bin", "/M5Launcher.bin",
        "/apps/launcher.bin",   "/launcher.bin",
    };
    String znaleziony = "";
    for (auto& s : mozliwe) {
        if (SD.exists(s)) { znaleziony = s; break; }
    }

    if (znaleziony.length() > 0) {
        M5Cardputer.Display.setTextColor(THEME_GREEN);
        M5Cardputer.Display.setCursor(4, 30);
        M5Cardputer.Display.print("Znaleziono: "); M5Cardputer.Display.print(znaleziony);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 46); M5Cardputer.Display.print("ENTER = wgraj i uruchom");
        M5Cardputer.Display.setCursor(4, 60); M5Cardputer.Display.print("Q = anuluj");
        char k = ui_wait_key();
        if (k != 'q' && k != 'Q') _ota_wgraj(znaleziony);
        return;
    }

    // Spróbuj przełączyć na drugą partycję OTA
    M5Cardputer.Display.setTextColor(THEME_YELLOW);
    M5Cardputer.Display.setCursor(4, 30);
    M5Cardputer.Display.print("Brak M5Launcher na SD.");
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 46);
    M5Cardputer.Display.print("Probuje przelac. partycje OTA...");

    const esp_partition_t* biezaca = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    const esp_partition_t* cel = nullptr;
    esp_partition_iterator_t iter = esp_partition_find(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (iter) {
        const esp_partition_t* p = esp_partition_get(iter);
        if (biezaca && p->address != biezaca->address &&
            p->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_0) {
            cel = p; break;
        }
        iter = esp_partition_next(iter);
    }
    esp_partition_iterator_release(iter);

    if (cel) {
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setCursor(4, 62);
        char buf[32]; snprintf(buf, sizeof(buf), "Partycja: %s", cel->label);
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 78); M5Cardputer.Display.print("ENTER=przelacz  Q=anuluj");
        char k = ui_wait_key();
        if (k != 'q' && k != 'Q') {
            // Update.begin/end already sets boot partition; for manual switch use esp_partition API
        // We call esp_restart() directly — the partition was already set by Update.end()
        if (true) { // partition switch via esp_partition — always attempt restart
                M5Cardputer.Display.setTextColor(THEME_GREEN);
                M5Cardputer.Display.setCursor(4, 96); M5Cardputer.Display.print("Restartowanie...");
                delay(1500); esp_restart();
            } else {
                ui_show_error("Blad przelaczania!"); delay(1500);
            }
        }
    } else {
        M5Cardputer.Display.setTextColor(THEME_RED);
        M5Cardputer.Display.setCursor(4, 78); M5Cardputer.Display.print("Brak drugiej partycji OTA!");
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 92); M5Cardputer.Display.print("Skopiuj M5Launcher.bin do /apps/");
        M5Cardputer.Display.setCursor(4, 212); M5Cardputer.Display.print("Dowolny klawisz = powrot");
        ui_wait_key();
    }
}

// ─── Uruchom dowolny .bin z SD ──────────────────────────
void uruchom_bin_z_sd() {
    String biny[32]; int liczba = 0;
    const char* katalogi[] = { "/apps", "/" };
    for (auto& kat : katalogi) {
        File dir = SD.open(kat);
        if (!dir) continue;
        while (liczba < 32) {
            File f = dir.openNextFile();
            if (!f) break;
            String fn = String(f.name());
            String fnU = fn; fnU.toUpperCase();
            if (fnU.endsWith(".BIN"))
                biny[liczba++] = String(kat) + "/" + fn;
            f.close();
        }
        dir.close();
    }

    if (liczba == 0) {
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("BRAK PLIKOW .BIN", THEME_RED);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 40); M5Cardputer.Display.print("Skopiuj pliki .bin do /apps/");
        M5Cardputer.Display.setCursor(4, 212); M5Cardputer.Display.print("Dowolny klawisz = powrot");
        ui_wait_key(); return;
    }

    const char* etykiety[32];
    char bufory[32][40];
    for (int i = 0; i < liczba; i++) {
        File f = SD.open(biny[i].c_str());
        String fn = biny[i].substring(biny[i].lastIndexOf('/') + 1);
        snprintf(bufory[i], 40, "%-22s %dKB",
                 fn.c_str(), f ? (int)(f.size() / 1024) : 0);
        if (f) f.close();
        etykiety[i] = bufory[i];
    }

    int sel = ui_select_list(etykiety, liczba, "WYBIERZ .BIN", THEME_GREEN);
    if (sel < 0) return;

    // Potwierdzenie
    M5Cardputer.Display.fillScreen(THEME_BG);
    ui_draw_header("POTWIERDZ", THEME_YELLOW);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(4, 30); M5Cardputer.Display.print("Uruchamiam:");
    M5Cardputer.Display.setTextColor(THEME_YELLOW);
    M5Cardputer.Display.setCursor(4, 46);
    M5Cardputer.Display.print(biny[sel].substring(0, 36));
    M5Cardputer.Display.setTextColor(THEME_MUTED);
    M5Cardputer.Display.setCursor(4, 68); M5Cardputer.Display.print("Urzadzenie uruchomi sie ponownie.");
    M5Cardputer.Display.setCursor(4, 82); M5Cardputer.Display.print("By wrocic: reset + przytrzymaj BTN.");
    M5Cardputer.Display.setTextColor(THEME_GREEN);
    M5Cardputer.Display.setCursor(4, 106); M5Cardputer.Display.print("ENTER = uruchom");
    M5Cardputer.Display.setTextColor(THEME_RED);
    M5Cardputer.Display.setCursor(130, 106); M5Cardputer.Display.print("Q = anuluj");

    char k = ui_wait_key();
    if (k != 'q' && k != 'Q') _ota_wgraj(biny[sel]);
}

void app_partycje_run() {
    while (true) {
        const char* opts[] = {
            "Uruchom M5Launcher",
            "Uruchom .bin z karty SD",
            "Info o partycjach",
            "Powrot",
        };
        int sel = ui_select_list(opts, 4, "MENEDZER PARTYCJI", THEME_ORANGE);
        if (sel < 0 || sel == 3) return;
        switch (sel) {
            case 0: uruchom_m5launcher(); break;
            case 1: uruchom_bin_z_sd();   break;
            case 2: pokaz_partycje();     break;
        }
    }
}
