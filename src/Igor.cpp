#include <Main.h>
#include <Printer.h>
#include <Input.h>
#include <Buffer.h>
#include <Task.h>
#include <StacklessTask.h>
#include <TaskMan.h>
#include <HttpServer.h>

#include "PELoader.h"
#include "IgorDatabase.h"
#include "IgorHttp.h"

using namespace Balau;

#ifdef USE_WXWIDGETS
#include "../wxIgor/wxIgorShared.h"

class wxIdler : public StacklessTask {
    void Do() throw (GeneralException) {
        for (;;) {
            auto r = wxIgorLoop();
            if (r.first)
                throw Exit(r.second);
            m_evt.resetMaybe();
            m_evt.set(0.01);
            waitFor(&m_evt);
            yield();
        }
    }
    const char * getName() const { return "wxIdler"; }
    Events::Timeout m_evt;
};
#endif

void MainTask::Do() {
    Printer::log(M_STATUS, "Igor starting up");
    
    Printer::enable(M_ALL);

#ifdef USE_WXWIDGETS
    bool wxStarted = wxIgorStartup(argc, argv);
    IAssert(wxStarted, "wxWidgets couldn't start...");
    TaskMan::registerTask(new wxIdler());
#endif
    
    if (argc > 2)
        return;

    stopTaskManOnExit(false);

    if (argc == 2) {
        IO<Input> file(new Input(argv[1]));
        file->open();

        size_t size = file->getSize();
        uint8_t * buffer = (uint8_t *)malloc(size);
        file->forceRead(buffer, size);
        IO<Buffer> reader(new Buffer(buffer, file->getSize()));

        IgorAnalysis * analysis = new IgorAnalysis();

		s_igorDatabase * db = new s_igorDatabase;

        c_PELoader PELoader;
		PELoader.loadPE(db, reader, analysis);

        reader->close();
        free(buffer);

        TaskMan::registerTask(analysis);
    }

    igor_setup_httpserver();

    yield();
}
