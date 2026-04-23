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

// ── JSON ───────────────────────────────────────────────────────────────────

class JsonHighlighter : public SyntaxHighlighter {
public:
    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        std::vector<ColorSpan> spans;
        size_t n = line.size();
        size_t i = 0;

        while (i < n) {
            while (i < n && (line[i] == ' ' || line[i] == '\t')) ++i;
            if (i >= n) break;

            if (line[i] == '"') {
                size_t start = i++;
                while (i < n && !(line[i] == '"' && line[i-1] != '\\')) ++i;
                if (i < n) ++i;
                size_t end = i;
                // peek for ':' → this is a key
                size_t j = end;
                while (j < n && (line[j] == ' ' || line[j] == '\t')) ++j;
                bool isKey = (j < n && line[j] == ':');
                spans.push_back({(int)start, (int)(end - start),
                                 isKey ? HL::TYPE : HL::STRING});
                if (isKey) i = j + 1;
                continue;
            }
            if (line[i] == '-' || std::isdigit(line[i])) {
                size_t start = i;
                if (line[i] == '-') ++i;
                while (i < n && (std::isdigit(line[i]) || line[i] == '.' ||
                                  line[i] == 'e' || line[i] == 'E' ||
                                  line[i] == '+' || line[i] == '-')) ++i;
                spans.push_back({(int)start, (int)(i - start), HL::NUMBER});
                continue;
            }
            if (std::isalpha(line[i])) {
                size_t start = i;
                while (i < n && std::isalpha(line[i])) ++i;
                std::string w = line.substr(start, i - start);
                if (w == "true" || w == "false" || w == "null")
                    spans.push_back({(int)start, (int)(i - start), HL::KEYWORD});
                continue;
            }
            ++i;
        }
        return spans;
    }

    bool supportsFile(const std::string& f) const override {
        return fs::path(f).extension().string() == ".json";
    }
};

// ── YAML ───────────────────────────────────────────────────────────────────

class YamlHighlighter : public SyntaxHighlighter {
public:
    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        std::vector<ColorSpan> spans;
        size_t n = line.size();
        size_t i = 0;

        while (i < n && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (i >= n) return spans;

        if (line[i] == '#') {
            spans.push_back({(int)i, (int)(n - i), HL::COMMENT});
            return spans;
        }
        if (n - i >= 3 && (line.substr(i,3) == "---" || line.substr(i,3) == "...")) {
            spans.push_back({(int)i, (int)(n - i), HL::KEYWORD});
            return spans;
        }
        if (line[i] == '-' && i+1 < n && line[i+1] == ' ') {
            spans.push_back({(int)i, 1, HL::KEYWORD});
            i += 2;
        }
        if (i < n && (line[i] == '&' || line[i] == '*')) {
            size_t start = i++;
            while (i < n && (std::isalnum(line[i]) || line[i] == '_' || line[i] == '-')) ++i;
            spans.push_back({(int)start, (int)(i - start), HL::PREPROC});
        }

        // find colon separator for key: value
        size_t colonPos = std::string::npos;
        {
            bool inStr = false; char sq = 0;
            for (size_t j = i; j < n; ++j) {
                if (!inStr && (line[j] == '"' || line[j] == '\'')) { inStr = true; sq = line[j]; }
                else if (inStr && line[j] == sq) inStr = false;
                else if (!inStr && line[j] == ':' && (j+1 >= n || line[j+1] == ' ')) {
                    colonPos = j; break;
                }
            }
        }

        if (colonPos != std::string::npos) {
            spans.push_back({(int)i, (int)(colonPos - i), HL::TYPE});
            i = colonPos + 1;
        }

        while (i < n) {
            while (i < n && (line[i] == ' ' || line[i] == '\t')) ++i;
            if (i < n && (line[i] == '"' || line[i] == '\'')) {
                char q = line[i]; size_t start = i++;
                while (i < n && line[i] != q) ++i;
                if (i < n) ++i;
                spans.push_back({(int)start, (int)(i - start), HL::STRING});
            } else if (i < n && line[i] == '#') {
                spans.push_back({(int)i, (int)(n - i), HL::COMMENT});
                break;
            } else if (i < n && (std::isdigit(line[i]) || line[i] == '-')) {
                size_t start = i;
                while (i < n && (std::isdigit(line[i]) || line[i] == '.' || line[i] == '-')) ++i;
                spans.push_back({(int)start, (int)(i - start), HL::NUMBER});
            } else if (i < n && std::isalpha(line[i])) {
                size_t start = i;
                while (i < n && (std::isalnum(line[i]) || line[i] == '_')) ++i;
                std::string w = line.substr(start, i - start);
                if (w=="true"||w=="false"||w=="null"||w=="yes"||w=="no"||w=="on"||w=="off")
                    spans.push_back({(int)start, (int)(i - start), HL::KEYWORD});
                continue;
            } else { ++i; }
        }
        return spans;
    }

