#pragma once
#include <string>
#include <vector>

namespace git {

struct BlameEntry {
    std::string hash;      // short (8 hex)
    std::string author;
    std::string date;      // YYYY-MM-DD
    std::string summary;   // commit subject
};

// Returns one BlameEntry per line (0-based). filepath is absolute.
std::vector<BlameEntry> fileBlame(const std::string& repoRoot,
                                   const std::string& filepath);

} // namespace git
