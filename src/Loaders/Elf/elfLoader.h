#pragma once

#include "Igor.h"
#include "IgorAPI.h"

struct s_igorDatabase;
class IgorSession;

class c_elfLoader
{
public:
    igor_result load(BFile reader, IgorLocalSession *);
};