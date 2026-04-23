#pragma once
#include <string>
#include <vector>

struct ColorSpan {
    int col;
    int len;
    int color;
};

namespace HL {
    constexpr int RESET    =  0;
    constexpr int KEYWORD  =  1;  // bright blue
    constexpr int STRING   =  2;  // green
    constexpr int COMMENT  =  3;  // dark gray
    constexpr int NUMBER   =  4;  // yellow
    constexpr int PREPROC  =  5;  // magenta  (C++ #include, Python @decorator)
    constexpr int TYPE     =  6;  // cyan
    constexpr int TAG      =  7;  // bright blue  (HTML tag name)
    constexpr int ATTR     =  8;  // bright cyan  (HTML attribute)
    constexpr int SELECTOR =  9;  // magenta      (CSS selector)
    constexpr int PROPERTY = 10;  // cyan         (CSS property)

    inline const char* ansi(int c) {
        switch (c) {
            case KEYWORD:  return "\033[94m";
            case STRING:   return "\033[32m";
            case COMMENT:  return "\033[90m";
            case NUMBER:   return "\033[33m";
            case PREPROC:  return "\033[35m";
            case TYPE:     return "\033[36m";
            case TAG:      return "\033[94m";
            case ATTR:     return "\033[96m";
            case SELECTOR: return "\033[35m";
            case PROPERTY: return "\033[36m";
            default:       return "\033[0m";
        }
    }
}

class SyntaxHighlighter {
public:
    virtual ~SyntaxHighlighter() = default;
    virtual std::vector<ColorSpan> highlight(const std::string& line, int row) = 0;
    virtual bool supportsFile(const std::string& filename) const = 0;
};
