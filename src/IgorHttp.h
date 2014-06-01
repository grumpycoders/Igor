#pragma once

#include <map>
#include <list>
#include <atomic>
#include <memory>

#include <BString.h>
#include <Threads.h>
#include <StacklessTask.h>
#include "IgorUsers.h"

namespace Balau { class HttpServer; }

void igor_setup_httpserver();
void igor_setup_websocket(Balau::HttpServer *);

class IgorHttpSession;

class IgorHttpSessionsManager : public Balau::StacklessTask {
  public:
    virtual void Do() override;
    virtual const char * getName() const override { return "IgorHttpSessionsManager"; }
    std::shared_ptr<IgorHttpSession> findSession(const Balau::String & uuid) const;
    std::shared_ptr<IgorHttpSession> createSession();
  private:
    std::map<Balau::String, std::shared_ptr<IgorHttpSession>> m_sessions;
    Balau::Events::Timeout m_clock;
    mutable Balau::RWLock m_lock;
};

class IgorHttpSession {
    friend IgorHttpSessionsManager;
  public:
      IgorHttpSession();
    const Balau::String & getUUID() const { return m_uuid; }
    SRP * getSRP() { return &m_srp; }
    bool authenticated();
private:
    bool isExpired(time_t now) { return now > m_expiration; }
    void bumpExpiration();
    SRP m_srp;
    Balau::String m_uuid;
    time_t m_expiration;
    bool m_authenticated = false;
};

extern IgorHttpSessionsManager * g_igorHttpSessionsManager;
