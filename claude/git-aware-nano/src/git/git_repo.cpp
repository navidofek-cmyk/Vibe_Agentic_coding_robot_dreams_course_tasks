#include "git_repo.h"
#include "git_process.h"
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;
namespace git {

RepoInfo detect(const std::string& path) {
    RepoInfo info;
    // git -C needs a directory, not a file path
    fs::path p(path);
    std::string dir = fs::is_directory(p) ? p.string() : p.parent_path().string();
    if (dir.empty()) dir = ".";
    std::string root = run(dir, "rev-parse --show-toplevel");
    if (root.empty()) return info;
    // strip trailing newline
    while (!root.empty() && (root.back() == '\n' || root.back() == '\r'))
        root.pop_back();
    info.root  = root;
    info.valid = true;
    refresh(info);
    return info;
}

void refresh(RepoInfo& info) {
    if (!info.valid) return;

    info.dirty      = false;
    info.hasStaged  = false;
    info.hasConflict = false;
    info.ahead = info.behind = 0;
    info.branch.clear();

    // porcelain v2 --branch gives us everything in one call
    std::string out = run(info.root, "status --porcelain=v2 --branch");
    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.size() < 2) continue;

        if (line[0] == '#') {
            // header lines
            if (line.rfind("# branch.head ", 0) == 0)
                info.branch = line.substr(14);
            else if (line.rfind("# branch.ab ", 0) == 0) {
                // format: +ahead -behind
                int a = 0, b = 0;
                if (sscanf(line.c_str() + 12, "+%d -%d", &a, &b) == 2) {
                    info.ahead  = a;
                    info.behind = b;
                }
            }
        } else if (line[0] == '1' || line[0] == '2' || line[0] == 'u') {
            // changed entry: "1 XY ..."
            if (line.size() >= 4) {
                char x = line[2], y = line[3];
                if (x == 'U' || y == 'U' ||
                    (x == 'A' && y == 'A') ||
                    (x == 'D' && y == 'D'))
                    info.hasConflict = true;
                if (y != ' ' && y != '.') info.dirty     = true;
                if (x != ' ' && x != '.') info.hasStaged = true;
            }
        } else if (line[0] == '?') {
            info.dirty = true;
        }
    }

    if (info.branch.empty() || info.branch == "(detached)")
        info.branch = run(info.root, "rev-parse --short HEAD");
    while (!info.branch.empty() &&
           (info.branch.back() == '\n' || info.branch.back() == '\r'))
        info.branch.pop_back();
}

} // namespace git
