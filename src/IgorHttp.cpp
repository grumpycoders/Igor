#include <HttpServer.h>
#include <Task.h>
#include <SimpleMustache.h>
#include <BWebSocket.h>

using namespace Balau;

const char htmlTemplateStr[] =
"<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN'\n"
"'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>\n"
"<html xmlns='http://www.w3.org/1999/xhtml'>\n"
"  <head>\n"
"    <meta http-equiv='content-type' content='text/html; charset=utf-8' />\n"
"    <title>{{title}}</title>\n"
"    <link rel='stylesheet' href='{{dojo_path}}/dijit/themes/claro/claro.css'>\n"

"    <style type='text/css'>\n"
"      html, body { height: 100%; width: 100%; padding: 0; border: 0; }\n"
"      .claro {\n"
"        font-family: Verdana, Arial, Helvetica, sans-serif;\n"
"      }"
"      #main { height: 100%; width: 100%; border: 0; }\n"
"      #header { margin: 0; }\n"
"      /* pre-loader specific stuff to prevent unsightly flash of unstyled content */\n"
"      #loader {\n"
"        padding: 0;\n"
"        margin: 0;\n"
"        position: absolute;\n"
"        top:0; left:0;\n"
"        width: 100%; height: 100%;\n"
"        background: #ededed;\n"
"        z-index: 999;\n"
"        vertical-align: middle;\n"
"      }\n"
"      #loaderInner {\n"
"        padding: 5px;\n"
"        position: relative;\n"
"        left: 0;\n"
"        top: 0;\n"
"        width: 175px;\n"
"        background:#33c;\n"
"        color:#fff;\n"
"      }\n"
"      hr.spacer{ border:0; background-color: #ededed; width: 80%; height: 1px; }\n"
"    </style>\n"
"    <script src='{{dojo_path}}/dojo/dojo.js' data-dojo-config='has:{\"dojo-firebug\": true}, async: true'></script>\n"
"    <script>\n"
"      require(["
"        'dojo', "
"        'dijit/dijit', "
"        'dojo/parser', "
"        'dojo/ready', "
"        'dojo/dom', "
"        'dojo/on', "
"        'dojo/request', "
"        'dijit/dijit-all', "
"        'dojox/socket', "
"        'dojox/socket/Reconnect', "
"        'dojo/domReady!'], "
"      function(dojo, dijit, parser, ready, dom, on, request) {\n"
"        ready(function() {\n"
"          dojo.fadeOut({\n"
"            node: 'loader',\n"
"            duration: 500,\n"
"            onEnd: function(n) { n.style.display = 'none'; }\n"
"          }).play();"
"          parser.parse();\n"
"        });\n"
"        var socket = dojox.socket({url:'/igorws'});\n"
"        socket = dojox.socket.Reconnect(socket);\n"
"\n"
"        dojo.connect(socket, 'onopen', function(event) {\n"
"        });\n"
"\n"
"        dojo.connect(socket, 'onmessage', function(event) {\n"
"          var message = event.data;\n"
"        });\n"
"\n"
"      });\n"
"    </script>\n"
"  </head>\n"
"\n"
"  <body class='claro'>\n"
"    <div id='loader'><div id='loaderInner' style='direction:ltr;'>Loading...</div></div>\n"
"    <div data-dojo-type='dijit/MenuBar', id='navMenu'>\n"
"      <div data-dojo-type='dijit/PopupMenuBarItem'>\n"
"        <span>File</span>\n"
"        <div data-dojo-type='dijit/DropDownMenu' id='fileMenu'>\n"
"          <div data-dojo-type='dijit/MenuItem' data-dojo-props='onClick:function() { alert(\"open\"); }'>Open</div>\n"
"        </div>"
"      </div>\n"
"    </div>\n"
"  </body>\n"
"</html>\n"
;

class TestHtmlTemplateTask : public Task {
    virtual void Do() { m_template.setTemplate(htmlTemplateStr); }
    virtual const char * getName() const { return "TestHtmlTemplateTask"; }
    SimpleMustache & m_template;
  public:
      TestHtmlTemplateTask(SimpleMustache & htmlTemplate) : m_template(htmlTemplate) { }
};

class TestHtmlTemplate : public AtStartAsTask {
  public:
      TestHtmlTemplate() : AtStartAsTask(10), htmlTemplate(m_template) { }
    virtual Task * createStartTask() { return new TestHtmlTemplateTask(m_template); }
    const SimpleMustache & htmlTemplate;
  public:
    SimpleMustache m_template;
};

static TestHtmlTemplate testHtmlTemplate;

static Regex rootURL("^/$");

class RootAction : public HttpServer::Action {
  public:
      RootAction() : Action(rootURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool RootAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    SimpleMustache::Context ctx;
    HttpServer::Response response(server, req, out);

    ctx["dojo_path"] = "//ajax.googleapis.com/ajax/libs/dojo/1.9.2";
    ctx["title"] = "Test";
    ctx["msg"] = "This is a Dojo WebSocket test.";

    testHtmlTemplate.htmlTemplate.render(response.get(), &ctx);
    response.Flush();
    return true;
}

static Regex igorWSURL("/igorws$");

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

void igor_setup_httpserver() {
    HttpServer * s = new HttpServer();
    s->registerAction(new RootAction());
    s->registerAction(new IgorWSAction());
    s->setPort(8080);
    s->start();
}
