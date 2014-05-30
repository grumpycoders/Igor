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

/*

$ openssl dhparam -text 1024
Generating DH parameters, 1024 bit long safe prime, generator 2
This is going to take a long time
...............................................................................
.......................................................................+.......
..................+.......................+.........+..........................
............+.......................................................+.+........
...............................................+....+..........................
.........................+.....................................................
............+.................+................................................
...............+...............................................+.....+.........
..................................+...................................++*++*++*
PKCS#3 DH Parameters: (1024 bit)
prime:
00:f6:dd:cf:59:98:06:d2:b1:07:7a:96:38:42:0a:
e3:b8:81:5d:92:1e:a5:cd:95:a6:e5:8c:58:dc:cc:
68:18:16:6f:7e:b9:1d:57:7d:52:70:ad:c6:3d:8a:
9a:a5:98:42:49:69:18:51:60:3a:45:12:c7:e8:53:
26:30:63:6c:bf:e7:22:ab:03:05:e5:8e:72:3f:db:
e9:0e:31:d2:79:f8:f9:4e:86:29:be:5c:2f:da:7d:
0f:1e:b4:21:15:c7:46:cb:84:c9:ba:ac:aa:1d:cc:
89:af:48:7b:60:c7:19:af:b8:99:f5:ce:d7:a7:1b:
7a:f3:40:3f:53:7f:7f:66:03
generator: 2 (0x2)
-----BEGIN DH PARAMETERS-----
MIGHAoGBAPbdz1mYBtKxB3qWOEIK47iBXZIepc2VpuWMWNzMaBgWb365HVd9UnCt
xj2KmqWYQklpGFFgOkUSx+hTJjBjbL/nIqsDBeWOcj/b6Q4x0nn4+U6GKb5cL9p9
Dx60IRXHRsuEybqsqh3Mia9Ie2DHGa+4mfXO16cbevNAP1N/f2YDAgEC
-----END DH PARAMETERS-----


*/

class SRPBigNums : public AtStart {
  public:
      SRPBigNums() : AtStart(100) { }
    void doStart() {
        static const char N_str[] =
            "00f6ddcf599806d2b1077a9638420a"
            "e3b8815d921ea5cd95a6e58c58dccc"
            "6818166f7eb91d577d5270adc63d8a"
            "9aa5984249691851603a4512c7e853"
            "2630636cbfe722ab0305e58e723fdb"
            "e90e31d279f8f94e8629be5c2fda7d"
            "0f1eb42115c746cb84c9baacaa1dcc"
            "89af487b60c719afb899f5ced7a71b"
            "7af3403f537f7f6603";
        static BigInt N;
        N.set(N_str, 16);
        m_N = &N;

        static BigInt g;
        g.set(2);
        m_g = &g;

        static BigInt k = SRP::H(N, g).toBigInt();
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
    updateBString(v.toString());
}

void SRP::Hash::final() {
    IAssert(!m_finalized, "Can't finalize a finalized hash.");
    sha256_done(&m_state, m_digest);
    m_finalized = true;
}

BigInt SRP::Hash::toBigInt() {
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
    BigInt x = H(s, I, p).toBigInt();
    BigInt v = g.modpow(x, N);

    IAssert(V_LEN >= v.exportUSize(), "Not enough bytes to export password verifier?!");

    Json::Value vstr;
    vstr["data"]["v"] = v.toString(16).to_charp();
    vstr["data"]["s"] = s.toString(16).to_charp();
    vstr["type"] = "SRP6a";

    Json::StyledWriter writer;
    return writer.write(vstr);
}

bool SRP::setPassword(const Balau::String & password, mode_t mode) {
    Json::Reader reader;
    Json::Value values;

    const char * pcharp = password.to_charp();
    ssize_t len = password.strlen();
    ssize_t lenV = V_LEN, lenS = SALT_LEN;

    switch (mode) {
    case SRP6_CLIENT:
        p = password;
        return true;
    case SRP6_SERVER:
        if (!reader.parse(pcharp, pcharp + len, values))
            return false;
        if (values["type"] != "SRP6a")
            return false;
        v.set(values["data"]["v"].asString(), 16);
        s.set(values["data"]["s"].asString(), 16);
        return true;
    }

    return false;
}

bool SRP::selfTest() {
    String I = "testUsername";
    String p = "testPassword";
    String pv = generateVerifier(I, p).to_charp();
    String msg;
    bool success;

    Printer::log(M_DEBUG, "Generated test pv: %s", pv.to_charp());

    SRP client, server;
    client.setUsername(I);
    success = client.setPassword(p, SRP6_CLIENT);
    if (!success) return false;

    msg = client.clientSendPacketA();
    Printer::log(M_DEBUG, "Generated test packetA: %s", msg.to_charp());
    server.setUsername(I);
    success = server.setPassword(pv, SRP6_SERVER);
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

    return true;
}

String SRP::clientSendPacketA() {
    const BigInt & N = srpBigNums.N();
    const BigInt & g = srpBigNums.g();

    AAssert(I != "", "username not set");
    a = rand();
    A = g.modpow(a, N);

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

    b = rand();
    B = srpBigNums.k() * v;
    B.do_modadd(g.modpow(b, N), N);

    Json::Value packet;
    packet["serverPacketB"]["s"] = s.toString(16).to_charp();
    packet["serverPacketB"]["B"] = B.toString(16).to_charp();

    u = H(A, B).toBigInt();

    S = A * v.modpow(u, N);
    S.do_modpow(b, N);
    K = H(S).toBigInt();

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

    u = H(A, B).toBigInt();

    BigInt x = H(s, I, p).toBigInt();
    S = B - k * g.modpow(x, N);
    S.do_modpow(a + u * x, N);
    K = H(S).toBigInt();

    return true;
}

String SRP::clientSendProof() {
    const BigInt & N = srpBigNums.N();
    const BigInt & g = srpBigNums.g();

    M = H(H(N).toBigInt() ^ H(g).toBigInt(), H(I).toBigInt(), s, A, B, K).toBigInt();

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

    M = H(H(N).toBigInt() ^ H(g).toBigInt(), H(I).toBigInt(), s, A, B, K).toBigInt();

    BigInt Mc;
    Mc.set(values["clientProof"]["M"].asString(), 16);

    return M == Mc;
}

String SRP::serverSendProof() {
    Json::Value packet;
    packet["serverProof"]["M"] = H(A, M, K).toBigInt().toString(16).to_charp();

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

    return H(A, M, K).toBigInt() == Ms;
}
