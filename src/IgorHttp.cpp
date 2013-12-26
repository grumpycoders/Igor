#include <atomic>

#include <json/reader.h>
#include <json/writer.h>

#include <HttpServer.h>
#include <HttpActionStatic.h>
#include <Task.h>
#include <SimpleMustache.h>
#include <BWebSocket.h>
#include <Input.h>
#include <HelperTasks.h>
#include <TaskMan.h>

using namespace Balau;

static std::atomic<SimpleMustache *> s_template;

static Regex rootURL("^/$");

class RootAction : public HttpServer::Action {
  public:
      RootAction() : Action(rootURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool RootAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    HttpServer::Response response(server, req, out);

    response.get()->writeString("Redirecting...");
    response.SetResponseCode(302);
    response.AddHeader("Location: /dyn/main");
    response.Flush();
    return true;
}

static Regex mainURL("^/dyn/main$");

class MainAction : public HttpServer::Action {
  public:
      MainAction() : Action(mainURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool MainAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    SimpleMustache::Context ctx;
    HttpServer::Response response(server, req, out);

    ctx["dojo_path"] = "//ajax.googleapis.com/ajax/libs/dojo/1.9.2";

    s_template.load()->render(response.get(), &ctx);
    response.Flush();
    return true;
}

class IgorWSWorker;

class Listeners;

class Listener {
  public:
      Listener(const char * destination, Listeners *);
    virtual void receive(IgorWSWorker * worker, const std::string & call, const Json::Value & data) = 0;
};

class Listeners {
  public:
    void receive(IgorWSWorker * worker, const std::string & destination, const std::string & call, const Json::Value & data);
  private:
    void registerListener(Listener *, const char * destination);
    RWLock m_lock;
    std::map<String, Listener *> m_map;

    friend class Listener;
};

static Listeners s_listeners;

Listener::Listener(const char * destination, Listeners * listeners) {
    listeners->registerListener(this, destination);
}

void Listeners::registerListener(Listener * listener, const char * destination) {
    ScopeLockW sl(m_lock);

    if (m_map.find(destination) == m_map.end())
        m_map[destination] = listener;
}

void Listeners::receive(IgorWSWorker * worker, const std::string & destination, const std::string & call, const Json::Value & data) {
    ScopeLockR sl(m_lock);

    auto l = m_map.find(destination);
    if (l != m_map.end())
        l->second->receive(worker, call, data);
}

static std::list<IgorWSWorker *> s_websockets;
static Balau::RWLock s_websocketsLock;

static Regex igorWSURL("^/dyn/igorws$");

class IgorWSWorker : public WebSocketWorker {
  public:
      IgorWSWorker(IO<Handle> socket, const String & url);
      virtual ~IgorWSWorker();
    virtual void receiveMessage(const uint8_t * msg, size_t len, bool binary);
    void Do() override;
    void dispatch(const std::string & destination, const std::string & call, const Json::Value & data);

