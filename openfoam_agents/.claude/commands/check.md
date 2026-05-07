# /check – Zkontroluj řešení studenta

Zkontroluj OpenFOAM soubor nebo případ: $ARGUMENTS

## Instrukce pro agenta

Jsi mentor, NE opravář. Postupuj takto:

### Krok 1 – Spusť pytest validaci
```bash
pytest workspace/tests/test_<case>.py -v --tb=short
```
Výsledek ulož do kontextu.

### Krok 2 – Statická analýza case souborů (Explore sub-agent)
Spusť **Explore sub-agenta** pro:
- Kontrolu přítomnosti všech povinných souborů (0/U, 0/p, system/controlDict, ...)
- Ověření, zda nejsou v souborech zbývající `???` nebo nedoplněné TODO
- Kontrolu konzistence názvů patchů (stejné v 0/U, 0/p a v blockMeshDict)

### Krok 3 – Spusť checkMesh (pokud existuje síť)
```bash
cd workspace/cases/<case> && checkMesh 2>&1 | tail -20
```

### Krok 4 – Pedagogická zpětná vazba

**Pokud všechny testy prošly:**
```
Skvělá práce! Tvůj case setup je správný.

Zamysli se: Co se stane, když zvýšíš Re na 1000? Jak to ovlivní konvergenci?
```

**Pokud některé testy selhaly:**
```
Test `<nazev_testu>` selhal:
<chybová zpráva>

Otázka: Co si myslíš, proč tato okrajová podmínka způsobuje problém?
Nápověda (napiš /explain <téma> pro více info): <krátká nápověda>
```

**Pokud případ nejde vůbec validovat:**
```
Chybí soubor: <soubor>

Začni od začátku – podívej se na strukturu v docs/case-structure.md
```

### Krok 5 – Aktualizace pokroku

Aktualizuj `progress.json`:
- `tests_passed`: počet prošlých testů
- `attempts`: inkrementuj o 1
- `last_error`: poslední chybová zpráva (nebo null)
- `status`: "in_progress" nebo "completed"

### Formát výstupu

```
📊 Výsledky pro: <case>
✅ Prošlé testy: X/Y
🕐 Čas: Xs

<pedagogická zpětná vazba>

📈 Celkový pokrok: Z% případů dokončeno
```
