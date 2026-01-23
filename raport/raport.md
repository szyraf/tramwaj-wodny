# Raport - Symulator Tramwaju Wodnego

**Przedmiot:** Systemy Operacyjne  
**Projekt:** Tramwaj wodny Kraków Wawel - Tyniec  
**Autor:** Szymon Rafałowski
**Repozytorium:** https://github.com/szyraf/tramwaj-wodny

---

## 1. Założenia projektowe

### 1.1. Opis problemu

Symulacja tramwaju wodnego kursującego między przystankami Tyniec i Wawel. Program modeluje:

- Pasażerów czekających w kolejkach na obu przystankach
- Statek o ograniczonej pojemności (N osób, M rowerów)
- Mostek (trap) o ograniczonej przepustowości (K slotów)
- Kapitana zarządzającego fazami rejsu
- Dyspozytora obsługującego sygnały sterujące

### 1.2. Parametry konfiguracyjne

| Parametr | Opis |
|----------|------|
| N | Pojemność statku (osoby) |
| M | Pojemność statku (rowery) |
| K | Pojemność mostka |
| T1 | Maksymalny czas załadunku (ms) |
| T2 | Czas podróży (ms) |
| R | Liczba rejsów dziennie |

### 1.3. Architektura wieloprocesowa

Program składa się z niezależnych procesów:

1. **Main** - proces główny, tworzy IPC i spawnuje procesy potomne
2. **Captain** - zarządza fazami: załadunek → podróż → rozładunek
3. **Dispatcher** - obsługuje sygnały z klawiatury (1=wcześniejszy odpływ, 2=koniec dnia)
4. **Passenger** (N procesów) - każdy pasażer to osobny proces

---

## 2. Ogólny opis kodu

### 2.1. Struktura plików

```
src/
├── main.cpp        - Inicjalizacja IPC, fork() procesów
├── captain.cpp     - Logika kapitana (fazy rejsu)
├── passenger.cpp   - Automat stanowy pasażera
├── dispatcher.cpp  - Obsługa klawiatury
├── ipc.cpp/h       - Wrappery System V IPC
├── config.cpp/h    - Parsowanie konfiguracji + walidacja
├── logger.cpp/h    - Logowanie z timestampem
└── common.h        - Struktury danych, enumy
```

### 2.2. Mechanizmy IPC

Program wykorzystuje **trzy mechanizmy IPC System V**:

1. **Pamięć współdzielona** - przechowuje stan symulacji (`SharedState`)
2. **Semafory** - synchronizacja (mutex + semafor per pasażer)
3. **Kolejki komunikatów** - potwierdzenia akcji (ACK)

### 2.3. Fazy symulacji

```
PHASE_LOADING → PHASE_BRIDGE_CLEAR → PHASE_SAILING → PHASE_UNLOADING
     ↑                                                      |
     +------------------------- (następny rejs) -----------+
```

### 2.4. Stany pasażera

```
STATE_QUEUE → STATE_BRIDGE → STATE_SHIP → STATE_BRIDGE → STATE_EXITED
```

---

## 3. Co udało się zrobić

- Pelna symulacja z wieloma procesami (fork + exec)
- Synchronizacja przez semafory System V
- Komunikacja przez pamiec wspoldzielona + kolejki komunikatow
- Obsluga dwoch sygnalow (Signal1, Signal2)
- Walidacja danych wejsciowych (limity procesow, parametry)
- Obsluga bledow z `perror()`
- Minimalne prawa dostepu (0600)
- Sprzatanie zasobow IPC po zakonczeniu
- Logowanie z timestampem do pliku i terminala
- Obsluga pasazerow z rowerami (zajmuja 2 sloty na mostku)

---

## 4. Kluczowe decyzje projektowe

### 4.1. Ochrona sekcji krytycznych

Dostep do `SharedState` wymaga synchronizacji - zastosowano semafor `SEM_MUTEX` jako mutex, aby tylko jeden proces mogl modyfikowac wspoldzielone dane w danym momencie.

### 4.2. Mechanizm oprozniania mostka

Przed odplynieciem statku pasazerowie pozostajacy na mostku musza wrocic do kolejki. Wprowadzono faze `PHASE_BRIDGE_CLEAR`, w ktorej kapitan aktywnie budzi pasazerow semaforami.

### 4.3. Obsluga przerwan systemowych (EINTR)

Sygnaly (np. SIGCHLD) moga przerwac wywolania `semop()` i `msgrcv()`. Operacje IPC sa powtarzane w petli `while` gdy `errno == EINTR`.

### 4.4. Sprzatanie zasobow IPC

