#pragma once

class c_IgorLoader
{
public:
    // load binary
    virtual igor_result load(BFile reader, IgorLocalSession *session) = 0;
};
