#pragma once
#include <string>
#include <vector>

class Database {
public:
    explicit Database(const std::string& path);

    void addRecentFile(const std::string& path);
    void removeRecentFile(const std::string& path);
    std::vector<std::string> getRecentFiles(int limit = 20) const;

private:
    std::string path_;
};
