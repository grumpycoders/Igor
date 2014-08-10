#pragma once

#include "sqlite/sqlite3.h"

#undef max

class IgorSqlite3 {
public:
      IgorSqlite3() { }
      IgorSqlite3(const IgorSqlite3 &) = delete;
      IgorSqlite3(IgorSqlite3 &&) = delete;
      IgorSqlite3 & operator=(const IgorSqlite3 &) = delete;
      ~IgorSqlite3() { closeDB(); }
    void openDB(const char * filename);
    void closeDB();
    void createVersionnedDB(std::function<int(int)> upgradeFunc, int desiredVersion, const char * db = "main");
    sqlite3_stmt * safeStmt(const char * stmtStr, ssize_t L = -1) {
        AAssert(L < std::numeric_limits<int>::max(), "Query too long");
        int r;
        sqlite3_stmt * stmt = NULL;
        r = sqlite3_prepare_v2(m_sqlite, stmtStr, (int) L, &stmt, NULL);
        RAssert(r == SQLITE_OK, "Unable to prepare statement: %s", getError(r).to_charp());
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
        RAssert(r == SQLITE_DONE, "Unable to run statement: %s", getError(r).to_charp());
        r = sqlite3_finalize(stmt);
        RAssert(r == SQLITE_OK, "Unable to finalize statement: %s", getError(r).to_charp());
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
        RAssert(r == SQLITE_DONE, "Unable to run statement: %s", getError(r).to_charp());
        r = sqlite3_finalize(stmt);
        RAssert(r == SQLITE_OK, "Unable to finalize statement: %s", getError(r).to_charp());
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
        RAssert(r == SQLITE_OK, "Unable to bind value: %s", getError(r).to_charp());
    }
    void safeBind(sqlite3_stmt * stmt, int pt, int value) {
        int r = sqlite3_bind_int(stmt, pt, value);
        RAssert(r == SQLITE_OK, "Unable to bind value: %s", getError(r).to_charp());
    }
    void safeBind(sqlite3_stmt * stmt, int pt, sqlite3_int64 value) {
        int r = sqlite3_bind_int64(stmt, pt, value);
        RAssert(r == SQLITE_OK, "Unable to bind value: %s", getError(r).to_charp());
    }
    void safeBind(sqlite3_stmt * stmt, int pt) {
        int r = sqlite3_bind_null(stmt, pt);
        RAssert(r == SQLITE_OK, "Unable to bind value: %s", getError(r).to_charp());
    }
    void safeBind(sqlite3_stmt * stmt, int pt, const char * stmtStr, ssize_t L = -1) {
        AAssert(L < std::numeric_limits<int>::max(), "Query too long");
        int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void(*)(void*));
        int r = sqlite3_bind_text(stmt, pt, stmtStr, (int) L, NULL);
        RAssert(r == SQLITE_OK, "Unable to bind value: %s", getError(r).to_charp());
    }
    template<size_t L>
    void safeBind(sqlite3_stmt * stmt, int pt, const char(&stmtStr)[L]) {
        safeBind(stmt, pt, stmtStr, L);
    }
    void safeBind(sqlite3_stmt * stmt, int pt, const Balau::String & stmtStr) {
        safeBind(stmt, pt, stmtStr.to_charp(), stmtStr.strlen());
    }
    void safeBindBlob(sqlite3_stmt * stmt, int pt, int n) {
        int r = sqlite3_bind_zeroblob(stmt, pt, n);
        RAssert(r == SQLITE_OK, "Unable to bind zeroblob: %s", getError(r).to_charp());
    }
    int safeStep(sqlite3_stmt * stmt) {
        int r = sqlite3_step(stmt);
        RAssert(r == SQLITE_DONE || r == SQLITE_ROW, "Unable to step statement: %s", getError(r).to_charp());
        return r;
    }
    void safeWriteStep(sqlite3_stmt * stmt) {
        int r = sqlite3_step(stmt);
        RAssert(r == SQLITE_DONE, "Unable to step statement: %s", getError(r).to_charp());
    }
    void safeReset(sqlite3_stmt * stmt) {
        int r;
        r = sqlite3_reset(stmt);
        RAssert(r == SQLITE_OK, "Unable to reset statement: %s", getError(r).to_charp());
        r = sqlite3_clear_bindings(stmt);
        RAssert(r == SQLITE_OK, "Unable to clear bindings: %s", getError(r).to_charp());
    }
    void safeFinalize(sqlite3_stmt *& stmt) {
        int r = sqlite3_finalize(stmt);
        RAssert(r == SQLITE_OK, "Unable to finalize statement: %s", getError(r).to_charp());
        stmt = NULL;
    }
    sqlite3_int64 lastInsertRowID() {
        return sqlite3_last_insert_rowid(m_sqlite);
    }
    sqlite3_blob * safeOpenBlob(const char * db, const char * table, const char * column, sqlite3_int64 rowid, bool readOnly = false) {
        sqlite3_blob * h = NULL;
        int r = sqlite3_blob_open(m_sqlite, db, table, column, rowid, readOnly ? 0 : 1, &h);
        RAssert(r == SQLITE_OK, "Unable to open blob %s.%s:%s#%lli: %s", db, table, column, rowid, getError(r).to_charp());
        return h;
    }
    void safeReadBlob(sqlite3_blob * blob, void * z, int n, int offset) {
        int r = sqlite3_blob_read(blob, z, n, offset);
        RAssert(r == SQLITE_OK, "Unable to read from blob: %s", getError(r).to_charp());
    }
    void safeWriteBlob(sqlite3_blob * blob, const void * z, int n, int offset) {
        int r = sqlite3_blob_write(blob, z, n, offset);
        RAssert(r == SQLITE_OK, "Unable to write to blob: %s", getError(r).to_charp());
    }
    void safeCloseBlob(sqlite3_blob *& blob) {
        int r = sqlite3_blob_close(blob);
        RAssert(r == SQLITE_OK, "Unable to close blob: %s", getError(r).to_charp());
        blob = NULL;
    }
    void safeReadWholeBlob(const char * db, const char * table, const char * column, sqlite3_int64 rowid, void * z, int n) {
        sqlite3_blob * blob = safeOpenBlob(db, table, column, rowid, true);
        safeReadBlob(blob, z, n, 0);
        safeCloseBlob(blob);
    }
    void safeWriteWholeBlob(const char * db, const char * table, const char * column, sqlite3_int64 rowid, const void * z, int n) {
        sqlite3_blob * blob = safeOpenBlob(db, table, column, rowid, false);
        safeWriteBlob(blob, z, n, 0);
        safeCloseBlob(blob);
    }
    Balau::String getError(int code) {
        const char * details = sqlite3_errmsg(m_sqlite);
        return Balau::String(sqlite3_errstr(code)) + " - " + details;
    }
private:
    sqlite3 * m_sqlite = NULL;
};

extern IgorSqlite3 * const g_igorSqlite3;
