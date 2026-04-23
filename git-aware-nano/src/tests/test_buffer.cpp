#include "../buffer.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cassert>

static int g_passed = 0, g_total = 0;

static void check(const char* label, bool ok) {
    ++g_total;
    if (ok) { ++g_passed; std::cout << "  PASS: " << label << "\n"; }
    else    { std::cerr  << "  FAIL: " << label << "\n"; }
}

// ── Construction ──────────────────────────────────────────────────────────────

static void test_construction() {
    Buffer b;
    check("new buffer has 1 line",     b.lines().size() == 1);
    check("new buffer line is empty",  b.lines()[0].empty());
    check("cursor at 0,0",             b.cursorRow() == 0 && b.cursorCol() == 0);
    check("not dirty",                 !b.isDirty());
    check("no filename",               b.filename().empty());
}

// ── insertChar / deleteCharBefore / deleteCharAfter ───────────────────────────

static void test_insert_delete() {
    Buffer b;
    b.insertChar('H');
    b.insertChar('i');
    check("insert: line is 'Hi'",      b.lines()[0] == "Hi");
    check("insert: cursor at col 2",   b.cursorCol() == 2);
    check("insert marks dirty",        b.isDirty());

    b.deleteCharBefore();
    check("deleteCharBefore: 'H'",     b.lines()[0] == "H");
    check("deleteCharBefore: col 1",   b.cursorCol() == 1);

    b.insertChar('e');
    b.insertChar('y');
    check("after re-insert: 'Hey'",    b.lines()[0] == "Hey");

    // deleteCharAfter from beginning
    b.moveCursorHome();
    b.deleteCharAfter();
    check("deleteCharAfter: 'ey'",     b.lines()[0] == "ey");
    check("deleteCharAfter: col 0",    b.cursorCol() == 0);
}

// ── insertNewline ─────────────────────────────────────────────────────────────

static void test_newline() {
    Buffer b;
    b.insertChar('A');
    b.insertChar('B');
    b.insertChar('C');
    b.moveCursorHome();
    b.moveCursor(0, 1); // col 1, between A and B
    b.insertNewline();
    check("newline: 2 lines",          b.lines().size() == 2);
    check("newline: first line 'A'",   b.lines()[0] == "A");
    check("newline: second line 'BC'", b.lines()[1] == "BC");
    check("newline: cursor row 1",     b.cursorRow() == 1);
    check("newline: cursor col 0",     b.cursorCol() == 0);
}

// ── moveCursor / Home / End / PageUp / PageDown ───────────────────────────────

static void test_cursor_movement() {
    Buffer b;
    // Build 5 lines: "line0".."line4"
    b.insertChar('0');
    for (int i = 1; i < 5; ++i) {
        b.insertNewline();
        b.insertChar(('0' + i));
    }
    check("5 lines created",           b.lines().size() == 5);

    // moveCursor up/down
    b.moveCursor(-1, 0);
    check("move up: row 3",            b.cursorRow() == 3);
    b.moveCursor(-10, 0); // clamp to 0
    check("move up clamp: row 0",      b.cursorRow() == 0);
    b.moveCursor(10, 0);  // clamp to last
    check("move down clamp: row 4",    b.cursorRow() == 4);

    // Home / End
    b.moveCursorHome();
    check("Home: col 0",               b.cursorCol() == 0);
    b.moveCursorEnd();
    check("End: col 1",                b.cursorCol() == 1); // "4" is 1 char

    // PageUp / PageDown
    b.gotoLine(4, 0);
    b.moveCursorPageUp(3);
    check("PageUp by 3: row 1",        b.cursorRow() == 1);
    b.moveCursorPageDown(3);
    check("PageDown by 3: row 4",      b.cursorRow() == 4);
}

// ── find ──────────────────────────────────────────────────────────────────────

static void test_find() {
    Buffer b;
    // Build: "hello world\nfoo bar\nhello again"
    const char* words[] = {"hello world", "foo bar", "hello again"};
    for (int i = 0; i < 3; ++i) {
        if (i > 0) b.insertNewline();
        for (char c : std::string(words[i])) b.insertChar(c);
    }
    // find(q, false) searches from beginning
    bool found = b.find("hello", false);
    check("find 'hello' from start: found",  found);
    check("find 'hello' from start: row 0",  b.cursorRow() == 0);

    found = b.find("hello");  // find next — skips current pos, wraps
    check("find next 'hello': row 2",        b.cursorRow() == 2);

    found = b.find("zzznope");
    check("find missing: not found",   !found);

    found = b.find("bar");
    check("find 'bar': found",         found);
    check("find 'bar': row 1",         b.cursorRow() == 1);
}

