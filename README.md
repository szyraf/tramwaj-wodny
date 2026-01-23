# Water Tram Simulator

Symulator tramwaju wodnego na trasie Kraków Wawel - Tyniec.

## Wymagania

- Linux/WSL
- CMake 3.10+
- GCC z obsługą C++17

## Kompilacja

```bash
mkdir build && cd build
cmake ..
make
```

## Uruchomienie

```bash
cd build
./main ../config.env
```

## Sterowanie

Podczas symulacji można użyć klawiszy:
- `1` - Sygnał1: Wcześniejszy odpływ
- `2` - Sygnał2: Koniec dnia

## Konfiguracja (config.env)

```
N=3                      # Pojemność statku (ludzie)
M=1                      # Pojemność statku (rowery)
K=2                      # Pojemność mostka
T1=10000                 # Max czas załadunku (ms)
T2=10000                 # Czas podróży (ms)
R=2                      # Liczba rejsów dziennie

QUEUE_TO_BRIDGE_TIME=100     # Czas przejścia kolejka->mostek
BRIDGE_TO_SHIP_TIME=500      # Czas przejścia mostek->statek
SHIP_TO_BRIDGE_TIME=100      # Czas przejścia statek->mostek
BRIDGE_TO_EXIT_TIME=500      # Czas opuszczenia mostka

TYNIEC_PEOPLE=6          # Ludzie w Tyńcu
TYNIEC_BIKES=0           # Ludzie z rowerami w Tyńcu
WAWEL_PEOPLE=6           # Ludzie na Wawelu
WAWEL_BIKES=0            # Ludzie z rowerami na Wawelu
```

## Struktura projektu

- `main.cpp` - Proces główny, tworzy IPC i procesy potomne
- `captain.cpp` - Proces kapitana, zarządza fazami
- `passenger.cpp` - Proces pasażera
- `dispatcher.cpp` - Proces dyspozytora, obsługuje sygnały
- `common.h` - Wspólne definicje i struktury
- `config.*` - Wczytywanie konfiguracji
- `ipc.*` - Funkcje System V IPC
- `logger.*` - Logowanie do pliku

## Logi

Logi zapisywane są do pliku `simulation_YYYYMMDD_HHMMSS.log` w katalogu build.
