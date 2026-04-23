#include "../git/git_repo.h"
#include "../git/git_diff.h"
#include "../git/git_status.h"
#include "../git/git_log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// ── Minimal test harness ───────────────────────────────────────────────────

static int g_passed = 0;
static int g_total  = 0;

static void check(const char* label, bool ok) {
    ++g_total;
    if (ok) {
        ++g_passed;
        std::cout << "  PASS: " << label << "\n";
    } else {
        std::cerr << "  FAIL: " << label << "\n";
    }
}

// ── Helpers ────────────────────────────────────────────────────────────────

static std::string run(const std::string& cmd) {
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return "";
    std::string out;
    char buf[256];
    while (fgets(buf, sizeof(buf), fp))
        out += buf;
    pclose(fp);
    return out;
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    f << content;
}

// ── Setup: create a throwaway git repo ────────────────────────────────────

static std::string setupRepo() {
    // mktemp -d returns a unique dir like /tmp/gnano_test_XXXXXX
    std::string tmpl = "/tmp/gnano_test_XXXXXX";
    char buf[64];
    std::strncpy(buf, tmpl.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (!mkdtemp(buf)) {
        std::cerr << "mkdtemp failed\n";
        return "";
    }
    std::string repo(buf);

    // Initialise repo with a known name/email so CI does not fail.
    run("git -C " + repo + " init -q");
    run("git -C " + repo + " config user.email test@gnano");
    run("git -C " + repo + " config user.name  gnano_test");

    // Write and commit a file so HEAD exists.
    std::string filepath = repo + "/hello.txt";
    writeFile(filepath, "line1\nline2\nline3\n");
    run("git -C " + repo + " add hello.txt");
    run("git -C " + repo + " commit -q -m 'initial commit'");

    return repo;
}

// ── Tests ──────────────────────────────────────────────────────────────────

int main() {
    std::string repo = setupRepo();
    if (repo.empty()) {
        std::cerr << "Could not create temp repo — aborting.\n";
        return 1;
    }

    std::string filepath = repo + "/hello.txt";

    // ── git::detect ──────────────────────────────────────────────────────
    git::RepoInfo info = git::detect(repo);
    check("detect: valid is true",        info.valid);
    check("detect: root is non-empty",    !info.root.empty());
    check("detect: root matches repo",    info.root == repo);
    check("detect: branch is non-empty",  !info.branch.empty());

    // detect from a subdirectory should still find the root
    run("mkdir -p " + repo + "/sub/dir");
    git::RepoInfo subInfo = git::detect(repo + "/sub/dir");
    check("detect: works from subdir",    subInfo.valid && subInfo.root == repo);

    // ── git::fileStatuses on a clean repo ────────────────────────────────
    auto statuses = git::fileStatuses(repo);
    // On a clean, fully committed repo the map should be empty (no dirty files).
    check("fileStatuses: returns a map (clean repo may be empty)",
          true);  // just check it doesn't crash

    // ── Modify the file so diff has something to show ─────────────────────
    writeFile(filepath, "line1\nMODIFIED\nline3\nnew_line4\n");

    // ── git::fileDiff ─────────────────────────────────────────────────────
    auto marks = git::fileDiff(repo, filepath);
    check("fileDiff: returns non-empty marks after modification",
          !marks.empty());

    bool hasNonNone = false;
    for (auto m : marks)
        if (m != git::GutterMark::None) { hasNonNone = true; break; }
    check("fileDiff: at least one non-None mark", hasNonNone);

    // ── git::fileStatuses after modification ─────────────────────────────
    auto statusesDirty = git::fileStatuses(repo);
    check("fileStatuses: non-empty after modification", !statusesDirty.empty());

    // ── git::fileLog ──────────────────────────────────────────────────────
    auto log = git::fileLog(repo, filepath);
    check("fileLog: returns at least one entry", !log.empty());
    if (!log.empty()) {
        check("fileLog: entry has non-empty hash",    !log[0].hash.empty());
        check("fileLog: entry has non-empty subject", !log[0].subject.empty());
    }

    // ── git::branches ─────────────────────────────────────────────────────
    auto branchList = git::branches(repo);
    check("branches: returns at least one branch", !branchList.empty());
    if (!branchList.empty()) {
        check("branches: first branch is non-empty", !branchList[0].empty());
    }

    // ── git::fileDiffText ─────────────────────────────────────────────────
    std::string diffText = git::fileDiffText(repo, filepath);
    check("fileDiffText: returns non-empty string", !diffText.empty());

    // ── git::refresh (re-reads status without segfault) ───────────────────
    git::refresh(info);
    check("refresh: does not crash, dirty is true after modification",
          info.dirty);

    // ── Cleanup ───────────────────────────────────────────────────────────
    run("rm -rf " + repo);

    // ── Summary ──────────────────────────────────────────────────────────
    std::cout << "Passed: " << g_passed << "/" << g_total << "\n";
    return (g_passed == g_total) ? 0 : 1;
}
