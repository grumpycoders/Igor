#pragma once

#include "cpu/cpuModule.h"
#include "Exceptions.h"

namespace llvm{
    class Target;
    class MCDisassembler;
    class MCInstPrinter;
    class MCRegisterInfo;
    class MCAsmInfo;
    class MCSubtargetInfo;
    class MCInstrInfo;
}


class c_x86_llvm_analyse_result : public c_cpu_analyse_result
{

};

class c_cpu_x86_llvm : public c_cpu_module
{
public:
    enum e_cpu_type
    {
        X86,
        X64,
    };

    c_cpu_x86_llvm(e_cpu_type cpuType);
    ~c_cpu_x86_llvm();

    virtual Balau::String getTag() const override;
    igor_result analyze(s_analyzeState* pState);
    igor_result printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor = false);
    c_cpu_analyse_result* allocateCpuSpecificAnalyseResult(){ return new c_x86_llvm_analyse_result; }

    igor_result getMnemonic(s_analyzeState* pState, Balau::String& outputString);
    int getNumOperands(s_analyzeState* pState);
    igor_result getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor = false);
    void generateReferences(s_analyzeState* pState);

    const llvm::Target* m_pTarget;

    llvm::MCDisassembler* m_pDisassembler = NULL;
    class IgorLLVMX86InstAnalyzer* m_pAnalyzer = NULL;
    class IgorLLVMX86InstPrinter* m_pPrinter = NULL;

    const llvm::MCRegisterInfo* m_pMRI = NULL;
    const llvm::MCAsmInfo* m_pMAI = NULL;
    const llvm::MCSubtargetInfo* m_pSTI = NULL;
    const llvm::MCInstrInfo* m_pMII = NULL;
};
