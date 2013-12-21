#include <HttpServer.h>
#include <Task.h>
#include <SimpleMustache.h>

using namespace Balau;

const char htmlTemplateStr[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
"  <head>\n"
"    <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n"
"    <title>{{title}}</title>\n"
"    <style type=\"text/css\">\n"
"        body { font-family: arial, helvetica, sans-serif; }\n"
"    </style>\n"
"  </head>\n"
"\n"
"  <body>\n"
"    <h1>{{title}}</h1>\n"
"    <h2>{{msg}}</h2>\n"
"    <div id=\"resultDiv\"></div>\n"
"    <input type=\"button\" id=\"textButton\" value=\"Load\"></input>\n"
"    <script src=\"//ajax.googleapis.com/ajax/libs/dojo/1.9.2/dojo/dojo.js\" data-dojo-config=\"has:{'dojo-firebug': true}, async: true\"></script>\n"
"    <script>\n"
"      require([\"dojo/dom\", \"dojo/on\", \"dojo/request\", \"dojo/domReady!\"], function(dom, on, request) {\n"
"        var resultDiv = dom.byId(\"resultDiv\");\n"
"\n"
"        on(dom.byId(\"textButton\"), \"click\", function(evt) {\n"
"          request.get(\"test.txt\").then(\n"
"            function(text) {\n"
"              resultDiv.innerHTML = text;\n"
"            },\n"
"            function(error) {\n"
"              resultDiv.innerHTML = error;\n"
"            }\n"
"          );\n"
"        });\n"
"      });\n"
"    </script>\n"
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

    ctx["title"] = "Test";
    ctx["msg"] = "This is a Dojo Ajax test.";

    testHtmlTemplate.htmlTemplate.render(response.get(), &ctx);
    response.Flush();
    return true;
}

static Regex testURL("/test.txt$");

class TestAction : public HttpServer::Action {
  public:
      TestAction() : Action(testURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool TestAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    HttpServer::Response response(server, req, out);

    response.get()->writeString("Ajax");
    response.SetContentType("text/plain");
    response.Flush();
    return true;
}

void igor_setup_httpserver() {
    HttpServer * s = new HttpServer();
    s->registerAction(new RootAction());
    s->registerAction(new TestAction());
    s->setPort(8080);
    s->setLocal("localhost");
    s->start();
}
