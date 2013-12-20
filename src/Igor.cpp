#include <Main.h>
#include <Printer.h>
#include <Input.h>
#include <BStream.h>
#include <Task.h>
#include <TaskMan.h>

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

    Events::TaskEvent evt;
    Task * analysis = TaskMan::registerTask(new IgorAnalysis(), &evt);
    waitFor(&evt);
    yield();
}
