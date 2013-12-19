#include <Main.h>
#include <Printer.h>
#include <Input.h>
#include <BStream.h>

#include "PELoader.h"
#include "IgorDatabase.h"


using namespace Balau;

void MainTask::Do() {
	Printer::log(M_STATUS, "Igor starting up");

	if (argc < 2)
		return;

	IO<Input> reader(new Input(argv[1]));
	reader->open();

	c_PELoader PELoader;
	PELoader.loadPE(reader);

	igor_analysis_run();
}
