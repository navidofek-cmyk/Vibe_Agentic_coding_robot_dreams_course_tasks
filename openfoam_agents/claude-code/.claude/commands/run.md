# /run – Spusť simulaci a analyzuj výsledky

Spusť OpenFOAM simulaci pro případ: $ARGUMENTS

## Instrukce pro agenta

### Krok 1 – Příprava

```bash
cd workspace/cases/<case>
blockMesh 2>&1 | tail -5
checkMesh 2>&1 | grep -E "cells|faces|Max|Overall"
```

### Krok 2 – Spuštění simulace

```bash
foamRun 2>&1 | tee log.<solver>
```

Nebo přes Docker (pokud OpenFOAM není lokálně):
```bash
docker run --rm -v $(pwd):/case openfoam/openfoam10-paraview510 \
  bash -c "source /opt/openfoam10/etc/bashrc && cd /case && foamRun 2>&1 | tee log.solver"
```

### Krok 3 – Analýza konvergence

```bash
python3 examples/check_convergence.py log.<solver>
```

Výstup ukáže:
- Průběh residuálů pro každé pole (p, U, k, epsilon)
- Zda simulace konvergovala (residuál < 1e-5)
- Počet iterací

### Krok 4 – Vykreslení residuálů

```bash
python3 examples/plot_residuals.py log.<solver>
```

Uloží `residuals.png` do adresáře případu.

### Krok 5 – Pedagogická zpětná vazba

Po každém spuštění:
- Ukaž výsledné residuály
- Pokud divergovalo: polož otázku o příčině
- Pokud konvergovalo: navrhni změnu parametrů pro prozkoumání
- Aktualizuj `progress.json`

### Formát výstupu

```
🚀 Spouštím simulaci: <case>
   Solver: <solver>
   Mesh: X buněk

⏱  Simulace dokončena za Xs

📉 Konvergence:
   p:       residuál = X.XXe-X  ✅/❌
   Ux:      residuál = X.XXe-X  ✅/❌
   k:       residuál = X.XXe-X  ✅/❌

💡 Zamysli se: <otázka ke konvergenci nebo fyzice>
```
