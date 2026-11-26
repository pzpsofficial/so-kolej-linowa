# Kolej linowa: opis projektu i testów

| | |
|---|---|
| **Autor** | Bartosz Karpiel, nr albumu 155276 |
| **Przedmiot** | Systemy operacyjne, studia niestacjonarne, 2026/2027 |
| **Temat** | Temat 4 - Kolej linowa (lista tematów 2025/2026, kontynuacja) |
| **Repozytorium** | https://github.com/pzpsofficial/so-kolej-linowa |

## 1. Treść tematu

W pewnej górskiej miejscowości znajduje się z krzesełkowa kolej linowa czynna w okresie letnim obsługująca m.in. rowerowe trasy zjazdowe. Z kolei krzesełkowej korzystają piesi oraz rowerzyści. Kolej składa się z 4 osobowych krzesełek o łącznej liczbie 72 sztuk. Jednocześnie może być zajętych 36 krzesełek. Na krzesełku może maksymalnie siedzieć dwóch rowerzystów lub jeden rowerzysta i dwóch pieszych lub czterech pieszych.

Rowerzyści/turyści przychodzą na teren stacji w losowych momentach czasu (nie wszyscy z nich muszą korzystać z kolei linowej). Wejście na teren kolejki linowej odbywa się po opłaceniu biletu/karnetu w kasie. Karnety są jednorazowe, czasowe (Tk1, Tk2, Tk3) lub dzienne. Dzieci poniżej 10 roku życia oraz seniorzy powyżej 65 roku życia mają 25% zniżkę. Dzieci poniżej 8 roku życia znajdują się pod stałą opieką osoby dorosłej. Aby dostać się na peron i usiąść na krzesełku należy przejść dwa rodzaje bramek:

- wejście na teren dolnej stacji odbywa się czterema bramkami jednocześnie - na tych bramkach sprawdzany jest bilet/karnet.
- Wejście na peron odbywa się trzema bramkami - na tych bramkach następuje kontrola osób, które mają usiąść na jednym krzesełku - bramki te otwiera pracownik1;

Na terenie dolnej stacji (po przejściu pierwszych bramek, a przed wyjściem drugimi bramkami) może przebywać maksymalnie N osób. Wyjazd/wyjście z peronu stacji górnej odbywa się dwoma drogami jednocześnie (ruch jednokierunkowy). Stacja dolna jest obsługiwana przez pracownika1, stacja górna jest obsługiwana przez pracownika2. W przypadku zagrożenia pracownik1 lub pracownik2 zatrzymują kolej linową (sygnał1). Aby wznowić działanie pracownik, który zatrzymał kolej komunikuje się z drugim pracownikiem - po otrzymaniu komunikatu zwrotnego o gotowości, kolej jest uruchamiana ponownie (sygnał2).

Rowerzyści zjeżdżają trzema trasami o różnym stopniu trudności - średni czas przejazdu dla poszczególnych tras jest różny i wynosi odpowiednio T1, T2 i T3 (T1<T2<T3).

Zasady działania stacji ustalone przez kierownika są następujące:

- Kolej linowa jest czynna w godzinach od Tp do Tk, W momencie osiągnięcia czasu Tk na bramkach przestają działać karnety. Wszystkie osoby, które weszły na peron mają zostać przetransportowane do stacji górnej. Następnie po 3 sekundach kolej ma zostać wyłączona.
- Dzieci w wieku od 4 do 8 lat siadają na krzesełko pod opieką osoby dorosłej;
- Osoba dorosła może opiekować się jednocześnie co najwyżej dwoma dziećmi w wieku od 4 do 8 lat;
- Każde przejście przez bramki (użycie danego karnetu) jest rejestrowane (id karnetu - godzina) - na koniec dnia jest generowany raport/podsumowanie ilości wykonanych zjazdów przez poszczególne osoby/karnety.
- Osoby uprawnione VIP (ok. 1%) wchodzą na teren dolnej stacji bez kolejki (używając karnetu!);

Napisz procedury Kasjer, Pracownik i Turysta symulujące działanie kolei linowej. Raport z przebiegu symulacji zapisać w pliku (plikach) tekstowym.

