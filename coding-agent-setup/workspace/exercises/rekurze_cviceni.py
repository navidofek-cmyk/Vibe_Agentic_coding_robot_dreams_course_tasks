"""
Cvičení: Rekurze
Instrukce: Implementuj každou funkci pomocí rekurze (funkce volá samu sebe).
           Nesmíš použít cyklus (for, while).
"""


def faktorial(n: int) -> int:
    """
    Vrátí faktoriál čísla n.
    Příklad: faktorial(5) == 120
    """
    if n == 0:
        return 1
    return n * faktorial(n - 1)


def fibonacci(n: int) -> int:
    """
    Vrátí n-té Fibonacciho číslo (indexováno od 0).
    Příklad: fibonacci(0)==0, fibonacci(1)==1, fibonacci(6)==8
    """
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)


def hanoi(n: int, odkud: str, kam: str, pres: str) -> list[tuple]:
    """
    Vrátí seznam tahů pro přesunutí n disků z tyče 'odkud' na tyč 'kam'.
    Každý tah je tuple (odkud, kam).
    Příklad: hanoi(2, 'A', 'C', 'B') == [('A','B'), ('A','C'), ('B','C')]
    """
    if n == 1:
        return [(odkud, kam)]
    return hanoi(n - 1, odkud, pres, kam) + [(odkud, kam)] + hanoi(n - 1, pres, kam, odkud)
