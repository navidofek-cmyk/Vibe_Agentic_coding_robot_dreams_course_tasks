#include "../buffer.h"
#include "../highlighters.h"
#include "../markdown_preview.h"
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <cassert>

static int g_passed = 0, g_total = 0;

static void check(const char* label, bool ok) {
    ++g_total;
    if (ok) { ++g_passed; std::cout << "  PASS: " << label << "\n"; }
    else    { std::cerr  << "  FAIL: " << label << "\n"; }
}

static std::mt19937 rng(42);  // fixed seed for reproducibility

static std::string randStr(int minLen = 0, int maxLen = 80) {
    int len = std::uniform_int_distribution<int>(minLen, maxLen)(rng);
    std::string s;
    s.reserve(len);
    // printable ASCII + some special chars
    static const char chars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789 \t!@#$%^&*()_+-=[]{}|;':\",./<>?`~";
    for (int i = 0; i < len; ++i)
        s += chars[rng() % (sizeof(chars) - 1)];
    return s;
}

// ── Property: Buffer always has at least 1 line ───────────────────────────────

static void prop_buffer_never_empty() {
    const int RUNS = 200;
    bool ok = true;
    for (int run = 0; run < RUNS; ++run) {
        Buffer b;
        int ops = std::uniform_int_distribution<int>(1, 50)(rng);
        for (int i = 0; i < ops; ++i) {
            int op = rng() % 5;
            switch (op) {
                case 0: b.insertChar('A' + rng() % 26); break;
                case 1: b.insertNewline(); break;
                case 2: b.deleteCharBefore(); break;
                case 3: b.deleteCharAfter(); break;
                case 4: b.moveCursor((int)(rng()%3)-1, (int)(rng()%3)-1); break;
            }
            if (b.lines().empty()) { ok = false; break; }
        }
        if (!ok) break;
    }
    check("Buffer: always >= 1 line after random ops", ok);
}

// ── Property: cursor always within bounds ────────────────────────────────────

static void prop_cursor_in_bounds() {
    const int RUNS = 200;
    bool ok = true;
    for (int run = 0; run < RUNS; ++run) {
        Buffer b;
        int ops = std::uniform_int_distribution<int>(1, 100)(rng);
        for (int i = 0; i < ops; ++i) {
            int op = rng() % 6;
            switch (op) {
                case 0: b.insertChar('x'); break;
                case 1: b.insertNewline(); break;
                case 2: b.deleteCharBefore(); break;
                case 3: b.deleteCharAfter(); break;
                case 4: b.moveCursor((int)(rng()%5)-2, (int)(rng()%5)-2); break;
                case 5: b.moveCursorPageDown(3); break;
            }
            int row = b.cursorRow();
            int col = b.cursorCol();
            int maxRow = (int)b.lines().size() - 1;
            int maxCol = (int)b.lines()[row].size();
            if (row < 0 || row > maxRow || col < 0 || col > maxCol) {
                ok = false; break;
            }
        }
        if (!ok) break;
    }
    check("Buffer: cursor always in valid bounds", ok);
}

// ── Property: insert+deleteCharBefore is identity ────────────────────────────

static void prop_insert_delete_identity() {
    const int RUNS = 100;
    bool ok = true;
    for (int run = 0; run < RUNS; ++run) {
        Buffer b;
        // insert some initial content
        std::string initial = randStr(5, 20);
        for (char c : initial) b.insertChar(c);
        std::string before = b.lines()[0];
        int col_before = b.cursorCol();

        // insert a char then delete it
        char ch = 'A' + rng() % 26;
        b.insertChar(ch);
        b.deleteCharBefore();

        if (b.lines()[0] != before || b.cursorCol() != col_before) {
            ok = false; break;
        }
    }
    check("Buffer: insert+deleteCharBefore is identity", ok);
}

// ── Property: gotoLine always puts cursor on valid line ───────────────────────

