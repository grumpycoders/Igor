#include <Exceptions.h>
#include <Printer.h>
#include <AtStartExit.h>

#include "IgorSqlite.h"

void IgorSqlite3::createVersionnedDB(std::function<int(int)> upgradeFunc, int desiredVersion, const char * db) {
    int r;
    int version = 0;
    sqlite3_int64 rowid = 0;
    bool hasToInsert = true;
    Balau::String stmtStr;
    stmtStr.set("CREATE TABLE IF NOT EXISTS %s.version (version);", db);
    safeWriteStmt(stmtStr.to_charp());
    stmtStr.set("SELECT version, ROWID FROM %s.version;", db);
    sqlite3_stmt * stmt = safeStmt(stmtStr);
    while (true) {
        r = safeStep(stmt);
        if (r == SQLITE_DONE)
            break;
        IAssert(version == 0, "Table %s.version contains multiple rows", db);
        version = sqlite3_column_int(stmt, 0);
        rowid = sqlite3_column_int64(stmt, 1);
        hasToInsert = false;
    }
    safeFinalize(stmt);
    IAssert(version <= desiredVersion, "Database's %s.version is too high.", db);
    if (version == desiredVersion)
        return;
    if (hasToInsert) {
        stmtStr.set("INSERT INTO %s.version (version) VALUES (?1);", db);
        safeWriteStmt(stmtStr, [=](sqlite3_stmt * stmt) {
            safeBind(stmt, 1, desiredVersion);
        });
    }
    while (version < desiredVersion)
        version = upgradeFunc(version);
    stmtStr.set("UPDATE %s.version SET version = ?1 WHERE ROWID = ?2;", db);
    safeWriteStmt(stmtStr, [=](sqlite3_stmt * stmt) {
        safeBind(stmt, 1, version);
        safeBind(stmt, 2, rowid);
    });
}

void IgorSqlite3::openDB(const char * name) {
    EAssert(m_sqlite == NULL, "Can't open the database twice");
    int r = sqlite3_open(name, &m_sqlite);
    RAssert(r == SQLITE_OK, "Unable to open %s: %s", name, getError(r).to_charp());
}

void IgorSqlite3::closeDB() {
    if (m_sqlite) {
        int r;
        r = sqlite3_close_v2(m_sqlite);
        RAssert(r == SQLITE_OK, "Unable to close database: %s", getError(r).to_charp());
    }
    m_sqlite = NULL;
}

namespace {

class IgorSqlite3MainInit : public IgorSqlite3, public Balau::AtStart, public Balau::AtExit {
public:
    static const int CONFIG_VERSION = 3;
    IgorSqlite3MainInit() : AtStart(100), AtExit(100) { }
    int upgradeMainDB(int version) {
        switch (version) {
        case 0:
            version = 1;
            safeWriteStmt("CREATE TABLE config (name TEXT PRIMARY KEY, value);");
        case 1:
            version = 2;
            safeWriteStmt("CREATE TABLE users (name TEXT PRIMARY KEY, password TEXT);");
        case 2:
            version = 3;
            safeWriteStmt("ALTER TABLE users ADD COLUMN is_admin;");
            break;
        default:
            Failure("Upgrade case not supported");
        }
        return version;
    }
    static void log(void *, int level, const char * msg) {
        Balau::Printer::log(Balau::M_STATUS, "sqlite(%i): %s", level, msg);
    }
    virtual void doStart() override {
        int r;
        r = sqlite3_config(SQLITE_CONFIG_LOG, log, NULL);
        RAssert(r == SQLITE_OK, "Unable to set config value CONFIG_LOG: %s", getError(r).to_charp());
        r = sqlite3_initialize();
        RAssert(r == SQLITE_OK, "Unable to initialize sqlite3: %s", getError(r).to_charp());
        openDB("Igor.db");
        createVersionnedDB([&](int version) { return upgradeMainDB(version); }, CONFIG_VERSION);
    }
    virtual void doExit() override {
        closeDB();
        sqlite3_shutdown();
    }
};

}

static IgorSqlite3MainInit igorSqlite3;

IgorSqlite3 * const g_igorSqlite3 = &igorSqlite3;
