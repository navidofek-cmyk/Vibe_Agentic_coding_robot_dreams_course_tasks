#include "git_process.h"
#include <cstdio>
#include <array>

namespace git {

std::string shellQuote(const std::string& s) {
    std::string r = "'";
    for (char c : s) {
        if (c == '\'') r += "'\\''";
        else           r += c;
    }
    return r + "'";
}

std::string run(const std::string& repoRoot, const std::string& args) {
    std::string cmd = "git -C " + shellQuote(repoRoot) + " " + args + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    std::string result;
    std::array<char, 512> buf;
    while (fgets(buf.data(), (int)buf.size(), pipe))
        result += buf.data();
    pclose(pipe);
    return result;
}

} // namespace git
