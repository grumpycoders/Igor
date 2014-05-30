#include <malloc.h>

#include <Base64.h>
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
    m_state = h.m_state;
    m_finalized = h.m_finalized;
    memcpy(m_digest, h.m_digest, DIGEST_SIZE);
}

SRP::Hash::Hash() {
    sha256_init(&m_state);
}

void SRP::Hash::updateString(const char * str) {
    while (*str)
        sha256_process(&m_state, (unsigned char *) str++, 1);
}

void SRP::Hash::update(const unsigned char * data, size_t l) {
    sha256_process(&m_state, data, l);
}

void SRP::Hash::updateBString(const String & str) {
    sha256_process(&m_state, (const unsigned char *)str.to_charp(), str.strlen());
}

void SRP::Hash::updateBigInt(const BigInt & v) {
    updateBString(v.toString());
}

void SRP::Hash::final() {
    sha256_done(&m_state, m_digest);
    m_finalized = true;
}

BigInt SRP::Hash::toBigInt() {
    BigInt v;
    v.importBin(m_digest, DIGEST_SIZE);
    return v;
}

String SRP::generateVerifier(const String & I, const String & p) {
    static const int V_LEN = 1024 / 8;
    uint8_t salt[SALT_LEN + V_LEN];
    unsigned long r = rng_get_bytes(salt, SALT_LEN, NULL);
    RAssert(r == SALT_LEN, "Unable to generate enough random bytes for our salt.");
    String s((char *)salt, r);
    BigInt x = H(s, I, p).toBigInt();
    BigInt v = srpBigNums.g().modpow(x, srpBigNums.N());

    IAssert(V_LEN >= v.exportSize(), "Not enough bytes to export password verifier?!");

    uint8_t exportV[V_LEN];
    v.exportBin(exportV);

    Json::Value vstr;
    vstr["s"] = Base64::encode(salt, SALT_LEN).to_charp();
    vstr["v"] = Base64::encode(exportV, v.exportSize()).to_charp();

    Json::FastWriter writer;
    return writer.write(vstr);
}
