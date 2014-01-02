#pragma once

void igor_setup_httpserver();

namespace Balau { class HttpServer; }

void igor_setup_websocket(Balau::HttpServer *);