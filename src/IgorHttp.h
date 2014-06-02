#pragma once

#include <map>
#include <list>
#include <atomic>
#include <memory>

#include <BString.h>
#include <Threads.h>
#include <StacklessTask.h>
#include "IgorUsers.h"

namespace Balau { class HttpServer; }

void igor_setup_httpserver();
void igor_setup_auth(Balau::HttpServer *);
void igor_setup_websocket(Balau::HttpServer *);
