# /explain – Vysvětli kód nebo koncept

Vysvětli: $ARGUMENTS

## Instrukce pro agenta

Vysvětlení musí respektovat úroveň studenta (začátečník/středně pokročilý).

### Detekce tématu

Pokud `$ARGUMENTS` je:
- **cesta k souboru** → načti soubor pomocí MCP `filesystem`, vysvětli co dělá
- **Python klíčové slovo** (for, while, class, lambda...) → vysvětli syntaxi + použití
- **chybová zpráva** (TypeError, IndexError...) → vysvětli příčinu + jak opravit
- **koncept** (rekurze, OOP, generátory...) → vysvětli teorii + ukázka

### Šablona vysvětlení

```
## Co to je?
<1-2 věty, jednoduše>

## Jak to funguje?
<analogie z reálného světa, pak kód>

## Ukázka
```python
# Minimální příklad
<kód>
```

## Kde to použít?
<2-3 praktické situace>

## Časté chyby
<1-2 nejčastější chyby začátečníků>

## Vyzkoušej sám
<konkrétní mini-úkol pro studenta>
```

### Použití sub-agentů

Pro složité koncepty (OOP, dekorátory, generátory):
1. Spusť **Plan sub-agenta** pro strukturování vysvětlení
2. Použij MCP `brave-search` pro nalezení aktuální Python dokumentace
3. Použij MCP `memory` pro zapamatování, co student v minulosti nepochopil

### Zákazy

- NIKDY nepiš víc než 30 řádků kódu v jednom bloku
- NIKDY nepoužívej termíny bez vysvětlení
- NIKDY neposkytuj kompletní řešení cvičení