## 2. Doprecyzowanie treści

Niektóre zdania w treści tematu można rozumieć na kilka sposobów. Poniżej jest przyjęta interpretacja (pkt 1.3 zasad dopuszcza takie doprecyzowanie).

### 2.1. Krzesełka

- 72 krzesełka są zamocowane na jednej zamkniętej pętli liny. Połowa z nich (36) jedzie w górę i tylko te krzesełka wiozą ludzi. Druga połowa wraca w dół pusta. Stąd najwyżej 36 zajętych krzesełek naraz.
- Krzesełko jest zajęte od chwili wsiadania na stacji dolnej do chwili wysiadania na stacji górnej.
- Kolej wozi ludzi tylko w górę.

### 2.2. Skład jednego krzesełka

- Pieszy zajmuje 1 miejsce, rowerzysta z rowerem 2 miejsca, dziecko 1 miejsce.
- Na krzesełku suma miejsc wynosi najwyżej 4 i jedzie najwyżej 2 rowerzystów. Dopuszczalne składy to więc: do 4 pieszych, 1 rowerzysta i do 2 pieszych, 2 rowerzystów.
- Krzesełko może odjechać niepełne, jeśli żadna z czekających osób nie pasuje już do składu. Bez tego np. samotny rowerzysta pod koniec dnia czekałby bez końca.
- Opiekun i jego dzieci zawsze jadą razem na jednym krzesełku.

### 2.3. Dzieci

- Dziecko w wieku 4-7 lat (czyli poniżej 8 lat) jest pod stałą opieką dorosłego: opiekun kupuje mu bilet, dziecko przechodzi z nim przez bramki i jedzie na tym samym krzesełku.
- Jeden dorosły opiekuje się najwyżej dwojgiem takich dzieci.
- Dzieci od 8 lat są samodzielnymi turystami.
- Dzieci poniżej 4 lat nie korzystają z kolei.

### 2.4. Bilety i kasa

- Bilet jednorazowy uprawnia do jednego wjazdu.
- Bilety czasowe Tk1, Tk2, Tk3 są ważne przez podany czas od pierwszego przejścia przez bramkę wejściową, najdłużej do Tk.
- Bilet dzienny jest ważny do Tk.
- Dzieci poniżej 10 lat i seniorzy powyżej 65 lat płacą 25% mniej (10-latek i 65-latek płacą pełną cenę).
- Część przychodzących osób (parametr, domyślnie ok. 5%) nie korzysta z kolei i odchodzi bez kupowania biletu.

### 2.5. Bramki i limit N

- 4 bramki wejściowe działają jednocześnie. Każda sprawdza, czy bilet jest ważny, i zapisuje przejście w rejestrze (id karnetu, numer bramki, godzina). Po Tk bramki odrzucają wszystkie karnety.
- Limit N dotyczy osób na terenie dolnej stacji, dzieci też się liczą. Osoba zajmuje miejsce w limicie przy przejściu przez bramkę wejściową i zwalnia je przy przejściu na peron. Kto się nie mieści, czeka przed bramkami.
- Pracownik1 kompletuje skład na najbliższe krzesełko i wpuszcza tę grupę na peron przez jedną z 3 bramek. To przejście też trafia do rejestru.

### 2.6. VIP

- Ok. 1% turystów to VIP-y. VIP ma karnet i przechodzi przez bramki wejściowe tak jak inni, ale bez kolejki: gdy zwolni się miejsce w limicie N, wchodzi przed czekającymi.
- Limit N obowiązuje również VIP-y.
- VIP nie ma pierwszeństwa w kasie ani przy wsiadaniu na krzesełko.

### 2.7. Stacja górna i kolejne wjazdy

- Z peronu stacji górnej prowadzą dwa wyjścia, ruch tylko w jedną stronę.
- Rowerzysta zjeżdża jedną z tras T1, T2, T3 (T1 < T2 < T3). Pieszy schodzi w losowym czasie.
- Jeśli bilet jest nadal ważny, turysta wraca do bramek wejściowych i może jechać ponownie.