    bool supportsFile(const std::string& f) const override {
        auto e = fs::path(f).extension().string();
        return e == ".yaml" || e == ".yml";
    }
};

// ── Shell ──────────────────────────────────────────────────────────────────

class ShellHighlighter : public SyntaxHighlighter {
    ScanConfig cfg_;
public:
    ShellHighlighter() {
        cfg_.keywords = {
            "if","then","else","elif","fi","for","while","do","done",
            "case","esac","in","function","return","exit","break","continue",
            "local","declare","readonly","export","unset","shift","trap",
            "echo","printf","read","source","eval","exec","alias","cd",
            "true","false","test",
        };
        cfg_.lineComment = "#";
    }

    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        auto spans = scanGeneric(line, cfg_);
        // $VAR and ${VAR}
        size_t n = line.size();
        for (size_t i = 0; i < n; ++i) {
            if (line[i] != '$') continue;
            size_t start = i++;
            if (i < n && line[i] == '{') {
                while (i < n && line[i] != '}') ++i;
                if (i < n) ++i;
            } else if (i < n && (std::isalpha(line[i]) || line[i] == '_' ||
                                   line[i] == '?' || line[i] == '#' ||
                                   line[i] == '@' || std::isdigit(line[i]))) {
                while (i < n && (std::isalnum(line[i]) || line[i] == '_')) ++i;
            } else { continue; }
            spans.push_back({(int)start, (int)(i - start), HL::TYPE});
        }
        return spans;
    }

    bool supportsFile(const std::string& f) const override {
        auto ext  = fs::path(f).extension().string();
        auto name = fs::path(f).filename().string();
        return ext==".sh"||ext==".bash"||ext==".zsh"||
               name==".bashrc"||name==".zshrc"||name==".profile"||
               name==".bash_profile"||name==".bash_aliases";
    }
};

// ── Makefile ───────────────────────────────────────────────────────────────

class MakefileHighlighter : public SyntaxHighlighter {
public:
    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        std::vector<ColorSpan> spans;
        size_t n = line.size();
        if (n == 0) return spans;

        // Comment
        if (line[0] == '#') { spans.push_back({0,(int)n,HL::COMMENT}); return spans; }

        // Directives: ifeq, include, define, ...
        static const std::vector<std::string> directives = {
            "include","ifeq","ifneq","ifdef","ifndef","else","endif",
            "define","endef","override","export","unexport","vpath",
        };
        for (const auto& d : directives) {
            if (line.compare(0, d.size(), d) == 0 &&
                (d.size() == n || line[d.size()] == ' ' || line[d.size()] == '\t')) {
                spans.push_back({0,(int)d.size(),HL::KEYWORD});
                break;
            }
        }

        // $(VAR) / ${VAR} and automatic vars $@ $< $^
        for (size_t i = 0; i < n; ++i) {
            if (line[i] != '$') continue;
            size_t start = i++;
            if (i < n && (line[i] == '(' || line[i] == '{')) {
                char close = (line[i] == '(') ? ')' : '}';
                while (i < n && line[i] != close) ++i;
                if (i < n) ++i;
                spans.push_back({(int)start,(int)(i-start),HL::TYPE});
            } else if (i < n && (line[i]=='@'||line[i]=='<'||line[i]=='^'||line[i]=='*')) {
                spans.push_back({(int)start, 2, HL::PREPROC});
                ++i;
            }
        }

        // Variable assignment: VAR = / := / ?= / +=
        if (line[0] != '\t') {
            size_t j = 0;
            while (j < n && (std::isalnum(line[j]) || line[j]=='_' || line[j]=='.')) ++j;
            size_t k = j;
            while (k < n && line[k] == ' ') ++k;
            if (k < n && (line[k]=='='||((line[k]==':'||line[k]=='?'||line[k]=='+')
                           && k+1 < n && line[k+1]=='='))) {
                if (j > 0) spans.push_back({0,(int)j,HL::TYPE});
                return spans;
            }
            // Target rule: target:
            size_t col = line.find(':');
            if (col != std::string::npos && (col+1 >= n || line[col+1] != '=')) {
                spans.push_back({0,(int)col,HL::KEYWORD});
            }
        }

        return spans;
    }

    bool supportsFile(const std::string& f) const override {
        auto name = fs::path(f).filename().string();
        auto ext  = fs::path(f).extension().string();
        return name=="Makefile"||name=="makefile"||name=="GNUmakefile"||
               ext==".mk"||ext==".make";
    }
};

