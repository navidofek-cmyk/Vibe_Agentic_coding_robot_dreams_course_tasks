"""Testy pro python-linter MCP server."""

from pathlib import Path
import sys

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))
from server import check_all, check_code, type_check


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture
def clean_file(tmp_path: Path) -> Path:
    f = tmp_path / "clean.py"
    f.write_text('def add(a: int, b: int) -> int:\n    return a + b\n')
    return f


@pytest.fixture
def dirty_file(tmp_path: Path) -> Path:
    f = tmp_path / "dirty.py"
    f.write_text('import os\nimport sys\nx=1\nprint(x)\n')  # unused import, no spaces
    return f


@pytest.fixture
def type_error_file(tmp_path: Path) -> Path:
    f = tmp_path / "type_error.py"
    f.write_text('def greet(name: str) -> str:\n    return 42\n')  # wrong return type
    return f


# ---------------------------------------------------------------------------
# check_code (ruff)
# ---------------------------------------------------------------------------

class TestCheckCode:
    def test_clean_file_passes(self, clean_file: Path) -> None:
        result = check_code(str(clean_file))
        assert "žádné problémy" in result

    def test_dirty_file_has_issues(self, dirty_file: Path) -> None:
        result = check_code(str(dirty_file))
        assert "ruff" in result

    def test_nonexistent_file(self) -> None:
        result = check_code("/tmp/neexistuje.py")
        assert "nenalezen" in result

    def test_non_python_file(self, tmp_path: Path) -> None:
        f = tmp_path / "config.json"
        f.write_text('{}')
        result = check_code(str(f))
        assert "Není Python soubor" in result


# ---------------------------------------------------------------------------
# type_check (mypy)
# ---------------------------------------------------------------------------

class TestTypeCheck:
    def test_clean_file_passes(self, clean_file: Path) -> None:
        result = type_check(str(clean_file))
        assert "žádné typové chyby" in result

    def test_type_error_detected(self, type_error_file: Path) -> None:
        result = type_check(str(type_error_file))
        assert "mypy" in result

    def test_nonexistent_file(self) -> None:
        result = type_check("/tmp/neexistuje.py")
        assert "nenalezen" in result


# ---------------------------------------------------------------------------
# check_all
# ---------------------------------------------------------------------------

class TestCheckAll:
    def test_returns_both_results(self, clean_file: Path) -> None:
        result = check_all(str(clean_file))
        assert "ruff" in result
        assert "mypy" in result

    def test_nonexistent_file(self) -> None:
        result = check_all("/tmp/neexistuje.py")
        assert "nenalezen" in result
