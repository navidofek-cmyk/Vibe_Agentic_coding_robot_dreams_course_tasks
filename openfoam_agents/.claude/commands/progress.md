# /progress – Přehled pokroku studenta

Zobraz aktuální stav studia OpenFOAM.

## Instrukce pro agenta

1. Načti `progress.json` pomocí MCP `filesystem` serveru.
2. Dotaž se SQLite DB pro historii simulací:
   ```sql
   SELECT case_name, attempts, last_run, convergence_status
   FROM simulations ORDER BY last_run DESC LIMIT 10;
   ```
3. Zobraz přehlednou tabulku pokroku.
4. Doporuč další cvičení na základě aktuální úrovně.

## Formát výstupu

```
📈 Tvůj pokrok v OpenFOAM

┌─────────────────────────┬──────────────┬──────────┬────────────┐
│ Případ                  │ Status       │ Pokusy   │ Konvergence│
├─────────────────────────┼──────────────┼──────────┼────────────┤
│ cavity_cviceni          │ ✅ completed  │ 3        │ ✅ ano     │
│ pitzDaily_cviceni       │ 🔄 in_progress│ 1        │ ❌ ne      │
│ elbow_cviceni           │ ⬜ not_started│ 0        │ –          │
└─────────────────────────┴──────────────┴──────────┴────────────┘

🎯 Celkový pokrok: 1/3 případů dokončeno (33%)

💡 Doporučení: Dokončíš pitzDaily – zkus zvýšit relaxační faktory v fvSolution.
```
