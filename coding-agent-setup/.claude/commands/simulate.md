# /simulate – Simulace výukové session mentor↔student

Spusť kompletní simulaci na téma: $ARGUMENTS

Každý krok vypiš IHNED na výstup (nepočkej na konec) – ať je dialog viditelný v reálném čase.

**Časový limit:** Celá session musí skončit do **3 minut**. Každý agent call max **60 sekund**.
Pokud agent neodpoví do 60s, ukonči krok s poznámkou "⏱ Timeout – přecházím dál."

## Krok 1 – Student píše kód

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
👨‍🎓 STUDENT píše kód pro téma: $ARGUMENTS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť Agent(subagent_type="student") s promptem:
```
Napiš Python funkci na téma: $ARGUMENTS
- Použij Write tool a zapiš ji přímo do workspace/exercises/simulate_cviceni.py
- Udělej JEDNU typickou začátečnickou chybu (špatný base case, chybějící return, apod.)
- Po zapsání vypiš: co jsi napsal a jakou chybu jsi záměrně udělal
```

Nepouštěj žádné bash příkazy před voláním student agenta – adresáře existují.

Vypiš odpověď studenta celou, ať je vidět co napsal.

## Krok 2 – Spuštění testů

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧪 MENTOR spouští testy
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Zkontroluj jestli `workspace/tests/test_simulate_cviceni.py` existuje (Read nebo list). Pokud existuje, použij ho. Pokud ne, vygeneruj ho podle cvičení – ale NEpokoušej se ho přepsat pokud existuje.

Spusť:
```bash
cd workspace && pytest tests/test_simulate_cviceni.py -v --tb=short 2>&1 || true
```

Vypiš celý výstup testů.

## Krok 3 – Mentor klade otázku

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧑‍🏫 MENTOR reaguje
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Na základě výsledků testů formuluj pedagogickou otázku. NEOPRAVUJ kód – ptej se.
Příklad: "Test `test_faktorial_zakladni` selhal. Co si myslíš, co vrátí tvoje funkce pro n=0?"

## Krok 4 – Student odpovídá a opravuje

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
👨‍🎓 STUDENT přemýšlí a opravuje
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť Agent(subagent_type="student") s kontextem:
```
Mentor se tě zeptal: <otázka mentora>
Výsledky testů byly: <výpis testů>
Přemýšlej nahlas co je špatně, pak oprav workspace/exercises/simulate_cviceni.py
```

Vypiš celou odpověď studenta – přemýšlení i opravu.

## Krok 5 – Finální testy

Vypiš:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧪 FINÁLNÍ TESTY po opravě
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Spusť testy znovu a vypiš výsledek.

## Závěr

Vypiš shrnutí session:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 VÝSLEDEK SESSION
  Téma:          $ARGUMENTS
  Chyba studenta: <co bylo špatně>
  Co se naučil:   <klíčový poznatek>
  Testy:          X/Y prošly
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
