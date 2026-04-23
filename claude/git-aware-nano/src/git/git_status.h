#pragma once
#include <string>
#include <map>

namespace git {

// Status char for a file: 'M'=modified, '+'=untracked, 'S'=staged, '!'=conflict
// Maps relative-to-root path → char
std::map<std::string, char> fileStatuses(const std::string& repoRoot);

} // namespace git
