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

#ifndef IGOR_DOJO_PATH
#define IGOR_DOJO_PATH "//ajax.googleapis.com/ajax/libs/dojo/1.9.2"
#endif

#ifndef IGOR_ROOT
#define IGOR_ROOT "/web-ui"
#endif

#ifndef IGOR_STATIC_ROOT
#define IGOR_STATIC_ROOT "/web-ui/static"
#endif

#ifndef IGOR_DYN_ROOT
#define IGOR_DYN_ROOT "/web-ui/dyn"
#endif
