#include "git_log.h"
#include "git_process.h"
#include <sstream>
#include <algorithm>
#include <map>

namespace git {

std::vector<LogEntry> fileLog(const std::string& repoRoot,
                               const std::string& filepath,
                               int n) {
    // \x1f = unit separator, safe delimiter
    std::string fmt = "--format=%H%x1f%s%x1f%an%x1f%ar";
    std::string cmd = "log " + fmt + " -n " + std::to_string(n);
    if (!filepath.empty())
        cmd += " -- " + shellQuote(filepath);

    std::string out = run(repoRoot, cmd);
    std::vector<LogEntry> result;
    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        // split by \x1f
        auto split = [&](const std::string& s) {
            std::vector<std::string> parts;
            std::string cur;
            for (char c : s) {
                if (c == '\x1f') { parts.push_back(cur); cur.clear(); }
                else cur += c;
            }
            parts.push_back(cur);
            return parts;
        };
        auto parts = split(line);
        if (parts.size() < 4) continue;
        LogEntry e;
        e.hashFull = parts[0];
        e.hash     = parts[0].size() >= 8 ? parts[0].substr(0, 8) : parts[0];
        e.subject  = parts[1];
        e.author   = parts[2];
        e.relDate  = parts[3];
        result.push_back(std::move(e));
    }
    return result;
}

std::vector<std::string> branches(const std::string& repoRoot) {
    std::string out = run(repoRoot, "branch --format=%(refname:short)");
    std::string current = run(repoRoot, "rev-parse --abbrev-ref HEAD");
    while (!current.empty() && (current.back()=='\n'||current.back()=='\r'))
        current.pop_back();

    std::vector<std::string> result;
    // current branch first
    if (!current.empty()) result.push_back(current);

    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back()=='\n'||line.back()=='\r'||line.back()==' '))
            line.pop_back();
        if (!line.empty() && line != current)
            result.push_back(line);
    }
    return result;
}

std::string checkout(const std::string& repoRoot, const std::string& branch) {
    // run without 2>/dev/null to capture errors
    std::string cmd = "git -C " + shellQuote(repoRoot)
                    + " checkout " + shellQuote(branch) + " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "popen failed";
    std::string err;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) err += buf;
    int rc = pclose(pipe);
    if (rc == 0) return "";
    while (!err.empty() && (err.back()=='\n'||err.back()=='\r')) err.pop_back();
    return err;
}

std::string stage(const std::string& repoRoot, const std::string& filepath) {
    std::string cmd = "git -C " + shellQuote(repoRoot)
                    + " add " + shellQuote(filepath) + " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "popen failed";
    std::string err; char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) err += buf;
    int rc = pclose(pipe);
    if (rc == 0) return "";
    while (!err.empty() && (err.back()=='\n'||err.back()=='\r')) err.pop_back();
    return err;
}

std::vector<TreeEntry> commitTree(const std::string& repoRoot,
                                   const std::string& hash) {
    // All files at this commit
    std::string allOut = run(repoRoot, "ls-tree -r --name-only " + shellQuote(hash));
    // Changes vs parent (first parent)
    std::string diffOut = run(repoRoot,
        "diff-tree --no-commit-id -r --name-status -M " + shellQuote(hash));

    // Build status map: path → {status, oldPath}
    std::map<std::string, std::pair<char, std::string>> statusMap;
    {
        std::istringstream ss(diffOut);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            char s = line[0];
            if (s == 'R') {
                // R<score>\told\tnew
                size_t t1 = line.find('\t');
                size_t t2 = (t1 != std::string::npos) ? line.find('\t', t1 + 1) : std::string::npos;
                if (t2 != std::string::npos) {
                    std::string oldP = line.substr(t1 + 1, t2 - t1 - 1);
                    std::string newP = line.substr(t2 + 1);
                    statusMap[newP] = {'R', oldP};
                }
            } else {
                size_t t = line.find('\t');
                if (t != std::string::npos)
                    statusMap[line.substr(t + 1)] = {s, ""};
            }
        }
    }

    std::vector<TreeEntry> result;
    std::istringstream ss(allOut);
    std::string path;
    while (std::getline(ss, path)) {
        while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
            path.pop_back();
        if (path.empty()) continue;
        TreeEntry e;
        e.path = path;
        auto it = statusMap.find(path);
        if (it != statusMap.end()) {
            e.status  = it->second.first;
            e.oldPath = it->second.second;
        }
        result.push_back(std::move(e));
    }
    // Sort: changed files first, then alphabetical
    std::sort(result.begin(), result.end(), [](const TreeEntry& a, const TreeEntry& b) {
        bool aChanged = (a.status != ' ');
        bool bChanged = (b.status != ' ');
        if (aChanged != bChanged) return aChanged > bChanged;
        return a.path < b.path;
    });
    return result;
}

std::string fileAtCommit(const std::string& repoRoot,
                          const std::string& hash,
                          const std::string& filepath) {
    return run(repoRoot, "show " + shellQuote(hash + ":" + filepath));
}

std::string unstage(const std::string& repoRoot, const std::string& filepath) {
    std::string cmd = "git -C " + shellQuote(repoRoot)
                    + " restore --staged " + shellQuote(filepath) + " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "popen failed";
    std::string err; char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) err += buf;
    int rc = pclose(pipe);
    if (rc == 0) return "";
    while (!err.empty() && (err.back()=='\n'||err.back()=='\r')) err.pop_back();
    return err;
}

} // namespace git
