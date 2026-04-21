#include "editor.h"
#include "terminal.h"
#include "highlighters.h"
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <signal.h>

namespace fs = std::filesystem;

static std::string recentDbPath() {
    const char* home = std::getenv("HOME");
    std::string dir = home ? std::string(home) + "/.local/share/nanoclone" : "/tmp/nanoclone";
    fs::create_directories(dir);
    return dir + "/recent.txt";
}

// ── Lifecycle ──────────────────────────────────────────────────────────────

Editor::Editor()
    : filePanel_(fs::current_path().string()), db_(recentDbPath()) {
    Term::enableRawMode();
    Term::hideCursor();
    Term::clearScreen();
    refreshSize();
    newBuffer();
}

Editor::~Editor() {
    Term::clearScreen();
    Term::moveCursor(0, 0);
    Term::showCursor();
    Term::disableRawMode();
}

void Editor::refreshSize() {
    Term::getSize(termRows_, termCols_);
}

// ── Public API ─────────────────────────────────────────────────────────────

void Editor::openFile(const std::string& path) {
    for (int i = 0; i < (int)buffers_.size(); ++i) {
        if (buffers_[i]->filename() == path) { switchBuffer(i); return; }
    }
    auto buf = std::make_unique<Buffer>();
    if (!buf->load(path)) { setStatus("Error: cannot open " + path); return; }
    buf->setHighlighter(makeHighlighter(path));
    buffers_.push_back(std::move(buf));
    currentBuf_ = (int)buffers_.size() - 1;
    db_.addRecentFile(path);
}

void Editor::run() {
    while (!shouldQuit_) {
        refreshSize();
        render();
        int key = Term::readKey();
        if (key == Key::RESIZE) continue;
        handleKey(key);
    }
}

// ── Helpers ────────────────────────────────────────────────────────────────

Buffer* Editor::currentBuffer() {
    if (buffers_.empty()) return nullptr;
    return buffers_[currentBuf_].get();
}

int Editor::editRows() const     { return std::max(1, termRows_ - 2); }
int Editor::editStartCol() const { return panelVisible_ ? kPanelWidth + 1 : 0; }
int Editor::editCols() const     { return std::max(1, termCols_ - editStartCol()); }
void Editor::setStatus(const std::string& msg) { statusMsg_ = msg; }

