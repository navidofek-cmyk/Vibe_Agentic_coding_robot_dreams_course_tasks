"""
Ukázka sub-agentního workflow v Claude Code

Tento soubor dokumentuje, jak Claude Code interně orchestruje
specializované sub-agenty pro komplexní úkoly.

Použití: claude "Přečti subagent_demo.py a spusť /workflow rekurze"
"""


def popis_subagentniho_systemu():
    """
    Claude Code má vestavěné typy sub-agentů:

    1. Explore Agent
       - Rychlé read-only prohledávání kódu
       - Hledání souborů, symbolů, vzorů
       - Nepíše kód, jen hledá

    2. Plan Agent
       - Architektonické rozhodování
       - Vytváří implementační plány
       - Identifikuje kritické soubory

    3. General-purpose Agent
       - Všestranný agent pro složité úkoly
       - Má přístup ke všem nástrojům
       - Paralelní zpracování nezávislých úkolů
    """
    pass


# Příklad: jak Claude Code spouští sub-agenty paralelně
# (pseudokód reprezentující vnitřní chování agenta)

SUBAGENT_WORKFLOW_PRIKLAD = """
# Uživatel napíše: /workflow rekurze
#
# Claude Code interně:
#
# 1. PARALELNĚ spustí:
#
#    Agent(
#        subagent_type="Explore",
#        prompt="Najdi v exercises/ a lessons/ všechny soubory
#                s rekurzivními funkcemi. Vrať seznam souborů."
#    )
#
#    Agent(
#        subagent_type="Plan",
#        prompt="Navrhni 5-bodovou osnovu lekce o rekurzi
#                pro Python začátečníky."
#    )
#
#    Agent(
#        subagent_type="general-purpose",
#        prompt="Pomocí brave-search najdi 3 nejlepší zdroje
#                o rekurzi v Pythonu. Vrať URL a stručný popis."
#    )
#
# 2. Čeká na dokončení VŠECH 3 agentů (~35s vs ~90s sekvenčně)
#
# 3. Zkombinuje výsledky:
#    - existující vzory z Explore
#    - osnovu z Plan
#    - dokumentaci z GP agent
#
# 4. Zapíše soubory:
#    - lessons/rekurze.md
#    - exercises/rekurze_cviceni.py
#    - tests/test_rekurze.py
"""


def vytvor_ukazku_lekce_rekurze():
    """Příklad jak by vypadal výstup /workflow rekurze."""

    lekce = """
# Lekce: Rekurze

## Co se naučíš
- Co je rekurze a jak funguje call stack
- Jak napsat správný base case
- Kdy použít rekurzi vs cyklus

## Teorie

Rekurze je technika, kdy funkce volá samu sebe.

```python
def faktorial(n):
    # Base case – bez toho bychom cyklili donekonečna
    if n <= 1:
        return 1
    # Rekurzivní případ
    return n * faktorial(n - 1)

print(faktorial(5))  # 120
```

Jak to funguje krok po kroku:
faktorial(5)
  → 5 * faktorial(4)
    → 4 * faktorial(3)
      → 3 * faktorial(2)
        → 2 * faktorial(1)
          → 1  (base case!)
        → 2 * 1 = 2
      → 3 * 2 = 6
    → 4 * 6 = 24
  → 5 * 24 = 120

## Tvůj úkol
Otevři `exercises/rekurze_cviceni.py` a dokonči cvičení.
Zkontroluj: `pytest tests/test_rekurze.py -v`
"""
    return lekce


if __name__ == "__main__":
    print("Sub-agentní workflow systém Claude Code")
    print("="*45)
    print(SUBAGENT_WORKFLOW_PRIKLAD)
    print("\nPříklad vygenerované lekce:")
    print(vytvor_ukazku_lekce_rekurze())
