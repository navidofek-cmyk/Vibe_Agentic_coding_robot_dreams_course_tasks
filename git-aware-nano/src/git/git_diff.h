#pragma once
#include <string>
#include <vector>

namespace git {

enum class GutterMark { None, Added, Modified, Deleted };

// Returns per-line gutter marks (0-based) for filepath vs HEAD.
// filepath is absolute. Result may be shorter than file if no git info.
std::vector<GutterMark> fileDiff(const std::string& repoRoot,
                                  const std::string& filepath);

// Returns unified diff of filepath vs HEAD as a string (for display).
std::string fileDiffText(const std::string& repoRoot,
                          const std::string& filepath);

} // namespace git
