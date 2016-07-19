#pragma once

#include <tuple>
#include "IgorLocalSession.h"

std::tuple<igor_result, IgorLocalSession *, Balau::String, Balau::String> IgorAsyncLoadBinary(const char * filename);

void startIgorAsyncWorker();
void stopIgorAsyncWorker();
