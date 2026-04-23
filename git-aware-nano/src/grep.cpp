#include "grep.h"
#include "fuzzy.h"
#include <fstream>
#include <algorithm>
#include <regex>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<GrepResult> GrepSearch::snapshot() const {
    std::lock_guard<std::mutex> lk(mu_);
    return results_;
}

void GrepSearch::stop() {
    stop_ = true;
    if (thread_.joinable()) thread_.join();
    stop_ = false;
}

void GrepSearch::start(const std::string& root,
                       const std::string& pattern,
                       GrepOptions        opts) {
    stop();
    {
        std::lock_guard<std::mutex> lk(mu_);
        results_.clear();
    }
    total_ = 0;
    if (pattern.empty()) { done_ = true; return; }
    done_ = false;
    thread_ = std::thread(&GrepSearch::run, this, root, pattern, opts);
}

static bool matchLine(const std::string& text,
                      const std::string& pattern,
                      const std::string& lowerPat,
                      const GrepOptions& opts,
                      const std::regex*  re,
                      int& outCol, int& outLen) {
    if (opts.useRegex && re) {
        std::smatch m;
        if (!std::regex_search(text, m, *re)) return false;
        outCol = (int)m.position(0);
        outLen = (int)m.length(0);
        return true;
    }

    auto isWord = [](unsigned char c) { return std::isalnum(c) || c == '_'; };
    size_t n = text.size(), m = pattern.size();
    if (m == 0 || n < m) return false;

    for (size_t i = 0; i + m <= n; ++i) {
        bool match = true;
        for (size_t j = 0; j < m; ++j) {
            char tc = opts.caseSensitive
                ? text[i + j]
                : (char)std::tolower((unsigned char)text[i + j]);
            if (tc != lowerPat[j]) { match = false; break; }
        }
        if (!match) continue;

        if (opts.wholeWord) {
            bool leftOk  = (i == 0     || !isWord(text[i - 1]));
            bool rightOk = (i + m >= n || !isWord(text[i + m]));
            if (!leftOk || !rightOk) continue;
        }

        outCol = (int)i;
        outLen = (int)m;
        return true;
    }
    return false;
}

void GrepSearch::run(std::string root, std::string pattern, GrepOptions opts) {
    std::unique_ptr<std::regex> re;
    if (opts.useRegex) {
        try {
            auto flags = std::regex::ECMAScript;
            if (!opts.caseSensitive) flags |= std::regex::icase;
            re = std::make_unique<std::regex>(pattern, flags);
        } catch (...) {
            done_ = true;
            return;
        }
    }

    std::string lowerPat = pattern;
    if (!opts.caseSensitive)
        std::transform(lowerPat.begin(), lowerPat.end(), lowerPat.begin(),
                       [](unsigned char c){ return (char)std::tolower(c); });

    static constexpr int kMaxResults = 2000;
    auto files = collectFiles(root);

    for (const auto& relPath : files) {
        if (stop_) break;

        std::ifstream f((fs::path(root) / relPath).string());
        if (!f.is_open()) continue;

        std::string line;
        int lineNum = 0;
        while (std::getline(f, line) && !stop_) {
            ++lineNum;
            int col = 0, len = 0;
            if (matchLine(line, pattern, lowerPat, opts, re.get(), col, len)) {
                std::lock_guard<std::mutex> lk(mu_);
                if ((int)results_.size() < kMaxResults)
                    results_.push_back({relPath, lineNum, line, col, len});
                total_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
    done_ = true;
}
