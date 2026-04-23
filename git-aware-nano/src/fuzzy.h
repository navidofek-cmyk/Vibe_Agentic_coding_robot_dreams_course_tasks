#pragma once
#include <string>
#include <vector>
#include <utility>

// Recursively scan `root`, skip build artifacts and ignored dirs.
// Returns paths relative to root, sorted alphabetically.
std::vector<std::string> collectFiles(const std::string& root);

// Filter and score `files` against `query`.
// Returns (score, path) sorted descending by score.
// If query is empty, returns all files with score=1.
std::vector<std::pair<int,std::string>> fuzzyFilter(
    const std::vector<std::string>& files,
    const std::string& query);
