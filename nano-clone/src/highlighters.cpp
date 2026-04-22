#include "highlighters.h"
#include <unordered_set>
#include <cctype>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// ── Generic scanner ────────────────────────────────────────────────────────

struct ScanConfig {
    std::unordered_set<std::string> keywords;
    std::unordered_set<std::string> types;
    std::string lineComment;          // e.g. "//" or "#"
    std::string blockOpen;            // e.g. "/*"
    std::string blockClose;           // e.g. "*/"
    bool hasPreproc = false;          // C-style # lines
    bool hasDecorator = false;        // Python @
    char extraStringQuote = 0;        // 0 = none (backtick for JS)
};

static bool startsWith(const std::string& s, size_t pos, const std::string& prefix) {
    if (prefix.empty()) return false;
    return s.compare(pos, prefix.size(), prefix) == 0;
}

static std::vector<ColorSpan> scanGeneric(const std::string& line, const ScanConfig& cfg) {
    std::vector<ColorSpan> spans;
    size_t n = line.size();
    size_t i = 0;

    // Preprocessor line (C/C++: starts with optional space then #)
    if (cfg.hasPreproc) {
        size_t j = 0;
        while (j < n && line[j] == ' ') ++j;
        if (j < n && line[j] == '#') {
            spans.push_back({0, (int)n, HL::PREPROC});
            return spans;
        }
    }

    while (i < n) {
        // Block comment
        if (!cfg.blockOpen.empty() && startsWith(line, i, cfg.blockOpen)) {
            size_t start = i;
            i += cfg.blockOpen.size();
            while (i < n && !startsWith(line, i, cfg.blockClose)) ++i;
            if (i < n) i += cfg.blockClose.size();
            spans.push_back({(int)start, (int)(i - start), HL::COMMENT});
            continue;
        }

        // Line comment
        if (!cfg.lineComment.empty() && startsWith(line, i, cfg.lineComment)) {
            spans.push_back({(int)i, (int)(n - i), HL::COMMENT});
            break;
        }

        // Strings: double quote
        if (line[i] == '"') {
            size_t start = i++;
            while (i < n && !(line[i] == '"' && line[i-1] != '\\')) ++i;
            if (i < n) ++i;
            spans.push_back({(int)start, (int)(i - start), HL::STRING});
            continue;
        }

        // Strings: single quote
        if (line[i] == '\'') {
            size_t start = i++;
            while (i < n && !(line[i] == '\'' && line[i-1] != '\\')) ++i;
            if (i < n) ++i;
            spans.push_back({(int)start, (int)(i - start), HL::STRING});
            continue;
        }

        // Extra string quote (e.g. backtick for JS)
        if (cfg.extraStringQuote && line[i] == cfg.extraStringQuote) {
            size_t start = i++;
            while (i < n && line[i] != cfg.extraStringQuote) ++i;
            if (i < n) ++i;
            spans.push_back({(int)start, (int)(i - start), HL::STRING});
            continue;
        }

        // Decorator (Python @name)
        if (cfg.hasDecorator && line[i] == '@') {
            size_t start = i++;
            while (i < n && (std::isalnum(line[i]) || line[i] == '_' || line[i] == '.')) ++i;
            spans.push_back({(int)start, (int)(i - start), HL::PREPROC});
            continue;
        }

        // Number
        if (std::isdigit(line[i]) || (line[i] == '.' && i + 1 < n && std::isdigit(line[i+1]))) {
            size_t start = i;
            while (i < n && (std::isalnum(line[i]) || line[i] == '.' || line[i] == '_')) ++i;
            spans.push_back({(int)start, (int)(i - start), HL::NUMBER});
            continue;
        }

        // Identifier / keyword
        if (std::isalpha(line[i]) || line[i] == '_') {
            size_t start = i;
            while (i < n && (std::isalnum(line[i]) || line[i] == '_')) ++i;
            std::string word = line.substr(start, i - start);
            if (cfg.keywords.count(word))
                spans.push_back({(int)start, (int)(i - start), HL::KEYWORD});
            else if (cfg.types.count(word))
                spans.push_back({(int)start, (int)(i - start), HL::TYPE});
            continue;
        }

        ++i;
    }

    return spans;
}

