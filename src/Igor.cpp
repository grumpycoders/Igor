#include <Main.h>
#include <Printer.h>
#include <Input.h>
#include <Buffer.h>
#include <Task.h>
#include <TaskMan.h>
#include <HttpServer.h>

#include "PELoader.h"
#include "IgorDatabase.h"
#include "IgorHttp.h"

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Igor starting up");
    
    Printer::enable(M_ALL);
    
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

        c_PELoader PELoader;
        PELoader.loadPE(reader, analysis);

        reader->close();
        free(buffer);

        TaskMan::registerTask(analysis);
    }

    igor_setup_httpserver();

    yield();
}
