#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

struct GrepResult {
    std::string file;
    int         line;       // 1-based
    std::string text;
    int         matchCol;
    int         matchLen;
};

struct GrepOptions {
    bool caseSensitive = false;
    bool wholeWord     = false;
    bool useRegex      = false;
};

class GrepSearch {
public:
    ~GrepSearch() { stop(); }

    void start(const std::string& root,
               const std::string& pattern,
               GrepOptions        opts);
    void stop();

    bool                    done()     const { return done_.load(); }
    std::vector<GrepResult> snapshot() const;
    int                     total()    const { return total_.load(); }

private:
    mutable std::mutex      mu_;
    std::thread             thread_;
    std::atomic<bool>       stop_{ false };
    std::atomic<bool>       done_{ true  };
    std::atomic<int>        total_{ 0 };
    std::vector<GrepResult> results_;

    void run(std::string root, std::string pattern, GrepOptions opts);
};
