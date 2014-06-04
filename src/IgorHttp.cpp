#include <atomic>

#include <json/reader.h>
#include <json/writer.h>

#include <HttpServer.h>
#include <HttpActionStatic.h>
#include <Task.h>
#include <SimpleMustache.h>
#include <Input.h>
#include <HelperTasks.h>
#include <TaskMan.h>

#include "IgorAnalysis.h"
#include "IgorSession.h"
#include "IgorHttp.h"
#include "IgorHttpSession.h"

#include "IgorMemory.h"

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

static Regex igorRestDisasmURL("^/dyn/rest/disasm/([a-fA-F0-9-]+)(/(.*))?$");

class RestDisasmAction : public AuthenticatedAction {
  public:
      RestDisasmAction() : AuthenticatedAction(igorRestDisasmURL) { }
  private:
    virtual bool safeDo(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) override;
};

bool RestDisasmAction::safeDo(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
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

    session = IgorSession::find(sessionUUID);

    if (!session) {
        response.SetResponseCode(404);
        response.SetContentType("text/plain");
        response->writeString("Session not found.");
        response.Flush();
        return true;
    }

    ScopedLambda release([&]() { session->release(); });

    igorAddress first, last;
    u64 linear, linearFirst, linearLast;
    uint64_t totalSize;
    std::tie(first, last, totalSize) = session->getRanges();
    Json::Value reply;

    if (idURL != "") {
        linearFirst = linearLast = idURL.to_int();
    } else {
        static Regex rangeMatch("items=([0-9]+)-([0-9]+)");
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
    igorAddress startPC = session->get_next_valid_address_before(currentPC);

    {
        c_cpu_module* pCpu = session->getCpuForAddress(startPC);

        s_analyzeState analyzeState;
        analyzeState.m_PC = startPC;
        analyzeState.pCpu = pCpu;
        analyzeState.pCpuState = session->getCpuStateForAddress(startPC);
        analyzeState.pSession = session;
        analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();

        while (linear <= linearLast) {
            if (currentPC != startPC) {
                String disassembledString;
                String val, address;
                igor_result r = pCpu->analyze(&analyzeState);
                EAssert(r == IGOR_SUCCESS, "Doesn't make sense to rewind when it's not an instruction (yet)");
                pCpu->printInstruction(&analyzeState, disassembledString);
                const uint64_t nBytes = analyzeState.m_cpu_analyse_result->m_instructionSize + (startPC - currentPC);
                Json::Value v;
                v["type"] = "instcont";
                v["disasm"] = disassembledString.to_charp();
                address.set("%016llx", startPC.offset);
                v["start"] = address.to_charp();
                analyzeState.m_PC = currentPC;
                address.set("%016llx", analyzeState.m_PC.offset);
                val.set("%02X", session->readU8(analyzeState.m_PC));
                v["byte"] = val.to_charp();
                v["address"] = address.to_charp();
                address.set("%lli", linear);
                v["id"] = address.to_charp();
                address.set("%lli", nBytes);
                v["instsize"] = address.to_charp();
                reply[linear++ - linearFirst] = v;

                for (int i = 1; i < nBytes; i++) {
                    if (linear > linearLast)
                        break;

                    v["type"] = "instcont";
                    val.set("%02X", session->readU8(analyzeState.m_PC + i));
                    v["byte"] = val.to_charp();
                    address.set("%016llx", (analyzeState.m_PC + i).offset);
                    v["address"] = address.to_charp();
                    address.set("%lli", linear);
                    v["id"] = address.to_charp();
                    reply[linear++ - linearFirst] = v;
                }
                analyzeState.m_PC += nBytes;
                EAssert(analyzeState.m_PC == analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize, "inconsistant state...");

                startPC = currentPC;
                continue;
            }
            currentPC = analyzeState.m_PC;
            if (session->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS)) {
                String disassembledString;
                String val, address;
                pCpu->printInstruction(&analyzeState, disassembledString);
                analyzeState.m_PC = currentPC;
                const uint64_t nBytes = analyzeState.m_cpu_analyse_result->m_instructionSize;
                Json::Value v;
                v["type"] = "inst";
                v["disasm"] = disassembledString.to_charp();
                address.set("%016llx", analyzeState.m_PC.offset);
                val.set("%02X", session->readU8(analyzeState.m_PC));
                v["byte"] = val.to_charp();
                v["address"] = address.to_charp();
                v["start"] = address.to_charp();
                address.set("%lli", linear);
                v["id"] = address.to_charp();
                address.set("%lli", nBytes);
                v["instsize"] = address.to_charp();
                reply[linear++ - linearFirst] = v;

                for (int i = 1; i < nBytes; i++) {
                    if (linear > linearLast)
                        break;

                    v["type"] = "instcont";
                    val.set("%02X", session->readU8(analyzeState.m_PC + i));
                    v["byte"] = val.to_charp();
                    address.set("%016llx", (analyzeState.m_PC + i).offset);
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
                address.set("%016llx", analyzeState.m_PC.offset);
                v["address"] = address.to_charp();
                address.set("%lli", linear);
                v["id"] = address.to_charp();
                reply[linear++ - linearFirst] = v;

                analyzeState.m_PC++;
            }
            startPC = currentPC;
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

class ListSessionsAction : public AuthenticatedAction {
  public:
      ListSessionsAction() : AuthenticatedAction(listSessionsURL) { }
  private:
    virtual bool safeDo(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) override;
};

bool ListSessionsAction::safeDo(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    Json::Value reply;
    Json::UInt idx = 0;
    HttpServer::Response response(server, req, out);
    Json::StyledWriter writer;

    reply["status"] = "ok";

    IgorSession::enumerate([&](IgorSession * session) -> void {
        Json::Value & entry = reply["list"][idx];
        String name = session->getSessionName();
        if (name == "")
            name = session->getUUID();
        entry["name"] = name.to_charp();
        entry["uuid"] = session->getUUID().to_charp();
        igorAddress entryPoint = session->getEntryPoint();
        String address;
        address.set("%016llx", entryPoint.offset);
        if (entryPoint != IGOR_INVALID_ADDRESS)
            entry["entryPoint"] = address.to_charp();
        idx++;
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

    HttpServer * s = new HttpServer();
    igor_setup_websocket(s);
    igor_setup_auth(s);

    s->registerAction(new RootAction());
    s->registerAction(new MainAction());
    s->registerAction(new ReloadAction());

    s->registerAction(new HttpActionStatic("data/web-ui/static/", igorStaticURL));

    s->registerAction(new RestDisasmAction());
    s->registerAction(new ListSessionsAction());

    s->setPort(8080);
    s->start();
}
