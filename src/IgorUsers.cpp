#include <malloc.h>

#include <Printer.h>
#include <BigInt.h>
#include <AtStartExit.h>
#include <tomcrypt.h>

#include <json/reader.h>
#include <json/writer.h>

#include "IgorUsers.h"
#include "IgorSqlite.h"

using namespace Balau;

std::vector<String> IgorUsers::getUsers() {
    int r;
    sqlite3_stmt * stmt = NULL;

    g_igorSqlite3->safeWriteStmt("BEGIN EXCLUSIVE TRANSACTION;");

    stmt = g_igorSqlite3->safeStmt("SELECT COUNT(name) FROM main.users;");
    r = g_igorSqlite3->safeStep(stmt);
    IAssert(r == SQLITE_ROW, "A count query didn't return anything... ?");
    std::vector<String> rows;
    size_t count = sqlite3_column_int(stmt, 0);
    rows.resize(count);
    g_igorSqlite3->safeFinalize(stmt);
    
    stmt = g_igorSqlite3->safeStmt("SELECT name FROM main.users;");
    for (size_t i = 0; i < count; i++) {
        r = g_igorSqlite3->safeStep(stmt);
        IAssert(r == SQLITE_ROW, "We didn't get enough rows... ?");
        rows[i] = (const char *) sqlite3_column_text(stmt, 0);
    }
    g_igorSqlite3->safeFinalize(stmt);

    g_igorSqlite3->safeWriteStmt("COMMIT TRANSACTION;");

    return rows;
}

std::pair<bool, String> IgorUsers::getV(const String & user) {
    int r;
    bool found = false;
    String v;
    sqlite3_stmt * stmt = NULL;

    stmt = g_igorSqlite3->safeStmt("SELECT password FROM main.users WHERE name=?1;");
    g_igorSqlite3->safeBind(stmt, 1, user);
    r = g_igorSqlite3->safeStep(stmt);
    if (r == SQLITE_ROW) {
        found = true;
        v = (const char *) sqlite3_column_text(stmt, 0);
        r = g_igorSqlite3->safeStep(stmt);
        IAssert(r == SQLITE_DONE, "More than one user with the same name ?!");
    }
    g_igorSqlite3->safeFinalize(stmt);

    return std::pair<bool, String>(found, v);
}

bool IgorUsers::addUser(const String & user, const String & v) {
    bool success = true;

    try {
        g_igorSqlite3->safeWriteStmt("INSERT INTO main.users (name, password) VALUES (?1, ?2);", [&](sqlite3_stmt * stmt) {
            g_igorSqlite3->safeBind(stmt, 1, user);
            g_igorSqlite3->safeBind(stmt, 2, v);
        });
    }
    catch (...) {
        success = false;
    }

    return success;
}

bool IgorUsers::changePassword(const String & user, const String & v) {
    bool success = true;

    try {
        g_igorSqlite3->safeWriteStmt("UPDATE main.users SET password=?1 WHERE name=?2;", [&](sqlite3_stmt * stmt) {
            g_igorSqlite3->safeBind(stmt, 1, v);
            g_igorSqlite3->safeBind(stmt, 2, user);
        });
    }
    catch (...) {
        success = false;
    }

    return success;
}

bool IgorUsers::delUser(const String & user) {
    bool success = true;

    try {
        g_igorSqlite3->safeWriteStmt("DELETE FROM main.users WHERE name=?1;", [&](sqlite3_stmt * stmt) {
            g_igorSqlite3->safeBind(stmt, 1, user);
        });
    }
    catch (...) {
        success = false;
    }

    return success;
}


static size_t Nlen = 0;

class SRPBigNums : public AtStart {
  public:
      SRPBigNums() : AtStart(100) { }
    void doStart() {
        static const char N_str[] =
            "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C"
            "9C256576D674DF7496EA81D3383B4813D692C6E0E0D5D8E250B98BE4"
            "8E495C1D6089DAD15DC7D7B46154D6B6CE8EF4AD69B15D4982559B29"
            "7BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA9A"
            "FD5138FE8376435B9FC61D2FC0EB06E3";
        static BigInt N;
        N.set(N_str, 16);
        m_N = &N;
        Nlen = N.exportUSize();

        static BigInt g;
        g.set(2);
        m_g = &g;

        static BigInt k = SRP::H(N, g)();
        String str = k.toString(16);
        m_k = &k;
    }
    const BigInt & N() const { return *m_N; }
    const BigInt & g() const { return *m_g; }
    const BigInt & k() const { return *m_k; }

  private:
    BigInt * m_N = NULL;
    BigInt * m_g = NULL;
    BigInt * m_k = NULL;
};

static SRPBigNums srpBigNums;

