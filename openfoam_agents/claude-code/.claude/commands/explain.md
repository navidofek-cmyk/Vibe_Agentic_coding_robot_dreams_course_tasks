# /explain – Vysvětli OpenFOAM koncept

Vysvětli studentovi: $ARGUMENTS

## Instrukce pro agenta

1. Použij MCP `brave-search` pro dohledání aktuální OpenFOAM dokumentace.
2. Vysvětli koncept ve třech vrstvách:
   - **Fyzika** – co se fyzikálně děje
   - **Numerika** – jak to OpenFOAM řeší matematicky
   - **Konfigurace** – jak to nastavit v case souborech
3. Vždy uveď konkrétní příklad z cvičení studenta.

## Témata a jak je vysvětlit

### Okrajové podmínky (boundary conditions)
- `fixedValue` – Dirichletova podmínka – předepisuješ hodnotu (rychlost víka)
- `zeroGradient` – Neumannova podmínka – předepisuješ nulový gradient (volný výstup)
- `noSlip` – zkratka pro `fixedValue (0 0 0)` – přilnavost na stěně
- `symmetryPlane` – zrcadlová podmínka pro symetrické geometrie

### Solvery
- `icoFoam` – nestacionární laminární proudění, PISO algoritmus
- `simpleFoam` – stacionární turbulentní proudění, SIMPLE algoritmus
- `buoyantSimpleFoam` – přenos tepla s přirozenou konvekcí

### Turbulentní modely
- `kEpsilon` – k-ε model, vhodný pro volné proudění
- `kOmegaSST` – k-ω SST, lepší pro obtékání stěn

### Konvergence a residuály
- Co znamená residuál – míra nesplnění rovnic v každé iteraci
- Jak číst log soubor
- Typické příčiny divergence (velký časový krok, špatná síť, chybné BC)

### Síť (mesh)
- `blockMesh` – strukturovaná síť z hexahedrů definovaná v blockMeshDict
- `checkMesh` – validace kvality sítě (max non-orthogonalita, skewness)

## Formát výstupu

```
📚 Vysvětlení: <téma>

🌊 Fyzika:
<výklad>

🔢 Numerika:
<výklad>

⚙️  Konfigurace v OpenFOAM:
<ukázkový kód z case souborů>

💡 V tvém cvičení to vidíš zde: <konkrétní soubor:řádek>
```