Zasoby System V IPC pozostaja w systemie po zakonczeniu procesu. Program obsluguje `SIGINT`/`SIGTERM` i wywoluje `cleanup_ipc()` rowniez na starcie (usuwa pozostalosci po poprzednim uruchomieniu).

---

## 5. Elementy specjalne

### 5.1. Dynamiczna liczba semaforów

Każdy pasażer ma dedykowany semafor do budzenia. Liczba semaforów = 2 + liczba_pasażerów.

### 5.2. Blokada pliku logów (`flock`)

Wieloprocesowy zapis do pliku logu z synchronizacją przez `flock()`.

### 5.3. Sprawdzanie limitu procesów

Przed startem program sprawdza `RLIMIT_NPROC`, aby nie przekroczyć limitu systemowego.

### 5.4. Timestampy względne

Logi pokazują czas od startu symulacji (nie czas systemowy).

---

## 6. Testy

### Test 1: Podstawowy (`basic.env`)

**Cel:** Prosta symulacja z kilkoma pasażerami  
**Konfiguracja:** N=5, M=2, K=3, T1=5s, T2=1s, R=2, 6 pasażerów  
**Wynik:** PASS - wszyscy pasazerowie przewiezieni, 2 rejsy wykonane

### Test 2: Pełny statek (`full_ship.env`)

**Cel:** Sprawdzenie limitu pojemności statku  
**Konfiguracja:** N=3, K=2, R=3, 6 osób w Tyńcu  
**Wynik:** PASS - statek nigdy nie przekracza 3 osob, osoby na mostku wracaja do kolejki

### Test 3: Signal1 - wcześniejszy odpływ (`signal1.env`)

**Cel:** Wczesne odpłynięcie po naciśnięciu '1'  
**Konfiguracja:** N=10, T1=30s, dużo pasażerów  
**Wynik:** PASS - statek odplywa natychmiast po Signal1

### Test 4: Signal2 - koniec dnia (`signal2.env`)

**Cel:** Zakończenie dnia po naciśnięciu '2'  
**Konfiguracja:** N=10, R=10, dużo pasażerów  
**Wynik:** PASS - symulacja konczy sie przed wykonaniem wszystkich rejsow

### Test 5: Rowery (`bikes.env`)

**Cel:** Sprawdzenie limitu rowerów (M)  
**Konfiguracja:** N=5, M=2, K=4, 2 osoby + 4 z rowerami  
**Wynik:** PASS - max 2 rowery na statku

### Test 6: Obciążeniowy (`stress.env`)

**Cel:** Stabilność przy dużej liczbie pasażerów  
**Konfiguracja:** N=50, M=20, K=30, R=20, 300 pasażerów  
**Wynik:** PASS - brak zawieszenia, program konczy sie normalnie

---

## 7. Linki do kodu (GitHub)

### 7.1. Tworzenie i obsługa plików

