#pragma once

#include <string>
#include <Exceptions.h>

class LLVMException : public Balau::GeneralException {
public:
    LLVMException(const std::string & msg) : GeneralException(msg) { }
};

void igor_register_llvm();
