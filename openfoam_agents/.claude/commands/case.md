# /case – Generování nového OpenFOAM cvičení

Vygeneruj nový OpenFOAM případ na téma: $ARGUMENTS

## Instrukce pro agenta

1. Použij **Plan sub-agenta** pro vytvoření struktury případu (jaký solver, geometrie, okrajové podmínky).
2. Pro každý soubor použij MCP `filesystem` server pro ověření, zda podobný případ neexistuje.
3. Pokud existuje podobný případ, upozorni studenta a nabídni rozšíření.
4. Vytvoř adresářovou strukturu OpenFOAM případu:
   - `workspace/cases/<tema>_cviceni/0/` – počáteční a okrajové podmínky (s TODO sekce)
   - `workspace/cases/<tema>_cviceni/constant/` – fyzikální vlastnosti, mesh
   - `workspace/cases/<tema>_cviceni/system/` – controlDict, fvSchemes, fvSolution
   - `workspace/tests/test_<tema>.py` – pytest testy pro validaci
5. Aktualizuj `progress.json` – přidej nový případ se stavem "not_started"

## Formát TODO v case souborech

Místa, která student musí doplnit, označuj takto:
```
// TODO: Nastav rychlost víka – zkus 1 m/s ve směru osy x
value           uniform (??? 0 0);
```

## Typy případů

- `cavity` – lid-driven cavity, icoFoam, začátečník
- `pipe` – proudění v potrubí, simpleFoam, začátečník
- `pitzDaily` – backward-facing step, simpleFoam + turbulence, středně pokročilý
- `elbow` – tepelný přenos, buoyantSimpleFoam, pokročilý

## Formát výstupu

```
📁 Vytvořen případ: workspace/cases/<tema>_cviceni/
   ├── 0/          (X polí – Y s TODO)
   ├── constant/   (transportProperties, turbulenceProperties)
   └── system/     (controlDict, fvSchemes, fvSolution)

🧪 Test: workspace/tests/test_<tema>.py (Z testovacích případů)

📖 Spusť: pytest workspace/tests/test_<tema>.py -v
```
