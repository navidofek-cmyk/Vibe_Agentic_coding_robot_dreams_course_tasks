#include "../highlighters.h"
#include "../highlight.h"
#include <iostream>
#include <string>
#include <vector>

// ── Minimal test harness ───────────────────────────────────────────────────

static int g_passed = 0;
static int g_total  = 0;

// Return the color at column `col` from the span list (-1 = out of range).
static int colorAt(const std::vector<ColorSpan>& spans, int col) {
    for (const auto& sp : spans) {
        if (col >= sp.col && col < sp.col + sp.len)
            return sp.color;
    }
    return HL::RESET;
}

static void check(const char* label, bool ok) {
    ++g_total;
    if (ok) {
        ++g_passed;
    } else {
        std::cerr << "  FAIL: " << label << "\n";
    }
}

// ── C++ highlighter ────────────────────────────────────────────────────────

static void testCpp() {
    auto hl = makeHighlighter("file.cpp");

    // "int main()" — 'int' starts at col 0, length 3 → HL::TYPE
    {
        std::string line = "int main()";
        auto spans = hl->highlight(line, 0);
        check("cpp: 'int' is HL::TYPE", colorAt(spans, 0) == HL::TYPE);
        check("cpp: 'int' col 1 still TYPE", colorAt(spans, 1) == HL::TYPE);
        check("cpp: 'int' col 2 still TYPE", colorAt(spans, 2) == HL::TYPE);
        // 'm' of 'main' is not a keyword or type
        check("cpp: 'main' is not highlighted", colorAt(spans, 4) == HL::RESET);
    }

    // "// comment" → whole line is HL::COMMENT
    {
        std::string line = "// comment";
        auto spans = hl->highlight(line, 0);
        check("cpp: '//' starts COMMENT", colorAt(spans, 0) == HL::COMMENT);
        check("cpp: comment body is COMMENT", colorAt(spans, 5) == HL::COMMENT);
    }

    // "#include <...>" → whole line is HL::PREPROC
    {
        std::string line = "#include <iostream>";
        auto spans = hl->highlight(line, 0);
        check("cpp: '#include' line is PREPROC", colorAt(spans, 0) == HL::PREPROC);
        check("cpp: preproc extends to end", colorAt(spans, 10) == HL::PREPROC);
    }

    // "42" → HL::NUMBER
    {
        std::string line = "return 42;";
        auto spans = hl->highlight(line, 0);
        // '4' is at col 7
        check("cpp: '42' is HL::NUMBER", colorAt(spans, 7) == HL::NUMBER);
        check("cpp: '2' of '42' is NUMBER", colorAt(spans, 8) == HL::NUMBER);
    }

    // '"hello"' → HL::STRING
    {
        std::string line = "\"hello\"";
        auto spans = hl->highlight(line, 0);
        check("cpp: opening '\"' is STRING", colorAt(spans, 0) == HL::STRING);
        check("cpp: 'h' inside string is STRING", colorAt(spans, 1) == HL::STRING);
        check("cpp: closing '\"' is STRING", colorAt(spans, 6) == HL::STRING);
    }
}

// ── Python highlighter ─────────────────────────────────────────────────────

static void testPython() {
    auto hl = makeHighlighter("script.py");

    // "def foo():" — 'def' at col 0 → HL::KEYWORD
    {
        std::string line = "def foo():";
        auto spans = hl->highlight(line, 0);
        check("py: 'def' is KEYWORD", colorAt(spans, 0) == HL::KEYWORD);
        check("py: 'd' col 0 is KEYWORD", colorAt(spans, 0) == HL::KEYWORD);
        check("py: 'f' col 1 is KEYWORD", colorAt(spans, 1) == HL::KEYWORD);
        check("py: 'f' of 'foo' is not highlighted", colorAt(spans, 4) == HL::RESET);
    }

    // "# comment" → HL::COMMENT
    {
        std::string line = "# comment";
        auto spans = hl->highlight(line, 0);
        check("py: '#' is COMMENT", colorAt(spans, 0) == HL::COMMENT);
        check("py: comment body is COMMENT", colorAt(spans, 3) == HL::COMMENT);
    }

    // "@decorator" → HL::PREPROC
    {
        std::string line = "@staticmethod";
        auto spans = hl->highlight(line, 0);
        check("py: '@' is PREPROC", colorAt(spans, 0) == HL::PREPROC);
        check("py: decorator body is PREPROC", colorAt(spans, 5) == HL::PREPROC);
    }
}

// ── JSON highlighter ───────────────────────────────────────────────────────

static void testJson() {
    auto hl = makeHighlighter("data.json");

    // '  "key": 42' — "key" is a JSON key → HL::TYPE; 42 → HL::NUMBER
    {
        std::string line = "  \"key\": 42";
        auto spans = hl->highlight(line, 0);
        // '"' at col 2 starts the key
        check("json: key '\"' is TYPE",   colorAt(spans, 2) == HL::TYPE);
        check("json: 'k' of key is TYPE", colorAt(spans, 3) == HL::TYPE);
        // '4' of 42 — after ": " which is 3 chars past closing '"' at col 6,
        // so col 6='"' col 7=':' col 8=' ' col 9='4' col 10='2'
        check("json: '4' of 42 is NUMBER", colorAt(spans, 9)  == HL::NUMBER);
        check("json: '2' of 42 is NUMBER", colorAt(spans, 10) == HL::NUMBER);
    }

    // '"value"' (no colon after) → HL::STRING
    {
        std::string line = "\"value\"";
        auto spans = hl->highlight(line, 0);
        check("json: non-key string is STRING", colorAt(spans, 0) == HL::STRING);
        check("json: string body is STRING",    colorAt(spans, 3) == HL::STRING);
    }
}

// ── Markdown highlighter ───────────────────────────────────────────────────

static void testMarkdown() {
    auto hl = makeHighlighter("README.md");

    // "# Heading" → whole line is HL::KEYWORD (h1 maps to hcol[0] = KEYWORD)
    {
        std::string line = "# Heading";
        auto spans = hl->highlight(line, 0);
        check("md: h1 '#' is KEYWORD",      colorAt(spans, 0) == HL::KEYWORD);
        check("md: h1 body is KEYWORD",      colorAt(spans, 3) == HL::KEYWORD);
    }

    // "**bold**" → HL::KEYWORD
    {
        std::string line = "**bold**";
        auto spans = hl->highlight(line, 0);
        check("md: bold '**' is KEYWORD",   colorAt(spans, 0) == HL::KEYWORD);
        check("md: bold body is KEYWORD",   colorAt(spans, 3) == HL::KEYWORD);
        check("md: closing '**' is KEYWORD",colorAt(spans, 6) == HL::KEYWORD);
    }
}

// ── supportsFile ───────────────────────────────────────────────────────────

static void testSupportsFile() {
    check("makeHighlighter(.cpp) not null", makeHighlighter("foo.cpp")  != nullptr);
    check("makeHighlighter(.py)  not null", makeHighlighter("foo.py")   != nullptr);
    check("makeHighlighter(.json) not null",makeHighlighter("foo.json") != nullptr);
    check("makeHighlighter(.md)  not null", makeHighlighter("foo.md")   != nullptr);
    check("makeHighlighter(.xyz) is null",  makeHighlighter("foo.xyz")  == nullptr);
}

// ── main ───────────────────────────────────────────────────────────────────

int main() {
    testCpp();
    testPython();
    testJson();
    testMarkdown();
    testSupportsFile();

    std::cout << "Passed: " << g_passed << "/" << g_total << "\n";
    return (g_passed == g_total) ? 0 : 1;
}
