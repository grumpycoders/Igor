#include <Main.h>
#include <Printer.h>
#include <Input.h>
#include <BStream.h>
#include <Task.h>
#include <TaskMan.h>
#include <HttpServer.h>

#include "PELoader.h"
#include "IgorDatabase.h"


using namespace Balau;

void MainTask::Do() {
	Printer::log(M_STATUS, "Igor starting up");

	if (argc < 2)
		return;

	Printer::enable(M_INFO);

	IO<Input> reader(new Input(argv[1]));
	reader->open();

	c_PELoader PELoader;
	PELoader.loadPE(reader);

    Events::TaskEvent evtAnalysis;
    Task * analysis = TaskMan::registerTask(new IgorAnalysis(), &evtAnalysis);
    waitFor(&evtAnalysis);

    HttpServer * s = new HttpServer();
    s->setPort(8080);
    s->setLocal("localhost");
    s->start();

    yield();
}
