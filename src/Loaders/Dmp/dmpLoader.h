#pragma once

#include "Igor.h"
#include "IgorAPI.h"
#include "Loaders/IgorLoader.h"

struct s_igorDatabase;
class IgorSession;

class c_dmpLoader : public c_IgorLoader
{
public:
    static bool isSupported(Balau::String& filename)
    {
        if (filename.strstr(".dmp") != -1)
            return true;

        return false;
    }
    static c_IgorLoader* createLoader()
    {
        return new c_dmpLoader;
    }

    igor_result load(BFile reader, IgorLocalSession *);
};
