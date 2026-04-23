#include "git_status.h"
#include "git_process.h"
#include <sstream>

namespace git {

std::map<std::string, char> fileStatuses(const std::string& repoRoot) {
    std::map<std::string, char> result;
    std::string out = run(repoRoot, "status --porcelain=v1 -u");
    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.size() < 4) continue;
        char x = line[0], y = line[1];
        std::string path = line.substr(3);
        // handle renames: "old -> new" → take "new"
        size_t arrow = path.find(" -> ");
        if (arrow != std::string::npos) path = path.substr(arrow + 4);

        char mark;
        if (x == '?' && y == '?')                        mark = '+';
        else if (x == 'U' || y == 'U' ||
                 (x == 'A' && y == 'A') ||
                 (x == 'D' && y == 'D'))                 mark = '!';
        else if (y != ' ' && y != '.')                   mark = 'M';
        else                                             mark = 'S';

        result[path] = mark;

        // propagate to parent directories
        size_t pos = path.rfind('/');
        while (pos != std::string::npos) {
            path = path.substr(0, pos);
            auto it = result.find(path);
            // don't downgrade ! → M, M → S
            if (it == result.end() || it->second == 'S')
                result[path] = mark;
            pos = path.rfind('/');
        }
    }
    return result;
}

} // namespace git
