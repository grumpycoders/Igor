#pragma once

#include "Igor.h"
#include "IgorAPI.h"
#include "Loaders/IgorLoader.h"

namespace llvm{
    namespace object{
        class ObjectFile;
    }
}

class c_LLVMLoader : public c_IgorLoader
{
public:
    igor_result load(BFile reader, IgorLocalSession *session);

private:
    igor_result loadObject(llvm::object::ObjectFile* o);

    IgorLocalSession* m_session;
};
