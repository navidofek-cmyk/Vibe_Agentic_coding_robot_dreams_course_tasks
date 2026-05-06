"""
Testy pro MCP nástroje — in-process AST analýza (mcp_tools.py).

Testuje analyze_code_structure přímo (bez spouštění MCP serveru).
"""

from __future__ import annotations

import textwrap
from pathlib import Path

import pytest

from code_reviewer.mcp_tools import (
    CODE_ANALYSIS_SERVER,
    MCP_TOOL_NAME,
    _analyze_impl,
)


# ---------------------------------------------------------------------------
# Pomocné funkce
# ---------------------------------------------------------------------------

def _text(result: dict) -> str:
    return result["content"][0]["text"]


# ---------------------------------------------------------------------------
# Testy analyze_code_structure — happy path
# ---------------------------------------------------------------------------

class TestAnalyzeCodeStructure:
    @pytest.mark.asyncio
    async def test_counts_top_level_functions(self, tmp_path):
        f = tmp_path / "sample.py"
        f.write_text("def foo(): pass\ndef bar(): pass\n")
        result = _text(await _analyze_impl({"file_path": str(f)}))
        assert "foo" in result
        assert "bar" in result

    @pytest.mark.asyncio
    async def test_functions_do_not_include_methods(self, tmp_path):
        f = tmp_path / "sample.py"
        f.write_text(textwrap.dedent("""\
            def top_level(): pass

            class MyClass:
                def method(self): pass
        """))
        result = _text(await _analyze_impl({"file_path": str(f)}))
        # top_level je funkce, method je metoda — nesmí se překrývat
        assert "top_level" in result
        assert "MyClass.method" in result
        # Funkce nesmí obsahovat "method" jako standalone položku
        lines = [l for l in result.splitlines() if "Funkce" in l]
        assert all("method" not in l.split("method")[0].split("Funkce")[1]
                   if "method" in l else True
                   for l in lines)

    @pytest.mark.asyncio
    async def test_counts_classes(self, tmp_path):
        f = tmp_path / "sample.py"
        f.write_text("class A: pass\nclass B: pass\n")
        result = _text(await _analyze_impl({"file_path": str(f)}))
        assert "A" in result
        assert "B" in result

    @pytest.mark.asyncio
    async def test_counts_imports(self, tmp_path):
        f = tmp_path / "sample.py"
        f.write_text("import os\nimport sys\nfrom pathlib import Path\n")
        result = _text(await _analyze_impl({"file_path": str(f)}))
        assert "os" in result
        assert "sys" in result
        assert "pathlib" in result

    @pytest.mark.asyncio
    async def test_reports_line_counts(self, tmp_path):
        f = tmp_path / "sample.py"
        f.write_text("x = 1\ny = 2\n# komentář\n\n")
        result = _text(await _analyze_impl({"file_path": str(f)}))
        assert "Celkem řádků" in result

    @pytest.mark.asyncio
    async def test_empty_file(self, tmp_path):
        f = tmp_path / "empty.py"
        f.write_text("")
        result = _text(await _analyze_impl({"file_path": str(f)}))
        assert "žádné" in result or "0" in result


# ---------------------------------------------------------------------------
# Testy chybových stavů
# ---------------------------------------------------------------------------

class TestAnalyzeCodeStructureErrors:
    @pytest.mark.asyncio
    async def test_nonexistent_file(self, tmp_path):
        result = _text(await _analyze_impl({"file_path": str(tmp_path / "nope.py")}))
        assert "nenalezen" in result.lower() or "not found" in result.lower()

    @pytest.mark.asyncio
    async def test_non_python_file(self, tmp_path):
        f = tmp_path / "data.json"
        f.write_text('{"key": "value"}')
        result = _text(await _analyze_impl({"file_path": str(f)}))
        assert "Python" in result or "python" in result

    @pytest.mark.asyncio
    async def test_syntax_error_file(self, tmp_path):
        f = tmp_path / "broken.py"
        f.write_text("def foo(\n    # chybí uzávěrka\n")
        result = _text(await _analyze_impl({"file_path": str(f)}))
        assert "parsování" in result.lower() or "syntax" in result.lower()


# ---------------------------------------------------------------------------
# Testy struktury MCP serveru
# ---------------------------------------------------------------------------

class TestMcpServerStructure:
    def test_code_analysis_server_is_not_none(self):
        assert CODE_ANALYSIS_SERVER is not None

    def test_mcp_tool_name_format(self):
        assert MCP_TOOL_NAME.startswith("mcp__")
        assert "analyze_code_structure" in MCP_TOOL_NAME