### 2.8. Zatrzymanie i wznowienie kolei

- Sygnał1 to `SIGUSR1`, sygnał2 to `SIGUSR2`.
- Zagrożenie wywołuje się z zewnątrz, wysyłając sygnał do procesu pracownika1 albo pracownika2 (np. poleceniem `kill`).
- Pracownik, który odebrał zagrożenie, zatrzymuje kolej i zapamiętuje, że to on ją zatrzymał. W czasie postoju nikt nie wsiada ani nie wysiada, a krzesełka stoją.
- Wznowić kolej może tylko ten pracownik. Wysyła drugiemu pracownikowi komunikat i czeka na odpowiedź o gotowości. Dopiero po jej otrzymaniu uruchamia kolej (sygnał2). Kolej nie rusza bez tej odpowiedzi.

### 2.9. Koniec dnia

- O godzinie Tk bramki przestają przyjmować karnety.
- Osoby, które są już na peronie, zostają dowiezione do stacji górnej.
- Osoby, które są na terenie dolnej stacji, ale nie weszły na peron, już nie wjeżdżają i opuszczają stację.
- 3 sekundy po tym, jak ostatnia osoba wysiądzie na stacji górnej, kolej zostaje wyłączona. Wszystkie procesy kończą pracę, a wszystkie użyte struktury IPC są usuwane.

### 2.10. Czas, parametry i raporty

- Symulacja działa w czasie przeskalowanym (np. 1 sekunda = 1 minuta czasu stacji).
- Parametry wejściowe: Tp, Tk, Tk1-Tk3, T1-T3, N, liczba turystów. Program sprawdza ich poprawność (m.in. Tp < Tk, T1 < T2 < T3, N > 0, liczba turystów w dopuszczalnym zakresie) i przy błędzie wyświetla komunikat.
- Przebieg symulacji trafia do pliku tekstowego z logiem.
- Na koniec dnia powstaje raport: liczba zjazdów każdego karnetu i każdej osoby (liczba przejść na peron) oraz podsumowanie sprzedaży według typu biletu i zniżek.

## 3. Planowana realizacja

- **Programy:** `main` (uruchamia symulację, pilnuje czasu Tp-Tk, sprząta zasoby, tworzy raport), `kasjer`, `pracownik` (jeden program uruchamiany jako pracownik1 albo pracownik2), `generator` (tworzy turystów w losowych odstępach czasu) i `turysta`. Wszystkie uruchamiane przez `fork()` i `exec()`.
- **Dzieci** pod opieką są wątkami w procesie opiekuna.
- **Mechanizmy:** semafory System V (limit N, bramki, krzesełka, sekcje krytyczne), pamięć dzielona (stan kolei, rejestr przejść, liczniki), kolejki komunikatów (kasa, wejście na peron, komunikacja między pracownikami), sygnały (zatrzymanie, wznowienie, zakończenie), pliki (log i raport).
- **Bez aktywnego czekania:** procesy czekają, blokując się na semaforach, kolejkach komunikatów i zmiennych warunkowych.
- **Obciążenie:** generator pilnuje liczby jednocześnie działających procesów, więc symulacja ma działać poprawnie również przy tysiącach turystów i przy ograniczonym limicie procesów użytkownika.
- Program będzie napisany w C i ma się kompilować oraz działać na serwerze torus.

## 4. Testy

Każdy test kończy się takim samym sprawdzeniem po zakończeniu symulacji: nie zostaje żaden proces symulacji (w tym procesy zombie i osierocone), a `ipcs` nie pokazuje żadnych semaforów, segmentów pamięci dzielonej ani kolejek komunikatów utworzonych przez program.

### Test 1: Limit N przy dużym obciążeniu

- **Cel:** sprawdzić, czy semafor limitu N działa przy wielu procesach naraz.
- **Przebieg:** 5000 turystów, N = 50, krótkie czasy przejść. Przez cały czas działania odczytujemy liczbę osób na terenie dolnej stacji.
- **Oczekiwany wynik:** liczba osób na terenie dolnej stacji nigdy nie przekracza N. Symulacja kończy się w przewidzianym czasie, żaden proces nie zostaje zablokowany na stałe.

