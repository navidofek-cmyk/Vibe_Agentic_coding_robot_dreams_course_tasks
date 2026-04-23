#include "buffer.h"
#include <fstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

Buffer::Buffer(const std::string& filename) : filename_(filename) {
    lines_.emplace_back("");
    if (!filename.empty()) load(filename);
}

bool Buffer::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    lines_.clear();
    std::string line;
    while (std::getline(f, line)) lines_.push_back(line);
    if (lines_.empty()) lines_.emplace_back("");
    filename_ = path;
    dirty_ = false;
    curRow_ = curCol_ = scrollRow_ = scrollCol_ = 0;
    return true;
}

bool Buffer::save() {
    if (filename_.empty()) return false;
    std::ofstream f(filename_);
    if (!f.is_open()) return false;
    for (size_t i = 0; i < lines_.size(); ++i) {
        f << lines_[i];
        if (i + 1 < lines_.size()) f << '\n';
    }
    dirty_ = false;
    return true;
}

bool Buffer::saveAs(const std::string& path) {
    filename_ = path;
    return save();
}

void Buffer::insertChar(char c) {
    lines_[curRow_].insert(curCol_, 1, c);
    ++curCol_;
    dirty_ = true;
}

void Buffer::deleteCharBefore() {
    if (curCol_ > 0) {
        lines_[curRow_].erase(curCol_ - 1, 1);
        --curCol_;
    } else if (curRow_ > 0) {
        std::string tail = lines_[curRow_];
        lines_.erase(lines_.begin() + curRow_);
        --curRow_;
        curCol_ = (int)lines_[curRow_].size();
        lines_[curRow_] += tail;
    }
    dirty_ = true;
}

void Buffer::deleteCharAfter() {
    if (curCol_ < (int)lines_[curRow_].size()) {
        lines_[curRow_].erase(curCol_, 1);
    } else if (curRow_ + 1 < (int)lines_.size()) {
        lines_[curRow_] += lines_[curRow_ + 1];
        lines_.erase(lines_.begin() + curRow_ + 1);
    }
    dirty_ = true;
}

void Buffer::insertNewline() {
    std::string tail = lines_[curRow_].substr(curCol_);
    lines_[curRow_].resize(curCol_);
    lines_.insert(lines_.begin() + curRow_ + 1, tail);
    ++curRow_;
    curCol_ = 0;
    dirty_ = true;
}

void Buffer::moveCursor(int dr, int dc) {
    if (dr != 0) {
        curRow_ = std::clamp(curRow_ + dr, 0, (int)lines_.size() - 1);
        curCol_ = std::min(curCol_, (int)lines_[curRow_].size());
    }
    if (dc != 0) {
        curCol_ += dc;
        if (curCol_ < 0 && curRow_ > 0) {
            --curRow_;
            curCol_ = (int)lines_[curRow_].size();
        } else if (curCol_ > (int)lines_[curRow_].size() && curRow_ + 1 < (int)lines_.size()) {
            ++curRow_;
            curCol_ = 0;
        } else {
            curCol_ = std::clamp(curCol_, 0, (int)lines_[curRow_].size());
        }
    }
}

void Buffer::moveCursorHome() { curCol_ = 0; }
void Buffer::moveCursorEnd()  { curCol_ = (int)lines_[curRow_].size(); }

void Buffer::moveCursorPageUp(int pageSize) {
    curRow_ = std::max(0, curRow_ - pageSize);
    curCol_ = std::min(curCol_, (int)lines_[curRow_].size());
}

void Buffer::moveCursorPageDown(int pageSize) {
    curRow_ = std::min((int)lines_.size() - 1, curRow_ + pageSize);
    curCol_ = std::min(curCol_, (int)lines_[curRow_].size());
}

void Buffer::updateScroll(int visibleRows, int visibleCols) {
    if (curRow_ < scrollRow_) scrollRow_ = curRow_;
    if (curRow_ >= scrollRow_ + visibleRows) scrollRow_ = curRow_ - visibleRows + 1;
    if (curCol_ < scrollCol_) scrollCol_ = curCol_;
    if (curCol_ >= scrollCol_ + visibleCols) scrollCol_ = curCol_ - visibleCols + 1;
}

std::string Buffer::displayName() const {
    if (filename_.empty()) return "[new]";
    return fs::path(filename_).filename().string();
}

void Buffer::gotoLine(int row, int col) {
    curRow_ = std::clamp(row, 0, (int)lines_.size() - 1);
    curCol_ = std::clamp(col, 0, (int)lines_[curRow_].size());
}

bool Buffer::find(const std::string& query, bool fromCursor) {
    if (query.empty()) return false;
    int startRow = fromCursor ? curRow_ : 0;
    int startCol = fromCursor ? curCol_ + 1 : 0;

    for (int r = startRow; r < (int)lines_.size(); ++r) {
        size_t pos = lines_[r].find(query, r == startRow ? startCol : 0);
        if (pos != std::string::npos) {
            curRow_ = r;
            curCol_ = (int)pos;
            return true;
        }
    }
    // wrap around
    for (int r = 0; r <= startRow; ++r) {
        size_t pos = lines_[r].find(query);
        if (pos != std::string::npos) {
            curRow_ = r;
            curCol_ = (int)pos;
            return true;
        }
    }
    return false;
}
