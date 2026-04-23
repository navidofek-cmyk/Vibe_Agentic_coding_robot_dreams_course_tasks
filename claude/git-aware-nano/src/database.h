#pragma once
#include <string>
#include <vector>
#include <utility>

struct RecentEntry {
    std::string path;
    int         cursorRow = 0;
    int         cursorCol = 0;
};

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    void addRecentFile(const std::string& path);
    void removeRecentFile(const std::string& path);
    std::vector<std::string> getRecentFiles(int limit = 20) const;

    // Cursor position persistence
    void savePosition(const std::string& path, int row, int col);
    std::pair<int,int> getPosition(const std::string& path) const;

private:
    struct Impl;
    Impl* impl_;
};
