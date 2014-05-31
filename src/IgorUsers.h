#pragma once

#include <vector>
#include <BString.h>
#include <BigInt.h>
#include <tomcrypt.h>

class IgorUsers {
  public:
    static std::vector<Balau::String> getUsers();
    static std::pair<bool, Balau::String> getV(const Balau::String & user);
    static bool addUser(const Balau::String & user, const Balau::String & v);
    static bool changePassword(const Balau::String & user, const Balau::String & v);
    static bool delUser(const Balau::String & user);
};

// Almost direct implementation from http://en.wikipedia.org/wiki/Secure_Remote_Password_protocol
class SRP {
  public:
    static bool selfTest();
    static Balau::BigInt rand(int s = V_LEN);
    static Balau::String generateVerifier(const Balau::String & username, const Balau::String & password);

    typedef enum { SRP6_CLIENT, SRP6_SERVER } mode_t;

    void setUsername(const Balau::String & username) { I = username; }
    void setPassword(const Balau::String & password) { p = password; }

    Balau::String getUsername() { return I; }

    // Client sequence
    Balau::String clientSendPacketA();
    bool clientRecvPacketB(const Balau::String & packet);
    Balau::String clientSendProof();
    bool clientRecvProof(const Balau::String & packet);

    // Server sequence
    bool serverRecvPacketA(const Balau::String & packet);
    Balau::String serverSendPacketB();
    bool serverRecvProof(const Balau::String & packet);
    Balau::String serverSendProof();

    Balau::String getSessionKey();

    class Hash {
      friend class SRP;
      public:
          Hash(const Hash &);
        static const int DIGEST_SIZE = 32;
        Balau::BigInt operator()() const { return toBigInt(); }
        Balau::BigInt toBigInt() const;
      private:
          Hash();
        void update(const unsigned char * data, size_t l);
        void updateString(const char * str);
        void updateBString(const Balau::String & str);
        void updateBigInt(const Balau::BigInt & v);
        void updateHash(const Hash & v);
        void final();
        hash_state m_state;
        bool m_finalized = false;
        unsigned char m_digest[DIGEST_SIZE];
    };
    template<typename... Args>
    static Hash H(Args... args) {
        Hash h;
        Hi(h, args...);
        h.final();
        return h;
    }
private:
    bool loadPassword(const Balau::String & password);
    static Balau::BigInt generateX(const Balau::BigInt & s, const Balau::String & I, const Balau::String & p);
    static const int SALT_LEN = 64 / 8;
    static const int V_LEN = 1024 / 8;
    Balau::String I, p;
    Balau::BigInt a, A, b, B, u, v, s, S, K, M;
    static void Hi(Hash &) { }
    template<typename... Args>
    static void Hi(Hash & h, const Hash & v, Args... args) {
        h.updateHash(v);
        Hi(h, args...);
    }
    template<typename... Args>
    static void Hi(Hash & h, const Balau::BigInt & v, Args... args) {
        h.updateBigInt(v);
        Hi(h, args...);
    }
    template<typename... Args>
    static void Hi(Hash & h, const Balau::String & val, Args... args) {
        h.updateBString(val);
        Hi(h, args...);
    }
    template<typename... Args>
    static void Hi(Hash & h, const char * str, Args... args) {
        h.updateString(str);
        Hi(h, args...);
    }
};
