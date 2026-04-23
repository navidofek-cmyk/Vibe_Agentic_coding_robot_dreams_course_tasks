#include "../highlighters.h"
#include "../markdown_preview.h"
#include "../buffer.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>

using clock_t2 = std::chrono::high_resolution_clock;

static double ms(clock_t2::time_point a, clock_t2::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

// ── Benchmark: renderHighlighted (C++ highlighter, 10 000 lines) ──────────────

static void bench_highlight() {
    auto hlPtr = makeHighlighter("bench.cpp");
    auto* hl = hlPtr.get();
    std::vector<std::string> lines = {
        "#include <vector>",
        "#include <string>",
        "// This is a comment",
        "int main(int argc, char* argv[]) {",
        "    std::vector<std::string> v;",
        "    for (int i = 0; i < 100; ++i) {",
        "        v.push_back(std::to_string(i));",
        "    }",
        "    return 0;",
        "}",
    };
    const int N = 10000;

    auto t0 = clock_t2::now();
    std::string total;
    for (int i = 0; i < N; ++i) {
        const std::string& line = lines[i % lines.size()];
        auto spans = hl->highlight(line, i);
        total += renderHighlighted(line, spans, 0, 120);
    }
    auto t1 = clock_t2::now();
    double elapsed = ms(t0, t1);

    std::cout << "  highlight 10k lines:  " << elapsed << " ms"
              << "  (" << (N / elapsed * 1000) << " lines/s)\n";
    if (elapsed > 500)
        std::cerr << "  SLOW: highlight took > 500ms\n";
}

// ── Benchmark: mdpreview::render (1 000 lines) ────────────────────────────────

static void bench_markdown() {
    std::vector<std::string> doc;
    for (int i = 0; i < 100; ++i) {
        doc.push_back("# Heading " + std::to_string(i));
        doc.push_back("");
        doc.push_back("Some **bold** and *italic* text with `code`.");
        doc.push_back("- item one");
        doc.push_back("- item two");
        doc.push_back("> A blockquote line");
        doc.push_back("```");
        doc.push_back("int x = 42;");
        doc.push_back("```");
        doc.push_back("");
    }

    const int N = 100;
    auto t0 = clock_t2::now();
    size_t total = 0;
    for (int i = 0; i < N; ++i) {
        auto result = mdpreview::render(doc, 120);
        total += result.size();
    }
    auto t1 = clock_t2::now();
    double elapsed = ms(t0, t1);

    std::cout << "  mdpreview 1k lines x" << N << ": " << elapsed << " ms"
              << "  (" << (int)(N * doc.size() / elapsed * 1000) << " lines/s)\n";
    if (elapsed > 1000)
        std::cerr << "  SLOW: mdpreview took > 1000ms\n";
    (void)total;
}

// ── Benchmark: Buffer insert (50 000 chars) ───────────────────────────────────

static void bench_buffer_insert() {
    const int N = 50000;
    auto t0 = clock_t2::now();
    Buffer b;
    for (int i = 0; i < N; ++i) {
        if (i % 80 == 0) b.insertNewline();
        else b.insertChar('A' + (i % 26));
    }
    auto t1 = clock_t2::now();
    double elapsed = ms(t0, t1);

    std::cout << "  buffer insert 50k:    " << elapsed << " ms"
              << "  (lines: " << b.lines().size() << ")\n";
    if (elapsed > 500)
        std::cerr << "  SLOW: buffer insert took > 500ms\n";
}

// ── Benchmark: Buffer find (worst case wrap) ──────────────────────────────────

static void bench_buffer_find() {
    Buffer b;
    for (int i = 0; i < 1000; ++i) {
        if (i > 0) b.insertNewline();
        for (char c : std::string("the quick brown fox jumps over the lazy dog"))
            b.insertChar(c);
    }
    b.gotoLine(0, 0);

    const int N = 1000;
    auto t0 = clock_t2::now();
    for (int i = 0; i < N; ++i)
        b.find("lazy");
    auto t1 = clock_t2::now();
    double elapsed = ms(t0, t1);

    std::cout << "  buffer find 1k:       " << elapsed << " ms"
              << "  (" << (int)(N / elapsed * 1000) << " finds/s)\n";
    if (elapsed > 200)
        std::cerr << "  SLOW: buffer find took > 200ms\n";
}

int main() {
    std::cout << "=== Benchmarks ===\n";
    bench_highlight();
    bench_markdown();
    bench_buffer_insert();
    bench_buffer_find();
    std::cout << "=== Done ===\n";
    return 0;
}
