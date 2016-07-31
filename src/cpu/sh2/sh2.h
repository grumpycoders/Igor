#pragma once

#include "cpu/cpuModule.h"

class c_sh2 : public c_cpu_module
{
public:
    static c_cpu_module* create(s_cpuConstructionFlags*)
    {
        return new c_sh2();
    }

    static void registerCpuModule(std::vector<const s_cpuInfo*>& cpuList);

    c_sh2();
    virtual Balau::String getTag() const override { return "sh2"; }
    virtual igor_result analyze(s_analyzeState* pState) override;
    virtual igor_result printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor = false) override;
    virtual igor_result getMnemonic(s_analyzeState* pState, Balau::String& outputString) override;
    virtual int getNumOperands(s_analyzeState* pState) override;
    virtual igor_result getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor = false) override;
    virtual void generateReferences(s_analyzeState* pState) override;


private:

};