// ── gotoLine ─────────────────────────────────────────────────────────────────

static void test_goto_line() {
    Buffer b;
    for (int i = 0; i < 10; ++i) {
        if (i > 0) b.insertNewline();
        b.insertChar(('0' + i));
    }
    b.gotoLine(5, 0);
    check("gotoLine(5): row 5",        b.cursorRow() == 5);

    b.gotoLine(999, 0);  // clamp
    check("gotoLine(999) clamps: row 9", b.cursorRow() == 9);

    b.gotoLine(-1, 0);   // clamp
    check("gotoLine(-1) clamps: row 0",  b.cursorRow() == 0);
}

// ── updateScroll ──────────────────────────────────────────────────────────────

static void test_scroll() {
    Buffer b;
    for (int i = 0; i < 50; ++i) {
        if (i > 0) b.insertNewline();
        b.insertChar(('A' + i % 26));
    }
    b.gotoLine(49, 0);
    b.updateScroll(10, 80);
    check("scroll: row 49 visible",    b.scrollRow() <= 49 && b.scrollRow() + 10 > 49);

    b.gotoLine(0, 0);
    b.updateScroll(10, 80);
    check("scroll: row 0 → scrollRow 0", b.scrollRow() == 0);
}

// ── load / save ───────────────────────────────────────────────────────────────

static void test_load_save() {
    // Write temp file
    const char* tmpPath = "/tmp/test_buffer_tmp.txt";
    {
        std::ofstream f(tmpPath);
        f << "line one\nline two\nline three\n";
    }
    Buffer b;
    bool ok = b.load(tmpPath);
    check("load: returns true",        ok);
    check("load: 3 lines",             b.lines().size() == 3);
    check("load: first line",          b.lines()[0] == "line one");
    check("load: third line",          b.lines()[2] == "line three");
    check("load: not dirty",           !b.isDirty());

    // Modify and save
    b.insertChar('!');
    check("after insert: dirty",       b.isDirty());
    ok = b.save();
    check("save: returns true",        ok);
    check("save: clears dirty",        !b.isDirty());

    // Reload and verify
    Buffer b2;
    b2.load(tmpPath);
    check("reload: first line modified", b2.lines()[0] == "!line one");

    // saveAs
    const char* tmpPath2 = "/tmp/test_buffer_tmp2.txt";
    ok = b2.saveAs(tmpPath2);
    check("saveAs: returns true",       ok);
    Buffer b3;
    b3.load(tmpPath2);
    check("saveAs: content matches",    b3.lines()[0] == "!line one");

    std::remove(tmpPath);
    std::remove(tmpPath2);
}

// ── deleteCharBefore across newline ──────────────────────────────────────────

static void test_delete_join_lines() {
    Buffer b;
    b.insertChar('A');
    b.insertNewline();
    b.insertChar('B');
    check("setup: 2 lines",            b.lines().size() == 2);

    // cursor is at row 1, col 1 (after 'B')
    b.moveCursorHome();
    b.deleteCharBefore(); // should join lines
    check("join: 1 line",              b.lines().size() == 1);
    check("join: content 'AB'",        b.lines()[0] == "AB");
    check("join: cursor row 0",        b.cursorRow() == 0);
    check("join: cursor col 1",        b.cursorCol() == 1);
}

// ── previewMode toggle ────────────────────────────────────────────────────────

static void test_preview_mode() {
    Buffer b;
    check("preview: default off",      !b.previewMode());
    b.togglePreview();
    check("preview: on after toggle",  b.previewMode());
    b.togglePreview();
    check("preview: off again",        !b.previewMode());
}

// ── forceScrollRow ────────────────────────────────────────────────────────────

static void test_force_scroll() {
    Buffer b;
    for (int i = 0; i < 20; ++i) {
        if (i > 0) b.insertNewline();
        b.insertChar('X');
    }
    b.forceScrollRow(10);
    check("forceScrollRow(10): 10",    b.scrollRow() == 10);
    b.forceScrollRow(999);
    check("forceScrollRow clamp high", b.scrollRow() == 19);
    b.forceScrollRow(-1);
    check("forceScrollRow clamp low",  b.scrollRow() == 0);
}

int main() {
    test_construction();
    test_insert_delete();
    test_newline();
    test_cursor_movement();
    test_find();
    test_goto_line();
    test_scroll();
    test_load_save();
    test_delete_join_lines();
    test_preview_mode();
    test_force_scroll();

    std::cout << "\nPassed: " << g_passed << "/" << g_total << "\n";
    return (g_passed == g_total) ? 0 : 1;
}
