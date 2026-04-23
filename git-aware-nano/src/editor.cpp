#include "editor.h"
#include "terminal.h"
#include "highlighters.h"
#include "fuzzy.h"
#include "git/git_process.h"
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <fstream>
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
    refreshGit();
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
    auto [row, col] = db_.getPosition(path);
    buf->gotoLine(row, col);
    buffers_.push_back(std::move(buf));
    currentBuf_ = (int)buffers_.size() - 1;
    db_.addRecentFile(path);
    refreshGit();
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

int Editor::gutterWidth() const  { return repoInfo_.valid ? 2 : 0; }
int Editor::editRows() const     { return std::max(1, termRows_ - 2); }
int Editor::editStartCol() const { return panelVisible_ ? kPanelWidth + 1 : 0; }
int Editor::lineNumWidth() {
    auto* buf = currentBuffer();
    if (!buf) return 0;
    int n = (int)buf->lines().size();
    int w = 1;
    while (n >= 10) { n /= 10; w++; }
    return w + 1;  // digits + space
}
int Editor::editCols()           { return std::max(1, termCols_ - editStartCol() - gutterWidth() - lineNumWidth()); }
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
    // persist cursor position before closing
    auto* buf = currentBuffer();
    if (buf && !buf->filename().empty())
        db_.savePosition(buf->filename(), buf->cursorRow(), buf->cursorCol());
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
    // git info right-aligned in tabbar
    if (repoInfo_.valid) {
        std::string dot = repoInfo_.hasConflict ? "!" :
                          (repoInfo_.dirty || repoInfo_.hasStaged) ? "*" : " ";
        std::string gi = " " + repoInfo_.branch + dot;
        if (repoInfo_.ahead  > 0) gi += "↑" + std::to_string(repoInfo_.ahead);
        if (repoInfo_.behind > 0) gi += "↓" + std::to_string(repoInfo_.behind);
        gi += " ";
        int giLen = (int)gi.size();
        int fill  = termCols_ - x - giLen;
        if (fill > 0) out += std::string(fill, ' ');
        // colour the indicator
        if      (repoInfo_.hasConflict)              out += "\033[0m\033[41m\033[97m";
        else if (repoInfo_.dirty || repoInfo_.hasStaged) out += "\033[0m\033[43m\033[30m";
        else                                          out += "\033[0m\033[42m\033[30m";
        out += gi;
    } else {
        if (x < termCols_) out += std::string(termCols_ - x, ' ');
    }
    out += "\033[0m";

    // --- Markdown preview lines (pre-computed if preview mode active) ---
    std::vector<std::string> previewLines;
    if (auto* pbuf = currentBuffer(); pbuf && pbuf->previewMode()) {
        const auto& allLines = pbuf->lines();
        int scrollR = pbuf->scrollRow();
        int need    = editRows();
        int from    = std::min(scrollR, (int)allLines.size());
        int to      = std::min(from + need + need, (int)allLines.size()); // extra for heading underlines
        std::vector<std::string> slice(allLines.begin() + from, allLines.begin() + to);
        previewLines = mdpreview::render(slice, editCols());
        // pad to at least `need` entries
        while ((int)previewLines.size() < need)
            previewLines.push_back(std::string(editCols(), ' '));
    }

    // --- Split mode: synchronize scroll (passive pane mirrors active) ---
    if (splitMode_ && splitBuf_ >= 0 && splitBuf_ < (int)buffers_.size()) {
        Buffer* active  = (splitFocus_ == 0) ? buffers_[currentBuf_].get()
                                              : buffers_[splitBuf_].get();
        Buffer* passive = (splitFocus_ == 0) ? buffers_[splitBuf_].get()
                                              : buffers_[currentBuf_].get();
        if (active && passive) {
            passive->forceScrollRow(active->scrollRow());
        }
    }

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

                    // git status marker (last char of kPanelWidth)
                    char gitMark = 0;
                    if (repoInfo_.valid && !fileGitStatus_.empty()) {
                        namespace fs2 = std::filesystem;
                        std::string full = (fs2::path(filePanel_.currentPath()) / entry.name).string();
                        // compute relative to repo root
                        std::error_code ec;
                        auto rel = fs2::relative(full, repoInfo_.root, ec);
                        if (!ec) {
                            auto it = fileGitStatus_.find(rel.string());
                            if (it != fileGitStatus_.end()) gitMark = it->second;
                        }
                    }

                    std::string name = entry.name;
                    int maxName = kPanelWidth - (gitMark ? 2 : 0);
                    if ((int)name.size() >= maxName) name = name.substr(0, maxName - 1) + '>';

                    if (isSel) {
                        out += "\033[0m\033[46m\033[30m\033[1m";  // bg cyan, fg black, bold
                        out += pad(name, maxName);
                        if (gitMark) out += std::string(" ") + gitMark;
                        out += "\033[0m";
                    } else if (entry.isDir) {
                        out += "\033[36m";  // fg cyan
                        out += pad(name, maxName);
                        if (gitMark) {
                            const char* gc = (gitMark=='!')?"\033[31m":(gitMark=='+')?"\033[32m":(gitMark=='S')?"\033[34m":"\033[33m";
                            out += std::string(" ") + gc + gitMark + "\033[36m";
                        }
                        out += "\033[97m";
                    } else {
                        out += "\033[0m\033[44m\033[97m";
                        out += pad(name, maxName);
                        if (gitMark) {
                            const char* gc = (gitMark=='!')?"\033[31m":(gitMark=='+')?"\033[32m":(gitMark=='S')?"\033[34m":"\033[33m";
                            out += std::string(" ") + gc + gitMark + "\033[97m";
                        }
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

        // ── Split mode render ──────────────────────────────────────────
        if (splitMode_ && splitBuf_ >= 0 && splitBuf_ < (int)buffers_.size()) {
            Buffer* lb = buffers_[currentBuf_].get();
            Buffer* rb = buffers_[splitBuf_].get();
            int totalCols = editCols();
            int leftCols  = (totalCols - 1) / 2;
            int rightCols = totalCols - leftCols - 1;
            int baseCol   = editStartCol() + 1;
            int sepCol    = baseCol + leftCols;
            int rightCol  = sepCol + 1;

            // row 0 is header; content starts at r=1 → buffer row r-1
            int contentRow = r - 1;

            // compute diff lines for highlight
            auto getLine = [](Buffer* b, int row) -> std::string {
                if (!b || row < 0) return "";
                int idx = b->scrollRow() + row;
                const auto& ls = b->lines();
                return (idx < (int)ls.size()) ? ls[idx] : "";
            };
            std::string lline = getLine(lb, contentRow);
            std::string rline = getLine(rb, contentRow);
            const std::string* ldiff = (lline != rline) ? &rline : nullptr;
            const std::string* rdiff = (lline != rline) ? &lline : nullptr;

            // line number helper
            int lnw = lineNumWidth();
            auto renderLN = [&](Buffer* b, int col) {
                if (!b || lnw <= 0 || contentRow < 0) return;
                out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(col) + "H";
                int li = b->scrollRow() + contentRow;
                if (li < (int)b->lines().size()) {
                    std::string ln = std::to_string(li + 1);
                    while ((int)ln.size() < lnw - 1) ln = " " + ln;
                    out += "\033[90m" + ln + " \033[0m";
                } else {
                    out += std::string(lnw, ' ');
                }
            };

            // row 0 = legend header
            if (r == 0) {
                std::string lname = lb ? lb->displayName() : "?";
                std::string rname = rb ? rb->displayName() : "?";
                // left header: green bg
                out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(baseCol) + "H";
                std::string lhdr = " \033[1mNEW\033[0m\033[42m\033[30m " + lname + " (HEAD)";
                out += "\033[42m\033[30m" + lhdr;
                out += std::string(std::max(0, leftCols - (int)lname.size() - 10), ' ') + "\033[0m";
                // separator
                out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(sepCol) + "H";
                out += "\033[97m│\033[0m";
                // right header: red bg
                out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(rightCol) + "H";
                std::string rhdr = " \033[1mOLD\033[0m\033[41m\033[97m " + rname;
                out += "\033[41m\033[97m" + rhdr;
                out += std::string(std::max(0, rightCols - (int)rname.size() - 6), ' ') + "\033[0m";
                continue;
            }

            // left pane
            renderLN(lb, baseCol);
            out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(baseCol + lnw) + "H";
            renderOnePaneLines(out, lb, baseCol + lnw, leftCols - lnw, r - 1, ldiff, 0);

            // separator — active pane gets bright color
            out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(sepCol) + "H";
            out += (splitFocus_ == 0 ? "\033[97m│\033[0m" : "\033[90m│\033[0m");

            // right pane
            renderLN(rb, rightCol);
            out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(rightCol + lnw) + "H";
            renderOnePaneLines(out, rb, rightCol + lnw, rightCols - lnw, r - 1, rdiff, 1);
            out += "\033[0m";
            continue;
        }

        // Edit area — gutter first
        int editCol = editStartCol() + 1;  // 1-based for ANSI
        if (gutterWidth() > 0) {
            out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(editCol) + "H";
            auto* buf = currentBuffer();
            int lineIdx = buf ? (buf->scrollRow() + r) : -1;
            git::GutterMark mark = (lineIdx >= 0 && lineIdx < (int)gutterMarks_.size())
                ? gutterMarks_[lineIdx] : git::GutterMark::None;
            switch (mark) {
                case git::GutterMark::Added:    out += "\033[32m+\033[0m "; break;
                case git::GutterMark::Modified: out += "\033[33m~\033[0m "; break;
                case git::GutterMark::Deleted:  out += "\033[31m_\033[0m "; break;
                default:                        out += "  ";                  break;
            }
            editCol += gutterWidth();
        }
        // Line number
        int lnw = lineNumWidth();
        if (lnw > 0) {
            out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(editCol) + "H";
            auto* buf = currentBuffer();
            int lineIdx = buf ? (buf->scrollRow() + r) : -1;
            if (buf && lineIdx < (int)buf->lines().size()) {
                std::string ln = std::to_string(lineIdx + 1);
                while ((int)ln.size() < lnw - 1) ln = " " + ln;
                out += "\033[90m" + ln + " \033[0m";
            } else {
                out += std::string(lnw, ' ');
            }
            editCol += lnw;
        }
        out += "\033[" + std::to_string(r + 2) + ";" + std::to_string(editCol) + "H";

        auto* buf = currentBuffer();
        if (buf) {
            const auto& lines = buf->lines();
            int lineIdx = buf->scrollRow() + r;
            if (buf->previewMode()) {
                out += (r < (int)previewLines.size())
                    ? previewLines[r]
                    : std::string(editCols(), ' ');
            } else if (lineIdx < (int)lines.size()) {
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

    // --- Fuzzy finder overlay ---
    if (mode_ == Mode::FuzzyFinder) {
        renderFuzzyOverlay(out);
    }

    // --- Grep overlay ---
    if (mode_ == Mode::GrepSearch) {
        renderGrepOverlay(out);
    }

    // --- Git overlay ---
    if (mode_ == Mode::GitOverlay) {
        renderGitOverlay(out);
    }

    // --- Status bar (last row) ---
    out += "\033[" + std::to_string(termRows_) + ";1H";
    out += "\033[47m\033[30m";  // bg white, fg black

    // Line/col indicator — right-aligned
    {
        Buffer* ibuf = (splitMode_ && splitFocus_ == 1 && splitBuf_ >= 0
                        && splitBuf_ < (int)buffers_.size())
                       ? buffers_[splitBuf_].get() : currentBuffer();
        if (ibuf) {
            std::string pos = " Ln " + std::to_string(ibuf->cursorRow() + 1)
                            + ", Col " + std::to_string(ibuf->cursorCol() + 1) + " ";
            // write right-aligned into last row
            int col = termCols_ - (int)pos.size() + 1;
            if (col > 0) {
                out += "\033[" + std::to_string(termRows_) + ";"
                     + std::to_string(col) + "H";
                out += "\033[34m\033[1m" + pos + "\033[0m\033[47m\033[30m";
                out += "\033[" + std::to_string(termRows_) + ";1H";
            }
        }
    }

    if (!statusMsg_.empty()) {
        out += "\033[1m" + pad(statusMsg_, termCols_) + "\033[0m";
    } else {
        auto hint = [&](const std::string& key, const std::string& desc) {
            return "\033[34m\033[1m" + key + "\033[0m\033[47m\033[30m " + desc + "  ";
        };

        std::string bar;
        switch (mode_) {
            case Mode::Edit: {
                bar += hint("^X", "Exit") + hint("^S", "Save") + hint("^O", "Panel")
                     + hint("^P", "Files") + hint("^F", "Find") + hint("^N", "New")
                     + hint("^R", "Recent") + hint("^A/^E", "Tabs");
                auto* ebuf = currentBuffer();
                if (ebuf) {
                    bar += ebuf->previewMode()
                        ? hint("^V", "Source")
                        : hint("^V", "Preview");
                }
                break;
            }
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
            case Mode::FuzzyFinder:
                bar = "\033[47m\033[30mType to search   Up/Down = select   Enter = open   Esc = cancel";
                break;
            case Mode::GrepSearch:
                bar = "\033[47m\033[30mGrep: type pattern   Tab=case  ^W=word  ^X=regex   Enter=open   Esc=cancel";
                break;
            case Mode::GitOverlay:
                bar = "\033[47m\033[30mUp/Down = select   Enter = confirm   Esc = cancel";
                break;
        }
        // strip ANSI for length calc is complex; just pad with spaces roughly
        out += bar + "\033[K";
    }
    out += "\033[0m";

    // Position cursor
    if (mode_ == Mode::Edit) {
        Buffer* cbuf = (splitMode_ && splitFocus_ == 1 && splitBuf_ >= 0
                        && splitBuf_ < (int)buffers_.size())
                       ? buffers_[splitBuf_].get() : currentBuffer();
        if (cbuf) {
            int cy = cbuf->cursorRow() - cbuf->scrollRow() + 2;
            int cx;
            if (splitMode_) {
                int leftCols = (editCols() - 1) / 2;
                if (splitFocus_ == 0) {
                    cx = cbuf->cursorCol() - cbuf->scrollCol() + editStartCol() + 1;
                } else {
                    // right pane starts after separator
                    cx = cbuf->cursorCol() - cbuf->scrollCol() + editStartCol() + leftCols + 2;
                }
            } else {
                cx = cbuf->cursorCol() - cbuf->scrollCol() + editStartCol() + gutterWidth() + 1;
            }
            out += "\033[" + std::to_string(cy) + ";" + std::to_string(cx) + "H";
            out += "\033[?25h";
        }
    } else if (mode_ == Mode::Find || mode_ == Mode::SaveAs) {
        int prefix = (mode_ == Mode::Find) ? 6 : 9;
        int cx = std::min(prefix + (int)inputStr_.size() + 1, termCols_);
        out += "\033[" + std::to_string(termRows_) + ";" + std::to_string(cx) + "H";
        out += "\033[?25h";
    } else if (mode_ == Mode::FuzzyFinder) {
        int ow = std::min(termCols_ - 4, 72);
        int oh = std::max(5, std::min((int)fuzzyResults_.size() + 3, termRows_ - 4));
        int oy = (termRows_ - oh) / 2;
        int ox = (termCols_ - ow) / 2;
        // cursor after " > " prefix (3 chars) + query
        int cx = ox + 1 + 3 + (int)inputStr_.size();
        int cy = oy + 1;
        out += "\033[" + std::to_string(cy) + ";" + std::to_string(cx) + "H";
        out += "\033[?25h";
    } else if (mode_ == Mode::GrepSearch) {
        int ow = std::min(termCols_ - 4, 78);
        int oy = (termRows_ - std::max(5, std::min(18, termRows_ - 4))) / 2;
        int ox = (termCols_ - ow) / 2;
        // cursor after "/ " prefix (2 chars) + opts indicator (5 chars) + query
        int cx = ox + 1 + 2 + (int)inputStr_.size();
        int cy = oy + 1;
        out += "\033[" + std::to_string(cy) + ";" + std::to_string(cx) + "H";
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
        case Mode::FuzzyFinder: handleFuzzyKey(key);       break;
        case Mode::GrepSearch:  handleGrepKey(key);        break;
        case Mode::GitOverlay:  handleGitOverlayKey(key);  break;
    }
}

void Editor::handleEditKey(int key) {
    if (gitPrefixActive_) {
        gitPrefixActive_ = false;
        statusMsg_.clear();
        if (key >= 'a' && key <= 'z') key = key - 'a' + 'A';  // normalise to upper
        doGitCommand((char)key);
        return;
    }

    // In split mode, route input to the focused pane
    Buffer* buf = (splitMode_ && splitFocus_ == 1 && splitBuf_ >= 0
                   && splitBuf_ < (int)buffers_.size())
                  ? buffers_[splitBuf_].get()
                  : currentBuffer();

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
        case Key::CTRL_R:      startRecentFiles();   break;
        case Key::CTRL_A:      disableSplit(); prevBuffer(); break;
        case Key::CTRL_E:      disableSplit(); nextBuffer(); break;
        case Key::CTRL_P:      startFuzzyFinder();   break;
        case Key::CTRL_T:      startGrepSearch();    break;
        case Key::CTRL_G:      gitPrefixActive_ = true; setStatus("Git: B=blame D=diff L=log H=history T=tree C=branch S=stage R=refresh"); return;
        case Key::CTRL_B:
            if (splitMode_) {
                disableSplit();
                setStatus("Split off");
            } else if (buffers_.size() >= 2) {
                int other = (currentBuf_ == 0) ? 1 : 0;
                enableSplit(other);
                setStatus("Split on — Tab=switch focus  ^B=close");
            } else {
                setStatus("Need 2 open buffers for split");
            }
            return;
        case '\t':
            if (splitMode_) {
                splitFocus_ = 1 - splitFocus_;
                setStatus(splitFocus_ == 0 ? "Focus: left pane" : "Focus: right pane");
            }
            return;
        case Key::CTRL_V:
            if (buf) {
                buf->togglePreview();
                setStatus(buf->previewMode() ? "Preview mode (^V to exit)" : "Source mode");
            }
            return;
        default:
            if (buf && key >= 32 && key < 256)
                buf->insertChar((char)key);
            break;
    }
    if (buf) {
        int cols = splitMode_ ? (editCols() - 1) / 2 : editCols();
        buf->updateScroll(editRows(), cols);
        // keep sync: update passive pane too
        if (splitMode_ && splitBuf_ >= 0 && splitBuf_ < (int)buffers_.size()) {
            Buffer* passive = (splitFocus_ == 0) ? buffers_[splitBuf_].get() : currentBuffer();
            if (passive && passive != buf)
                passive->updateScroll(editRows(), cols);
        }
    }
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
    if (buf->save()) {
        db_.savePosition(buf->filename(), buf->cursorRow(), buf->cursorCol());
        setStatus("Saved: " + buf->filename());
        refreshGit();
    } else {
        setStatus("Error: could not save!");
    }
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

// ── Fuzzy finder ───────────────────────────────────────────────────────────

void Editor::startFuzzyFinder() {
    fuzzyRoot_ = filePanel_.currentPath();
    fuzzyAllFiles_ = collectFiles(fuzzyRoot_);
    inputStr_.clear();
    fuzzySelected_ = 0;
    fuzzyScroll_   = 0;
    updateFuzzyResults();
    mode_ = Mode::FuzzyFinder;
}

void Editor::updateFuzzyResults() {
    fuzzyResults_ = fuzzyFilter(fuzzyAllFiles_, inputStr_);
    fuzzySelected_ = 0;
    fuzzyScroll_   = 0;
}

void Editor::handleFuzzyKey(int key) {
    switch (key) {
        case Key::ESC:
            mode_ = Mode::Edit;
            inputStr_.clear();
            break;
        case Key::ENTER:
            if (!fuzzyResults_.empty()) {
                namespace fs = std::filesystem;
                std::string path =
                    (fs::path(fuzzyRoot_) / fuzzyResults_[fuzzySelected_].second).string();
                mode_ = Mode::Edit;
                inputStr_.clear();
                openFile(path);
            }
            break;
        case Key::ARROW_UP:
            if (fuzzySelected_ > 0) --fuzzySelected_;
            break;
        case Key::ARROW_DOWN:
            if (fuzzySelected_ + 1 < (int)fuzzyResults_.size()) ++fuzzySelected_;
            break;
        case Key::BACKSPACE:
            if (!inputStr_.empty()) {
                inputStr_.pop_back();
                updateFuzzyResults();
            }
            break;
        default:
            if (key >= 32 && key < 256) {
                inputStr_ += (char)key;
                updateFuzzyResults();
            }
            break;
    }
}

static std::string fuzzyEntryLine(const std::string& path, int width, bool selected) {
    size_t slash = path.rfind('/');
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    std::string dir  = (slash == std::string::npos) ? "" : path.substr(0, slash);

    int visLen = 2 + (int)base.size() + (dir.empty() ? 0 : 2 + (int)dir.size());
    int pad    = std::max(0, width - visLen);

    std::string out;
    if (selected) {
        out += "\033[0m\033[46m\033[30m\033[1m  " + base;
        if (!dir.empty()) out += "  " + dir;
        out += std::string(pad, ' ');
        out += "\033[0m";
    } else {
        out += "  \033[1m" + base + "\033[0m";
        if (!dir.empty()) out += "  \033[90m" + dir + "\033[0m";
        out += std::string(pad, ' ');
    }
    return out;
}

// ── Git ────────────────────────────────────────────────────────────────────

void Editor::refreshGutterForCurrentFile() {
    gutterMarks_.clear();
    auto* buf = currentBuffer();
    if (!buf || !repoInfo_.valid || buf->filename().empty()) return;
    gutterMarks_ = git::fileDiff(repoInfo_.root, buf->filename());
}

void Editor::refreshGit() {
    auto* buf = currentBuffer();
    std::string path = buf ? buf->filename() : filePanel_.currentPath();
    if (!path.empty()) {
        if (!repoInfo_.valid)
            repoInfo_ = git::detect(path);
        else
            git::refresh(repoInfo_);
    }
    if (repoInfo_.valid) {
        fileGitStatus_ = git::fileStatuses(repoInfo_.root);
        refreshGutterForCurrentFile();
    }
}

std::string Editor::gitStatusIndicator() const {
    if (!repoInfo_.valid) return "";
    std::string s;
    s += "\033[32m \033[0m\033[47m\033[30m";  // branch icon space
    s += repoInfo_.branch;
    // ● indicator
    if (repoInfo_.hasConflict)
        s += " \033[31m●\033[0m\033[47m\033[30m";
    else if (repoInfo_.hasStaged && repoInfo_.dirty)
        s += " \033[33m●\033[0m\033[47m\033[30m";
    else if (repoInfo_.hasStaged)
        s += " \033[34m●\033[0m\033[47m\033[30m";
    else if (repoInfo_.dirty)
        s += " \033[33m●\033[0m\033[47m\033[30m";
    else
        s += " \033[32m●\033[0m\033[47m\033[30m";
    // ahead/behind
    if (repoInfo_.ahead  > 0) s += " \033[32m↑" + std::to_string(repoInfo_.ahead)  + "\033[0m\033[47m\033[30m";
    if (repoInfo_.behind > 0) s += " \033[31m↓" + std::to_string(repoInfo_.behind) + "\033[0m\033[47m\033[30m";
    return s;
}

void Editor::doGitCommand(char cmd) {
    if (!repoInfo_.valid) { setStatus("Not a git repository"); return; }
    auto* buf = currentBuffer();
    std::string filepath = buf ? buf->filename() : "";

    switch (cmd) {
        case 'B': {  // blame current line
            if (filepath.empty()) { setStatus("No file open"); return; }
            auto entries = git::fileBlame(repoInfo_.root, filepath);
            int row = buf ? buf->cursorRow() : 0;
            if (row < (int)entries.size()) {
                const auto& e = entries[row];
                setStatus(e.hash + " " + e.date + " " + e.author + ": " + e.summary);
            } else {
                setStatus("Blame: no data for this line");
            }
            break;
        }
        case 'D': {  // diff file vs HEAD → read-only buffer
            if (filepath.empty()) { setStatus("No file open"); return; }
            std::string diff = git::fileDiffText(repoInfo_.root, filepath);
            if (diff.empty()) { setStatus("No diff (file unchanged)"); return; }
            auto dbuf = std::make_unique<Buffer>();
            std::istringstream ss(diff);
            std::string line;
            std::vector<std::string> dlines;
            while (std::getline(ss, line)) dlines.push_back(line);
            // Load diff lines into buffer via temporary file trick — use saveAs
            // Simpler: populate buffer directly by building content
            // Buffer has no setLines, so create temp file
            std::string tmpPath = "/tmp/gnano_diff_" + std::to_string(getpid()) + ".diff";
            {
                std::ofstream f(tmpPath);
                for (auto& l : dlines) f << l << '\n';
            }
            dbuf->load(tmpPath);
            dbuf->setHighlighter(makeHighlighter(tmpPath));
            buffers_.push_back(std::move(dbuf));
            currentBuf_ = (int)buffers_.size() - 1;
            setStatus("Diff vs HEAD (read-only)");
            break;
        }
        case 'L': {  // log → overlay
            if (filepath.empty()) { setStatus("No file open"); return; }
            auto entries = git::fileLog(repoInfo_.root, filepath, 50);
            if (entries.empty()) { setStatus("No commits for this file"); return; }
            gitOverlayLines_.clear();
            gitOverlayData_.clear();
            for (auto& e : entries) {
                gitOverlayLines_.push_back(e.hash + "  " + e.relDate + "  " + e.author + "  " + e.subject);
                gitOverlayData_.push_back(e.hashFull);
            }
            gitOverlayKind_    = GitOverlayKind::LogView;
            gitOverlayTitle_   = "Log: " + fs::path(filepath).filename().string();
            gitOverlaySelected_ = 0;
            gitOverlayScroll_   = 0;
            gitTreeHash_        = "";
            mode_ = Mode::GitOverlay;
            break;
        }
        case 'C': {  // branch picker
            auto brs = git::branches(repoInfo_.root);
            if (brs.empty()) { setStatus("No branches found"); return; }
            gitOverlayLines_ = brs;
            gitOverlayData_  = brs;
            gitOverlayKind_    = GitOverlayKind::BranchPick;
            gitOverlayTitle_   = "Switch branch";
            gitOverlaySelected_ = 0;
            gitOverlayScroll_   = 0;
            mode_ = Mode::GitOverlay;
            break;
        }
        case 'S': {  // stage / unstage current file
            if (filepath.empty()) { setStatus("No file open"); return; }
            // Check current staged status
            auto it = fileGitStatus_.find(
                fs::relative(filepath, repoInfo_.root).string());
            bool staged = (it != fileGitStatus_.end() && it->second == 'S');
            std::string err = staged
                ? git::unstage(repoInfo_.root, filepath)
                : git::stage(repoInfo_.root, filepath);
            if (err.empty()) {
                refreshGit();
                setStatus(staged ? "Unstaged" : "Staged");
            } else {
                setStatus("Git error: " + err);
            }
            break;
        }
        case 'T': {  // repo tree at a commit
            // Let user pick a commit first via log overlay, then show tree
            auto entries = git::fileLog(repoInfo_.root, "", 50);
            if (entries.empty()) { setStatus("No commits found"); return; }
            gitOverlayLines_.clear();
            gitOverlayData_.clear();
            for (auto& e : entries) {
                gitOverlayLines_.push_back(e.hash + "  " + e.relDate + "  " + e.author + "  " + e.subject);
                gitOverlayData_.push_back(e.hashFull);
            }
            gitOverlayKind_     = GitOverlayKind::LogView;  // reuse log to pick commit
            gitOverlayTitle_    = "Select commit for tree view";
            gitOverlaySelected_ = 0;
            gitOverlayScroll_   = 0;
            // Signal that after Enter we go to TreeView instead of show
            gitTreeHash_        = "__pick__";
            mode_ = Mode::GitOverlay;
            break;
        }
        case 'R': {  // refresh git status
            refreshGit();
            setStatus("Git refreshed");
            break;
        }
        case 'H': {  // history — git log --stat for whole repo
            std::string out = git::run(repoInfo_.root,
                "log --stat --pretty=format:'commit %h  %ad  %an%n  %s' --date=short");
            if (out.empty()) { setStatus("No history found"); return; }
            std::string tmpPath = "/tmp/gnano_hist_" + std::to_string(getpid()) + ".diff";
            { std::ofstream f(tmpPath); f << out; }
            auto hbuf = std::make_unique<Buffer>();
            hbuf->load(tmpPath);
            buffers_.push_back(std::move(hbuf));
            currentBuf_ = (int)buffers_.size() - 1;
            setStatus("Ctrl+G H: repo history (read-only)");
            break;
        }
        default:
            setStatus("Unknown git command: " + std::string(1, cmd));
    }
}

void Editor::renderGitOverlay(std::string& out) {
    int ow   = std::min(termCols_ - 4, 80);
    int listH = std::min((int)gitOverlayLines_.size(), 15);
    int oh   = std::max(4, std::min(listH + 3, termRows_ - 4));
    listH    = oh - 3;
    int oy   = (termRows_ - oh) / 2;
    int ox   = (termCols_ - ow) / 2;

    if (gitOverlayScroll_ > gitOverlaySelected_) gitOverlayScroll_ = gitOverlaySelected_;
    if (gitOverlayScroll_ + listH <= gitOverlaySelected_)
        gitOverlayScroll_ = gitOverlaySelected_ - listH + 1;

    for (int r = 0; r < oh; ++r) {
        out += "\033[" + std::to_string(oy + r + 1) + ";"
             + std::to_string(ox + 1) + "H";

        if (r == 0) {
            out += "\033[44m\033[97m";
            std::string hdr = " " + gitOverlayTitle_;
            if ((int)hdr.size() < ow) hdr += std::string(ow - hdr.size(), ' ');
            else hdr.resize(ow);
            out += hdr + "\033[0m";
        } else if (r == 1) {
            out += "\033[90m" + std::string(ow, '-') + "\033[0m";
        } else if (r == oh - 1) {
            out += "\033[90m";
            std::string footer = " " + std::to_string(gitOverlayLines_.size()) + " items";
            if ((int)footer.size() < ow) footer += std::string(ow - footer.size(), ' ');
            out += footer + "\033[0m";
        } else {
            int idx = gitOverlayScroll_ + (r - 2);
            if (idx < (int)gitOverlayLines_.size()) {
                bool sel = (idx == gitOverlaySelected_);
                const std::string& raw = gitOverlayLines_[idx];

                // colour prefix for tree view (+/~/-)
                std::string colorPfx, colorReset;
                if (!sel && gitOverlayKind_ == GitOverlayKind::TreeView && raw.size() >= 2) {
                    char s = raw[0];
                    if      (s == '+') { colorPfx = "\033[32m"; colorReset = "\033[0m"; }
                    else if (s == '~') { colorPfx = "\033[33m"; colorReset = "\033[0m"; }
                    else if (s == '-') { colorPfx = "\033[31m"; colorReset = "\033[0m"; }
                    else if (s == 'R') { colorPfx = "\033[34m"; colorReset = "\033[0m"; }
                }

                std::string line = " " + raw;
                if ((int)line.size() > ow) line.resize(ow);
                int fill = ow - (int)line.size();
                if (sel) {
                    out += "\033[0m\033[46m\033[30m\033[1m" + line
                         + std::string(fill, ' ') + "\033[0m";
                } else {
                    out += colorPfx + line + colorReset + std::string(fill, ' ');
                }
            } else {
                out += std::string(ow, ' ');
            }
        }
    }
}

void Editor::handleGitOverlayKey(int key) {
    switch (key) {
        case Key::ESC:
            mode_ = Mode::Edit;
            return;
        case Key::ARROW_UP:
            if (gitOverlaySelected_ > 0) --gitOverlaySelected_;
            return;
        case Key::ARROW_DOWN:
            if (gitOverlaySelected_ + 1 < (int)gitOverlayLines_.size())
                ++gitOverlaySelected_;
            return;
        case Key::ENTER:
            if (gitOverlaySelected_ < (int)gitOverlayData_.size()) {
                if (gitOverlayKind_ == GitOverlayKind::BranchPick) {
                    mode_ = Mode::Edit;
                    std::string err = git::checkout(repoInfo_.root,
                                                    gitOverlayData_[gitOverlaySelected_]);
                    if (err.empty()) { refreshGit(); setStatus("Switched branch"); }
                    else             setStatus("Error: " + err);

                } else if (gitOverlayKind_ == GitOverlayKind::LogView) {
                    std::string hash = gitOverlayData_[gitOverlaySelected_];

                    if (gitTreeHash_ == "__pick__") {
                        // Transition: show tree of selected commit
                        gitTreeHash_ = hash;
                        auto tree = git::commitTree(repoInfo_.root, hash);
                        gitOverlayLines_.clear();
                        gitOverlayData_.clear();
                        for (auto& e : tree) {
                            std::string prefix = "  ";
                            std::string color;
                            switch (e.status) {
                                case 'A': prefix = "+ "; break;
                                case 'M': prefix = "~ "; break;
                                case 'D': prefix = "- "; break;
                                case 'R': prefix = "R "; break;
                            }
                            std::string label = prefix + e.path;
                            if (e.status == 'R') label += "  (was " + e.oldPath + ")";
                            gitOverlayLines_.push_back(label);
                            gitOverlayData_.push_back(e.path);
                        }
                        gitOverlayKind_     = GitOverlayKind::TreeView;
                        gitOverlayTitle_    = "Tree @ " + hash.substr(0, 8);
                        gitOverlaySelected_ = 0;
                        gitOverlayScroll_   = 0;
                        // stay in GitOverlay mode

                    } else {
                        // Normal log → show commit diff
                        mode_ = Mode::Edit;
                        std::string diff = git::run(repoInfo_.root,
                            "show --stat " + hash.substr(0, 8));
                        std::string tmpPath = "/tmp/gnano_show_" + std::to_string(getpid()) + ".diff";
                        { std::ofstream f(tmpPath); f << diff; }
                        auto dbuf = std::make_unique<Buffer>();
                        dbuf->load(tmpPath);
                        dbuf->setHighlighter(makeHighlighter(tmpPath));
                        buffers_.push_back(std::move(dbuf));
                        currentBuf_ = (int)buffers_.size() - 1;
                    }

                } else if (gitOverlayKind_ == GitOverlayKind::TreeView) {
                    // Enter = open old version in split view (left=current, right=old)
                    mode_ = Mode::Edit;
                    std::string filepath = gitOverlayData_[gitOverlaySelected_];
                    std::string content  = git::fileAtCommit(repoInfo_.root, gitTreeHash_, filepath);
                    std::string ext      = fs::path(filepath).extension().string();
                    std::string tmpPath  = "/tmp/gnano_tree_" + std::to_string(getpid()) + ext;
                    { std::ofstream f(tmpPath); f << content; }
                    auto dbuf = std::make_unique<Buffer>();
                    dbuf->load(tmpPath);
                    dbuf->setHighlighter(makeHighlighter(filepath));
                    buffers_.push_back(std::move(dbuf));
                    int newIdx = (int)buffers_.size() - 1;
                    // activate split: current file left, old version right
                    enableSplit(newIdx);
                    setStatus(fs::path(filepath).filename().string()
                              + " @ " + gitTreeHash_.substr(0, 8)
                              + "  |  Tab=switch  ^B=close split");
                }
            }
            return;
        case 'd':
        case 'D': {
            // In TreeView: show diff of selected file in this commit vs parent
            if (gitOverlayKind_ == GitOverlayKind::TreeView
                && gitOverlaySelected_ < (int)gitOverlayData_.size()) {
                mode_ = Mode::Edit;
                std::string filepath = gitOverlayData_[gitOverlaySelected_];
                std::string diff = git::run(repoInfo_.root,
                    "show " + git::shellQuote(gitTreeHash_) + " -- " + git::shellQuote(filepath));
                if (diff.empty()) { setStatus("No diff for this file"); return; }
                std::string tmpPath = "/tmp/gnano_cdiff_" + std::to_string(getpid()) + ".diff";
                { std::ofstream f(tmpPath); f << diff; }
                auto dbuf = std::make_unique<Buffer>();
                dbuf->load(tmpPath);
                dbuf->setHighlighter(makeHighlighter(tmpPath));
                buffers_.push_back(std::move(dbuf));
                currentBuf_ = (int)buffers_.size() - 1;
                setStatus("diff " + fs::path(filepath).filename().string()
                          + " @ " + gitTreeHash_.substr(0, 8));
            }
            return;
        }
        default: break;
    }
}

// ── Grep search ────────────────────────────────────────────────────────────

void Editor::startGrepSearch() {
    inputStr_.clear();
    grepOpts_ = {};
    grepSnapshot_.clear();
    grepSelected_ = 0;
    grepScroll_   = 0;
    grep_.stop();
    mode_ = Mode::GrepSearch;
}

void Editor::handleGrepKey(int key) {
    switch (key) {
        case Key::ESC:
            grep_.stop();
            mode_ = Mode::Edit;
            inputStr_.clear();
            return;

        case Key::ENTER:
            if (!grepSnapshot_.empty()) {
                const auto& r = grepSnapshot_[grepSelected_];
                std::string path = (std::filesystem::path(fuzzyRoot_) / r.file).string();
                grep_.stop();
                mode_ = Mode::Edit;
                inputStr_.clear();
                openFile(path);
                if (auto* buf = currentBuffer()) {
                    buf->gotoLine(r.line - 1, r.matchCol);
                    buf->updateScroll(editRows(), editCols());
                }
            }
            return;

        case Key::ARROW_UP:
            if (grepSelected_ > 0) --grepSelected_;
            return;

        case Key::ARROW_DOWN:
            if (grepSelected_ + 1 < (int)grepSnapshot_.size()) ++grepSelected_;
            return;

        case '\t':  // toggle case-sensitive
            grepOpts_.caseSensitive = !grepOpts_.caseSensitive;
            grep_.start(fuzzyRoot_, inputStr_, grepOpts_);
            grepSelected_ = grepScroll_ = 0;
            return;

        case Key::CTRL_W:  // toggle whole word
            grepOpts_.wholeWord = !grepOpts_.wholeWord;
            grep_.start(fuzzyRoot_, inputStr_, grepOpts_);
            grepSelected_ = grepScroll_ = 0;
            return;

        case 24:  // Ctrl+X — toggle regex (24 = ^X, reuse since we're in grep mode)
            grepOpts_.useRegex = !grepOpts_.useRegex;
            grep_.start(fuzzyRoot_, inputStr_, grepOpts_);
            grepSelected_ = grepScroll_ = 0;
            return;

        case Key::BACKSPACE:
            if (!inputStr_.empty()) {
                inputStr_.pop_back();
                grep_.start(fuzzyRoot_, inputStr_, grepOpts_);
                grepSelected_ = grepScroll_ = 0;
            }
            return;

        default:
            if (key >= 32 && key < 256) {
                inputStr_ += (char)key;
                grep_.start(fuzzyRoot_, inputStr_, grepOpts_);
                grepSelected_ = grepScroll_ = 0;
            }
            return;
    }
}

static std::string grepResultLine(const GrepResult& r, int width, bool selected) {
    // format: "  filename:line  context"
    std::string loc = r.file + ":" + std::to_string(r.line);
    std::string ctx = r.text;
    // trim leading whitespace from context
    size_t trim = ctx.find_first_not_of(" \t");
    if (trim != std::string::npos) ctx = ctx.substr(trim);
    // truncate if needed
    int locW = (int)loc.size() + 2;
    int ctxAvail = std::max(0, width - locW - 2);
    if ((int)ctx.size() > ctxAvail) ctx = ctx.substr(0, ctxAvail);

    std::string out;
    if (selected) {
        out += "\033[0m\033[46m\033[30m\033[1m";
        out += "  " + loc + "  " + ctx;
        int fill = width - locW - (int)ctx.size();
        if (fill > 0) out += std::string(fill, ' ');
        out += "\033[0m";
    } else {
        out += "  \033[90m" + loc + "\033[0m  " + ctx;
        int fill = width - locW - (int)ctx.size();
        if (fill > 0) out += std::string(fill, ' ');
    }
    return out;
}

void Editor::renderGrepOverlay(std::string& out) {
    // Refresh snapshot every render
    grepSnapshot_ = grep_.snapshot();

    int ow   = std::min(termCols_ - 4, 78);
    int listH = std::min((int)grepSnapshot_.size(), 15);
    int oh   = std::max(5, std::min(listH + 3, termRows_ - 4));
    listH    = oh - 3;
    int oy   = (termRows_ - oh) / 2;
    int ox   = (termCols_ - ow) / 2;

    // keep selected visible
    if (grepScroll_ > grepSelected_) grepScroll_ = grepSelected_;
    if (grepScroll_ + listH <= grepSelected_)
        grepScroll_ = grepSelected_ - listH + 1;

    auto optTag = [](bool on, const char* label) -> std::string {
        if (on) return std::string("\033[32m\033[1m[") + label + "]\033[0m\033[44m\033[97m";
        return std::string("\033[90m[") + label + "]\033[0m\033[44m\033[97m";
    };

    for (int r = 0; r < oh; ++r) {
        out += "\033[" + std::to_string(oy + r + 1) + ";"
             + std::to_string(ox + 1) + "H";

        if (r == 0) {
            // header: "/ query [C][W][R]"
            out += "\033[44m\033[97m";
            std::string header = "/ " + inputStr_;
            // pad then options on the right
            std::string opts = " " + optTag(grepOpts_.caseSensitive, "C")
                             + optTag(grepOpts_.wholeWord,     "W")
                             + optTag(grepOpts_.useRegex,      "R") + " ";
            // opts has ANSI so compute raw length separately
            int optsRaw = 9;  // " [C][W][R] " visible chars
            int headerMax = ow - optsRaw;
            if ((int)header.size() > headerMax) header.resize(headerMax);
            int pad = headerMax - (int)header.size();
            if (pad > 0) header += std::string(pad, ' ');
            out += header + opts + "\033[0m";

        } else if (r == 1) {
            out += "\033[90m" + std::string(ow, '-') + "\033[0m";

        } else if (r == oh - 1) {
            out += "\033[90m";
            std::string footer;
            int total = grep_.total();
            if (!grep_.done()) footer = " searching...  " + std::to_string(total) + " hits";
            else               footer = " " + std::to_string(total) + " hits";
            if ((int)footer.size() < ow) footer += std::string(ow - footer.size(), ' ');
            else footer.resize(ow);
            out += footer + "\033[0m";

        } else {
            int idx = grepScroll_ + (r - 2);
            if (idx < (int)grepSnapshot_.size()) {
                out += grepResultLine(grepSnapshot_[idx], ow, idx == grepSelected_);
            } else {
                out += std::string(ow, ' ');
            }
        }
    }
}

void Editor::renderFuzzyOverlay(std::string& out) {
    int ow = std::min(termCols_ - 4, 72);
    // title(1) + sep(1) + results + footer(1), min 5 rows
    int listH = std::min((int)fuzzyResults_.size(), 15);
    int oh    = std::max(5, std::min(listH + 3, termRows_ - 4));
    listH     = oh - 3;
    int oy    = (termRows_ - oh) / 2;
    int ox    = (termCols_ - ow) / 2;

    // Adjust scroll so selected is visible
    if (fuzzyScroll_ > fuzzySelected_) fuzzyScroll_ = fuzzySelected_;
    if (fuzzyScroll_ + listH <= fuzzySelected_)
        fuzzyScroll_ = fuzzySelected_ - listH + 1;

    for (int r = 0; r < oh; ++r) {
        out += "\033[" + std::to_string(oy + r + 1) + ";"
             + std::to_string(ox + 1) + "H";

        if (r == 0) {
            out += "\033[44m\033[97m";
            std::string title = " > " + inputStr_;
            if ((int)title.size() < ow) title += std::string(ow - title.size(), ' ');
            else title.resize(ow);
            out += title + "\033[0m";
        } else if (r == 1) {
            out += "\033[90m" + std::string(ow, '-') + "\033[0m";
        } else if (r == oh - 1) {
            out += "\033[90m";
            std::string footer = " " + std::to_string(fuzzyResults_.size()) + " files";
            if ((int)footer.size() < ow) footer += std::string(ow - footer.size(), ' ');
            out += footer + "\033[0m";
        } else {
            int idx = fuzzyScroll_ + (r - 2);
            if (idx < (int)fuzzyResults_.size()) {
                out += fuzzyEntryLine(fuzzyResults_[idx].second, ow,
                                      idx == fuzzySelected_);
            } else {
                out += std::string(ow, ' ');
            }
        }
    }
}

// ── Split screen ───────────────────────────────────────────────────────────

void Editor::enableSplit(int rightBufIdx) {
    splitBuf_   = rightBufIdx;
    splitMode_  = true;
    splitFocus_ = 0;
    splitSync_  = true;
    // both panes start from line 1 for comparison
    if (auto* lb = currentBuffer())
        lb->gotoLine(0, 0);
    if (rightBufIdx >= 0 && rightBufIdx < (int)buffers_.size())
        buffers_[rightBufIdx]->gotoLine(0, 0);
}

void Editor::disableSplit() {
    splitMode_ = false;
    splitBuf_  = -1;
    splitFocus_ = 0;
}

void Editor::renderOnePaneLines(std::string& out, Buffer* buf,
                                 int startCol1, int cols, int row,
                                 const std::string* diffLine, int side) {
    if (!buf || cols <= 0) {
        if (cols > 0) out += std::string(cols, ' ');
        return;
    }
    const auto& lines = buf->lines();
    int lineIdx = buf->scrollRow() + row;

    // Diff highlight: dark red bg for changed lines, dark green for added
    bool isDiff = false;
    bool isAdded = false;  // line exists here but not in other pane
    if (diffLine) {
        if (lineIdx < (int)lines.size()) {
            isDiff = (lines[lineIdx] != *diffLine);
        } else {
            // this pane has no line here but other does → added in other
            isDiff = !diffLine->empty();
        }
    }
    // empty diffLine means other pane ran out of lines
    if (diffLine && diffLine->empty() && lineIdx < (int)lines.size())
        isAdded = true;

    if (isDiff || isAdded) {
        // left(new): green bg   right(old): red bg
        out += (side == 0) ? "\033[48;5;22m" : "\033[48;5;52m";
    }

    if (lineIdx < (int)lines.size()) {
        const std::string& line = lines[lineIdx];
        auto* hl = buf->highlighter();
        if (hl) {
            auto spans = hl->highlight(line, lineIdx);
            out += renderHighlighted(line, spans, buf->scrollCol(), cols);
        } else {
            int sc = buf->scrollCol();
            std::string visible;
            if (sc < (int)line.size())
                visible = line.substr(sc, cols);
            out += visible;
            int rem = cols - (int)visible.size();
            if (rem > 0) out += std::string(rem, ' ');
        }
    } else {
        out += std::string(cols, ' ');
    }

    if (isDiff || isAdded) out += "\033[0m";
    (void)startCol1;
}
