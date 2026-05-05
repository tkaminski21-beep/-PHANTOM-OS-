                 🚀 Zobacz prezentację PHANTOM OS online!](https://tkaminski21-beep.github.io/-PHANTOM-OS-/
 
 
 
 
 CardPuter Advanced OS v3.0.0-PL

Potężny system operacyjny dla **M5Stack CardPuter** w języku polskim.  
**30+ aplikacji**, kompatybilny z M5Launcher, obsługa Bruce i innych `.bin`.

---

 Pełna lista aplikacji

 Narzędzia (zakładka 1)
| Aplikacja | Opis |
|---|---|
| Notatnik | Twórz/czytaj/usuwaj notatki — SD |
| Kalkulator | +−×÷ sqrt backspace |
| Kalkulator naukowy | sin cos tan asin acos atan sqrt log ln abs ^ — tryb RAD/DEG |
| Rezystory 4/5p. | Wizualny kalkulator z graficznym rezystorem |
| Konwerter jednostek | Długość · masa · temperatura · prędkość · powierzchnia · objętość · dane |
| Generator haseł | Długość 4–32 · małe/duże/cyfry/znaki spec. |
| Enkoder/Dekoder | Base64 koduj/dekoduj · Tekst↔HEX |
| Zegar / Czas | Zegar NTP · stoper z okrążeniami · minutnik z animacją · kalendarz |
| Notatnik AES | Notatki szyfrowane kluczem — zapis `.enc` na SD |

 Sieć (zakładka 2)
| Aplikacja | Opis |
|---|---|
| Wi-Fi Manager | Skanuj · łącz · zapisz hasło · NTP · status IP |
| Bluetooth | On/off · skanuj · terminal BT Serial |
| Analizator sieci WiFi | Wszystkie sieci · kanały · RSSI · wykres zajętości kanałów |
| Ping / Porty / HTTP | Ping z TTL · skaner 15 portów · przeglądarka HTTP z przewijaniem |
| Serwer WWW | Wbudowany serwer HTTP na porcie 80 z panelem statusu i API JSON |
| Wake-on-LAN | Wyślij Magic Packet przez UDP broadcast |
| Klient MQTT | Połącz z brokerem · publish do tematu |
| Odtwarzacz MP3 | Pliki `.mp3` z SD · I2S · lista · głośność |

 Hardware / Elektronika (zakładka 3)
| Aplikacja | Opis |
|---|---|
| Oscyloskop ADC | Próbkowanie GPIO1 · wykres · Vpp · Vavg · ~freq · wyzwalanie |
| Analizator logiczny | 4 kanały GPIO · zbocza · zapis próbek · regulacja prędkości |
| Generator PWM | LEDC ESP32 · częstotliwość 1Hz–100kHz · wypełnienie 1–99% |
| Miernik częstotliwości | Zliczanie zboczy GPIO5 · wykres historii · auto co 100ms |
| Miernik ADC/napięcia | Wykres 0–3.3V · historia 120 próbek |
| Tester IR | Nadaj/odbierz NEC · pilot TV demo |
| Skaner I2C | Skanuj 0x01–0x77 · rozpoznawanie znanych urządzeń |
| Skaner BLE | BLE advertise scan · nazwy · adresy · RSSI |
| Wygaszacz ekranu | Matrix · gwiazdki · zegar bounce |

 System / Gry (zakładka 4)
| Aplikacja | Opis |
|---|---|
| **M5Launcher / .bin** | Uruchom Bruce/M5Launcher z SD przez OTA · przełącz partycję |
| Monitor systemu | RAM · PSRAM · temp CPU · uptime · bateria · wykres historii |
| Benchmark | Integer · float · sin/cos · RAM · SD · ekran |
| Edytor HEX | Podgląd binarny pliku z SD · adres + HEX + ASCII |
| Menedżer plików SD | Przeglądaj · podglądaj · kopiuj · przenoś · usuwaj |
| Czytnik e-booków | Czytaj `.txt` z SD ze scrollowaniem i paskiem postępu |
| Terminal UART | TX=GPIO17 RX=GPIO18 · wybór baudratu · echo lokalne |
| Log systemowy | Każde uruchomienie logowane na SD z timestampem |
| Gry | Snake · Tetris (7 klocków) · Pong (2 graczy) |
| Ustawienia | Jasność · dźwięk · obrót · info · reset · restart |

---

 Sterowanie

| Klawisz | Akcja |
|---|---|
| `FN + ;` | W górę |
| `FN + .` | W dół |
| `FN + ,` | Poprzednia zakładka |
| `FN + /` | Następna zakładka |
| `TAB` | Następna zakładka |
| `ENTER` lub `BTN` | Uruchom aplikację |
| `1`–`8` | Szybki dostęp do aplikacji 1–8 |
| `FN + B` | Podświetlenie off/on |
| `FN + Q` | Wyjście z aplikacji |

 W aplikacjach
| Klawisz | Akcja |
|---|---|
| `W/S` | Góra/dół · scroll |
| `A/D` | Lewo/prawo |
| `SPACJA` | Play/Pauza · drop (Tetris) |
| `+/−` | Głośność · zmiana wartości |
| `R` | Reset (stoper) |
| `L` | Okrążenie (stoper) |
| `G` | Generuj nowe hasło |
| `FN+S` | Zapisz (notatnik) |
| `FN+M` | Tryb 4/5 pasm (rezystory) |
| `C` | Wyczyść (kalkulator) |
| `R` | RAD/DEG (kalk. naukowy) |
| `T` | Skala czasu (oscyloskop) |
| `W` | Wyzwalanie (oscyloskop) |

---

 Instalacja

 Krok 1 — VS Code + PlatformIO
```
code.visualstudio.com → zainstaluj
VS Code Extensions → PlatformIO IDE → Install
```

 Krok 2 — Otwórz projekt
```
File → Open Folder → wybierz CardputerOS3
```

 Krok 3 — Wgraj
```
PlatformIO (mrówka) → m5stack-cardputer → Upload
```
lub dolny pasek → strzałka → Upload

 Krok 4 — Karta SD (FAT32)
```
/apps/          ← .bin aplikacje (Bruce, M5Launcher, itp.)
/music/         ← pliki .mp3
/notes/         ← notatki (auto)
/books/         ← e-booki .txt
/enc_notes/     ← notatki szyfrowane (auto)
```

---

Uruchamianie Bruce / M5Launcher

1. Skopiuj `bruce.bin` lub `M5Launcher.bin` do `/apps/` na karcie SD
2. W OS: zakładka **SYS** → **M5Launcher / .bin**
3. Wybierz plik → **Uruchom**
4. System wgrywa przez OTA i restartuje

**Powrót do CardPuter OS:**  
Reset + przytrzymaj BTN przy starcie → wybierz partycję

Lub skopiuj `CardputerOS.bin` do `/apps/` i uruchom z M5Launcher.

---

 Piny sprzętowe

| Funkcja | Pin |
|---|---|
| SD CS | GPIO4 |
| I2S BCLK | GPIO41 |
| I2S LRCK | GPIO43 |
| I2S DOUT | GPIO42 |
| IR TX | GPIO44 |
| IR RX | GPIO0 |
| ADC / Oscyloskop | GPIO1 |
| Analizator log. | GPIO1,2,3,4 |
| PWM wyjście | GPIO10 |
| Miernik freq. | GPIO5 |
| UART TX | GPIO17 |
| UART RX | GPIO18 |

---

  Specyfikacja

- **Chipset**: ESP32-S3 @ 240 MHz  
- **RAM**: 320 KB + 8 MB PSRAM  
- **Flash**: 16 MB (partycje OTA)  
- **Wyświetlacz**: 240×135 ST7789  
- **Klawiatura**: pełna QWERTY (I2C)

---



