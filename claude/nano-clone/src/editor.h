#pragma once
#include "buffer.h"
#include "filepanel.h"
#include "database.h"
#include <vector>
#include <string>
#include <memory>

enum class Mode {
    Edit,
    FilePanel,
    Find,
    SaveAs,
    ConfirmQuit,
    RecentFiles,
};

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

    Buffer* currentBuffer();
    int editRows() const;
    int editCols() const;
    int editStartCol() const;
    void setStatus(const std::string& msg);
    void refreshSize();
};
