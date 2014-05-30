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
        IO<Input> reader(new Input(argv[1]));
        reader->open();

        IgorLocalSession * session = new IgorLocalSession();

		if (strstr(argv[1], ".exe"))
		{
			c_PELoader PELoader;
			PELoader.loadPE(reader, session);
		}
		else if (strstr(argv[1], ".elf"))
		{
			c_elfLoader elfLoader;
			elfLoader.load(reader, session);
		}

        reader->close();
        session->loaded(argv[1]);

        TaskMan::registerTask(session);
    }

    IAssert(SRP::selfTest(), "SRP6a failed its self tests");

    startWX(argc, argv);
    
    stopTaskManOnExit(false);

    igor_setup_httpserver();

    yield();
}
