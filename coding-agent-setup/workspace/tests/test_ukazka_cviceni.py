"""
Testy pro ukázkové cvičení – spusť: pytest tests/test_ukazka.py -v
"""
import pytest
from exercises.ukazka_cviceni import fibonacci, faktorial


def test_fibonacci_zakladni():
    assert fibonacci(0) == 0
    assert fibonacci(1) == 1
    assert fibonacci(2) == 1


def test_fibonacci_dalsi():
    assert fibonacci(5) == 5
    assert fibonacci(10) == 55


def test_faktorial_zakladni():
    assert faktorial(0) == 1
    assert faktorial(1) == 1


def test_faktorial_dalsi():
    assert faktorial(5) == 120
    assert faktorial(10) == 3628800
