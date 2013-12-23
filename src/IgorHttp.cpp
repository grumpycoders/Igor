#include <atomic>
#include <HttpServer.h>
#include <Task.h>
#include <SimpleMustache.h>
#include <BWebSocket.h>
#include <Input.h>

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

static Regex igorWSURL("/dyn/igorws$");

class IgorWSWorker : public WebSocketWorker {
public:
    virtual void receiveMessage(const uint8_t * msg, size_t len, bool binary) {
        if (binary)
            Printer::log(M_INFO, "got binary message");
        else
            Printer::log(M_INFO, "got text message '%s'", msg);
        WebSocketFrame * frame = new WebSocketFrame(String("Hello World!"), 1);
        sendFrame(frame);
    }
    IgorWSWorker(IO<Handle> socket, const String & url) : WebSocketWorker(socket, url) {
        enforceServer();
    }
};

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

static Regex igorReloadURL("/dyn/reloadui$");

class ReloadAction : public HttpServer::Action {
public:
    ReloadAction() : Action(igorReloadURL) { }
private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool ReloadAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    bool error = false;
    String errorMsg = "Just testing...";

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

void igor_setup_httpserver() {
    loadTemplate();

    HttpServer * s = new HttpServer();
    s->registerAction(new RootAction());
    s->registerAction(new MainAction());
    s->registerAction(new IgorWSAction());
    s->registerAction(new ReloadAction());
    s->setPort(8080);
    s->start();
}