static void prop_goto_line_valid() {
    Buffer b;
    for (int i = 0; i < 20; ++i) {
        if (i > 0) b.insertNewline();
        b.insertChar('A' + i);
    }

    bool ok = true;
    for (int i = 0; i < 500; ++i) {
        int target = std::uniform_int_distribution<int>(-5, 25)(rng);
        b.gotoLine(target);
        int row = b.cursorRow();
        if (row < 0 || row >= (int)b.lines().size()) { ok = false; break; }
    }
    check("Buffer: gotoLine always lands on valid row", ok);
}

// ── Property: highlight spans never exceed line length ───────────────────────

static void prop_spans_in_bounds() {
    auto cpp = makeHighlighter("test.cpp");
    auto py  = makeHighlighter("test.py");
    const int RUNS = 500;
    bool ok = true;

    for (int run = 0; run < RUNS && ok; ++run) {
        std::string line = randStr(0, 100);
        for (auto* hl : {cpp.get(), py.get()}) {
            if (!hl) continue;
            auto spans = hl->highlight(line, run);
            for (const auto& sp : spans) {
                if (sp.col < 0 || sp.len < 0 ||
                    sp.col + sp.len > (int)line.size()) {
                    ok = false; break;
                }
            }
        }
    }
    check("Highlighter: spans always within line bounds", ok);
}

// ── Property: mdpreview output count >= input count ──────────────────────────

static void prop_mdpreview_output_count() {
    const int RUNS = 100;
    bool ok = true;
    for (int run = 0; run < RUNS; ++run) {
        int n = std::uniform_int_distribution<int>(1, 30)(rng);
        std::vector<std::string> lines;
        for (int i = 0; i < n; ++i)
            lines.push_back(randStr(0, 60));
        auto result = mdpreview::render(lines, 80);
        // output can be >= input (headings add underline)
        if ((int)result.size() < n) { ok = false; break; }
    }
    check("mdpreview: output lines >= input lines", ok);
}

// ── Property: mdpreview output lines padded to `cols` ────────────────────────

static void prop_mdpreview_width() {
    const int RUNS = 100;
    bool ok = true;
    int cols = 40;
    for (int run = 0; run < RUNS; ++run) {
        std::vector<std::string> lines;
        for (int i = 0; i < 10; ++i)
            lines.push_back(randStr(0, 50));
        auto result = mdpreview::render(lines, cols);
        for (const auto& r : result) {
            // count visible chars (strip ANSI)
            int vis = 0;
            bool inEsc = false;
            for (char c : r) {
                if (c == '\033') { inEsc = true; continue; }
                if (inEsc) { if (c == 'm') inEsc = false; continue; }
                vis++;
            }
            if (vis > cols) { ok = false; break; }
        }
        if (!ok) break;
    }
    check("mdpreview: visible width never exceeds cols", ok);
}

// ── Property: dirty flag set after every mutation ────────────────────────────

static void prop_dirty_after_mutation() {
    const int RUNS = 100;
    bool ok = true;
    for (int run = 0; run < RUNS; ++run) {
        Buffer b;
        b.clearDirty();
        int op = rng() % 3;
        switch (op) {
            case 0: b.insertChar('x'); break;
            case 1: b.insertNewline(); break;
            case 2: b.insertChar('y'); b.deleteCharBefore(); break;
        }
        if (!b.isDirty()) { ok = false; break; }
    }
    check("Buffer: isDirty() true after any mutation", ok);
}

int main() {
    prop_buffer_never_empty();
    prop_cursor_in_bounds();
    prop_insert_delete_identity();
    prop_goto_line_valid();
    prop_spans_in_bounds();
    prop_mdpreview_output_count();
    prop_mdpreview_width();
    prop_dirty_after_mutation();

    std::cout << "\nPassed: " << g_passed << "/" << g_total << "\n";
    return (g_passed == g_total) ? 0 : 1;
}