// ── Markdown ───────────────────────────────────────────────────────────────

class MarkdownHighlighter : public SyntaxHighlighter {
public:
    std::vector<ColorSpan> highlight(const std::string& line, int) override {
        std::vector<ColorSpan> spans;
        size_t n = line.size();
        size_t i = 0;
        if (n == 0) return spans;

        // Headings
        if (line[0] == '#') {
            size_t lvl = 0;
            while (lvl < n && line[lvl] == '#') ++lvl;
            static const int hcol[] = {HL::KEYWORD,HL::TYPE,HL::STRING,
                                        HL::NUMBER,HL::PREPROC,HL::COMMENT};
            spans.push_back({0,(int)n, hcol[std::min((int)lvl-1,5)]});
            return spans;
        }
        // Code fence
        if (n >= 3 && (line.substr(0,3)=="```"||line.substr(0,3)=="~~~")) {
            spans.push_back({0,(int)n,HL::STRING});
            return spans;
        }
        // Blockquote
        if (line[0] == '>') {
            spans.push_back({0,1,HL::COMMENT});
            i = 1;
        }
        // Horizontal rule: --- or *** (3+ same chars with optional spaces)
        {
            bool isHr = n >= 3;
            char c0 = line[0];
            if (isHr && (c0=='-'||c0=='*'||c0=='_')) {
                for (size_t j = 0; j < n; ++j)
                    if (line[j]!=c0 && line[j]!=' ') { isHr=false; break; }
                if (isHr) { spans.push_back({0,(int)n,HL::COMMENT}); return spans; }
            }
        }

        while (i < n) {
            // Bold **text**
            if (i+1 < n && line[i]=='*' && line[i+1]=='*') {
                size_t start = i; i += 2;
                while (i+1 < n && !(line[i]=='*' && line[i+1]=='*')) ++i;
                if (i+1 < n) i += 2;
                spans.push_back({(int)start,(int)(i-start),HL::KEYWORD});
                continue;
            }
            // Italic *text*
            if (line[i]=='*' && i+1 < n && line[i+1]!='*' && line[i+1]!=' ') {
                size_t start = i++;
                while (i < n && !(line[i]=='*' && (i+1>=n||line[i+1]!='*'))) ++i;
                if (i < n) ++i;
                spans.push_back({(int)start,(int)(i-start),HL::TYPE});
                continue;
            }
            // Inline code `...`
            if (line[i] == '`') {
                size_t start = i++;
                while (i < n && line[i] != '`') ++i;
                if (i < n) ++i;
                spans.push_back({(int)start,(int)(i-start),HL::STRING});
                continue;
            }
            // Link [text](url)
            if (line[i] == '[') {
                size_t start = i++;
                while (i < n && line[i] != ']') ++i;
                if (i < n && i+1 < n && line[i+1] == '(') {
                    spans.push_back({(int)start,(int)(i-start+1),HL::KEYWORD});
                    i += 2;
                    size_t us = i;
                    while (i < n && line[i] != ')') ++i;
                    if (i < n) { spans.push_back({(int)us,(int)(i-us),HL::TYPE}); ++i; }
                }
                continue;
            }
            // List marker - or *
            if ((line[i]=='-'||line[i]=='+') && i+1 < n && line[i+1]==' ') {
                spans.push_back({(int)i,1,HL::KEYWORD}); ++i; continue;
            }
            ++i;
        }
        return spans;
    }

    bool supportsFile(const std::string& f) const override {
        auto e = fs::path(f).extension().string();
        return e==".md"||e==".markdown"||e==".mkd";
    }
};

// ── Factory ────────────────────────────────────────────────────────────────

std::unique_ptr<SyntaxHighlighter> makeHighlighter(const std::string& filename) {
    auto ext  = fs::path(filename).extension().string();
    auto name = fs::path(filename).filename().string();

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

    if (ext==".json")
        return std::make_unique<JsonHighlighter>();

    if (ext==".yaml"||ext==".yml")
        return std::make_unique<YamlHighlighter>();

    if (ext==".sh"||ext==".bash"||ext==".zsh"||
        name==".bashrc"||name==".zshrc"||name==".profile"||
        name==".bash_profile"||name==".bash_aliases")
        return std::make_unique<ShellHighlighter>();

    if (name=="Makefile"||name=="makefile"||name=="GNUmakefile"||
        ext==".mk"||ext==".make")
        return std::make_unique<MakefileHighlighter>();

    if (ext==".md"||ext==".markdown"||ext==".mkd")
        return std::make_unique<MarkdownHighlighter>();

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
