#pragma once

class c_IgorLoader
{
public:
    virtual igor_result load(BFile reader, IgorLocalSession *session) = 0;
};
