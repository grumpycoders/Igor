#pragma once

#include <StacklessTask.h>
#include "IgorUsers.h"

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
    bool authenticated() { return m_authenticated; }
    void setAuthenticated() { m_authenticated = true; }
private:
    bool isExpired(time_t now) { return now > m_expiration; }
    void bumpExpiration();
    SRP m_srp;
    Balau::String m_uuid;
    time_t m_expiration;
    bool m_authenticated = false;
};

extern IgorHttpSessionsManager * g_igorHttpSessionsManager;