// ── C / C++ ────────────────────────────────────────────────────────────────

class CppHighlighter : public SyntaxHighlighter {
    ScanConfig cfg_;
public:
    CppHighlighter() {
        cfg_.keywords = {
            "if","else","for","while","do","switch","case","default","break","continue",
            "return","goto","try","catch","throw","new","delete","sizeof","typeof",
            "namespace","using","class","struct","union","enum","template","typename",
            "public","private","protected","virtual","override","final","explicit",
            "inline","static","extern","const","constexpr","consteval","constinit",
            "volatile","mutable","register","auto","decltype","nullptr","true","false",
            "operator","friend","this","noexcept","static_assert","co_await","co_yield",
            "co_return","export","import","module",
        };
        cfg_.types = {
            "void","int","long","short","char","unsigned","signed","float","double",
            "bool","wchar_t","size_t","ptrdiff_t","nullptr_t","string","vector","map",
            "set","unordered_map","unordered_set","list","deque","array","pair","tuple",
            "optional","variant","any","unique_ptr","shared_ptr","weak_ptr","function",
            "thread","mutex","atomic","uint8_t","uint16_t","uint32_t","uint64_t",
            "int8_t","int16_t","int32_t","int64_t",
        };
        cfg_.lineComment  = "//";
        cfg_.blockOpen    = "/*";
        cfg_.blockClose   = "*/";
        cfg_.hasPreproc   = true;
    }

    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        return scanGeneric(line, cfg_);
    }

    bool supportsFile(const std::string& f) const override {
        auto ext = fs::path(f).extension().string();
        return ext==".cpp"||ext==".cc"||ext==".cxx"||ext==".c"||
               ext==".hpp"||ext==".hxx"||ext==".h";
    }
};

// ── Python ─────────────────────────────────────────────────────────────────

class PythonHighlighter : public SyntaxHighlighter {
    ScanConfig cfg_;
public:
    PythonHighlighter() {
        cfg_.keywords = {
            "False","None","True","and","as","assert","async","await","break","class",
            "continue","def","del","elif","else","except","finally","for","from",
            "global","if","import","in","is","lambda","nonlocal","not","or","pass",
            "raise","return","try","while","with","yield","print","len","range",
            "type","isinstance","super","property","staticmethod","classmethod",
        };
        cfg_.types = {
            "int","float","str","bool","bytes","list","dict","set","tuple","object",
            "Exception","ValueError","TypeError","KeyError","IndexError","IOError",
            "OSError","RuntimeError","StopIteration","GeneratorExit",
        };
        cfg_.lineComment  = "#";
        cfg_.blockOpen    = "\"\"\"";
        cfg_.blockClose   = "\"\"\"";
        cfg_.hasDecorator = true;
    }

    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        return scanGeneric(line, cfg_);
    }

    bool supportsFile(const std::string& f) const override {
        return fs::path(f).extension().string() == ".py";
    }
};

// ── JavaScript / TypeScript ────────────────────────────────────────────────

class JsTsHighlighter : public SyntaxHighlighter {
    ScanConfig cfg_;
    bool ts_;
public:
    explicit JsTsHighlighter(bool ts) : ts_(ts) {
        cfg_.keywords = {
            "break","case","catch","class","const","continue","debugger","default",
            "delete","do","else","export","extends","finally","for","function","if",
            "import","in","instanceof","let","new","of","return","static","super",
            "switch","this","throw","try","typeof","var","void","while","with","yield",
            "async","await","from","as","default","null","undefined","true","false",
            "get","set","target","constructor",
        };
        cfg_.types = {
            "string","number","boolean","object","symbol","bigint","never","unknown",
            "any","void","Array","Map","Set","Promise","Error","Date","RegExp",
            "console","Math","JSON","Object","Function","Number","String","Boolean",
        };
        if (ts) {
            cfg_.keywords.insert({"interface","type","enum","implements","abstract",
                                  "readonly","declare","namespace","module","override",
                                  "public","private","protected","keyof","typeof",
                                  "infer","extends","satisfies","using"});
        }
        cfg_.lineComment    = "//";
        cfg_.blockOpen      = "/*";
        cfg_.blockClose     = "*/";
        cfg_.extraStringQuote = '`';
    }

    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        return scanGeneric(line, cfg_);
    }

    bool supportsFile(const std::string& f) const override {
        auto ext = fs::path(f).extension().string();
        if (ts_) return ext==".ts"||ext==".tsx";
        return ext==".js"||ext==".jsx"||ext==".mjs"||ext==".cjs";
    }
};

