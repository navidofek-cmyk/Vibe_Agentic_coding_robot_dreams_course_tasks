#include "database.h"
#include "sqlite/sqlite3.h"
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

struct Database::Impl {
    sqlite3* db = nullptr;

    void exec(const char* sql) {
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
};

static void initSchema(sqlite3* db) {
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS recent_files ("
        "  path        TEXT PRIMARY KEY,"
        "  last_opened INTEGER NOT NULL,"
        "  cursor_row  INTEGER NOT NULL DEFAULT 0,"
        "  cursor_col  INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_last_opened"
        "  ON recent_files(last_opened DESC);",
        nullptr, nullptr, nullptr);
}

Database::Database(const std::string& path) : impl_(new Impl) {
    fs::create_directories(fs::path(path).parent_path());
    if (sqlite3_open(path.c_str(), &impl_->db) != SQLITE_OK) {
        sqlite3_close(impl_->db);
        impl_->db = nullptr;
        return;
    }
    sqlite3_busy_timeout(impl_->db, 1000);
    impl_->exec("PRAGMA journal_mode=WAL;");
    impl_->exec("PRAGMA synchronous=NORMAL;");
    initSchema(impl_->db);
}

Database::~Database() {
    if (impl_->db) sqlite3_close(impl_->db);
    delete impl_;
}

void Database::addRecentFile(const std::string& path) {
    if (!impl_->db) return;
    sqlite3_stmt* stmt;
    const char* sql =
        "INSERT INTO recent_files(path, last_opened)"
        "  VALUES(?, ?)"
        "  ON CONFLICT(path) DO UPDATE SET last_opened=excluded.last_opened;";
    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)std::time(nullptr));
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::removeRecentFile(const std::string& path) {
    if (!impl_->db) return;
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(impl_->db,
            "DELETE FROM recent_files WHERE path=?;",
            -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<std::string> Database::getRecentFiles(int limit) const {
    std::vector<std::string> result;
    if (!impl_->db) return result;
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(impl_->db,
            "SELECT path FROM recent_files"
            "  ORDER BY last_opened DESC LIMIT ?;",
            -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* p = (const char*)sqlite3_column_text(stmt, 0);
        if (p) result.emplace_back(p);
    }
    sqlite3_finalize(stmt);
    return result;
}

void Database::savePosition(const std::string& path, int row, int col) {
    if (!impl_->db) return;
    sqlite3_stmt* stmt;
    const char* sql =
        "INSERT INTO recent_files(path, last_opened, cursor_row, cursor_col)"
        "  VALUES(?, ?, ?, ?)"
        "  ON CONFLICT(path) DO UPDATE SET"
        "    cursor_row=excluded.cursor_row,"
        "    cursor_col=excluded.cursor_col;";
    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)std::time(nullptr));
    sqlite3_bind_int(stmt, 3, row);
    sqlite3_bind_int(stmt, 4, col);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::pair<int,int> Database::getPosition(const std::string& path) const {
    if (!impl_->db) return {0, 0};
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(impl_->db,
            "SELECT cursor_row, cursor_col FROM recent_files WHERE path=?;",
            -1, &stmt, nullptr) != SQLITE_OK) return {0, 0};
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    std::pair<int,int> pos = {0, 0};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        pos.first  = sqlite3_column_int(stmt, 0);
        pos.second = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    return pos;
}
