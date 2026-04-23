#include "git_blame.h"
#include "git_process.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <ctime>

namespace git {

std::vector<BlameEntry> fileBlame(const std::string& repoRoot,
                                   const std::string& filepath) {
    std::string out = run(repoRoot,
        "blame --porcelain " + shellQuote(filepath));
    if (out.empty()) return {};

    // porcelain format:
    //   <40hex> <orig-line> <final-line> [<count>]
    //   author <name>
    //   author-time <unix-ts>
    //   summary <subject>
    //   \t<line-content>

    std::unordered_map<std::string, BlameEntry> cache;
    std::vector<BlameEntry> result;
    std::istringstream ss(out);
    std::string line;
    BlameEntry cur;
    std::string curHash;

    while (std::getline(ss, line)) {
        if (line.empty()) continue;

        if (line[0] == '\t') {
            // line content → emit entry for this line
            if (!curHash.empty()) {
                cache[curHash] = cur;
                result.push_back(cur);
                curHash.clear();
            }
            continue;
        }

        // new commit header: 40 hex chars at start
        if (line.size() >= 40 &&
            std::all_of(line.begin(), line.begin() + 40,
                        [](char c){ return std::isxdigit((unsigned char)c); })) {
            curHash = line.substr(0, 40);
            auto it = cache.find(curHash);
            if (it != cache.end()) {
                cur = it->second;
            } else {
                cur = {};
                cur.hash = curHash.substr(0, 8);
            }
            continue;
        }

        if (line.rfind("author ", 0) == 0 && line.find("author-") == std::string::npos)
            cur.author = line.substr(7);
        else if (line.rfind("author-time ", 0) == 0) {
            time_t t = (time_t)std::stol(line.substr(12));
            struct tm tm {};
            localtime_r(&t, &tm);
            char buf[16];
            strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
            cur.date = buf;
        } else if (line.rfind("summary ", 0) == 0) {
            cur.summary = line.substr(8);
        }
    }

    return result;
}

} // namespace git
