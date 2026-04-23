#include "fuzzy.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace fs = std::filesystem;

static bool isIgnoredDir(const std::string& name) {
    static const std::unordered_set<std::string> ignored = {
        ".git", "build", "node_modules", ".cache", "__pycache__",
        ".venv", "venv", "target", "dist", ".next", "CMakeFiles",
        ".idea", ".vscode", ".mypy_cache", "coverage"
    };
    return ignored.count(name) > 0;
}

static bool isIgnoredFile(const std::string& name) {
    static const std::vector<std::string> exts = {
        ".o", ".so", ".a", ".dylib", ".exe", ".out",
        ".pyc", ".pyo", ".class",
        ".jpg", ".jpeg", ".png", ".gif", ".ico", ".webp",
        ".pdf", ".zip", ".tar", ".gz", ".bz2", ".xz",
        ".db", ".sqlite", ".sqlite3", ".lock"
    };
    for (const auto& e : exts) {
        if (name.size() >= e.size() &&
            name.compare(name.size() - e.size(), e.size(), e) == 0)
            return true;
    }
    return false;
}

static void scanDir(const fs::path& dir, const fs::path& root,
                    std::vector<std::string>& out, int depth) {
    if (depth > 12) return;
    try {
        for (auto& entry : fs::directory_iterator(dir)) {
            auto name = entry.path().filename().string();
            if (entry.is_directory()) {
                if (!isIgnoredDir(name))
                    scanDir(entry.path(), root, out, depth + 1);
            } else if (entry.is_regular_file()) {
                if (!isIgnoredFile(name))
                    out.push_back(fs::relative(entry.path(), root).string());
            }
        }
    } catch (...) {}
}

std::vector<std::string> collectFiles(const std::string& root) {
    std::vector<std::string> result;
    scanDir(fs::path(root), fs::path(root), result, 0);
    std::sort(result.begin(), result.end());
    return result;
}

// Score `query` against `candidate`. Returns -1 if not a subsequence match.
// Higher = better match.
static int scoreMatch(const std::string& query, const std::string& candidate) {
    if (query.empty()) return 1;

    size_t slash = candidate.rfind('/');
    const std::string& base = (slash == std::string::npos)
        ? candidate : candidate.substr(slash + 1);

    int qi = 0, qn = (int)query.size();
    int cm = (int)candidate.size();
    int score = 0, consecutive = 0, lastMatch = -1;
    bool inBase = (slash == std::string::npos);

    for (int ci = 0; ci < cm && qi < qn; ++ci) {
        if (ci == (int)(slash + 1)) inBase = true;

        char qc = (char)std::tolower((unsigned char)query[qi]);
        char cc = (char)std::tolower((unsigned char)candidate[ci]);
        if (qc != cc) { consecutive = 0; continue; }

        int ms = 1;

        // Consecutive run bonus
        if (ci == lastMatch + 1) { consecutive++; ms += consecutive * 2; }
        else consecutive = 0;

        // Word-boundary bonus: after /, _, -, ., or camelCase transition
        if (ci == 0 || candidate[ci-1] == '/' || candidate[ci-1] == '_' ||
            candidate[ci-1] == '-' || candidate[ci-1] == '.' ||
            (std::isupper((unsigned char)candidate[ci]) && ci > 0 &&
             std::islower((unsigned char)candidate[ci-1])))
            ms += 5;

        // Basename bonus
        if (inBase) ms += 2;

        // Case-sensitive bonus
        if (query[qi] == candidate[ci]) ms += 1;

        score += ms;
        lastMatch = ci;
        ++qi;
    }

    if (qi < qn) return -1;  // didn't consume whole query

    // Prefer shorter candidates
    score += std::max(0, 80 - cm);

    // Extra bonus if full query matches just the basename
    {
        int bqi = 0, bn = (int)base.size();
        for (int bi = 0; bi < bn && bqi < qn; ++bi) {
            if (std::tolower((unsigned char)base[bi]) ==
                std::tolower((unsigned char)query[bqi]))
                ++bqi;
        }
        if (bqi == qn) score += 10;
    }

    return score;
}

std::vector<std::pair<int,std::string>> fuzzyFilter(
    const std::vector<std::string>& files,
    const std::string& query)
{
    std::vector<std::pair<int,std::string>> result;
    result.reserve(files.size());

    for (const auto& f : files) {
        int s = scoreMatch(query, f);
        if (s > 0) result.push_back({s, f});
    }

    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b){ return a.first > b.first; });

    if ((int)result.size() > 200) result.resize(200);
    return result;
}
