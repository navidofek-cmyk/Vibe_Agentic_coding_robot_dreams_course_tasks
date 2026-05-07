"""
Ukázkové cvičení – Fibonacci
Instrukce: Implementuj funkci fibonacci(n) která vrátí n-té Fibonacciho číslo.
"""


def fibonacci(n: int) -> int:
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)


def faktorial(n: int) -> int:
    if n == 0:
        return 1
    return n * faktorial(n - 1)
