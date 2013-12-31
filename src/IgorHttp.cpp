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

#include "IgorAnalysis.h"

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

static Regex igorRestDisasmURL("^/dyn/rest/disasm/([a-fA-F0-9-]+)(/(.*))?$");

class RestDisasmAction : public HttpServer::Action {
  public:
      RestDisasmAction() : Action(igorRestDisasmURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool RestDisasmAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    HttpServer::Response response(server, req, out);
    Json::StyledWriter writer;
    String sessionUUID = match.uri[1];

    String idURL = match.uri.size() == 4 ? match.uri[3] : "";
    String rangeHeader;

    if (idURL == "") {
        rangeHeader = req.headers["Range"];
        if (rangeHeader == "") {
            response.SetResponseCode(406);
            response.SetContentType("text/plain");
            response->writeString("You can't request for the whole disassembly... please specify a range.");
            response.Flush();
            return true;
        }
    }

    IgorSession * session = NULL;

    IgorSession::enumerate([&](IgorSession * crawl) -> bool {
        if (sessionUUID == crawl->getUUID()) {
            session = crawl;
            return false;
        }
        return true;
    });

    if (!session) {
        response.SetResponseCode(404);
        response.SetContentType("text/plain");
        response->writeString("Session not found.");
        response.Flush();
        return true;
    }

    igorAddress first, last, linear, linearFirst, linearLast;
    size_t totalSize;
    std::tie(first, last, totalSize) = session->getRanges();
    Json::Value reply;

    if (idURL != "") {
        linearFirst = linearLast = idURL.to_int();
    } else {
        static Regex rangeMatch("Range: items=([0-9]+)-([0-9]+)");
        Regex::Captures matches = rangeMatch.match(rangeHeader.to_charp());
        if (matches.size() != 3) {
            response.SetResponseCode(400);
            response.SetContentType("text/plain");
            response->writeString("Invalid range header.");
            response.Flush();
            return true;
        }
        linearFirst = matches[1].to_int();
        linearLast = matches[2].to_int();
        if (linearFirst > linearLast) {
            response.SetResponseCode(400);
            response.SetContentType("text/plain");
            response->writeString("Invalid range header.");
            response.Flush();
            return true;
        }
        if (linearLast >= totalSize)
            linearLast = totalSize - 1;
    }

    if (linearFirst >= totalSize) {
        response.SetResponseCode(400);
        response.SetContentType("text/plain");
        response->writeString("Range exceeded");
        response.Flush();
        return true;
    }

    igorAddress currentPC = session->linearToVirtual(linear = linearFirst);

    {
        c_cpu_module* pCpu = session->getCpuForAddress(currentPC);

        s_analyzeState analyzeState;
        analyzeState.m_PC = currentPC;
        analyzeState.pCpu = pCpu;
        analyzeState.pCpuState = session->getCpuStateForAddress(currentPC);
        analyzeState.pSession = session;
        analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

        while (linear <= linearLast) {
            currentPC = analyzeState.m_PC;
            if (session->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS)) {
                String disassembledString;
                String val, address;
                pCpu->printInstruction(&analyzeState, disassembledString);
                analyzeState.m_PC = currentPC;
                const size_t nBytes = analyzeState.m_cpu_analyse_result->m_instructionSize;
                Json::Value v;
                v["type"] = "inst";
                v["disasm"] = disassembledString.to_charp();
                address.set("%016llx", analyzeState.m_PC);
                val.set("%02X", session->readU8(analyzeState.m_PC));
                v["byte"] = val.to_charp();
                v["address"] = address.to_charp();
                address.set("%lli", linear);
                v["id"] = address.to_charp();
                address.set("%lli", nBytes);
                v["instsize"] = address.to_charp();
                reply[linear++ - linearFirst] = v;

                for (int i = 1; i < nBytes; i++) {
                    v["type"] = "instcont";
                    val.set("%02X", session->readU8(analyzeState.m_PC + i));
                    v["byte"] = val.to_charp();
                    address.set("%016llx", analyzeState.m_PC + i);
                    v["address"] = address.to_charp();
                    address.set("%lli", linear);
                    v["id"] = address.to_charp();
                    reply[linear++ - linearFirst] = v;
                }
                analyzeState.m_PC += nBytes;
                EAssert(analyzeState.m_PC == analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize, "inconsistant state...");
            } else {
                Json::Value v;
                analyzeState.m_PC = currentPC;
                v["type"] = "rawdata";
                String val, address;
                val.set("%02X", session->readU8(analyzeState.m_PC));
                v["byte"] = val.to_charp();
                v["value"] = val.to_charp();
                address.set("%016llx", analyzeState.m_PC);
                v["address"] = address.to_charp();
                address.set("%lli", linear);
                v["id"] = address.to_charp();
                reply[linear++ - linearFirst] = v;

                analyzeState.m_PC++;
                linear++;
            }
        }

        delete analyzeState.m_cpu_analyse_result;
    }

    String jsonMsg = writer.write(reply);
    response->writeString(jsonMsg);
    response.SetContentType("application/json");
    String rangeHeaderResponse;
    rangeHeaderResponse.set("Content-Range: items %lli-%lli/%lli", linearFirst, linearLast, totalSize);
    if (rangeHeader != "")
        response.AddHeader(rangeHeaderResponse);
    response.Flush();
    return true;
}

static Regex listSessionsURL("^/dyn/listSessions$");

class ListSessionsAction : public HttpServer::Action {
  public:
      ListSessionsAction() : Action(listSessionsURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool ListSessionsAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    Json::Value reply;
    Json::UInt idx = 0;
    HttpServer::Response response(server, req, out);
    Json::StyledWriter writer;

    IgorSession::enumerate([&](IgorSession * session) -> bool {
        reply[idx++] = session->getUUID().to_charp();
        return true;
    });

    String jsonMsg = writer.write(reply);
    response->writeString(jsonMsg);
    response.SetContentType("application/json");
    response.Flush();

    return true;
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
    s->registerAction(new RestDisasmAction());
    s->registerAction(new ListSessionsAction());
    s->registerAction(new HttpActionStatic("data/web-ui/static/", igorStaticURL));
    s->setPort(8080);
    s->start();
}
