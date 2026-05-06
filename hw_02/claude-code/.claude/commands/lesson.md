# /lesson – Generování nové lekce

Vygeneruj novou lekci Pythonu na téma: $ARGUMENTS

## Instrukce pro agenta

1. Použij **Plan sub-agenta** pro vytvoření osnovy lekce (5 bodů).
2. Pro každý bod osnovy použij MCP `filesystem` server pro ověření, zda podobná lekce neexistuje.
3. Pokud existuje podobná lekce, upozorni studenta a nabídni rozšíření.
4. Vytvoř soubory:
   - `lessons/<tema>.md` – výkladový text s příklady
   - `exercises/<tema>_cviceni.py` – 3 cvičení (lehké, střední, těžké)
   - `tests/test_<tema>.py` – pytest testy ke každému cvičení
5. Aktualizuj `progress.json` – přidej novou lekci se stavem "not_started"

## Formát lekce (`lessons/<tema>.md`)

```markdown
# Lekce: <téma>

## Co se naučíš
- bod 1
- bod 2

## Teorie
<výklad s příklady>

## Tvůj úkol
Otevři `exercises/<tema>_cviceni.py` a dokonči cvičení.
Zkontroluj svůj pokrok: `pytest tests/test_<tema>.py -v`
```

## Formát cvičení (`exercises/<tema>_cviceni.py`)

```python
"""
Cvičení: <téma>
Instrukce: <co má student udělat>
"""

# Cvičení 1 – Lehké
def cviceni_1():
    # TODO: Doplň implementaci
    pass

# Cvičení 2 – Střední
def cviceni_2():
    # TODO: Doplň implementaci
    pass

# Cvičení 3 – Těžké
def cviceni_3():
    # TODO: Doplň implementaci
    pass
```

## Poznámka

Po vytvoření spusť: `pytest tests/test_<tema>.py -v` pro ověření, že testy fungují se vzorovou implementací.
