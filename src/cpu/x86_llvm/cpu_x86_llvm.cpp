#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAtom.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFunction.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCModule.h"
#include "llvm/MC/MCModuleYAML.h"
#include "llvm/MC/MCObjectDisassembler.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCObjectSymbolizer.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCRelocationInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/MachO.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/system_error.h"

#include "cpu_x86_llvm.h"

#include "Target/X86/InstPrinter/X86ATTInstPrinter.h"
#include "Target/X86/InstPrinter/X86IntelInstPrinter.h"

#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "Target/X86/MCTargetDesc/X86BaseInfo.h"

#include "IgorSession.h"

#include <Printer.h>
#include <Exceptions.h>

using namespace llvm;
using namespace Balau;
using namespace X86;

class LLVMMemoryObject : public MemoryObject
{
public:
    LLVMMemoryObject(s_analyzeState* pState)
        : m_state(pState)
    { }

    virtual uint64_t getBase() const override { return 0; }
    virtual uint64_t getExtent() const override { return static_cast<uint64_t>(-1); }

    virtual int readByte(uint64_t address, uint8_t * ptr) const override
    {
        *ptr = m_state->pSession->readU8(igorAddress(m_state->pSession, address, -1));
        return 0;
    }

    virtual int readBytes(uint64_t address, uint64_t size, uint8_t * buf) const override
    {
        for (uint64_t i = 0; i < size; i++)
            readByte(address + i, buf + i);
        return 0;
    }

private:
    s_analyzeState* m_state;
};

struct LLVMStatus {
    igorAddress PCBefore, PCAfter;
    Balau::String instructionText;
    bool gotPCRelImm = false;
    MCOperand PCRelImmOp;
};

class IgorLLVMX86InstAnalyzer : public X86IntelInstPrinter {
public:
    IgorLLVMX86InstAnalyzer(const MCAsmInfo &MAI, const MCInstrInfo &MII, const MCRegisterInfo &MRI)
        : X86IntelInstPrinter(MAI, MII, MRI)
    { }
    void setStatus(LLVMStatus * pStatus) { m_pStatus = pStatus; }
    virtual void printRegName(raw_ostream &OS, unsigned RegNo) const { }
    virtual void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O) { }
    virtual void printSSECC(const MCInst *MI, unsigned Op, raw_ostream &O) { }
    virtual void printAVXCC(const MCInst *MI, unsigned Op, raw_ostream &O) { }
    virtual void printLiteralText(const char * text, raw_ostream &OS) { }
    virtual void printLiteralChar(char c, raw_ostream &OS) { }

    virtual void printMemReference(const MCInst *MI, unsigned OpNo, raw_ostream &O) {

    }
    virtual void printPCRelImm(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
        m_pStatus->gotPCRelImm = true;
        m_pStatus->PCRelImmOp = MI->getOperand(OpNo);
    }
    virtual void printMemOffset(const MCInst *MI, unsigned OpNo, raw_ostream &O) {

    }
    virtual void printInstructionText(const char * text, raw_ostream &OS) {
        m_pStatus->instructionText = text;
    }
private:
    struct LLVMStatus * m_pStatus;
};

