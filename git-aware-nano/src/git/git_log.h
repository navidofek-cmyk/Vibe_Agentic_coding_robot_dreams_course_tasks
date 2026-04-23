#pragma once
#include <string>
#include <vector>

namespace git {

struct LogEntry {
    std::string hash;       // short (8 hex)
    std::string hashFull;   // 40 hex (for git show)
    std::string subject;
    std::string author;
    std::string relDate;    // "2 days ago"
};

// Last n commits touching filepath (or all if filepath empty).
std::vector<LogEntry> fileLog(const std::string& repoRoot,
                               const std::string& filepath,
                               int n = 50);

// List all local branches. Current branch first.
std::vector<std::string> branches(const std::string& repoRoot);

// Checkout branch. Returns error message or empty on success.
std::string checkout(const std::string& repoRoot, const std::string& branch);

// Stage / unstage filepath.
std::string stage(const std::string& repoRoot,   const std::string& filepath);
std::string unstage(const std::string& repoRoot, const std::string& filepath);

// ── Commit tree ──────────────────────────────────────────────────────────

struct TreeEntry {
    std::string path;
    char        status = ' ';  // 'A' added, 'M' modified, 'D' deleted, 'R' renamed, ' ' unchanged
    std::string oldPath;       // for renames
};

// Files in the repo at commit `hash`, annotated with changes vs parent.
std::vector<TreeEntry> commitTree(const std::string& repoRoot,
                                   const std::string& hash);

// Content of filepath at commit hash (raw bytes as string).
std::string fileAtCommit(const std::string& repoRoot,
                          const std::string& hash,
                          const std::string& filepath);

} // namespace git
