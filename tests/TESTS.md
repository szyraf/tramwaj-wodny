# Testy Manualne - Symulator Tramwaju Wodnego

## Uruchomienie

```bash
cd build
./main ../tests/<config>.env
```

## Sterowanie

- **'1'** - Signal1: wczesne odplyniecie
- **'2'** - Signal2: koniec dnia

---

## 1. Test Podstawowy (`basic.env`)

**Cel:** Prosta symulacja z kilkoma pasazerami

**Konfiguracja:** N=5, M=2, K=3, T1=5s, T2=1s, R=2, 3 osoby Tyniec, 3 osoby Wawel

**Oczekiwane logi:**
```
...
[CAPTAIN] === Trip 1: LOADING at TYNIEC ===
[P0] Entered bridge
[P0] Entered ship
...
[CAPTAIN] === SAILING from TYNIEC to WAWEL ===
[CAPTAIN] Sailing... 1000/1000 ms
[CAPTAIN] Arrived at WAWEL!
[CAPTAIN] === UNLOADING at WAWEL (3 passengers) ===
[P0] Disembarked to bridge
[P0] Left bridge
...
[CAPTAIN] === END OF DAY ===
```

**Sukces:** Wszyscy 6 pasazerow przewiezieni, 2 rejsy wykonane

---

## 2. Test Pelnego Statku (`full_ship.env`)

**Cel:** Sprawdzenie limitu pojemnosci statku (N)

**Konfiguracja:** N=3, M=1, K=2, R=3, 6 osob w Tyncu

**Oczekiwane logi:**
```
...
[CAPTAIN] Clearing bridge (1 people still on bridge)...
[P3] Left bridge (returned to queue)
[CAPTAIN] Bridge cleared!
...
```

**Sukces:** Statek nigdy nie ma >3 pasazerow, osoby na mostku wracaja do kolejki

---

## 3. Test Rowerow (`bikes.env`)

**Cel:** Sprawdzenie limitu rowerow (M)

**Konfiguracja:** N=5, M=2, K=4, 2 osoby + 4 z rowerami w Tyncu

**Oczekiwane logi:**
```
[P2B] Entered ship
[P3B] Entered ship
[P4B] Entered bridge
[CAPTAIN] Loading complete: 4 people, 2 bikes on board
```

**Sukces:** Max 2 rowery na statku, pasazerowie z rowerem maja "B" w nazwie (P2B)

---

## 4. Test Signal1 (`signal1.env`)

**Cel:** Wczesne odplyniecie po nacisnieciu '1'

**Konfiguracja:** N=10, T1=30s (dlugi), duzo pasazerow

**Instrukcja:** Uruchom, poczekaj 2-3s, nacisnij '1'

**Oczekiwane logi:**
```
[DISPATCHER] Signal1 sent - early departure
[CAPTAIN] Signal1 received - early departure
[CAPTAIN] Loading complete: X people...
```

**Sukces:** Statek odpływa natychmiast po Signal1

---

## 5. Test Signal2 (`signal2.env`)

**Cel:** Zakonczenie dnia po nacisnieciu '2'

**Konfiguracja:** N=10, R=10 (duzo rejsow), duzo pasazerow

**Instrukcja:** Uruchom, poczekaj na 1-2 rejsy, nacisnij '2'

**Oczekiwane logi:**
```
[DISPATCHER] Signal2 sent - ending day
[CAPTAIN] Signal2 received during loading - ending day
[CAPTAIN] === END OF DAY ===
```

**Sukces:** Symulacja konczy sie mimo ze R=10

---

## 6. Test Obciazeniowy (`stress.env`)

**Cel:** Stabilnosc przy duzej liczbie pasazerow

**Konfiguracja:** N=50, M=20, K=30, R=20, 300 pasazerow (150 Tyniec + 150 Wawel)

**Oczekiwane logi:**
```
[MAIN] Created 300 passengers
[MAIN] Tyniec queue: 150, Wawel queue: 150
...
[CAPTAIN] === END OF DAY ===
[MAIN] Simulation ended successfully.
```

**Sukces:** Brak zawieszenia, brak bledow fork/memory, program konczy sie normalnie
