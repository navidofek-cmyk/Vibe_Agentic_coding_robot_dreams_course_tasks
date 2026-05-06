"""
Testy pro cvičení: Rekurze
Spuštění: pytest tests/test_rekurze_cviceni.py -v
"""
import pytest
from exercises.rekurze_cviceni import faktorial, fibonacci, hanoi


# ── faktorial ─────────────────────────────────────────────────────────

def test_faktorial_zakladni():
    assert faktorial(0) == 1
    assert faktorial(1) == 1

def test_faktorial_male():
    assert faktorial(2) == 2
    assert faktorial(3) == 6
    assert faktorial(4) == 24

def test_faktorial_vetsi():
    assert faktorial(5) == 120
    assert faktorial(10) == 3628800


# ── fibonacci ─────────────────────────────────────────────────────────

def test_fibonacci_zakladni():
    assert fibonacci(0) == 0
    assert fibonacci(1) == 1

def test_fibonacci_dalsi():
    assert fibonacci(2) == 1
    assert fibonacci(3) == 2
    assert fibonacci(4) == 3
    assert fibonacci(5) == 5

def test_fibonacci_vetsi():
    assert fibonacci(6) == 8
    assert fibonacci(10) == 55


# ── hanoi ─────────────────────────────────────────────────────────────

def test_hanoi_jeden_disk():
    assert hanoi(1, 'A', 'C', 'B') == [('A', 'C')]

def test_hanoi_dva_disky():
    assert hanoi(2, 'A', 'C', 'B') == [('A', 'B'), ('A', 'C'), ('B', 'C')]

def test_hanoi_tri_disky():
    tahy = hanoi(3, 'A', 'C', 'B')
    assert len(tahy) == 7
    assert tahy[0] == ('A', 'C')
    assert tahy[-1] == ('A', 'C')

def test_hanoi_pocet_tahu():
    """Počet tahů pro n disků je vždy 2^n - 1."""
    for n in range(1, 6):
        tahy = hanoi(n, 'A', 'C', 'B')
        assert len(tahy) == 2**n - 1, f"Pro {n} disky očekáváme {2**n - 1} tahů"
