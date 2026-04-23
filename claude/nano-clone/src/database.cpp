#include "database.h"
#include <fstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

Database::Database(const std::string& path) : path_(path) {
    fs::create_directories(fs::path(path).parent_path());
}

void Database::addRecentFile(const std::string& file) {
    auto files = getRecentFiles(100);
    files.erase(std::remove(files.begin(), files.end(), file), files.end());
    files.insert(files.begin(), file);
    if (files.size() > 50) files.resize(50);

    std::ofstream f(path_);
    for (const auto& p : files) f << p << '\n';
}

void Database::removeRecentFile(const std::string& file) {
    auto files = getRecentFiles(100);
    files.erase(std::remove(files.begin(), files.end(), file), files.end());
    std::ofstream f(path_);
    for (const auto& p : files) f << p << '\n';
}

std::vector<std::string> Database::getRecentFiles(int limit) const {
    std::ifstream f(path_);
    std::vector<std::string> result;
    std::string line;
    while (std::getline(f, line) && (int)result.size() < limit)
        if (!line.empty()) result.push_back(line);
    return result;
}
