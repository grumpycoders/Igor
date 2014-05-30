#pragma once

#include <BString.h>
#include <BigInt.h>
#include <tomcrypt.h>

class SRP {
  public:
    static Balau::String generateVerifier(const Balau::String & username, const Balau::String & password);
    class Hash {
      friend class SRP;
      public:
          Hash(const Hash &);
        static const int DIGEST_SIZE = 32;
        const unsigned char * operator()() { return m_digest; }
        Balau::BigInt toBigInt();
      private:
          Hash();
        void update(const unsigned char * data, size_t l);
        void updateString(const char * str);
        void updateBString(const Balau::String & str);
        void updateBigInt(const Balau::BigInt & v);
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
    static const int SALT_LEN = 64;
    static void Hi(Hash &) { }
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