// ── HTML ───────────────────────────────────────────────────────────────────

class HtmlHighlighter : public SyntaxHighlighter {
public:
    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        std::vector<ColorSpan> spans;
        size_t n = line.size();
        size_t i = 0;

        while (i < n) {
            // Comment <!-- ... -->
            if (i + 3 < n && line.substr(i, 4) == "<!--") {
                size_t start = i;
                i += 4;
                while (i + 2 < n && line.substr(i, 3) != "-->") ++i;
                if (i + 2 < n) i += 3;
                spans.push_back({(int)start, (int)(i - start), HL::COMMENT});
                continue;
            }

            // Tag
            if (line[i] == '<') {
                ++i;
                bool closing = (i < n && line[i] == '/');
                if (closing) ++i;

                // Tag name
                size_t nameStart = i;
                while (i < n && (std::isalnum(line[i]) || line[i] == '-' || line[i] == ':')) ++i;
                if (i > nameStart)
                    spans.push_back({(int)nameStart, (int)(i - nameStart), HL::TAG});

                // Attributes until >
                while (i < n && line[i] != '>') {
                    while (i < n && line[i] == ' ') ++i;
                    if (i < n && line[i] == '>') break;

                    // attribute name
                    size_t attrStart = i;
                    while (i < n && line[i] != '=' && line[i] != ' ' && line[i] != '>') ++i;
                    if (i > attrStart)
                        spans.push_back({(int)attrStart, (int)(i - attrStart), HL::ATTR});

                    if (i < n && line[i] == '=') {
                        ++i;
                        if (i < n && (line[i] == '"' || line[i] == '\'')) {
                            char q = line[i];
                            size_t valStart = i++;
                            while (i < n && line[i] != q) ++i;
                            if (i < n) ++i;
                            spans.push_back({(int)valStart, (int)(i - valStart), HL::STRING});
                        }
                    }
                }
                if (i < n && line[i] == '>') ++i;
                continue;
            }

            // String outside tag (rare, but skip)
            ++i;
        }

        return spans;
    }

    bool supportsFile(const std::string& f) const override {
        auto ext = fs::path(f).extension().string();
        return ext==".html"||ext==".htm"||ext==".xhtml";
    }
};

// ── CSS ────────────────────────────────────────────────────────────────────

class CssHighlighter : public SyntaxHighlighter {
public:
    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        std::vector<ColorSpan> spans;
        size_t n = line.size();
        size_t i = 0;

        // Block comment
        if (i + 1 < n && line[i] == '/' && line[i+1] == '*') {
            size_t start = i;
            i += 2;
            while (i + 1 < n && !(line[i]=='*' && line[i+1]=='/')) ++i;
            if (i + 1 < n) i += 2;
            spans.push_back({(int)start, (int)(i - start), HL::COMMENT});
        }

        // Check if this looks like a selector line (has { or ends before {)
        bool hasBrace = line.find('{') != std::string::npos;
        bool hasColon = line.find(':') != std::string::npos;

