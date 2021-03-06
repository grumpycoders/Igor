#pragma once

#include <Exceptions.h>
#include <Local.h>

#include "cpu/cpuModule.h"

namespace llvm{
    class Target;
    class MCDisassembler;
    class MCInstPrinter;
    class MCRegisterInfo;
    class MCAsmInfo;
    class MCSubtargetInfo;
    class MCInstrInfo;
    class MCContext;
    class MCObjectFileInfo;
}


class c_cpu_x86_llvm : public c_cpu_module
{
public:
    enum e_cpu_type
    {
        X86,
        X64,
    };

    static c_cpu_module* create(s_cpuConstructionFlags*)
    {
        //return new c_cpu_x86_llvm(X64);
		return NULL;
    }

    static void registerCpuModule(std::vector<const s_cpuInfo*>& cpuList);

    c_cpu_x86_llvm(e_cpu_type cpuType);
    ~c_cpu_x86_llvm();

    virtual Balau::String getTag() const override;
    igor_result analyze(s_analyzeState* pState);
    c_cpu_analyse_result* allocateCpuSpecificAnalyseResult();

    igor_result getMnemonic(s_analyzeState* pState, Balau::String& outputString);
    int getNumOperands(s_analyzeState* pState);
    igor_result getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor = false);
    void generateReferences(s_analyzeState* pState);

    struct TLS {
        const llvm::Target* m_pTarget = NULL;

        llvm::MCDisassembler* m_pDisassembler = NULL;
        class IgorLLVMX86InstAnalyzer* m_pAnalyzer = NULL;
        class IgorLLVMX86InstPrinter* m_pPrinter = NULL;
        llvm::MCContext * m_pCtx = NULL;
        llvm::MCObjectFileInfo * m_pMOFI = NULL;

        const llvm::MCRegisterInfo* m_pMRI = NULL;
        const llvm::MCAsmInfo* m_pMAI = NULL;
        const llvm::MCSubtargetInfo* m_pSTI = NULL;
        const llvm::MCInstrInfo* m_pMII = NULL;

        ~TLS();
    };

    Balau::PThreadsTLSFactory<TLS> m_tls;
};
