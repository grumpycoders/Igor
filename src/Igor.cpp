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

	if (argc < 2)
		return;

	Printer::enable(M_ALL);

	IO<Input> file(new Input(argv[1]));
	file->open();

    size_t size = file->getSize();
    uint8_t * buffer = (uint8_t *) malloc(size);
    file->forceRead(buffer, size);
    IO<Buffer> reader(new Buffer(buffer, file->getSize()));

	c_PELoader PELoader;
	PELoader.loadPE(reader);

	reader->close();
	free(buffer);

    Events::TaskEvent evtAnalysis;
    Task * analysis = TaskMan::registerTask(new IgorAnalysis(), &evtAnalysis);
    waitFor(&evtAnalysis);

    igor_setup_httpserver();

    yield();
}
