#include <Main.h>
#include <Printer.h>
#include <Input.h>
#include <Buffer.h>
#include <Task.h>
#include <StacklessTask.h>
#include <TaskMan.h>
#include <HttpServer.h>
#include <LuaTask.h>
#include <IgorUsers.h>

#include "google/protobuf/stubs/common.h"

#include "Loaders/PE/PELoader.h"
#include "Loaders/Dmp/dmpLoader.h"
#include "Loaders/Elf/elfLoader.h"

#include "IgorDatabase.h"
#include "IgorLocalSession.h"
#include "IgorHttp.h"

#include "IgorMemory.h"

const igorAddress IGOR_MIN_ADDRESS(0);
const igorAddress IGOR_MAX_ADDRESS((u64) -1);
const igorAddress IGOR_INVALID_ADDRESS((u64) -1);

using namespace Balau;

LuaMainTask * g_luaTask = NULL;

class GoogleProtoBufs : public AtStart, public AtExit {
public:
      GoogleProtoBufs() : AtStart(10), AtExit(10) { }
    virtual void doStart() override {
        GOOGLE_PROTOBUF_VERIFY_VERSION;
    }
    virtual void doExit() override {
        google::protobuf::ShutdownProtobufLibrary();
    }
};

static GoogleProtoBufs gprotobufs;

#ifdef USE_WXWIDGETS
#include "wxIgor/wxIgorShared.h"

class wxIdler : public Task {
    void Do() throw (GeneralException) {
        for (;;) {
            auto r = wxIgorLoop();
            if (r.first)
                throw Exit(r.second);
            m_evt.resetMaybe();
            m_evt.set(0.05);
            waitFor(&m_evt);
            yield();
        }
    }
    const char * getName() const { return "wxIdler"; }
    Events::Timeout m_evt;
};

bool s_wxStarted = false;

class wxExit : public AtExit {
public:
    wxExit() : AtExit(12) { }
    void doExit() { if (s_wxStarted) wxIgorExit(); }
};

static wxExit wxexit;

static void startWX(int argc, char ** argv) {
    s_wxStarted = wxIgorStartup(argc, argv);
    IAssert(s_wxStarted, "wxWidgets couldn't start...");
    TaskMan::registerTask(new wxIdler());
}

#else

static void startWX(...) { }

#endif

void MainTask::Do() {
    Printer::log(M_STATUS, "Igor starting up");
    
    Printer::enable(M_INFO | M_STATUS | M_WARNING | M_ERROR | M_ALERT);

    g_luaTask = new LuaMainTask();
    TaskMan::registerTask(g_luaTask);
    LuaExecString strLuaHello(
        "local jitstatus = { jit.status() } "
        "local features = 'Features:' "
        "for i = 2, #jitstatus do "
        "  features = features .. ' ' .. jitstatus[i] "
        "end "
        "print(jit.version .. ' running on ' .. jit.os .. '/' .. jit.arch .. '.') "
        "print(features .. '.') "
        "print('Lua engine up and running, JIT compiler is ' .. (jitstatus[1] and 'enabled' or 'disabled') .. '.') "
    );
    strLuaHello.exec(g_luaTask);

    if (argc == 2) {
        IgorLocalSession * session;
        igor_result r;
        String errorMsg1, errorMsg2;
        
        std::tie(r, session, errorMsg1, errorMsg2) = IgorLocalSession::loadBinary(argv[1]);

        if (r != IGOR_SUCCESS) {
            Printer::log(M_WARNING, "Unable to load %s:", argv[1]);
            Printer::log(M_WARNING, "%s", errorMsg1.to_charp());
            Printer::log(M_WARNING, "%s", errorMsg2.to_charp());
        }
    }

    startWX(argc, argv);
    
    stopTaskManOnExit(false);

    igor_setup_httpserver();

    yield();
}