std::string Editor::pad(const std::string& s, int width) {
    if (width <= 0) return "";
    if ((int)s.size() >= width) return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

std::string Editor::truncLeft(const std::string& s, int width) {
    if ((int)s.size() <= width) return s;
    return ".." + s.substr(s.size() - (width - 2));
}

// ── Buffer management ──────────────────────────────────────────────────────

void Editor::newBuffer() {
    buffers_.push_back(std::make_unique<Buffer>());
    currentBuf_ = (int)buffers_.size() - 1;
}

void Editor::closeCurrentBuffer() {
    auto* buf = currentBuffer();
    if (buf && buf->isDirty()) { mode_ = Mode::ConfirmQuit; return; }
    doCloseCurrentBuffer();
}

void Editor::doCloseCurrentBuffer() {
    if (buffers_.empty()) return;
    buffers_.erase(buffers_.begin() + currentBuf_);
    if (buffers_.empty()) { shouldQuit_ = true; return; }
    if (currentBuf_ >= (int)buffers_.size()) currentBuf_ = (int)buffers_.size() - 1;
}

void Editor::switchBuffer(int idx) {
    if (idx >= 0 && idx < (int)buffers_.size()) currentBuf_ = idx;
}

void Editor::nextBuffer() {
    if (!buffers_.empty()) currentBuf_ = (currentBuf_ + 1) % (int)buffers_.size();
}

void Editor::prevBuffer() {
    if (!buffers_.empty())
        currentBuf_ = (currentBuf_ - 1 + (int)buffers_.size()) % (int)buffers_.size();
}

// ── Rendering ──────────────────────────────────────────────────────────────

void Editor::render() {
    std::string out;
    out.reserve(termRows_ * termCols_ * 4);

    // Move to top-left, hide cursor during redraw
    out += "\033[H";

    // --- Tabbar (row 0) ---
    out += "\033[44m\033[97m";  // bg blue, fg white
    int x = 0;
    for (int i = 0; i < (int)buffers_.size() && x < termCols_; ++i) {
        std::string label = " " + buffers_[i]->displayName();
        if (buffers_[i]->isDirty()) label += '*';
        label += ' ';
        int avail = termCols_ - x;
        if ((int)label.size() > avail) label.resize(avail);

        if (i == currentBuf_) {
            out += "\033[0m\033[1m\033[47m\033[30m";  // bold, bg white, fg black
            out += label;
            out += "\033[0m\033[44m\033[97m";
        } else if (buffers_[i]->isDirty()) {
            out += "\033[33m" + label + "\033[97m";   // yellow for dirty
        } else {
            out += label;
        }
        x += (int)label.size();
    }
    // fill rest of tabbar
    if (x < termCols_) out += std::string(termCols_ - x, ' ');
    out += "\033[0m";

    // --- Panel + Edit area (rows 1 .. termRows-2) ---
    for (int r = 0; r < editRows(); ++r) {
        // Position at start of this row
        out += "\033[" + std::to_string(r + 2) + ";1H";

        // Panel column
        if (panelVisible_) {
            out += "\033[44m\033[97m";  // bg blue, fg white
            if (r == 0) {
                // header: current path
                out += pad(truncLeft(filePanel_.currentPath(), kPanelWidth), kPanelWidth);
            } else {
                const auto& entries = filePanel_.entries();
                int idx = filePanel_.scrollOffset() + (r - 1);
                if (idx < (int)entries.size()) {
                    const auto& entry = entries[idx];
                    bool isSel = (idx == filePanel_.selected()) && (mode_ == Mode::FilePanel);
                    std::string name = entry.name;
                    if ((int)name.size() >= kPanelWidth) name = name.substr(0, kPanelWidth - 1) + '>';

                    if (isSel) {
                        out += "\033[0m\033[46m\033[30m\033[1m";  // bg cyan, fg black, bold
                        out += pad(name, kPanelWidth);
                        out += "\033[0m";
                    } else if (entry.isDir) {
                        out += "\033[36m";  // fg cyan
                        out += pad(name, kPanelWidth);
                        out += "\033[97m";
                    } else {
                        out += "\033[0m\033[44m\033[97m";
                        out += pad(name, kPanelWidth);
                    }
                } else {
                    out += std::string(kPanelWidth, ' ');
                }
            }
            out += "\033[0m";
            // Separator
            out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(kPanelWidth + 1) + "H";
            out += "\033[90m\342\224\202\033[0m";  // │ in dark gray
        }

        // Edit area
        int editCol = editStartCol() + 1;  // 1-based for ANSI
        out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(editCol) + "H";

        auto* buf = currentBuffer();
        if (buf) {
            const auto& lines = buf->lines();
            int lineIdx = buf->scrollRow() + r;
            if (lineIdx < (int)lines.size()) {
                const std::string& line = lines[lineIdx];
                auto* hl = buf->highlighter();
                if (hl) {
                    auto spans = hl->highlight(line, lineIdx);
                    out += renderHighlighted(line, spans, buf->scrollCol(), editCols());
                } else {
                    int sc = buf->scrollCol();
                    std::string visible;
                    if (sc < (int)line.size())
                        visible = line.substr(sc, editCols());
                    out += visible;
                    int rem = editCols() - (int)visible.size();
                    if (rem > 0) out += std::string(rem, ' ');
                }
            } else {
                out += std::string(editCols(), ' ');
            }
        } else {
            if (r == 0)
                out += pad("  ^N New   ^O Panel   ^R Recent files", editCols());
            else
                out += std::string(editCols(), ' ');
        }
    }

    // --- Recent files overlay ---
    if (mode_ == Mode::RecentFiles && !recentFiles_.empty()) {
        int ow = std::min(termCols_ - 4, 62);
        int oh = std::min((int)recentFiles_.size() + 2, termRows_ - 4);
        int oy = (termRows_ - oh) / 2;
        int ox = (termCols_ - ow) / 2;

        for (int r = 0; r < oh; ++r) {
            out += "\033[" + std::to_string(oy + r + 1) + ";" + std::to_string(ox + 1) + "H";
            out += "\033[43m\033[30m";  // bg yellow, fg black
            if (r == 0) {
                out += pad(" Recent Files", ow);
            } else if (r == oh - 1) {
                out += std::string(ow, ' ');
            } else {
                int idx = r - 1;
                bool sel = (idx == recentSelected_);
                std::string path = recentFiles_[idx];
                if ((int)path.size() > ow - 4) path = truncLeft(path, ow - 4);
                std::string row = "  " + path;
                if (sel) {
                    out += "\033[0m\033[47m\033[30m\033[1m";
                    out += pad(row, ow);
                    out += "\033[0m\033[43m\033[30m";
                } else {
                    out += pad(row, ow);
                }
            }
            out += "\033[0m";
        }
    }

    // --- Status bar (last row) ---
    out += "\033[" + std::to_string(termRows_) + ";1H";
    out += "\033[47m\033[30m";  // bg white, fg black

    if (!statusMsg_.empty()) {
        out += "\033[1m" + pad(statusMsg_, termCols_) + "\033[0m";
    } else {
        auto hint = [&](const std::string& key, const std::string& desc) {
            return "\033[34m\033[1m" + key + "\033[0m\033[47m\033[30m " + desc + "  ";
        };

        std::string bar;
        switch (mode_) {
            case Mode::Edit:
                bar += hint("^X", "Exit") + hint("^S", "Save") + hint("^O", "Panel")
                     + hint("^F", "Find") + hint("^N", "New")  + hint("^R", "Recent")
                     + hint("^A/^T", "Tabs");
                break;
            case Mode::FilePanel:
                bar += hint("Enter", "Open") + hint("Esc", "Back");
                break;
            case Mode::Find:
                bar = "\033[47m\033[30mFind: " + inputStr_ + "_";
                break;
            case Mode::SaveAs:
                bar = "\033[47m\033[30mSave as: " + inputStr_ + "_";
                break;
            case Mode::ConfirmQuit:
                bar = "\033[47m\033[30m\033[1mUnsaved changes \033[0m\033[47m\033[30m"
                      "— (S)ave  (D)iscard  (C)ancel";
                break;
            case Mode::RecentFiles:
                bar = "\033[47m\033[30mUp/Down = select   Enter = open   Esc = cancel";
                break;
        }
        // strip ANSI for length calc is complex; just pad with spaces roughly
        out += bar + "\033[K";
    }
    out += "\033[0m";

    // Position cursor
    if (mode_ == Mode::Edit) {
        if (auto* buf = currentBuffer()) {
            int cy = buf->cursorRow() - buf->scrollRow() + 2;  // +2: tabbar offset (1-based)
            int cx = buf->cursorCol() - buf->scrollCol() + editStartCol() + 1;
            out += "\033[" + std::to_string(cy) + ";" + std::to_string(cx) + "H";
            out += "\033[?25h";  // show cursor
        }
    } else if (mode_ == Mode::Find || mode_ == Mode::SaveAs) {
        int prefix = (mode_ == Mode::Find) ? 6 : 9;
        int cx = std::min(prefix + (int)inputStr_.size() + 1, termCols_);
        out += "\033[" + std::to_string(termRows_) + ";" + std::to_string(cx) + "H";
        out += "\033[?25h";
    } else {
        out += "\033[?25l";  // hide cursor
    }

    if (write(STDOUT_FILENO, out.c_str(), out.size()) < 0) {}
}

// ── Input dispatch ─────────────────────────────────────────────────────────

void Editor::handleKey(int key) {
    statusMsg_.clear();
    switch (mode_) {
        case Mode::Edit:        handleEditKey(key);        break;
        case Mode::FilePanel:   handlePanelKey(key);       break;
        case Mode::Find:        handleFindKey(key);        break;
        case Mode::SaveAs:      handleSaveAsKey(key);      break;
        case Mode::ConfirmQuit: handleConfirmQuitKey(key); break;
        case Mode::RecentFiles: handleRecentFilesKey(key); break;
    }
}

void Editor::handleEditKey(int key) {
    auto* buf = currentBuffer();
    switch (key) {
        case Key::ARROW_UP:    if (buf) buf->moveCursor(-1, 0); break;
        case Key::ARROW_DOWN:  if (buf) buf->moveCursor( 1, 0); break;
        case Key::ARROW_LEFT:  if (buf) buf->moveCursor(0, -1); break;
        case Key::ARROW_RIGHT: if (buf) buf->moveCursor(0,  1); break;
        case Key::HOME:        if (buf) buf->moveCursorHome();  break;
        case Key::END:         if (buf) buf->moveCursorEnd();   break;
        case Key::PAGE_UP:     if (buf) buf->moveCursorPageUp(editRows());   break;
        case Key::PAGE_DOWN:   if (buf) buf->moveCursorPageDown(editRows()); break;
        case Key::BACKSPACE:   if (buf) buf->deleteCharBefore(); break;
        case Key::DEL:         if (buf) buf->deleteCharAfter();  break;
        case Key::ENTER:       if (buf) buf->insertNewline();    break;
        case Key::CTRL_S:      saveCurrent();       break;
        case Key::CTRL_X:      closeCurrentBuffer(); break;
        case Key::CTRL_O:      togglePanel();       break;
        case Key::CTRL_N:      newBuffer();         break;
        case Key::CTRL_F:      startFind();         break;
        case Key::CTRL_R:      startRecentFiles();  break;
        case Key::CTRL_A:      prevBuffer();        break;
        case Key::CTRL_T:      nextBuffer();        break;
        default:
            if (buf && key >= 32 && key < 256)
                buf->insertChar((char)key);
            break;
    }
    if (buf) buf->updateScroll(editRows(), editCols());
}

void Editor::handlePanelKey(int key) {
    int panelRows = editRows() - 1;
    switch (key) {
        case Key::ARROW_UP:   filePanel_.moveUp();            break;
        case Key::ARROW_DOWN: filePanel_.moveDown();          break;
        case Key::PAGE_UP:    filePanel_.pageUp(panelRows);   break;
        case Key::PAGE_DOWN:  filePanel_.pageDown(panelRows); break;
        case Key::ENTER:
            if (filePanel_.selectedIsDir()) {
                filePanel_.cdSelected();
            } else {
                std::string path = filePanel_.selectedPath();
                mode_ = Mode::Edit;
                openFile(path);
            }
            break;
        case Key::ESC:
            mode_ = Mode::Edit;
            break;
        default: break;
    }
    filePanel_.updateScrollForVisible(panelRows);
}

void Editor::handleFindKey(int key) {
    switch (key) {
        case Key::ESC:
            mode_ = Mode::Edit; inputStr_.clear(); break;
        case Key::ENTER: {
            lastSearch_ = inputStr_; inputStr_.clear(); mode_ = Mode::Edit;
            if (auto* buf = currentBuffer()) {
                if (buf->find(lastSearch_, true))
                    buf->updateScroll(editRows(), editCols());
                else
                    setStatus("Not found: " + lastSearch_);
            }
            break;
        }
        case Key::BACKSPACE:
            if (!inputStr_.empty()) { inputStr_.pop_back(); } break;
        default:
            if (key >= 32 && key < 256) { inputStr_ += (char)key; } break;
    }
}

void Editor::handleSaveAsKey(int key) {
    switch (key) {
        case Key::ESC:
            mode_ = Mode::Edit; inputStr_.clear(); break;
        case Key::ENTER:
            if (!inputStr_.empty()) {
                if (auto* buf = currentBuffer()) {
                    if (buf->saveAs(inputStr_)) {
                        db_.addRecentFile(inputStr_);
                        setStatus("Saved: " + inputStr_);
                    } else {
                        setStatus("Error saving: " + inputStr_);
                    }
                }
            }
            inputStr_.clear(); mode_ = Mode::Edit; break;
        case Key::BACKSPACE:
            if (!inputStr_.empty()) { inputStr_.pop_back(); } break;
        default:
            if (key >= 32 && key < 256) { inputStr_ += (char)key; } break;
    }
}

void Editor::handleConfirmQuitKey(int key) {
    switch (key) {
        case 's': case 'S': {
            auto* buf = currentBuffer();
            if (buf && buf->filename().empty()) {
                mode_ = Mode::SaveAs; inputStr_.clear(); return;
            }
            if (buf) buf->save();
            doCloseCurrentBuffer(); mode_ = Mode::Edit; break;
        }
        case 'd': case 'D':
            if (auto* buf = currentBuffer()) buf->clearDirty();
            doCloseCurrentBuffer(); mode_ = Mode::Edit; break;
        case Key::ESC: case 'c': case 'C':
            mode_ = Mode::Edit; break;
    }
}

void Editor::handleRecentFilesKey(int key) {
    switch (key) {
        case Key::ARROW_UP:
            if (recentSelected_ > 0) { --recentSelected_; } break;
        case Key::ARROW_DOWN:
            if (recentSelected_ + 1 < (int)recentFiles_.size()) { ++recentSelected_; } break;
        case Key::ENTER:
            if (!recentFiles_.empty()) {
                mode_ = Mode::Edit;
                openFile(recentFiles_[recentSelected_]);
            }
            break;
        case Key::ESC:
            mode_ = Mode::Edit; break;
    }
}

// ── Actions ────────────────────────────────────────────────────────────────

void Editor::saveCurrent() {
    auto* buf = currentBuffer();
    if (!buf) return;
    if (buf->filename().empty()) { mode_ = Mode::SaveAs; inputStr_.clear(); return; }
    if (buf->save()) setStatus("Saved: " + buf->filename());
    else             setStatus("Error: could not save!");
}

void Editor::togglePanel() {
    if (mode_ == Mode::FilePanel) { mode_ = Mode::Edit; return; }
    panelVisible_ = true;
    mode_ = Mode::FilePanel;
}

void Editor::startFind() {
    mode_ = Mode::Find;
    inputStr_ = lastSearch_;
}

void Editor::startRecentFiles() {
    recentFiles_ = db_.getRecentFiles(20);
    recentSelected_ = 0;
    if (recentFiles_.empty()) { setStatus("No recent files."); return; }
    mode_ = Mode::RecentFiles;
}
