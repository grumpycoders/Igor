#include <Main.h>
#include <Printer.h>
#include <Input.h>
#include <Buffer.h>
#include <Task.h>
#include <StacklessTask.h>
#include <TaskMan.h>
#include <HttpServer.h>
#include <LuaTask.h>

#include <getopt.h>

//#include "google/protobuf/stubs/common.h"

#include "Loaders/IgorLoaders.h"

#include "IgorDatabase.h"
#include "IgorHttp.h"
#include "IgorLocalSession.h"
#include "IgorLLVM.h"
#include "IgorMemory.h"
#include "IgorScripting.h"
#include "IgorUsers.h"

FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
    return _iob;
}

using namespace Balau;

LuaMainTask * g_luaTask = NULL;

igorAddress::igorAddress(uint16_t sessionId, igorLinearAddress offset, igor_segment_handle sectionId)
    : m_segmentId(sectionId)
    , m_offset(offset)
    , m_sessionId(sessionId)
{ }

igorAddress::igorAddress(s_igorDatabase * db, igorLinearAddress offset, igor_segment_handle sectionId)
: igorAddress(db ? db->m_sessionId : 0, offset, sectionId)
{ }

igorAddress::igorAddress(IgorSession * session, igorLinearAddress offset, igor_segment_handle sectionId)
    : igorAddress(session ? session->getId() : 0, offset, sectionId)
{ }

IgorSession * igorAddress::getSession() const {
    IgorSession * session = IgorSession::find(m_sessionId);
    IAssert(session, "No session associated with id %u", m_sessionId);
    return session;
}

igorAddress & igorAddress::operator++() {
    IgorSession * session = getSession();
    session->do_incrementAddress(*this, 1);
    session->release();
    return *this;
}

igorAddress & igorAddress::operator--() {
    IgorSession * session = getSession();
    session->do_decrementAddress(*this, 1);
    session->release();
    return *this;
}

igorAddress & igorAddress::operator+=(igorLinearAddress d) {
    IgorSession * session = getSession();
    session->do_incrementAddress(*this, d);
    session->release();
    return *this;
}

igorAddress & igorAddress::operator-=(igorLinearAddress d) {
    IgorSession * session = getSession();
    session->do_decrementAddress(*this, d);
    session->release();
    return *this;
}

igorAddress igorAddress::operator+(igorLinearAddress d) const {
    IgorSession * session = getSession();
    igorAddress ret = session->incrementAddress(*this, d);
    session->release();
    return ret;
}

igorAddress igorAddress::operator-(igorLinearAddress d) const {
    IgorSession * session = getSession();
    igorAddress ret = session->decrementAddress(*this, d);
    session->release();
    return ret;
}

#if 0
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
#endif

#ifdef USE_WXWIDGETS
#include "wxIgor/wxIgorShared.h"

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
}

#else

static void startWX(...) { }

#endif

enum IgorUsers_functions_t {
    LUAIGOR_USERS_GETUSERS,
    LUAIGOR_USERS_ADDUSER,
    LUAIGOR_USERS_CHANGEPASSWORD,
    LUAIGOR_USERS_DELUSER,
};

struct lua_functypes_t IgorUsers_functions[] = {
    { LUAIGOR_USERS_GETUSERS,           "getUsers",             0, 0, { } },
    { LUAIGOR_USERS_ADDUSER,            "addUser",              1, 2, { BLUA_STRING, BLUA_STRING } },
    { LUAIGOR_USERS_CHANGEPASSWORD,     "changePassword",       2, 2, { BLUA_STRING, BLUA_STRING } },
    { LUAIGOR_USERS_DELUSER,            "delUser",              1, 1, { BLUA_STRING } },
    { -1, 0, 0, 0, 0 },
};

struct sLua_IgorUsers : public LuaExecCell {
    static int IgorUsers_proceed_static(Lua & L, int n, int caller);
    void registerMe(Lua & L);
    virtual void run(Lua & L) override { registerMe(L); }
};