SRP::Hash::Hash(const Hash & h) {
    IAssert(h.m_finalized, "Can't assign a non-finalized hash.");
    m_finalized = true;
    memcpy(m_digest, h.m_digest, DIGEST_SIZE);
}

SRP::Hash::Hash() {
    sha256_init(&m_state);
}

void SRP::Hash::updateString(const char * str) {
    IAssert(!m_finalized, "Can't update a finalized hash.");
    while (*str)
        sha256_process(&m_state, (unsigned char *) str++, 1);
}

void SRP::Hash::update(const unsigned char * data, size_t l) {
    IAssert(!m_finalized, "Can't update a finalized hash.");
    sha256_process(&m_state, data, l);
}

void SRP::Hash::updateBString(const String & str) {
    IAssert(!m_finalized, "Can't update a finalized hash.");
    sha256_process(&m_state, (const unsigned char *)str.to_charp(), str.strlen());
}

void SRP::Hash::updateBigInt(const BigInt & v) {
    uint8_t * data = (uint8_t *) alloca(Nlen);
    size_t vlen = v.exportUSize();
    IAssert(vlen <= Nlen, "Too many digits to hash...");

    memset(data, 0, Nlen);

    v.exportUBin(data + Nlen - vlen);
    update(data, Nlen);
}

void SRP::Hash::updateHash(const Hash & v) {
    updateBigInt(v());
}

void SRP::Hash::final() {
    IAssert(!m_finalized, "Can't finalize a finalized hash.");
    sha256_done(&m_state, m_digest);
    m_finalized = true;
}

BigInt SRP::Hash::toBigInt() const {
    BigInt v;
    IAssert(m_finalized, "Can't export a non-finalized hash.");
    v.importUBin(m_digest, DIGEST_SIZE);
    return v;
}

BigInt SRP::rand(int s) {
    uint8_t * rnd = (uint8_t *) alloca(s);
    unsigned long r = rng_get_bytes(rnd, s, NULL);
    RAssert(r == s, "Unable to generate enough random bytes; got only %i instead of %i.", r, s);
    BigInt ret;
    ret.importUBin(rnd, s);
    return ret % srpBigNums.N();
}

String SRP::generateVerifier(const String & I, const String & p) {
    const BigInt & N = srpBigNums.N();
    const BigInt & g = srpBigNums.g();

    BigInt s = rand(SALT_LEN);
    BigInt x = generateX(s, I, p);
    BigInt v = g.modpow(x, N);

    IAssert(V_LEN >= v.exportUSize(), "Not enough bytes to export password verifier?!");

    Json::Value vstr;
    vstr["data"]["v"] = v.toString(16).to_charp();
    vstr["data"]["s"] = s.toString(16).to_charp();
    vstr["type"] = "SRP6a";

    Json::StyledWriter writer;
    return writer.write(vstr);
}

bool SRP::loadPassword(const Balau::String & password) {
    Json::Reader reader;
    Json::Value values;

    const char * pcharp = password.to_charp();
    ssize_t len = password.strlen();
    ssize_t lenV = V_LEN, lenS = SALT_LEN;

    if (!reader.parse(pcharp, pcharp + len, values))
        return false;
    if (values["type"] != "SRP6a")
        return false;
    v.set(values["data"]["v"].asString(), 16);
    s.set(values["data"]["s"].asString(), 16);
    return true;
}

bool SRP::selfTest() {
    IAssert(srpBigNums.N().isPrime(), "N isn't prime ?!");
    IAssert(sha256_desc.hashsize == Hash::DIGEST_SIZE, "Hash::DIGEST_SIZE needs to be 32 for SHA-256");

    String I = "testUsername";
    String p = "testPassword";

    for (int round = 0; round < 20; round++) {
        String pv = generateVerifier(I, p).to_charp();
        String msg;
        bool success;

        Printer::log(M_DEBUG, "Generated test pv: %s", pv.to_charp());

        SRP client, server;
        client.setUsername(I);
        client.setPassword(p);

        msg = client.clientSendPacketA();
        Printer::log(M_DEBUG, "Generated test packetA: %s", msg.to_charp());
        server.setUsername(I);
        success = server.loadPassword(pv);
        if (!success) return false;

        success = server.serverRecvPacketA(msg);
        if (!success) return false;

        msg = server.serverSendPacketB();
        Printer::log(M_DEBUG, "Generated test packetB: %s", msg.to_charp());
        success = client.clientRecvPacketB(msg);
        if (!success) return false;

        msg = client.clientSendProof();
        Printer::log(M_DEBUG, "Generated test clientProof: %s", msg.to_charp());
        success = server.serverRecvProof(msg);
        if (!success) return false;

        msg = server.serverSendProof();
        Printer::log(M_DEBUG, "Generated test serverProof: %s", msg.to_charp());
        success = client.clientRecvProof(msg);
        if (!success) return false;

        if (client.getSessionKey() != server.getSessionKey()) return false;
    }

    return true;
}