        while (i < n) {
            // Comment
            if (i + 1 < n && line[i] == '/' && line[i+1] == '*') {
                size_t start = i; i += 2;
                while (i + 1 < n && !(line[i]=='*' && line[i+1]=='/')) ++i;
                if (i + 1 < n) i += 2;
                spans.push_back({(int)start, (int)(i - start), HL::COMMENT});
                continue;
            }

            // String value
            if (line[i] == '"' || line[i] == '\'') {
                char q = line[i];
                size_t attrValStart = i++;
                while (i < n && line[i] != q) ++i;
                if (i < n) ++i;
                spans.push_back({(int)attrValStart, (int)(i - attrValStart), HL::STRING});
                continue;
            }

            // Property name (before ':' when not a selector line)
            if (!hasBrace && hasColon && line[i] != ' ' && line[i] != '\t') {
                size_t colonPos = line.find(':', i);
                if (colonPos != std::string::npos) {
                    std::string prop = line.substr(i, colonPos - i);
                    // trim right
                    size_t end = prop.find_last_not_of(' ');
                    if (end != std::string::npos) {
                        spans.push_back({(int)i, (int)(end + 1), HL::PROPERTY});
                        i = colonPos + 1;
                        // value: color numbers and hex
                        while (i < n) {
                            while (i < n && line[i] == ' ') ++i;
                            if (i < n && line[i] == '#') {
                                size_t start = i++;
                                while (i < n && std::isxdigit(line[i])) ++i;
                                spans.push_back({(int)start, (int)(i - start), HL::NUMBER});
                            } else if (i < n && std::isdigit(line[i])) {
                                size_t start = i;
                                while (i < n && (std::isalnum(line[i]) || line[i] == '.')) ++i;
                                spans.push_back({(int)start, (int)(i - start), HL::NUMBER});
                            } else {
                                ++i;
                            }
                        }
                        break;
                    }
                }
            }

            // Selector line — color until { as SELECTOR
            if (hasBrace) {
                size_t bracePos = line.find('{');
                std::string sel = line.substr(i, bracePos - i);
                size_t end = sel.find_last_not_of(' ');
                if (end != std::string::npos)
                    spans.push_back({(int)i, (int)(end + 1), HL::SELECTOR});
                i = bracePos + 1;
                hasBrace = false;  // only first { on line
                continue;
            }

            ++i;
        }

        return spans;
    }

    bool supportsFile(const std::string& f) const override {
        auto ext = fs::path(f).extension().string();
        return ext==".css"||ext==".scss"||ext==".less";
    }
};

// ── Factory ────────────────────────────────────────────────────────────────

std::unique_ptr<SyntaxHighlighter> makeHighlighter(const std::string& filename) {
    auto ext = fs::path(filename).extension().string();

    if (ext==".cpp"||ext==".cc"||ext==".cxx"||ext==".c"||
        ext==".hpp"||ext==".hxx"||ext==".h")
        return std::make_unique<CppHighlighter>();

    if (ext==".py")
        return std::make_unique<PythonHighlighter>();

    if (ext==".ts"||ext==".tsx")
        return std::make_unique<JsTsHighlighter>(true);

    if (ext==".js"||ext==".jsx"||ext==".mjs"||ext==".cjs")
        return std::make_unique<JsTsHighlighter>(false);

    if (ext==".html"||ext==".htm"||ext==".xhtml")
        return std::make_unique<HtmlHighlighter>();

    if (ext==".css"||ext==".scss"||ext==".less")
        return std::make_unique<CssHighlighter>();

    return nullptr;
}

// ── Render helper ──────────────────────────────────────────────────────────

std::string renderHighlighted(
    const std::string& line,
    const std::vector<ColorSpan>& spans,
    int scrollCol,
    int cols)
{
    int lineLen = (int)line.size();

    // Build per-character color array
    std::vector<int> colors(lineLen, HL::RESET);
    for (const auto& sp : spans) {
        int end = std::min(sp.col + sp.len, lineLen);
        for (int c = sp.col; c < end; ++c) colors[c] = sp.color;
    }

    std::string out;
    int curColor = HL::RESET;
    int endCol = std::min(scrollCol + cols, lineLen);

    for (int c = scrollCol; c < endCol; ++c) {
        int col = colors[c];
        if (col != curColor) {
            out += "\033[0m";
            if (col != HL::RESET) out += HL::ansi(col);
            curColor = col;
        }
        out += line[c];
    }

    if (curColor != HL::RESET) out += "\033[0m";

    // Pad to fill cols
    int visible = endCol - scrollCol;
    int rem = cols - visible;
    if (rem > 0) out += std::string(rem, ' ');

    return out;
}
