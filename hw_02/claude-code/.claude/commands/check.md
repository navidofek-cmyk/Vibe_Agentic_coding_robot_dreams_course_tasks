# /check – Zkontroluj řešení studenta

Zkontroluj soubor: $ARGUMENTS

## Instrukce pro agenta

Jsi mentor, NE opravář. Postupuj takto:

### Krok 1 – Spusť testy
```bash
pytest tests/test_<soubor>.py -v --tb=short
```
Výsledek ulož do kontextu.

### Krok 2 – Statická analýza (Explore sub-agent)
Spusť **Explore sub-agenta** pro:
- Nalezení code smells (duplicitní kód, příliš dlouhé funkce)
- Kontrolu pojmenování proměnných (snake_case?)
- Hledání nepoužitých importů

### Krok 3 – Pedagogická zpětná vazba

Podle výsledků testů:

**Pokud všechny testy prošly:**
```
Skvělá práce! Tvůj kód funguje správně.

Zamysli se: Dalo by se to napsat jinak? Zkus přepsat funkci jako one-liner.
```

**Pokud některé testy selhaly:**
```
Test `<nazev_testu>` selhal s chybou:
<chybová zpráva>

Otázka k zamyšlení: Co si myslíš, proč to selhalo?
Nápověda (typ Enter pro zobrazení): <skrytá nápověda>
```

**Pokud kód nefunguje vůbec:**
```
Nastala chyba při importu/spuštění: <chyba>

Začni od začátku – podívej se na první příklad v lekci.
```

### Krok 4 – Aktualizace pokroku

Aktualizuj `progress.json`:
- `tests_passed`: počet prošlých testů
- `attempts`: inkrementuj o 1
- `last_error`: poslední chybová zpráva (nebo null)
- `status`: "in_progress" nebo "completed"

### Formát výstupu

```
📊 Výsledky pro: <soubor>
✅ Prošlé testy: X/Y
⏱  Čas: Xs

<pedagogická zpětná vazba>

📈 Celkový pokrok: <Z>% lekcí dokončeno
```
