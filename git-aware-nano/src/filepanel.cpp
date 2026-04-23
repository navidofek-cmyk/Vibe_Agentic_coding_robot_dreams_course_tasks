#include "filepanel.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

FilePanel::FilePanel(const std::string& startPath) {
    cd(fs::absolute(startPath).string());
}

void FilePanel::cd(const std::string& path) {
    currentPath_ = path;
    selected_ = 0;
    scrollOffset_ = 0;
    refresh();
}

void FilePanel::refresh() {
    entries_.clear();
    entries_.push_back({"[..]", true});

    std::vector<FileEntry> dirs, files;
    try {
        for (const auto& entry : fs::directory_iterator(currentPath_)) {
            FileEntry fe;
            fe.name = entry.path().filename().string();
            fe.isDir = entry.is_directory();
            (fe.isDir ? dirs : files).push_back(fe);
        }
    } catch (...) {}

    auto byName = [](const FileEntry& a, const FileEntry& b) { return a.name < b.name; };
    std::sort(dirs.begin(), dirs.end(), byName);
    std::sort(files.begin(), files.end(), byName);

    for (auto& d : dirs)  entries_.push_back(d);
    for (auto& f : files) entries_.push_back(f);
}

void FilePanel::cdUp() {
    fs::path p(currentPath_);
    if (p.has_parent_path()) cd(p.parent_path().string());
}

void FilePanel::cdSelected() {
    if (entries_.empty()) return;
    if (selected_ == 0) { cdUp(); return; }
    if (entries_[selected_].isDir)
        cd((fs::path(currentPath_) / entries_[selected_].name).string());
}

void FilePanel::moveUp()   { if (selected_ > 0) --selected_; }
void FilePanel::moveDown() { if (selected_ + 1 < (int)entries_.size()) ++selected_; }

void FilePanel::pageUp(int pageSize) {
    selected_ = std::max(0, selected_ - pageSize);
}

void FilePanel::pageDown(int pageSize) {
    selected_ = std::min((int)entries_.size() - 1, selected_ + pageSize);
}

void FilePanel::updateScrollForVisible(int visibleRows) {
    if (selected_ < scrollOffset_) scrollOffset_ = selected_;
    if (selected_ >= scrollOffset_ + visibleRows) scrollOffset_ = selected_ - visibleRows + 1;
}

std::string FilePanel::selectedPath() const {
    if (entries_.empty() || selected_ == 0)
        return fs::path(currentPath_).parent_path().string();
    return (fs::path(currentPath_) / entries_[selected_].name).string();
}

bool FilePanel::selectedIsDir() const {
    if (entries_.empty() || selected_ == 0) return true;
    return entries_[selected_].isDir;
}
