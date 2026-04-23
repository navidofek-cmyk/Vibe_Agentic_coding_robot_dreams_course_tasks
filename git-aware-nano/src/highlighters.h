#pragma once
#include "highlight.h"
#include <memory>
#include <string>

// Returns the right highlighter for the given filename, or nullptr.
std::unique_ptr<SyntaxHighlighter> makeHighlighter(const std::string& filename);

// Renders one line with ANSI color codes applied from spans.
// Handles scrollCol offset and clips to `cols` visible characters.
std::string renderHighlighted(
    const std::string& line,
    const std::vector<ColorSpan>& spans,
    int scrollCol,
    int cols
);
