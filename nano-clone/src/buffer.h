#pragma once
#include "highlight.h"
#include <string>
#include <vector>
#include <memory>

class Buffer {
public:
    explicit Buffer(const std::string& filename = "");

    bool load(const std::string& path);
    bool save();
    bool saveAs(const std::string& path);

    void insertChar(char c);
    void deleteCharBefore();
    void deleteCharAfter();
    void insertNewline();

    void moveCursor(int dr, int dc);
    void moveCursorHome();
    void moveCursorEnd();
    void moveCursorPageUp(int pageSize);
    void moveCursorPageDown(int pageSize);
    void updateScroll(int visibleRows, int visibleCols);

    bool find(const std::string& query, bool fromCursor = true);

    const std::vector<std::string>& lines() const { return lines_; }
    int cursorRow() const { return curRow_; }
    int cursorCol() const { return curCol_; }
    int scrollRow() const { return scrollRow_; }
    int scrollCol() const { return scrollCol_; }

    const std::string& filename() const { return filename_; }
    std::string displayName() const;
    bool isDirty() const { return dirty_; }
    void clearDirty() { dirty_ = false; }

    void setHighlighter(std::unique_ptr<SyntaxHighlighter> hl) { highlighter_ = std::move(hl); }
    SyntaxHighlighter* highlighter() const { return highlighter_.get(); }

private:
    std::vector<std::string> lines_;
    std::string filename_;
    bool dirty_ = false;
    int curRow_ = 0;
    int curCol_ = 0;
    int scrollRow_ = 0;
    int scrollCol_ = 0;
    std::unique_ptr<SyntaxHighlighter> highlighter_;
};
