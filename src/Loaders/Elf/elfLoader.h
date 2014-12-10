#pragma once

#include "Igor.h"
#include "IgorAPI.h"
#include "Loaders/IgorLoader.h"

struct s_igorDatabase;
class IgorSession;

class c_elfLoader : public c_IgorLoader
{
public:
    igor_result load(BFile reader, IgorLocalSession *);
};