String SRP::clientSendPacketA() {
    AAssert(I != "", "username not set");

    const BigInt & N = srpBigNums.N();
    const BigInt & g = srpBigNums.g();

    // g ^ a
    A = g.modpow(a = rand(), N);

    Json::Value packet;
    packet["clientPacketA"]["I"] = I.to_charp();
    packet["clientPacketA"]["A"] = A.toString(16).to_charp();

    Json::StyledWriter writer;
    return writer.write(packet);
}

bool SRP::serverRecvPacketA(const String & packetStr) {
    Json::Reader reader;
    Json::Value values;
    const char * pcharp = packetStr.to_charp();
    size_t len = packetStr.strlen();

    const BigInt & N = srpBigNums.N();

    if (!reader.parse(pcharp, pcharp + len, values))
        return false;

    I = values["clientPacketA"]["I"].asString();
    A.set(values["clientPacketA"]["A"].asString(), 16);

    if ((A % N) == 0)
        return false;

    return true;
}

String SRP::serverSendPacketB() {
    AAssert(v != 0, "password verifier not set");
    AAssert(s != 0, "password salt not set");
    AAssert(A != 0, "A not set");

    const BigInt & N = srpBigNums.N();
    const BigInt & g = srpBigNums.g();
    const BigInt & k = srpBigNums.k();

    // k * v + g ^ b
    B = k.modmul(v, N).modadd(g.modpow(b = rand(), N), N);

    Json::Value packet;
    packet["serverPacketB"]["s"] = s.toString(16).to_charp();
    packet["serverPacketB"]["B"] = B.toString(16).to_charp();

    u = H(A, B)();

    // (A * v ^ u) ^ b
    S = A.modmul(v.modpow(u, N), N).modpow(b, N);
    K = H(S)();

    Json::StyledWriter writer;
    return writer.write(packet);
}

bool SRP::clientRecvPacketB(const String & packetStr) {
    Json::Reader reader;
    Json::Value values;
    const char * pcharp = packetStr.to_charp();
    size_t len = packetStr.strlen();

    const BigInt & N = srpBigNums.N();
    const BigInt & g = srpBigNums.g();
    const BigInt & k = srpBigNums.k();

    if (!reader.parse(pcharp, pcharp + len, values))
        return false;

    s.set(values["serverPacketB"]["s"].asString(), 16);
    B.set(values["serverPacketB"]["B"].asString(), 16);

    if ((B % N) == 0)
        return false;

    BigInt x = generateX(s, I, p);
    u = H(A, B)();
    // (B - k * g ^ x) ^ (a + u * x)
    S = B.modsub(k.modmul(g.modpow(x, N), N), N).modpow(a.modadd(u.modmul(x, N), N), N);
    K = H(S)();

    return true;
}

BigInt SRP::generateX(const BigInt & s, const String & I, const String & p) {
    return H(s, H(I, ":", p))();
}

String SRP::clientSendProof() {
    const BigInt & N = srpBigNums.N();
    const BigInt & g = srpBigNums.g();

    M = H(H(N)() ^ H(g)(), H(I), s, A, B, K)();

    Json::Value packet;
    packet["clientProof"]["M"] = M.toString(16).to_charp();

    Json::StyledWriter writer;
    return writer.write(packet);
}

bool SRP::serverRecvProof(const String & packetStr) {
    Json::Reader reader;
    Json::Value values;
    const char * pcharp = packetStr.to_charp();
    size_t len = packetStr.strlen();

    const BigInt & N = srpBigNums.N();
    const BigInt & g = srpBigNums.g();
    const BigInt & k = srpBigNums.k();

    if (!reader.parse(pcharp, pcharp + len, values))
        return false;

    M = H(H(N)() ^ H(g)(), H(I), s, A, B, K)();

    BigInt Mc;
    Mc.set(values["clientProof"]["M"].asString(), 16);

    return M == Mc;
}

String SRP::serverSendProof() {
    Json::Value packet;
    packet["serverProof"]["M"] = H(A, M, K)().toString(16).to_charp();

    Json::StyledWriter writer;
    return writer.write(packet);
}

bool SRP::clientRecvProof(const String & packetStr) {
    Json::Reader reader;
    Json::Value values;
    const char * pcharp = packetStr.to_charp();
    size_t len = packetStr.strlen();

    if (!reader.parse(pcharp, pcharp + len, values))
        return false;

    BigInt Ms;
    Ms.set(values["serverProof"]["M"].asString(), 16);

    return H(A, M, K)() == Ms;
}

String SRP::getSessionKey() {
    return K.toString(16);
}
