# /workflow – Ukázka paralelního sub-agentního workflow

Spusť ukázkový workflow pro téma: $ARGUMENTS

## Instrukce pro agenta

Toto je ukázka jak Claude Code orchestruje více sub-agentů PARALELNĚ.

### Workflow: Vytvoření kompletní lekce

Spusť SIMULTÁNNĚ tyto 3 sub-agenty:

```
┌─────────────────────────────────────────────────┐
│           HLAVNÍ AGENT (orchestrátor)           │
│                                                 │
│   ┌──────────────┐  ┌──────────────┐  ┌──────┐ │
│   │ Explore Agent│  │  Plan Agent  │  │ GP   │ │
│   │              │  │              │  │Agent │ │
│   │ Prohledá     │  │ Navrhne      │  │      │ │
│   │ existující   │  │ strukturu    │  │Najde │ │
│   │ kód v repo   │  │ lekce        │  │doku- │ │
│   │              │  │              │  │menta-│ │
│   │ → seznam     │  │ → osnova     │  │ci    │ │
│   │   existují-  │  │   5 bodů     │  │      │ │
│   │   cích vzorů │  │              │  │→ url │ │
│   └──────────────┘  └──────────────┘  └──────┘ │
│          │                 │              │      │
│          └────────┬────────┘              │      │
│                   ▼                       │      │
│            Merge výsledků  ◄──────────────┘      │
│                   │                              │
│                   ▼                              │
│         Vytvoř soubory lekce                     │
└─────────────────────────────────────────────────┘
```

### Implementace

**Agent 1 – Explore** (rychlý):
```
Prohledej složku exercises/ a lessons/ a najdi všechny
vzory kódu týkající se tématu: $ARGUMENTS
Vrať: seznam existujících souborů a použitých vzorů.
```

**Agent 2 – Plan** (architekt):
```
Navrhni 5-bodovou osnovu lekce na téma: $ARGUMENTS
Úroveň: začátečník Python.
Vrať: osnovu jako seznam s popisem každého bodu.
```

**Agent 3 – General-purpose** (výzkum):
```
Pomocí MCP brave-search najdi:
1. Nejlepší Python dokumentaci pro téma: $ARGUMENTS
2. Nejčastější chyby začátečníků v tomto tématu
Vrať: 2-3 URL a seznam 3 nejčastějších chyb.
```

### Merge & výstup

Po dokončení všech 3 sub-agentů:
1. Zkombinuj výsledky
2. Vytvoř `lessons/$ARGUMENTS.md` s odkazem na zdroje
3. Vytvoř `exercises/$ARGUMENTS_cviceni.py`
4. Vytvoř `tests/test_$ARGUMENTS.py`

### Proč sub-agenti?

| Bez sub-agentů | S sub-agenty |
|---------------|-------------|
| 3 úkoly sekvenčně: ~90s | 3 úkoly paralelně: ~35s |
| Jeden context window | Izolované kontexty |
| Vše v hlavním agentovi | Specializace |