class IgorLLVMX86InstPrinter : public X86IntelInstPrinter {
public:
    IgorLLVMX86InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII, const MCRegisterInfo &MRI)
        : X86IntelInstPrinter(MAI, MII, MRI)
    { }
    void setStatus(LLVMStatus * pStatus) { m_pStatus = pStatus; }
    virtual void printInst(const MCInst *MI, raw_ostream &OS, StringRef Annot) {
        X86IntelInstPrinter::printInst(MI, OS, Annot);
    }
    virtual void printRegName(raw_ostream &OS, unsigned RegNo) const {
        X86IntelInstPrinter::printRegName(OS, RegNo);
    }
    virtual void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
        X86IntelInstPrinter::printOperand(MI, OpNo, O);
    }
    virtual void printMemReference(const MCInst *MI, unsigned Op, raw_ostream &O) {
        X86IntelInstPrinter::printMemReference(MI, Op, O);
    }
    virtual void printSSECC(const MCInst *MI, unsigned Op, raw_ostream &O) {
        X86IntelInstPrinter::printSSECC(MI, Op, O);
    }
    virtual void printAVXCC(const MCInst *MI, unsigned Op, raw_ostream &O) {
        X86IntelInstPrinter::printAVXCC(MI, Op, O);
    }
    virtual void printPCRelImm(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
        const MCOperand &Op = MI->getOperand(OpNo);
        if (Op.isImm()) {
            int64_t imm = Op.getImm();
            igorAddress abs = m_pStatus->PCAfter + imm;
            O << formatHex(abs.offset);
        } else {
            assert(Op.isExpr() && "unknown pcrel immediate operand");
            // If a symbolic branch target was added as a constant expression then print
            // that address in hex.
            const MCConstantExpr *BranchTarget = dyn_cast<MCConstantExpr>(Op.getExpr());
            int64_t Address;
            if (BranchTarget && BranchTarget->EvaluateAsAbsolute(Address)) {
                O << formatHex((uint64_t)Address);
            } else {
                // Otherwise, just print the expression.
                O << *Op.getExpr();
            }
        }
    }
    virtual void printMemOffset(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
        X86IntelInstPrinter::printMemOffset(MI, OpNo, O);
    }
    virtual void printInstructionText(const char * text, raw_ostream &OS) {
        int ns = 8;
        while (*text) {
            char c = *text++;
            if (c == '\t') {
                for (int i = 0; i < ns; i++)
                    OS << ' ';
                ns = 8;
            } else {
                if (--ns == 0)
                    ns = 8;
                OS << c;
            }
        }
        X86IntelInstPrinter::printInstructionText(text, OS);
    }
    virtual void printLiteralText(const char * text, raw_ostream &OS) {
        X86IntelInstPrinter::printLiteralText(text, OS);
    }
    virtual void printLiteralChar(char c, raw_ostream &OS) {
        X86IntelInstPrinter::printLiteralChar(c, OS);
    }
private:
    struct LLVMStatus * m_pStatus;
};

Balau::String c_cpu_x86_llvm::getTag() const
{
    return "igor_x86_llvm";
}

c_cpu_x86_llvm::c_cpu_x86_llvm(e_cpu_type cpuType)
{
    std::string error;
    std::string tripleName;

    Triple triple;

    switch (cpuType) {
    case c_cpu_x86_llvm::X86:
        triple.setArch(Triple::x86);
        break;
    case c_cpu_x86_llvm::X64:
        triple.setArch(Triple::x86_64);
        break;
    default:
        break;
    }

    triple.setVendor(Triple::PC);
    m_pTarget = TargetRegistry::lookupTarget("", triple, error);
    tripleName = triple.getTriple();

    switch (cpuType) {
    case c_cpu_x86_llvm::X86:
        IAssert(m_pTarget == &TheX86_32Target, "We didn't get the proper target for x86 32");
        break;
    case c_cpu_x86_llvm::X64:
        IAssert(m_pTarget == &TheX86_32Target, "We didn't get the proper target for x86 32");
        break;
    }

    m_pMRI = m_pTarget->createMCRegInfo(tripleName);
    m_pMAI = m_pTarget->createMCAsmInfo(*m_pMRI, tripleName);
    m_pSTI = m_pTarget->createMCSubtargetInfo(tripleName, "", "");
    m_pMII = m_pTarget->createMCInstrInfo();

    m_pDisassembler = m_pTarget->createMCDisassembler(*m_pSTI);
    m_pAnalyzer = new IgorLLVMX86InstAnalyzer(*m_pMAI, *m_pMII, *m_pMRI);
    m_pPrinter = new IgorLLVMX86InstPrinter(*m_pMAI, *m_pMII, *m_pMRI);
}

