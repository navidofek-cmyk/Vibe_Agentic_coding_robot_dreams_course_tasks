#pragma once
#include <string>

namespace git {

struct RepoInfo {
    bool        valid    = false;
    std::string root;           // absolute repo root
    std::string branch;         // current branch or "(detached)"
    int         ahead    = 0;
    int         behind   = 0;
    bool        dirty    = false;
    bool        hasStaged = false;
    bool        hasConflict = false;
};

// Walk up from path to find repo root. Returns invalid RepoInfo if none found.
RepoInfo detect(const std::string& path);

// Re-fetch branch/ahead/behind/dirty for an already-detected repo.
void refresh(RepoInfo& info);

} // namespace git
