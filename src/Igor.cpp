#include <Main.h>
#include <Printer.h>
#include <Input.h>
#include <Buffer.h>
#include <Task.h>
#include <StacklessTask.h>
#include <TaskMan.h>
#include <HttpServer.h>

#include "google/protobuf/stubs/common.h"

#include "PELoader.h"
#include "IgorDatabase.h"
#include "IgorLocalSession.h"
#include "IgorHttp.h"

#include "IgorMemory.h"

const igorAddress IGOR_MIN_ADDRESS(0);
const igorAddress IGOR_MAX_ADDRESS((u64) -1);
const igorAddress IGOR_INVALID_ADDRESS((u64) -1);

using namespace Balau;

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

#ifdef USE_WXWIDGETS
#include "../wxIgor/wxIgorShared.h"

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

class wxExit : public AtExit {
public:
    wxExit() : AtExit(12) { }
    void doExit() { wxIgorExit(); }
};
#endif

void MainTask::Do() {
    Printer::log(M_STATUS, "Igor starting up");
    
    Printer::enable(M_INFO | M_STATUS | M_WARNING | M_ERROR | M_ALERT);

    if (argc == 2) {
        IO<Input> reader(new Input(argv[1]));
        reader->open();

        IgorLocalSession * session = new IgorLocalSession();

        c_PELoader PELoader;
        PELoader.loadPE(reader, session);

        reader->close();
        session->loaded(argv[1]);

        TaskMan::registerTask(session);
    }

#ifdef USE_WXWIDGETS
    bool wxStarted = wxIgorStartup(argc, argv);
    IAssert(wxStarted, "wxWidgets couldn't start...");
    TaskMan::registerTask(new wxIdler());
#endif
    
    stopTaskManOnExit(false);

    igor_setup_httpserver();

    yield();
}