    void send(const char * msg);
  private:
    void setup();
    Events::Timeout m_clock;
    int m_searchMinute = -1;
    std::list<IgorWSWorker *>::iterator m_listPos;
    bool m_setupDone = false;
    bool m_foundMinute = false;
};

IgorWSWorker::IgorWSWorker(IO<Handle> socket, const String & url) : WebSocketWorker(socket, url) {
    enforceServer();
    ScopeLockW sl(s_websocketsLock);
    m_listPos = s_websockets.insert(s_websockets.begin(), this);
}

IgorWSWorker::~IgorWSWorker() {
    ScopeLockW sl(s_websocketsLock);
    s_websockets.erase(m_listPos);
}

void IgorWSWorker::receiveMessage(const uint8_t * msg, size_t len, bool binary) {
    if (binary) {
        Printer::log(M_DEBUG, "got binary message");
    } else {
        Printer::log(M_DEBUG, "got text message '%s'", msg);
        const char * jsonmsg = (const char *)msg;
        Json::Value root;
        Json::Reader reader;
        bool success = reader.parse(jsonmsg, jsonmsg + len, root);
        if (success) {
            const std::string destination = root["destination"].asString();
            const std::string call = root["call"].asString();
            dispatch(destination, call, root["data"]);
        } else {
            Printer::log(M_WARNING, "Error parsing json message '%s'", msg);
        }
    }
}

void IgorWSWorker::setup() {
    m_clock.set(1);
    waitFor(&m_clock);
    time_t rawtime;
    struct tm timeinfo;
    time(&rawtime);
#ifdef _MSC_VER
    localtime_s(&timeinfo, &rawtime);
#else
    localtime_r(&rawtime, &timeinfo);
#endif
    m_searchMinute = timeinfo.tm_min;
}

void IgorWSWorker::dispatch(const std::string & destination, const std::string & call, const Json::Value & data) {
    s_listeners.receive(this, destination, call, data);
}

void IgorWSWorker::Do() {
    bool gotEvent = false;

    if (!m_setupDone) {
        setup();
        m_setupDone = true;
    }

    if (m_clock.gotSignal()) {
        m_clock.reset();
        m_clock.set(m_foundMinute ? 9 : 0.9);
        waitFor(&m_clock);
        time_t rawtime;
        tm timeinfo;
        time(&rawtime);
#ifdef _MSC_VER
        localtime_s(&timeinfo, &rawtime);
#else
        localtime_r(&rawtime, &timeinfo);
#endif
        if (m_searchMinute != timeinfo.tm_min)
            m_foundMinute = true;
        String timeStr;
        timeStr.set("{ \"destination\": \"status\", \"call\": \"clock\", \"data\": { \"clock\": \"%i:%02i\" } }", timeinfo.tm_hour, timeinfo.tm_min);

        WebSocketFrame * frame = new WebSocketFrame(timeStr);
        sendFrame(frame);
    }

    WebSocketWorker::Do();
}

void IgorWSWorker::send(const char * msg) {
    WebSocketFrame * frame = new WebSocketFrame(msg);
    sendFrame(frame);
}

class IgorWSAction : public WebSocketServer<IgorWSWorker> {
public:
    IgorWSAction() : WebSocketServer(igorWSURL) { }
};

static void loadTemplate() {
    SimpleMustache * newTpl = new SimpleMustache();
    IO<Input> tplFile(new Input("data/web-ui/igor.tpl"));
    tplFile->open();
    newTpl->setTemplate(tplFile);
    SimpleMustache * oldTpl = s_template.exchange(newTpl);
    delete oldTpl;
}

static Regex igorReloadURL("^/dyn/reloadui$");

class ReloadAction : public HttpServer::Action {
  public:
      ReloadAction() : Action(igorReloadURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool ReloadAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    bool error = false;
    String errorMsg;

    try {
        loadTemplate();
    }
    catch (GeneralException & e) {
        error = true;
        errorMsg = e.getMsg();
    }

    HttpServer::Response response(server, req, out);

    if (error) {
        response.get()->writeString(String("{\"success\": false, \"msg\": \"") + errorMsg + "\"}");
    } else {
        response.get()->writeString("{\"success\": true}");
    }
    response.SetContentType("application/json");
    response.Flush();
    return true;
}

class MainListener : public Listener {
  public:
      MainListener(Listeners * listeners) : Listener("main", listeners) { }
    virtual void receive(IgorWSWorker * worker, const std::string & call, const Json::Value & data) override;
};

void MainListener::receive(IgorWSWorker * worker, const std::string & call, const Json::Value & data) {
    if (call == "broadcast") {
        ScopeLockR sl(s_websocketsLock);

        if (!data["message"].isString())
            return;

        const char * broadcast = data["message"].asCString();

        if (!broadcast[0])
            return;

        Json::Value msg;
        msg["destination"] = "main";
        msg["call"] = "notify";
        msg["data"]["message"] = broadcast;

        Json::StyledWriter writer;
        String jsonMsg = writer.write(msg);

        for (auto i : s_websockets) {
            if (worker != i)
                i->send(jsonMsg.to_charp());
        }
    }
}

static Regex igorStaticURL("^/static/(.+)");

void igor_setup_httpserver() {
    loadTemplate();

    new MainListener(&s_listeners);

    HttpServer * s = new HttpServer();
    s->registerAction(new RootAction());
    s->registerAction(new MainAction());
    s->registerAction(new IgorWSAction());
    s->registerAction(new ReloadAction());
    s->registerAction(new HttpActionStatic("data/web-ui/static/", igorStaticURL));
    s->setPort(8080);
    s->start();
}