| Funkcja | Lokalizacja |
|---------|-------------|
| `fopen()` | [logger.cpp:48](https://github.com/szyraf/tramwaj-wodny/blob/master/src/logger.cpp#L48) |
| `fclose()` | [logger.cpp:54](https://github.com/szyraf/tramwaj-wodny/blob/master/src/logger.cpp#L54) |
| `read()` | [dispatcher.cpp:37](https://github.com/szyraf/tramwaj-wodny/blob/master/src/dispatcher.cpp#L37) |
| `flock()` | [logger.cpp:50-53](https://github.com/szyraf/tramwaj-wodny/blob/master/src/logger.cpp#L50-L53) |

### 7.2. Tworzenie procesów

| Funkcja | Lokalizacja |
|---------|-------------|
| `fork()` | [main.cpp:119](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L119), [main.cpp:128](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L128), [main.cpp:138](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L138) |
| `execl()` | [main.cpp:122](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L122), [main.cpp:131](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L131), [main.cpp:143](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L143) |
| `exit()` / `_exit()` | [main.cpp:34](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L34), [main.cpp:124](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L124) |
| `wait()` | [main.cpp:156](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L156) |
| `waitpid()` | [main.cpp:24](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L24) |

### 7.3. Obsługa sygnałów

| Funkcja | Lokalizacja |
|---------|-------------|
| `signal()` | [main.cpp:111-113](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L111-L113) |
| `kill()` | [main.cpp:30](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L30) |
| Handler SIGINT/SIGTERM | [main.cpp:27-35](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L27-L35) |
| Handler SIGCHLD | [main.cpp:22-25](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L22-L25) |

### 7.4. Synchronizacja procesów (semafory)

| Funkcja | Lokalizacja |
|---------|-------------|
| `semget()` | [ipc.cpp:45](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L45), [ipc.cpp:54](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L54) |
| `semctl()` | [ipc.cpp:90](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L90), [ipc.cpp:97](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L97), [ipc.cpp:106](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L106) |
| `semop()` | [ipc.cpp:64](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L64), [ipc.cpp:73](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L73) |
| Użycie mutex | [captain.cpp:61](https://github.com/szyraf/tramwaj-wodny/blob/master/src/captain.cpp#L61), [passenger.cpp:110](https://github.com/szyraf/tramwaj-wodny/blob/master/src/passenger.cpp#L110) |

### 7.5. Segmenty pamięci współdzielonej

| Funkcja | Lokalizacja |
|---------|-------------|
| `shmget()` | [ipc.cpp:6](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L6), [ipc.cpp:15](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L15) |
| `shmat()` | [ipc.cpp:24](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L24) |
| `shmdt()` | [ipc.cpp:33](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L33) |
| `shmctl()` | [ipc.cpp:39](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L39), [main.cpp:13](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L13) |

### 7.6. Kolejki komunikatów

| Funkcja | Lokalizacja |
|---------|-------------|
| `msgget()` | [ipc.cpp:112](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L112), [ipc.cpp:121](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L121) |
| `msgsnd()` | [ipc.cpp:133](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L133) |
| `msgrcv()` | [ipc.cpp:143](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L143) |
| `msgctl()` | [ipc.cpp:154](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L154), [main.cpp:19](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L19) |

### 7.7. Obsługa błędów i walidacja

| Element | Lokalizacja |
|---------|-------------|
| `perror()` | [ipc.cpp:8](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L8), [ipc.cpp:17](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L17), [main.cpp:120](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp#L120) |
| Walidacja konfiguracji | [config.cpp:64-102](https://github.com/szyraf/tramwaj-wodny/blob/master/src/config.cpp#L64-L102) |
| Sprawdzanie RLIMIT_NPROC | [config.cpp:65-82](https://github.com/szyraf/tramwaj-wodny/blob/master/src/config.cpp#L65-L82) |
| Obsługa EINTR | [ipc.cpp:64-68](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp#L64-L68) |

### 7.8. Podział na moduły

| Moduł | Odpowiedzialność |
|-------|------------------|
| [main.cpp](https://github.com/szyraf/tramwaj-wodny/blob/master/src/main.cpp) | Inicjalizacja, tworzenie procesów |
| [captain.cpp](https://github.com/szyraf/tramwaj-wodny/blob/master/src/captain.cpp) | Logika kapitana |
| [passenger.cpp](https://github.com/szyraf/tramwaj-wodny/blob/master/src/passenger.cpp) | Automat stanowy pasażera |
| [dispatcher.cpp](https://github.com/szyraf/tramwaj-wodny/blob/master/src/dispatcher.cpp) | Obsługa klawiatury |
| [ipc.cpp](https://github.com/szyraf/tramwaj-wodny/blob/master/src/ipc.cpp) | Wrappery IPC |
| [config.cpp](https://github.com/szyraf/tramwaj-wodny/blob/master/src/config.cpp) | Konfiguracja |
| [logger.cpp](https://github.com/szyraf/tramwaj-wodny/blob/master/src/logger.cpp) | Logowanie |

---

## 8. Podsumowanie użytych konstrukcji

| Wymaganie | Status | Opis |
|-----------|--------|------|
| Tworzenie procesow | TAK | `fork()`, `execl()`, `wait()`, `waitpid()` |
| Mechanizmy synchronizacji | TAK | Semafory System V (`semget`, `semop`, `semctl`) |
| Dwa mechanizmy IPC | TAK | Pamiec wspoldzielona + kolejki komunikatow |
| Obsluga sygnalow | TAK | SIGINT, SIGTERM, SIGCHLD + Signal1/Signal2 |
| Walidacja danych | TAK | `validate_config()` w config.cpp |
| Podzial na moduly | TAK | 7 plikow zrodlowych |
| Obsluga bledow | TAK | `perror()`, `errno`, obsluga EINTR |
| Minimalne prawa dostepu | TAK | 0600 dla wszystkich zasobow IPC |
| Sprzatanie zasobow | TAK | `cleanup_ipc()` + obsluga sygnalow |

---

## 9. Nieużyte konstrukcje

- **Wątki (pthread)** - projekt oparty w całości na procesach
- **Łącza (pipe, mkfifo)** - zastąpione kolejkami komunikatów
- **Gniazda (socket)** - nie wymagane w projekcie
