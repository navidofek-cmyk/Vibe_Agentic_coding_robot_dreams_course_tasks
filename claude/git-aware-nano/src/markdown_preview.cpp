#include "markdown_preview.h"
#include <algorithm>
#include <cctype>

namespace mdpreview {

// ANSI helpers
static const char* RST  = "\033[0m";
static const char* BOLD = "\033[1m";
static const char* UND  = "\033[4m";
static const char* DIM  = "\033[2m";

static std::string col(const char* code, const std::string& s) {
    return std::string(code) + s + RST;
}

// Heading colours for levels 1-6
static const char* H_COLOR[6] = {
    "\033[1m\033[33m",   // H1 bold yellow
    "\033[1m\033[36m",   // H2 bold cyan
    "\033[1m\033[32m",   // H3 bold green
    "\033[1m\033[35m",   // H4 bold magenta
    "\033[1m\033[34m",   // H5 bold blue
    "\033[1m\033[90m",   // H6 bold gray
};

// Render inline markdown: bold, italic, inline code, links.
// Returns string with ANSI codes; visLen is the visible char count written.
static std::string renderInline(const std::string& text, int maxVis, int& visLen) {
    std::string out;
    size_t i = 0, n = text.size();
    visLen = 0;

    while (i < n && visLen < maxVis) {
        // Bold **...**
        if (i + 1 < n && text[i] == '*' && text[i+1] == '*') {
            size_t end = text.find("**", i + 2);
            if (end != std::string::npos) {
                std::string inner = text.substr(i + 2, end - (i + 2));
                int take = std::min((int)inner.size(), maxVis - visLen);
                out += std::string(BOLD) + inner.substr(0, take) + RST;
                visLen += take;
                i = end + 2;
                continue;
            }
        }
        // Italic *...* (not **)
        if (text[i] == '*' && (i + 1 >= n || text[i+1] != '*')
                            && (i == 0    || text[i-1] != '*')) {
            size_t end = text.find('*', i + 1);
            if (end != std::string::npos && text[end-1] != '*') {
                std::string inner = text.substr(i + 1, end - (i + 1));
                int take = std::min((int)inner.size(), maxVis - visLen);
                out += std::string(UND) + inner.substr(0, take) + RST;
                visLen += take;
                i = end + 1;
                continue;
            }
        }
        // Inline code `...`
        if (text[i] == '`') {
            size_t end = text.find('`', i + 1);
            if (end != std::string::npos) {
                std::string inner = text.substr(i + 1, end - (i + 1));
                int take = std::min((int)inner.size(), maxVis - visLen);
                out += "\033[33m" + inner.substr(0, take) + RST;
                visLen += take;
                i = end + 1;
                continue;
            }
        }
        // Link [text](url) → underlined text
        if (text[i] == '[') {
            size_t cb = text.find(']', i + 1);
            if (cb != std::string::npos && cb + 1 < n && text[cb+1] == '(') {
                size_t pe = text.find(')', cb + 2);
                if (pe != std::string::npos) {
                    std::string linkText = text.substr(i + 1, cb - (i + 1));
                    int take = std::min((int)linkText.size(), maxVis - visLen);
                    out += "\033[34m\033[4m" + linkText.substr(0, take) + RST;
                    visLen += take;
                    i = pe + 1;
                    continue;
                }
            }
        }
        // Image ![alt](url) → [img: alt]
        if (text[i] == '!' && i + 1 < n && text[i+1] == '[') {
            size_t cb = text.find(']', i + 2);
            if (cb != std::string::npos && cb + 1 < n && text[cb+1] == '(') {
                size_t pe = text.find(')', cb + 2);
                if (pe != std::string::npos) {
                    std::string alt = "[img: " + text.substr(i + 2, cb - (i + 2)) + "]";
                    int take = std::min((int)alt.size(), maxVis - visLen);
                    out += "\033[35m" + alt.substr(0, take) + RST;
                    visLen += take;
                    i = pe + 1;
                    continue;
                }
            }
        }
        // Task list checkbox at line start (handled before inline, but catch here too)
        out += text[i];
        visLen++;
        i++;
    }
    return out;
}

static std::string pad(const std::string& ansiStr, int visLen, int cols) {
    int rem = cols - visLen;
    if (rem <= 0) return ansiStr;
    return ansiStr + std::string(rem, ' ');
}

static bool isHRule(const std::string& line) {
    if (line.size() < 3) return false;
    char c = line[0];
    if (c != '-' && c != '*' && c != '_') return false;
    for (char ch : line)
        if (ch != c && ch != ' ') return false;
    return true;
}

std::vector<std::string> render(const std::vector<std::string>& lines, int cols) {
    std::vector<std::string> out;
    out.reserve(lines.size());

    bool inCodeBlock = false;

    for (const auto& raw : lines) {
        // ── Code fence ───────────────────────────────────────────────
        if (raw.size() >= 3 && (raw.substr(0,3) == "```" || raw.substr(0,3) == "~~~")) {
            inCodeBlock = !inCodeBlock;
            std::string vis = raw.size() > (size_t)cols ? raw.substr(0, cols) : raw;
            out.push_back(pad(col(DIM, vis), (int)vis.size(), cols));
            continue;
        }
        if (inCodeBlock) {
            std::string vis = raw.size() > (size_t)cols ? raw.substr(0, cols) : raw;
            out.push_back(pad("\033[33m" + vis + RST, (int)vis.size(), cols));
            continue;
        }

        // ── Horizontal rule ──────────────────────────────────────────
        if (isHRule(raw)) {
            out.push_back(col(DIM, std::string(cols, '-')));
            continue;
        }

        // ── Heading ──────────────────────────────────────────────────
        if (!raw.empty() && raw[0] == '#') {
            int lvl = 0;
            while (lvl < (int)raw.size() && raw[lvl] == '#') lvl++;
            std::string text = (lvl < (int)raw.size() - 1) ? raw.substr(lvl + 1) : "";
            const char* hc = H_COLOR[std::min(lvl - 1, 5)];
            int visLen = 0;
            std::string rendered = std::string(hc)
                + renderInline(text, cols, visLen)
                + RST;
            out.push_back(pad(rendered, visLen, cols));
            // H1/H2: underline with ─
            if (lvl <= 2) {
                int len = std::min((int)text.size(), cols);
                out.push_back(col(DIM, std::string(len, '-')) + std::string(cols - len, ' '));
            }
            continue;
        }

        // ── Blockquote ───────────────────────────────────────────────
        if (!raw.empty() && raw[0] == '>') {
            std::string inner = (raw.size() > 2) ? raw.substr(2) : "";
            int visLen = 0;
            std::string rendered = std::string(DIM) + "| " + RST
                + renderInline(inner, cols - 2, visLen);
            out.push_back(pad(rendered, 2 + visLen, cols));
            continue;
        }

        // ── Task list ────────────────────────────────────────────────
        if (raw.size() >= 6 && raw.substr(0, 3) == "- [") {
            char state = (raw.size() > 3) ? raw[3] : ' ';
            std::string rest = (raw.size() > 6) ? raw.substr(6) : "";
            std::string box  = (state == 'x' || state == 'X')
                ? "\033[32m[x]\033[0m " : "\033[90m[ ]\033[0m ";
            int visLen = 0;
            std::string rendered = box + renderInline(rest, cols - 6, visLen);
            out.push_back(pad(rendered, 6 + visLen, cols));
            continue;
        }

        // ── Bullet list ──────────────────────────────────────────────
        {
            size_t indent = 0;
            while (indent < raw.size() && raw[indent] == ' ') indent++;
            if (indent < raw.size() &&
                (raw[indent]=='-'||raw[indent]=='+'||raw[indent]=='*')
                && indent + 1 < raw.size() && raw[indent+1]==' ') {
                std::string inner = raw.substr(indent + 2);
                std::string pfx   = std::string(indent, ' ') + "\033[33m•\033[0m ";
                int visLen = 0;
                std::string rendered = pfx + renderInline(inner, cols - (int)indent - 2, visLen);
                out.push_back(pad(rendered, (int)indent + 2 + visLen, cols));
                continue;
            }
        }

        // ── Numbered list ────────────────────────────────────────────
        {
            size_t j = 0;
            while (j < raw.size() && std::isdigit((unsigned char)raw[j])) j++;
            if (j > 0 && j < raw.size() && raw[j] == '.' && j+1 < raw.size() && raw[j+1]==' ') {
                std::string num   = raw.substr(0, j);
                std::string inner = raw.substr(j + 2);
                std::string pfx   = "\033[33m" + num + ".\033[0m ";
                int visLen = 0;
                std::string rendered = pfx + renderInline(inner, cols - (int)j - 2, visLen);
                out.push_back(pad(rendered, (int)j + 2 + visLen, cols));
                continue;
            }
        }

        // ── Plain paragraph / inline ─────────────────────────────────
        if (raw.empty()) {
            out.push_back(std::string(cols, ' '));
        } else {
            int visLen = 0;
            std::string rendered = renderInline(raw, cols, visLen);
            out.push_back(pad(rendered, visLen, cols));
        }
    }

    return out;
}

} // namespace mdpreview
