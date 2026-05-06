"""
Ukázka: Codex CLI refaktoring

Tento soubor ukazuje záměrně špatně napsaný kód,
který student nechá Codex zrefaktorovat.

Použití:
  codex "zrefaktoruj tento kód, ale nejdřív mi vysvětli co je špatně"
"""


# PŘED refaktoringem (špatný kód pro studenty)
def f(x):
    r = []
    for i in range(len(x)):
        if x[i] % 2 == 0:
            r.append(x[i] * x[i])
    return r


def g(s):
    result = ""
    for i in range(len(s)):
        if i % 2 == 0:
            result = result + s[i].upper()
        else:
            result = result + s[i].lower()
    return result


def h(lst):
    d = {}
    for item in lst:
        if item in d:
            d[item] = d[item] + 1
        else:
            d[item] = 1
    return d


# PO refaktoringu – co by Codex navrhoval
# (student by to měl napsat sám po nápovědě)

def ctverec_sudych_cisel(cisla: list[int]) -> list[int]:
    """Vrátí čtverce sudých čísel ze seznamu."""
    return [x**2 for x in cisla if x % 2 == 0]


def stridat_velikost_pismen(text: str) -> str:
    """Střídá velká a malá písmena v textu."""
    return "".join(
        znak.upper() if i % 2 == 0 else znak.lower()
        for i, znak in enumerate(text)
    )


def pocet_vyskytu(seznam: list) -> dict:
    """Spočítá výskyt každého prvku v seznamu."""
    from collections import Counter
    return dict(Counter(seznam))


# Otázky pro studenta (Codex je položí):
OTAZKY_PRO_STUDENTA = [
    "Co dělá funkce f()? Jak bys ji přejmenoval?",
    "Proč je range(len(x)) horší než iterace přes x přímo?",
    "Jak funguje list comprehension? Napiš jeden sám.",
    "Co je Counter a kde ho najdeš v dokumentaci?",
]

if __name__ == "__main__":
    # Test původního kódu
    test_data = [1, 2, 3, 4, 5, 6, 7, 8]
    assert f(test_data) == ctverec_sudych_cisel(test_data), "Funkce f() a refaktorovaná verze se liší!"
    print("✅ Obě verze vrací stejný výsledek")
    print(f"   Vstup: {test_data}")
    print(f"   Výstup: {ctverec_sudych_cisel(test_data)}")

    print("\nOtázky k zamyšlení:")
    for i, otazka in enumerate(OTAZKY_PRO_STUDENTA, 1):
        print(f"  {i}. {otazka}")
