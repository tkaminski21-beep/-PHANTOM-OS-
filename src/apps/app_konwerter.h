#pragma once
// apps/app_konwerter.h — Konwerter jednostek

#include "M5Cardputer.h"
#include "../core/ui.h"
#include "../core/theme.h"

struct Jednostka { const char* nazwa; double wspolczynnik; };

struct KategoriaKonwersji {
    const char* nazwa;
    const char* bazowa;
    Jednostka jednostki[8];
    int liczba;
};

static KategoriaKonwersji KATEGORIE[] = {
    { "Dlugosc", "metry", {
        {"milimetry",    0.001},  {"centymetry", 0.01},
        {"metry",        1.0},   {"kilometry",  1000.0},
        {"cale",         0.0254},{"stopy",      0.3048},
        {"jardy",        0.9144},{"mile",       1609.34}
    }, 8},
    { "Masa", "kilogramy", {
        {"miligramy",   0.000001},{"gramy",      0.001},
        {"kilogramy",   1.0},    {"tony",        1000.0},
        {"uncje",       0.02835},{"funty",       0.45359},
        {"kamienie",    6.35029},{nullptr,0}
    }, 7},
    { "Temperatura", "Celsjusz", {
        {"Celsjusz",    1.0},    {"Fahrenheit",  1.0},
        {"Kelvin",      1.0},   {nullptr,0},
        {nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0}
    }, 3},
    { "Predkosc", "m/s", {
        {"m/s",         1.0},    {"km/h",        0.27778},
        {"mph",         0.44704},{"wezly",       0.51444},
        {"Mach",        340.29}, {nullptr,0},{nullptr,0},{nullptr,0}
    }, 5},
    { "Powierzchnia", "m2", {
        {"mm2",         0.000001},{"cm2",        0.0001},
        {"m2",          1.0},    {"km2",         1000000.0},
        {"ha",          10000.0},{"akry",        4046.86},
        {nullptr,0},{nullptr,0}
    }, 6},
    { "Objetosc", "litry", {
        {"mililitry",   0.001},  {"litry",       1.0},
        {"m3",          1000.0}, {"fl.oz(US)",   0.02957},
        {"pinty(US)",   0.47318},{"galony(US)",  3.78541},
        {nullptr,0},{nullptr,0}
    }, 6},
    { "Dane (bajty)", "bajty", {
        {"bajty",       1.0},    {"kilobajty",   1024.0},
        {"megabajty",   1048576.0},{"gigabajty", 1073741824.0},
        {"terabajty",   1099511627776.0},{nullptr,0},{nullptr,0},{nullptr,0}
    }, 5},
};
static int LICZBA_KATEGORII = 7;

double konwertuj_temperature(double val, int z, int na) {
    // Najpierw do Celsjusza
    double c;
    if (z == 0) c = val;
    else if (z == 1) c = (val - 32) * 5.0 / 9.0;
    else c = val - 273.15;
    // Z Celsjusza do docelowej
    if (na == 0) return c;
    if (na == 1) return c * 9.0 / 5.0 + 32;
    return c + 273.15;
}

void app_konwerter_run() {
    while (true) {
        // Wybierz kategorię
        const char* nazwy[8];
        for (int i = 0; i < LICZBA_KATEGORII; i++) nazwy[i] = KATEGORIE[i].nazwa;
        int kat = ui_select_list(nazwy, LICZBA_KATEGORII, "KONWERTER JEDNOSTEK", THEME_CYAN);
        if (kat < 0) return;

        KategoriaKonwersji& K = KATEGORIE[kat];

        // Wybierz jednostkę źródłową
        const char* jnazwy[8];
        for (int i = 0; i < K.liczba; i++) jnazwy[i] = K.jednostki[i].nazwa;
        int z = ui_select_list(jnazwy, K.liczba, "Z JEDNOSTKI", THEME_CYAN);
        if (z < 0) continue;

        // Wybierz jednostkę docelową
        int na = ui_select_list(jnazwy, K.liczba, "NA JEDNOSTKE", THEME_CYAN);
        if (na < 0) continue;

        // Wpisz wartość
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("KONWERTUJ", THEME_CYAN);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 30);
        M5Cardputer.Display.print("Z: "); M5Cardputer.Display.print(K.jednostki[z].nazwa);
        M5Cardputer.Display.setCursor(4, 44);
        M5Cardputer.Display.print("Na: "); M5Cardputer.Display.print(K.jednostki[na].nazwa);

        String wejscie = ui_input_string("Wartosc:", 4, 62, 16);
        if (wejscie.length() == 0) continue;
        double val = wejscie.toDouble();

        // Oblicz
        double wynik;
        if (kat == 2) { // Temperatura — specjalna logika
            wynik = konwertuj_temperature(val, z, na);
        } else {
            double wBazowej = val * K.jednostki[z].wspolczynnik;
            wynik = wBazowej / K.jednostki[na].wspolczynnik;
        }

        // Pokaż wynik
        M5Cardputer.Display.fillScreen(THEME_BG);
        ui_draw_header("WYNIK", THEME_GREEN);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 35);
        char buf[40]; snprintf(buf, sizeof(buf), "%.6g %s", val, K.jednostki[z].nazwa);
        M5Cardputer.Display.print(buf);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 50);
        M5Cardputer.Display.print("=");
        M5Cardputer.Display.setTextColor(THEME_GREEN);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setCursor(4, 68);
        char res[30]; snprintf(res, sizeof(res), "%.6g", wynik);
        M5Cardputer.Display.print(res);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setTextColor(THEME_CYAN);
        M5Cardputer.Display.setCursor(4, 92);
        M5Cardputer.Display.print(K.jednostki[na].nazwa);
        M5Cardputer.Display.setTextColor(THEME_MUTED);
        M5Cardputer.Display.setCursor(4, 212);
        M5Cardputer.Display.print("Dowolny klawisz = nowa konwersja  Q=wyjscie");
        char k = ui_wait_key();
        if (k == 'q' || k == 'Q') return;
    }
}
