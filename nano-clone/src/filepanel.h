#pragma once
#include <string>
#include <vector>

struct FileEntry {
    std::string name;
    bool isDir;
};

class FilePanel {
public:
    explicit FilePanel(const std::string& startPath = ".");

    void cd(const std::string& path);
    void cdUp();
    void cdSelected();

    void moveUp();
    void moveDown();
    void pageUp(int pageSize);
    void pageDown(int pageSize);
    void updateScrollForVisible(int visibleRows);

    const std::vector<FileEntry>& entries() const { return entries_; }
    int selected() const { return selected_; }
    int scrollOffset() const { return scrollOffset_; }
    const std::string& currentPath() const { return currentPath_; }

    std::string selectedPath() const;
    bool selectedIsDir() const;

    void refresh();

private:
    std::string currentPath_;
    std::vector<FileEntry> entries_;
    int selected_ = 0;
    int scrollOffset_ = 0;
};
