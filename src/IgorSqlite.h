#pragma once

#include "sqlite/sqlite3.h"

class IgorSqlite3 {
public:
    void createVersionnedDB(std::function<int(int)> upgradeFunc, const char * db, int desiredVersion);
    sqlite3_stmt * safeStmt(const char * stmtStr, ssize_t L = -1) {
        int r;
        sqlite3_stmt * stmt = NULL;
        r = sqlite3_prepare_v2(g_sqlite, stmtStr, L, &stmt, NULL);
        RAssert(r == SQLITE_OK, "Unable to prepare statement; check sqlite logs");
        return stmt;
    }
    template<size_t L>
    sqlite3_stmt * safeStmt(const char(&stmtStr)[L]) {
        return safeStmt(stmtStr, L);
    }
    sqlite3_stmt * safeStmt(const Balau::String & stmtStr) {
        return safeStmt(stmtStr.to_charp(), stmtStr.strlen());
    }
    sqlite3_stmt * safeStmt(const char * stmtStr, std::function<void(sqlite3_stmt *)> binds, ssize_t L = -1) {
        sqlite3_stmt * stmt = safeStmt(stmtStr, L);
        binds(stmt);
        return stmt;
    }
    template<size_t L>
    sqlite3_stmt * safeStmt(const char(&stmtStr)[L], std::function<void(sqlite3_stmt *)> binds) {
        return safeStmt(stmtStr, binds, L);
    }
    sqlite3_stmt * safeStmt(const Balau::String & stmtStr, std::function<void(sqlite3_stmt *)> binds) {
        return safeStmt(stmtStr.to_charp(), binds, stmtStr.strlen());
    }
    void safeWriteStmt(const char * stmtStr, ssize_t L = -1) {
        int r;
        sqlite3_stmt * stmt = safeStmt(stmtStr, L);
        r = sqlite3_step(stmt);
        RAssert(r == SQLITE_DONE, "Unable to run statement");
        r = sqlite3_finalize(stmt);
        RAssert(r == SQLITE_OK, "Unable to finalize statement");
    }
    template<size_t L>
    void safeWriteStmt(const char(&stmtStr)[L]) {
        return safeWriteStmt(stmtStr, L);
    }
    void safeWriteStmt(const Balau::String & stmtStr) {
        return safeWriteStmt(stmtStr.to_charp(), stmtStr.strlen());
    }
    void safeWriteStmt(const char * stmtStr, std::function<void(sqlite3_stmt *)> binds, ssize_t L = -1) {
        int r;
        sqlite3_stmt * stmt = safeStmt(stmtStr, L);
        binds(stmt);
        r = sqlite3_step(stmt);
        RAssert(r == SQLITE_DONE, "Unable to run statement");
        r = sqlite3_finalize(stmt);
        RAssert(r == SQLITE_OK, "Unable to finalize statement");
    }
    template<size_t L>
    void safeWriteStmt(const char(&stmtStr)[L], std::function<void(sqlite3_stmt *)> binds) {
        return safeWriteStmt(stmtStr, binds, L);
    }
    void safeWriteStmt(const Balau::String & stmtStr, std::function<void(sqlite3_stmt *)> binds) {
        return safeWriteStmt(stmtStr.to_charp(), binds, stmtStr.strlen());
    }
    void safeBind(sqlite3_stmt * stmt, int pt, double value) {
        int r = sqlite3_bind_double(stmt, pt, value);
        RAssert(r == SQLITE_OK, "Unable to bind value");
    }
    void safeBind(sqlite3_stmt * stmt, int pt, int value) {
        int r = sqlite3_bind_int(stmt, pt, value);
        RAssert(r == SQLITE_OK, "Unable to bind value");
    }
    void safeBind(sqlite3_stmt * stmt, int pt, sqlite3_int64 value) {
        int r = sqlite3_bind_int64(stmt, pt, value);
        RAssert(r == SQLITE_OK, "Unable to bind value");
    }
    void safeBind(sqlite3_stmt * stmt, int pt) {
        int r = sqlite3_bind_null(stmt, pt);
        RAssert(r == SQLITE_OK, "Unable to bind value");
    }
    template<size_t L>
    void safeBind(sqlite3_stmt * stmt, int pt, const char(&stmtStr)[L]) {
        int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void(*)(void*));
        int r = sqlite3_bind_text(stmt, pt, stmtStr, L, NULL);
        RAssert(r == SQLITE_OK, "Unable to bind value");
    }
    void safeBind(sqlite3_stmt * stmt, int pt, const char * stmtStr) {
        int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void(*)(void*));
        int r = sqlite3_bind_text(stmt, pt, stmtStr, -1, NULL);
        RAssert(r == SQLITE_OK, "Unable to bind value");
    }
    void safeFinalize(sqlite3_stmt *& stmt) {
        int r = sqlite3_finalize(stmt);
        RAssert(r == SQLITE_OK, "Unable to finalize statement");
        stmt = NULL;
    }
protected:
    static sqlite3 * g_sqlite;
};
