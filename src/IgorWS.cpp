#include <atomic>

#include <json/reader.h>
#include <json/writer.h>

#include <HttpServer.h>
#include <Task.h>
#include <SimpleMustache.h>
#include <BWebSocket.h>
#include <Input.h>
#include <HelperTasks.h>
#include <TaskMan.h>

#include "IgorAnalysis.h"
#include "IgorHttp.h"

using namespace Balau;

class IgorWSWorker;

class Listeners;

class WSPrinter : public Printer {
  public:
      WSPrinter() : m_printer(getPrinter()) { setGlobal(); }
      ~WSPrinter() { m_printer->setGlobal(); }
  private:
    virtual void _print(const char * fmt, va_list ap);
    Printer * m_printer;
};

static DefaultTmpl<WSPrinter> wsPrinter(100);

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

    void print(const char * fmt, va_list ap);

  private:
    void setup();
    Events::Timeout m_clock;
    int m_searchMinute = -1;
    std::list<IgorWSWorker *>::iterator m_listPos;
    bool m_setupDone = false;
    bool m_foundMinute = false;
    String m_printing;
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
    }
    else {
        Printer::log(M_DEBUG, "got text message '%s'", msg);
        const char * jsonmsg = (const char *)msg;
        Json::Value root;
        Json::Reader reader;
        bool success = reader.parse(jsonmsg, jsonmsg + len, root);
        if (success) {
            const std::string destination = root["destination"].asString();
            const std::string call = root["call"].asString();
            dispatch(destination, call, root["data"]);
        }
        else {
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

void IgorWSWorker::print(const char * fmt, va_list ap) {
    m_printing.append(fmt, ap);
    ssize_t lfpos;
    
    while ((lfpos = m_printing.strchr('\n')) >= 0) {
        String toPrint = m_printing.extract(0, lfpos);
        Json::Value msg;
        msg["destination"] = "console";
        msg["call"] = "add";
        msg["data"]["str"] = toPrint.to_charp();
        
        Json::StyledWriter writer;
        String jsonMsg = writer.write(msg);

        send(jsonMsg.to_charp());

        m_printing = m_printing.extract(lfpos + 1);
    }
}

void WSPrinter::_print(const char * fmt, va_list ap) {
    ScopeLockW sl(s_websocketsLock);
    for (auto & i : s_websockets)
        i->print(fmt, ap);
    m_printer->_print(fmt, ap);
}

class IgorWSAction : public WebSocketServer<IgorWSWorker> {
  public:
      IgorWSAction() : WebSocketServer(igorWSURL) { }
};

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

void igor_setup_websocket(HttpServer * s) {
    new MainListener(&s_listeners);

    s->registerAction(new IgorWSAction());
}
