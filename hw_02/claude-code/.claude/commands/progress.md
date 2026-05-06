# /progress – Přehled pokroku studenta

Zobraz přehled pokroku studenta.

## Instrukce pro agenta

### Krok 1 – Načti data
Pomocí MCP `sqlite` serveru spusť:
```sql
SELECT
    lesson_name,
    status,
    tests_passed,
    tests_total,
    attempts,
    last_activity
FROM student_progress
ORDER BY last_activity DESC;
```

### Krok 2 – Spočítej statistiky
Pomocí MCP `filesystem`:
- Počet souborů v `exercises/` (celkem cvičení)
- Počet souborů v `lessons/` (celkem lekcí)
- Spusť `pytest tests/ --co -q` pro přehled testů

### Krok 3 – Vygeneruj zprávu

```
═══════════════════════════════════════
  PYTHON LEARNING ASSISTANT – POKROK
═══════════════════════════════════════

Celkový pokrok:    [████████░░] 80%
Dokončené lekce:   8 / 10
Úspěšné testy:     24 / 30

LEKCE:
  ✅ Základní datové typy     (3/3 testy)
  ✅ Podmínky a cykly         (4/4 testy)
  ✅ Funkce                   (3/3 testy)
  🔄 Seznamy a slovníky       (2/4 testy) ← právě zde
  ⬜ OOP – základy
  ⬜ OOP – dědičnost

SILNÉ STRÁNKY:
  • Dobré pojmenování proměnných
  • Čistá syntaxe

OBLASTI KE ZLEPŠENÍ:
  • List comprehensions (3 neúspěšné pokusy)
  • Rekurzivní funkce

DOPORUČENÍ:
  Zkus: /lesson list-comprehensions
═══════════════════════════════════════
```

### Krok 4 – Uložení do paměti
Pomocí MCP `memory` serveru ulož aktuální stav:
- Kde student skončil
- Co mu dělá problémy
- Doporučená další lekce
