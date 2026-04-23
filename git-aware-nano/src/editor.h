#pragma once
#include "buffer.h"
#include "filepanel.h"
#include "database.h"
#include "grep.h"
#include "markdown_preview.h"
#include "git/git_repo.h"
#include "git/git_diff.h"
#include "git/git_status.h"
#include "git/git_blame.h"
#include "git/git_log.h"
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <map>

enum class Mode {
    Edit,
    FilePanel,
    Find,
    SaveAs,
    ConfirmQuit,
    RecentFiles,
    FuzzyFinder,
    GrepSearch,
    GitOverlay,   // branch picker / log viewer / blame view
};

enum class GitOverlayKind { BranchPick, LogView, BlameView, DiffView, TreeView };

class Editor {
public:
    Editor();
    ~Editor();

    void openFile(const std::string& path);
    void run();

private:
    int termRows_ = 24, termCols_ = 80;
    static constexpr int kPanelWidth = 24;
    bool panelVisible_ = true;
    bool shouldQuit_   = false;

    std::vector<std::unique_ptr<Buffer>> buffers_;
    int currentBuf_ = 0;
    FilePanel filePanel_;
    Database  db_;
    Mode mode_ = Mode::Edit;

    std::string inputStr_;
    std::string statusMsg_;
    std::string lastSearch_;
    std::vector<std::string> recentFiles_;
    int recentSelected_ = 0;

    // fuzzy finder
    std::vector<std::string> fuzzyAllFiles_;
    std::vector<std::pair<int,std::string>> fuzzyResults_;
    int fuzzySelected_ = 0;
    int fuzzyScroll_   = 0;
    std::string fuzzyRoot_;

    // grep search
    GrepSearch             grep_;
    GrepOptions            grepOpts_;
    std::vector<GrepResult> grepSnapshot_;
    int grepSelected_ = 0;
    int grepScroll_   = 0;

    // git
    git::RepoInfo                    repoInfo_;
    std::vector<git::GutterMark>     gutterMarks_;
    std::map<std::string, char>      fileGitStatus_;  // rel-to-root → 'M','+'etc
    bool                             gitPrefixActive_ = false;

    // split-screen
    bool splitMode_   = false;
    int  splitBuf_    = -1;   // index of right-pane buffer (-1 = off)
    int  splitFocus_  = 0;    // 0 = left (currentBuf_), 1 = right (splitBuf_)
    bool splitSync_   = true; // synchronized scroll
    void enableSplit(int rightBufIdx);
    void disableSplit();
    void renderSplitEdit(std::string& out);
    void renderOnePaneLines(std::string& out, Buffer* buf,
                            int startCol, int cols, int gutterCol);

    // git overlays (branch picker, log, blame, diff)
    GitOverlayKind                   gitOverlayKind_  = GitOverlayKind::BranchPick;
    std::vector<std::string>         gitOverlayLines_; // display lines
    std::vector<std::string>         gitOverlayData_;  // associated data (hashes, branch names)
    int                              gitOverlaySelected_ = 0;
    int                              gitOverlayScroll_   = 0;
    std::string                      gitOverlayTitle_;
    std::string                      gitTreeHash_;   // hash used for TreeView

    // rendering
    void render();
    void renderTabbar();
    void renderPanel();
    void renderEdit();
    void renderRecentOverlay();
    void renderStatus();

    // helpers
    std::string pad(const std::string& s, int width);
    std::string truncLeft(const std::string& s, int width);

    // input
    void handleKey(int key);
    void handleEditKey(int key);
    void handlePanelKey(int key);
    void handleFindKey(int key);
    void handleSaveAsKey(int key);
    void handleConfirmQuitKey(int key);
    void handleRecentFilesKey(int key);
    void handleFuzzyKey(int key);
    void handleGrepKey(int key);

    // buffer management
    void newBuffer();
    void closeCurrentBuffer();
    void doCloseCurrentBuffer();
    void switchBuffer(int idx);
    void nextBuffer();
    void prevBuffer();

    // actions
    void saveCurrent();
    void togglePanel();
    void startFind();
    void startRecentFiles();
    void startFuzzyFinder();
    void updateFuzzyResults();
    void renderFuzzyOverlay(std::string& out);

    void startGrepSearch();
    void renderGrepOverlay(std::string& out);

    // git helpers
    int  gutterWidth() const;
    void refreshGit();
    void refreshGutterForCurrentFile();
    void doGitCommand(char cmd);
    void renderGitOverlay(std::string& out);
    void handleGitOverlayKey(int key);
    std::string gitStatusIndicator() const;

    Buffer* currentBuffer();
    int editRows() const;
    int editCols() const;
    int editStartCol() const;
    void setStatus(const std::string& msg);
    void refreshSize();
};
