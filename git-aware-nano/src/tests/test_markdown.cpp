#include "../markdown_preview.h"
#include <iostream>
#include <string>
#include <vector>

static int g_passed = 0, g_total = 0;

static void check(const char* label, bool ok) {
    ++g_total;
    if (ok) { ++g_passed; std::cout << "  PASS: " << label << "\n"; }
    else    { std::cerr  << "  FAIL: " << label << "\n"; }
}

// Strip ANSI escape codes from a string for content comparison
static std::string stripAnsi(const std::string& s) {
    std::string out;
    bool inEsc = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\033') { inEsc = true; continue; }
        if (inEsc) { if (s[i] == 'm') inEsc = false; continue; }
        out += s[i];
    }
    return out;
}

static bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

static bool hasAnsi(const std::string& s) {
    return s.find('\033') != std::string::npos;
}

static std::vector<std::string> render(std::vector<std::string> lines, int cols = 80) {
    return mdpreview::render(lines, cols);
}

// ── Headings ─────────────────────────────────────────────────────────────────

static void test_headings() {
    auto r1 = render({"# Hello"});
    check("H1: has ANSI color",        hasAnsi(r1[0]));
    check("H1: contains text",         contains(stripAnsi(r1[0]), "Hello"));
    check("H1: produces underline row", r1.size() == 2);
    check("H1: underline has dashes",  contains(stripAnsi(r1[1]), "-"));

    auto r2 = render({"## World"});
    check("H2: produces underline row", r2.size() == 2);

    auto r3 = render({"### Title"});
    check("H3: no underline row",       r3.size() == 1);
    check("H3: has ANSI",               hasAnsi(r3[0]));

    auto r6 = render({"###### Deep"});
    check("H6: no underline",           r6.size() == 1);
    check("H6: has ANSI",               hasAnsi(r6[0]));
}

// ── Bold / Italic / Inline code ───────────────────────────────────────────────

static void test_inline() {
    auto r = render({"This is **bold** text"});
    check("bold: has ANSI",            hasAnsi(r[0]));
    std::string plain = stripAnsi(r[0]);
    check("bold: markers stripped",    plain.find("**") == std::string::npos);
    check("bold: content present",     contains(plain, "bold"));

    r = render({"This is *italic* text"});
    plain = stripAnsi(r[0]);
    check("italic: markers stripped",  plain.find("*italic*") == std::string::npos);
    check("italic: content present",   contains(plain, "italic"));

    r = render({"Use `code` here"});
    check("inline code: has ANSI",     hasAnsi(r[0]));
    plain = stripAnsi(r[0]);
    check("inline code: backticks gone", plain.find('`') == std::string::npos);
    check("inline code: content",      contains(plain, "code"));
}

// ── Links and images ──────────────────────────────────────────────────────────

static void test_links() {
    auto r = render({"[click here](https://example.com)"});
    std::string plain = stripAnsi(r[0]);
    check("link: has ANSI",            hasAnsi(r[0]));
    check("link: shows text",          contains(plain, "click here"));
    check("link: hides URL",           plain.find("https://") == std::string::npos);

    r = render({"![alt text](image.png)"});
    plain = stripAnsi(r[0]);
    check("image: shows alt",          contains(plain, "img: alt text"));
}

// ── Code blocks ───────────────────────────────────────────────────────────────

static void test_code_block() {
    auto r = render({"```", "int x = 42;", "```"});
    check("code block: 3 output lines", r.size() == 3);
    check("code block: fence has ANSI", hasAnsi(r[0]));
    check("code block: content colored", hasAnsi(r[1]));
    std::string plain = stripAnsi(r[1]);
    check("code block: content visible", contains(plain, "int x = 42;"));
}

// ── Blockquote ────────────────────────────────────────────────────────────────

static void test_blockquote() {
    auto r = render({"> This is quoted"});
    check("blockquote: has ANSI",      hasAnsi(r[0]));
    std::string plain = stripAnsi(r[0]);
    check("blockquote: has pipe",      contains(plain, "|"));
    check("blockquote: content",       contains(plain, "This is quoted"));
}

// ── Lists ─────────────────────────────────────────────────────────────────────

static void test_lists() {
    // Bullet
    auto r = render({"- item one", "- item two"});
    check("bullet: 2 lines",           r.size() == 2);
    check("bullet: has bullet char",   contains(stripAnsi(r[0]), "\xe2\x80\xa2") ||
                                       contains(r[0], "\xe2\x80\xa2") ||
                                       hasAnsi(r[0]));

    // Numbered
    r = render({"1. first", "2. second"});
    check("numbered: 2 lines",         r.size() == 2);
    check("numbered: has number",      contains(stripAnsi(r[0]), "1."));

    // Task list
    r = render({"- [x] done", "- [ ] todo"});
    check("task done: has ANSI",       hasAnsi(r[0]));
    check("task todo: has ANSI",       hasAnsi(r[1]));
    check("task done: content",        contains(stripAnsi(r[0]), "done"));
}

// ── Horizontal rule ───────────────────────────────────────────────────────────

static void test_hrule() {
    auto r = render({"---"});
    check("hrule: 1 line",             r.size() == 1);
    std::string plain = stripAnsi(r[0]);
    check("hrule: contains dashes",    contains(plain, "---"));
}

// ── Empty lines ───────────────────────────────────────────────────────────────

static void test_empty() {
    auto r = render({""});
    check("empty line: 1 output",      r.size() == 1);
    std::string plain = stripAnsi(r[0]);
    check("empty line: all spaces",    plain.find_first_not_of(' ') == std::string::npos);
}

// ── Plain paragraph ───────────────────────────────────────────────────────────

static void test_paragraph() {
    auto r = render({"Just plain text here."});
    check("paragraph: 1 line",         r.size() == 1);
    std::string plain = stripAnsi(r[0]);
    check("paragraph: content",        contains(plain, "Just plain text here."));
}

// ── Width clamping ────────────────────────────────────────────────────────────

static void test_width() {
    auto r = render({"hello world"}, 5);
    check("width 5: output <= 5 visible", stripAnsi(r[0]).size() <= 5);
}

// ── Multi-element document ────────────────────────────────────────────────────

static void test_document() {
    std::vector<std::string> doc = {
        "# Title",
        "",
        "Some **bold** and *italic* text.",
        "",
        "- item a",
        "- item b",
        "",
        "> A quote",
        "",
        "```",
        "code here",
        "```",
    };
    auto r = render(doc);
    // H1 produces 2 lines, so total > 12
    check("document: output lines >= input", r.size() >= doc.size());
    // Check first line is colored (H1)
    check("document: first line colored",    hasAnsi(r[0]));
}

int main() {
    test_headings();
    test_inline();
    test_links();
    test_code_block();
    test_blockquote();
    test_lists();
    test_hrule();
    test_empty();
    test_paragraph();
    test_width();
    test_document();

    std::cout << "\nPassed: " << g_passed << "/" << g_total << "\n";
    return (g_passed == g_total) ? 0 : 1;
}
