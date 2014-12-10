#pragma once

#include "Igor.h"
#include "IgorAPI.h"
#include "Loaders/IgorLoader.h"

struct s_igorDatabase;
class IgorSession;

class c_elfLoader : public c_IgorLoader
{
public:
    static bool isSupported(Balau::String& filename)
    {
        if (filename.strstr(".elf") != -1)
            return true;

        return false;
    }
    static c_IgorLoader* createLoader()
    {
        return new c_elfLoader;
    }

    igor_result load(BFile reader, IgorLocalSession *);
};
