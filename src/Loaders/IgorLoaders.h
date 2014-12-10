#pragma once
#include <vector>
#include <BString.h>

using namespace Balau;

#include "Igor.h"
#include "IgorAPI.h"

class c_IgorLoader;

class c_IgorLoaders
{
public:
    static void initialize();
    static igor_result load(String fileName, BFile reader, IgorLocalSession *session);

    struct s_registeredLoader
    {
        bool(*m_isSupported)(String& filename);
        c_IgorLoader*(*m_createLoader)();
    };

    static std::vector<s_registeredLoader> m_loaders;
};
