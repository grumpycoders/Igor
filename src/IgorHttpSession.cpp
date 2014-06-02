#include <memory>

#include <BRegex.h>
#include <HttpServer.h>
#include <TaskMan.h>

#include "IgorSession.h"
#include "IgorHttpSession.h"

using namespace Balau;

IgorHttpSessionsManager * g_igorHttpSessionsManager = NULL;

static Regex authClientPacketAURL("^/dyn/auth/clientPacketA$");

class AuthClientPacketAAction : public HttpServer::Action {
  public:
      AuthClientPacketAAction() : Action(authClientPacketAURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool AuthClientPacketAAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    HttpServer::Response response(server, req, out);

    std::shared_ptr<IgorHttpSession> session = g_igorHttpSessionsManager->createSession();

    SRP * srp = session ? session->getSRP() : NULL;

    if (!session || !srp || !srp->serverRecvPacketA(req.variables["msg"])) {
        response->writeString("{}");
        response.SetContentType("application/json");
        response.Flush();
        return true;
    }

    response.AddHeader(String("Set-Cookie: session=") + session->getUUID() + "; Path=/; HttpOnly");
    response->writeString(srp->serverSendPacketB());
    response.SetContentType("application/json");
    response.Flush();

    return true;
}

static Regex authClientProofURL("^/dyn/auth/clientProof$");

class AuthClientProofAction : public HttpServer::Action {
  public:
      AuthClientProofAction() : Action(authClientProofURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool AuthClientProofAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    HttpServer::Response response(server, req, out);

    std::shared_ptr<IgorHttpSession> session = g_igorHttpSessionsManager->findSession(req.cookies["session"]);

    SRP * srp = session ? session->getSRP() : NULL;

    if (!session || !srp || !srp->serverRecvProof(req.variables["msg"])) {
        response->writeString("{}");
        response.SetContentType("application/json");
        response.Flush();
        return true;
    }

    response->writeString(srp->serverSendProof());
    response.SetContentType("application/json");
    response.Flush();

    session->setAuthenticated();

    return true;
}

IgorHttpSession::IgorHttpSession() {
    m_uuid = IgorSession::generateUUID();
    m_expiration = time(NULL);
}

void IgorHttpSession::bumpExpiration() {
    m_expiration = time(NULL) + 24 * 3600;
}

bool IgorHttpSession::isProofValid(const String & proof) {
    if (!m_authenticated)
        return false;

    return m_srp.verifyProof(proof);
}

void IgorHttpSessionsManager::Do() {
    if (!m_state) {
        m_state = 1;
        m_clock.set(3600);
        waitFor(&m_clock);
        yield();
    }

    if (m_clock.gotSignal()) {
        m_clock.reset();
        m_clock.set(3600);
        waitFor(&m_clock);
    } else {
        yield();
    }

    ScopeLockW lock(m_lock);
    time_t now;
    time(&now);

    for (auto session = m_sessions.begin(); session != m_sessions.end(); session++)
        if (session->second->isExpired(now))
            session = m_sessions.erase(session);

    yield();
}

std::shared_ptr<IgorHttpSession> IgorHttpSessionsManager::findSession(const String & uuid) const {
    ScopeLockR lock(m_lock);

    auto session = m_sessions.find(uuid);

    if (session == m_sessions.end())
        return NULL;

    session->second->bumpExpiration();
    return session->second;
}

std::shared_ptr<IgorHttpSession> IgorHttpSessionsManager::createSession() {
    std::shared_ptr<IgorHttpSession> ret(new IgorHttpSession());

    ScopeLockW lock(m_lock);
    m_sessions[ret->getUUID()] = ret;

    return ret;
}

void igor_setup_auth(Balau::HttpServer * s) {
    TaskMan::registerTask(g_igorHttpSessionsManager = new IgorHttpSessionsManager());
    s->registerAction(new AuthClientPacketAAction);
    s->registerAction(new AuthClientProofAction);
}

bool AuthenticatedAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    const String & sessionUUID = req.cookies["session"];
    const String & proofHeader = req.headers["X-Auth-SRP-proof"];
    std::shared_ptr<IgorHttpSession> session = g_igorHttpSessionsManager->findSession(sessionUUID);

    if (!session || !session->isProofValid(proofHeader)) {
        HttpServer::Response response(server, req, out);

        response->writeString("{ \"status\": \"needAuth\" }");
        response.SetContentType("application/json");
        response.Flush();
        return true;
    }

    return safeDo(server, req, match, out);
}