c_cpu_x86_llvm::~c_cpu_x86_llvm()
{
    delete m_pMRI;
    delete m_pMAI;
    delete m_pSTI;
    delete m_pMII;
    delete m_pDisassembler;
    delete m_pAnalyzer;
    delete m_pPrinter;
}

igor_result c_cpu_x86_llvm::analyze(s_analyzeState * pState)
{
    pState->m_cpu_analyse_result->m_startOfInstruction = pState->m_PC;
    pState->m_cpu_analyse_result->m_instructionSize = 0;

    MCInst inst;
    uint64_t size;

    LLVMMemoryObject memoryObject(pState);
    raw_null_ostream ns1, ns2;

    MCDisassembler::DecodeStatus result = m_pDisassembler->getInstruction(inst, size, memoryObject, pState->m_PC.offset, ns1, ns2);

    if (result != MCDisassembler::Success)
        return IGOR_FAILURE;

    pState->m_cpu_analyse_result->m_instructionSize = size;

    const MCInstrDesc & desc = m_pMII->get(inst.getOpcode());
    uint64_t tsflags = desc.TSFlags;

    if (desc.isUnconditionalBranch())
        pState->m_analyzeResult = stop_analysis;

    LLVMStatus llvmStatus;

    m_pAnalyzer->setStatus(&llvmStatus);
    m_pAnalyzer->printInst(&inst, ns1, "");
    pState->m_PC += pState->m_cpu_analyse_result->m_instructionSize;

    if (llvmStatus.gotPCRelImm) {
        const MCOperand & Op = llvmStatus.PCRelImmOp;
        if (Op.isImm()) {
            int64_t imm = Op.getImm();
            igorAddress immAbs = pState->m_PC;
            immAbs += imm;

            pState->pSession->add_code_analysis_task(immAbs);
        } else {
            IAssert(Op.isExpr(), "unknown pcrel immediate operand");
            const MCConstantExpr * branchTarget = dyn_cast<MCConstantExpr>(Op.getExpr());
            int64_t address;
            if (branchTarget && branchTarget->EvaluateAsAbsolute(address)) {
                // yay Address
            } else {
                const MCExpr * expr = Op.getExpr();
            }
        }
    }

    return IGOR_SUCCESS;
}

igor_result c_cpu_x86_llvm::printInstruction(s_analyzeState * pState, Balau::String& outputString, bool bUseColor)
{
    MCInst inst;
    uint64_t size;

    LLVMMemoryObject memoryObject(pState);
    raw_null_ostream ns1, ns2;

    MCDisassembler::DecodeStatus result = m_pDisassembler->getInstruction(inst, size, memoryObject, pState->m_cpu_analyse_result->m_startOfInstruction.offset, ns1, ns2);

    if (result != MCDisassembler::Success)
        return IGOR_FAILURE;

    const MCInstrDesc & desc = m_pMII->get(inst.getOpcode());
    uint64_t tsflags = desc.TSFlags;

    std::string outStr;
    raw_string_ostream out(outStr);

    LLVMStatus llvmStatus;
    llvmStatus.PCBefore = pState->m_cpu_analyse_result->m_startOfInstruction;
    llvmStatus.PCAfter = llvmStatus.PCBefore + size;

    m_pPrinter->setStatus(&llvmStatus);
    m_pPrinter->printInst(&inst, out, "");

    out.flush();
    Balau::String instruction = outStr;
    instruction.do_replace_all('\t', ' ');
    instruction.do_trim();
    outputString += instruction;

    return IGOR_SUCCESS;
}

igor_result c_cpu_x86_llvm::getMnemonic(s_analyzeState * pState, Balau::String& outputString)
{
    return IGOR_FAILURE;
}

int c_cpu_x86_llvm::getNumOperands(s_analyzeState * pState)
{
    return 0;
}

igor_result c_cpu_x86_llvm::getOperand(s_analyzeState * pState, int operandIndex, Balau::String& outputString, bool bUseColor)
{
    return IGOR_FAILURE;
}

void c_cpu_x86_llvm::generateReferences(s_analyzeState * pState)
{

}
