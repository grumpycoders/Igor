#pragma once

#include <malloc.h>

#if defined(_MSC_VER) && defined(_DEBUG)

#include <crtdbg.h>

#undef malloc
#undef free
#define malloc(x) _malloc_dbg(x, _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(x) _free_dbg(x, _NORMAL_BLOCK)

#define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)

#endif