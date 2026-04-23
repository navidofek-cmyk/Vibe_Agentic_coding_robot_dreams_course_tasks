#include "git_diff.h"
#include "git_process.h"
#include <sstream>
#include <cstring>

namespace git {

static int parseHunkNewStart(const std::string& line) {
    // @@ -old_start[,old_count] +new_start[,new_count] @@
    const char* p = strstr(line.c_str(), "+");
    if (!p) return -1;
    return atoi(p + 1);
}

std::vector<GutterMark> fileDiff(const std::string& repoRoot,
                                  const std::string& filepath) {
    std::string out = run(repoRoot,
        "diff HEAD -- " + shellQuote(filepath));
    if (out.empty()) return {};

    std::vector<GutterMark> marks;
    marks.reserve(512);

    std::istringstream ss(out);
    std::string line;
    int newLine = 0;   // 1-based current position in new file
    int delCount = 0;  // pending unmatched deletions

    auto ensureSize = [&](int idx) {
        if (idx >= (int)marks.size())
            marks.resize(idx + 1, GutterMark::None);
    };

    while (std::getline(ss, line)) {
        if (line.size() >= 2 && line[0] == '@' && line[1] == '@') {
            // flush pending deletions at current position
            if (delCount > 0 && newLine > 0) {
                int idx = newLine - 1;
                ensureSize(idx);
                if (marks[idx] == GutterMark::None)
                    marks[idx] = GutterMark::Deleted;
                delCount = 0;
            }
            int ns = parseHunkNewStart(line);
            if (ns > 0) newLine = ns;
            continue;
        }
        if (newLine == 0) continue;
        if (line.empty())  continue;

        char c = line[0];
        if (c == ' ') {
            // context line: flush any pending deletions
            if (delCount > 0) {
                int idx = newLine - 1;
                ensureSize(idx);
                if (marks[idx] == GutterMark::None)
                    marks[idx] = GutterMark::Deleted;
                delCount = 0;
            }
            newLine++;
        } else if (c == '-') {
            delCount++;
        } else if (c == '+') {
            int idx = newLine - 1;
            ensureSize(idx);
            if (delCount > 0) {
                marks[idx] = GutterMark::Modified;
                delCount--;
            } else {
                marks[idx] = GutterMark::Added;
            }
            newLine++;
        }
    }
    // flush trailing deletions
    if (delCount > 0 && newLine > 0) {
        int idx = newLine - 1;
        ensureSize(idx);
        if (marks[idx] == GutterMark::None)
            marks[idx] = GutterMark::Deleted;
    }

    return marks;
}

std::string fileDiffText(const std::string& repoRoot,
                          const std::string& filepath) {
    return run(repoRoot, "diff HEAD -- " + shellQuote(filepath));
}

} // namespace git