### Test 2: Krzesełka i składy

- **Cel:** sprawdzić synchronizację przy kompletowaniu składów i zajmowaniu krzesełek.
- **Przebieg:** duża liczba pieszych, rowerzystów i opiekunów z dziećmi. Każdy odjazd krzesełka jest zapisywany w logu razem ze składem.
- **Oczekiwany wynik:** nigdy więcej niż 36 zajętych krzesełek naraz. Każdy skład spełnia zasady z pkt 2.2. Każde dziecko 4-7 lat jedzie na tym samym krzesełku co jego opiekun, a opiekun ma przy sobie najwyżej dwoje dzieci.

### Test 3: Zatrzymanie i wznowienie kolei

- **Cel:** sprawdzić obsługę sygnałów i komunikację między pracownikami.
- **Przebieg:** w trakcie ruchu wysyłamy `SIGUSR1` do pracownika1, a po chwili do pracownika2. Za drugim razem drugi pracownik celowo opóźnia odpowiedź o gotowości.
- **Oczekiwany wynik:** w czasie postoju nikt nie wsiada ani nie wysiada. Kolej rusza dopiero po odpowiedzi o gotowości od drugiego pracownika, także wtedy, gdy ta odpowiedź się opóźnia. Po wznowieniu ruch odbywa się normalnie.

### Test 4: Koniec dnia

- **Cel:** sprawdzić zamykanie stacji o Tk i kończenie pracy wszystkich procesów.
- **Przebieg:** krótki dzień (Tk kilkanaście sekund po Tp) przy dużym ruchu, tak żeby o Tk na peronie i na terenie dolnej stacji byli ludzie.
- **Oczekiwany wynik:** po Tk bramki odrzucają karnety. Wszyscy z peronu dojeżdżają na górę, a osoby z terenu dolnej stacji wychodzą bez wjazdu. 3 sekundy po ostatniej wysiadce kolej jest wyłączona, a wszystkie procesy zakończone.

### Test 5: Limit procesów użytkownika

- **Cel:** sprawdzić zachowanie programu, gdy system odmawia utworzenia procesu.
- **Przebieg:** uruchomienie z obniżonym limitem procesów (`ulimit -u`) i liczbą turystów znacznie większą niż limit.
- **Oczekiwany wynik:** gdy `fork()` zwraca błąd, program zgłasza go i czeka na zakończenie innych procesów, zamiast przerwać działanie. Symulacja kończy się poprawnie, bez procesów zombie.

### Test 6: Przerwanie w trakcie działania

- **Cel:** sprawdzić sprzątanie zasobów przy nieoczekiwanym zakończeniu procesów.
- **Przebieg:** w trakcie ruchu zabijamy (`SIGKILL`) kilku losowych turystów, a na koniec przerywamy całą symulację (`SIGINT`, czyli Ctrl+C).
- **Oczekiwany wynik:** miejsca zajmowane przez zabitych turystów wracają do puli, więc liczniki limitu N i krzesełek pozostają poprawne. Po Ctrl+C wszystkie procesy się kończą, nie zostają procesy osierocone, a struktury IPC są usunięte.

### Test 7: VIP

- **Cel:** sprawdzić pierwszeństwo VIP-ów przy bramkach wejściowych.
- **Przebieg:** przed bramkami czeka kolejka zwykłych turystów, limit N jest zapełniony, przychodzą VIP-y.
- **Oczekiwany wynik:** gdy zwalnia się miejsce, VIP wchodzi przed czekającymi zwykłymi turystami. Limit N nie zostaje przekroczony.

### Test 8: Rejestr przejść a raport dzienny

- **Cel:** sprawdzić spójność danych zapisywanych przez wiele procesów do wspólnego rejestru.
- **Przebieg:** pełny dzień symulacji z wieloma wjazdami na karnetach czasowych i dziennych.
- **Oczekiwany wynik:** dla każdego karnetu liczba zjazdów w raporcie dziennym równa się liczbie przejść na peron zapisanych w rejestrze. Żaden wpis nie ginie i nie jest zdublowany.
