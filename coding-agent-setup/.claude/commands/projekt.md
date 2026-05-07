# /projekt – Simulace mini projektu mentor↔student

Spusť simulaci projektu: **Bankovní účet (BankAccount)**

Jde o složitější simulaci než /simulate – student píše celou třídu,
mentor dává zpětnou vazbu ve **dvou kolech**.

---

## Krok 1 – Student navrhuje a píše třídu

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
👨‍🎓 STUDENT navrhuje třídu BankAccount
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť Agent(subagent_type="student") s promptem:
```
Napiš Python třídu BankAccount do workspace/exercises/bank_account.py

Třída musí mít:
- __init__(self, owner, balance=0)
- deposit(amount)     – vložení peněz
- withdraw(amount)    – výběr peněz
- get_balance()       – vrátí zůstatek
- history             – list transakcí

Udělej 1-2 typické začátečnické OOP chyby, například:
- withdraw() nedokontroluje jestli je dostatek peněz (záporný zůstatek)
- deposit() přijme zápornou částku
- chybí validace vstupů

Po napsání vypiš: jaký kód jsi napsal a jaké chyby jsi záměrně udělal.
```

Vypiš celou odpověď studenta.

---

## Krok 2 – První kolo testů

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧪 KOLO 1 – Mentor spouští testy
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť:
```bash
cd workspace && python -m pytest tests/test_bank_account.py -v --tb=short 2>&1 || true
```

Vypiš celý výstup.

---

## Krok 3 – Mentor: první zpětná vazba

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧑‍🏫 MENTOR – první zpětná vazba
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Vybral JEDNO nejdůležitější selhání. Polož pedagogickou otázku – NEOPRAVUJ kód.
Příklad: "Co se stane s účtem když zavoláš withdraw(500) ale máš jen 100?"

---

## Krok 4 – Student opravuje (1. oprava)

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
👨‍🎓 STUDENT opravuje – kolo 1
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť Agent(subagent_type="student"):
```
Mentor se zeptal: <otázka>
Oprav workspace/exercises/bank_account.py – jen problém který mentor zmínil.
Přemýšlej nahlas, pak oprav.
```

---

## Krok 5 – Druhé kolo testů + druhá zpětná vazba

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧪 KOLO 2 – Testy po první opravě
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť testy znovu. Pokud ještě selhávají, mentor dá druhou otázku:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧑‍🏫 MENTOR – druhá zpětná vazba
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť Agent(subagent_type="student") pro druhou opravu.

---

## Krok 6 – Finální testy

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧪 FINÁLNÍ TESTY
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť testy naposledy a vypiš výsledek.

---

## Závěr

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 VÝSLEDEK PROJEKTU
  Projekt:         BankAccount
  Chyby studenta:  <seznam chyb>
  Co se naučil:    <klíčové poznatky>
  Testy:           X/Y prošly
  Kola oprav:      N
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**Časový limit:** Max 8 minut celkem. Každý agent call max 90 sekund.