int sLua_IgorUsers::IgorUsers_proceed_static(Lua & L, int n, int caller) {
    int r = 0;

    switch (caller) {
    case LUAIGOR_USERS_GETUSERS:
        {
            std::vector<String> users = IgorUsers::getUsers();
            r = 1;
            L.newtable();
            int i = 1;
            for (auto user = users.begin(); user != users.end(); user++) {
                L.push((lua_Number) i++);
                L.push(*user);
                L.settable();
            }
        }
        break;
    case LUAIGOR_USERS_ADDUSER:
        {
            String user = L.tostring(1);
            String password = n == 2 ? L.tostring(2) : "default";
            r = 1;
            L.push(IgorUsers::addUser(user, SRP::generateVerifier(user, password)));
        }
        break;
    case LUAIGOR_USERS_CHANGEPASSWORD:
        r = 1;
        L.push(IgorUsers::changePassword(L.tostring(1), SRP::generateVerifier(L.tostring(1), L.tostring(2))));
        break;
    case LUAIGOR_USERS_DELUSER:
        r = 1;
        L.push(IgorUsers::delUser(L.tostring()));
        break;
    }

    return r;
}

void sLua_IgorUsers::registerMe(Lua & L) {
    CHECK_FUNCTIONS(IgorUsers);
    PUSH_CLASS(IgorUsers);
    PUSH_STATIC(IgorUsers, LUAIGOR_USERS_GETUSERS);
    PUSH_STATIC(IgorUsers, LUAIGOR_USERS_ADDUSER);
    PUSH_STATIC(IgorUsers, LUAIGOR_USERS_CHANGEPASSWORD);
    PUSH_STATIC(IgorUsers, LUAIGOR_USERS_DELUSER);
    PUSH_CLASS_DONE();
}

class LuaPrinterRedirect : public Printer, public LuaExecCell {
    virtual void run(Lua & L) override { setLocal(); }
    virtual void _print(const char * fmt, va_list ap) override {
        // We can redirect Lua's console output here.
        vfprintf(stderr, fmt, ap);
    }
};
/*
class LuaPrinter : public AtStart, public AtExit {
    virtual void doStart() override { m_luaPrinter = new LuaPrinterRedirect; };
    virtual void doExit() override { delete m_luaPrinter; };
    LuaPrinterRedirect * m_luaPrinter;
public:
    LuaPrinterRedirect * operator->() { return m_luaPrinter; }
};

LuaPrinter s_luaPrinter;
*/
void MainTask::Do() {
    Printer::log(M_STATUS, "Igor starting up");

    Printer::enable(M_INFO | M_STATUS | M_WARNING | M_ERROR | M_ALERT);

    c_cpu_factory::initialize();
    c_IgorLoaders::initialize();

    igor_register_llvm();

    {
        g_luaTask = new LuaMainTask();
        TaskMan::registerTask(g_luaTask);
//        s_luaPrinter->exec(g_luaTask);
        IgorScriptingRegister(g_luaTask);
        sLua_IgorUsers luaIgorUsers;
        luaIgorUsers.exec(g_luaTask);
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
    }

    int opt;
    while ((opt = getopt(argc, argv, "hvl:e:")) != EOF) {
        switch (opt) {
        case 'v':
            Printer::enable(M_ALL);
            break;
        case 'l':
            {
                IO<Input> scriptFile(new Input(optarg));
                LuaExecFile script(scriptFile);

                scriptFile->open();
                script.exec(g_luaTask);
            }
            break;
        case 'e':
            {
                LuaExecString cmd(optarg);
                cmd.exec(g_luaTask);
            }
            break;
        case 'h':
        default:
            Printer::print("Usage: %s [-h] [-v] [-l script.lua] [-e \"lua command\"] <file> <file> ...\n", argv[0]);
            throw Exit(opt == 'h' ? 0 : -1);
        }
    }

    for (; optind < argc; optind++) {
        IgorLocalSession * session;
        igor_result r;
        String errorMsg1, errorMsg2;

        std::tie(r, session, errorMsg1, errorMsg2) = IgorLocalSession::loadBinary(argv[optind]);

        if (r != IGOR_SUCCESS) {
            Printer::log(M_WARNING, "Unable to load %s:", argv[optind]);
            Printer::log(M_WARNING, "%s", errorMsg1.to_charp());
            Printer::log(M_WARNING, "%s", errorMsg2.to_charp());
        }
    }

    startWX(argc, argv);

    stopTaskManOnExit(false);

    igor_setup_httpserver();

    yield();
}
