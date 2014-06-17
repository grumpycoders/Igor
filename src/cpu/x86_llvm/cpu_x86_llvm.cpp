#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAtom.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler.h"
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

class LLVMTestMemoryObject : public MemoryObject
{
public:
    LLVMTestMemoryObject(s_analyzeState* pState)
    {
        m_state = pState;
    }

    virtual uint64_t getBase() const override
    {
        return 0;
    }

    virtual uint64_t getExtent() const override
    {
        return static_cast<uint64_t>(-1);
    }

    virtual int readByte(uint64_t address, uint8_t * ptr) const override
    {
        *ptr = m_state->pSession->readU8(igorAddress(m_state->pSession, address, -1));
        return 0;
    }

    virtual int readBytes(uint64_t address, uint64_t size, uint8_t *buf) const override
    {
        memset(buf, 0x90, size);
        return 0;
    }

    s_analyzeState* m_state;
};

class IgorLLVMX86InstPrinter : public X86IntelInstPrinter {
public:
    IgorLLVMX86InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII, const MCRegisterInfo &MRI)
        : X86IntelInstPrinter(MAI, MII, MRI)
    { }
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
        X86IntelInstPrinter::printPCRelImm(MI, OpNo, O);
    }
    virtual void printMemOffset(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
        X86IntelInstPrinter::printMemOffset(MI, OpNo, O);
    }
    virtual void printInstructionText(const char * text, raw_ostream &OS) {
        X86IntelInstPrinter::printInstructionText(text, OS);
    }
    virtual void printLiteralText(const char * text, raw_ostream &OS) {
        X86IntelInstPrinter::printLiteralText(text, OS);
    }
    virtual void printLiteralChar(char c, raw_ostream &OS) {
        X86IntelInstPrinter::printLiteralChar(c, OS);
    }
};

void test_x86_llvm() {
    /*
    std::string error;
    std::string tripleName;

    Triple triple;
    triple.setArch(Triple::x86);
    triple.setVendor(Triple::PC);
    const Target * target = TargetRegistry::lookupTarget("", triple, error);
    tripleName = triple.getTriple();

    IAssert(target == &TheX86_32Target, "We didn't get the proper target for x86 32");

    OwningPtr<const MCRegisterInfo>  MRI(target->createMCRegInfo(tripleName));
    OwningPtr<const MCAsmInfo>       MAI(target->createMCAsmInfo(*MRI, tripleName));
    OwningPtr<const MCSubtargetInfo> STI(target->createMCSubtargetInfo(tripleName, "", ""));
    OwningPtr<const MCInstrInfo>     MII(target->createMCInstrInfo());

    OwningPtr<MCDisassembler> DisAsm(target->createMCDisassembler(*STI));
    OwningPtr<MCInstPrinter>  IP(new IgorLLVMX86InstPrinter(*MAI, *MII, *MRI));

    MCInst inst;
    uint64_t size;

    LLVMTestMemoryObject memoryObject;
    std::string outStr1, cmtStr1;
    raw_string_ostream out1(outStr1), cmt1(cmtStr1);

    DisAsm->getInstruction(inst, size, memoryObject, 0, out1, cmt1);
    out1.flush();
    cmt1.flush();
    Printer::log(M_STATUS, "get inst - out: %s", outStr1.c_str());
    Printer::log(M_STATUS, "get inst - cmt: %s", cmtStr1.c_str());

    const MCInstrDesc & desc = MII->get(inst.getOpcode());
    uint64_t tsflags = desc.TSFlags;

    Printer::log(M_STATUS, "Instruction flags: %" PRIX64, tsflags);

    if (tsflags & X86II::FS)
        Printer::log(M_STATUS, "Has FS: prefix");

    if (tsflags & X86II::GS)
        Printer::log(M_STATUS, "Has GS: prefix");

    if (desc.isCall())
        Printer::log(M_STATUS, "Is a call");

    std::string outStr2, cmtStr2;
    raw_string_ostream out2(outStr2), cmt2(cmtStr2);

    IP->setCommentStream(cmt2);
    IP->printInst(&inst, out2, "");
    out2.flush();
    cmt2.flush();
    Printer::log(M_STATUS, "print inst - out: %s", outStr2.c_str());
    Printer::log(M_STATUS, "print inst - cmt: %s", cmtStr2.c_str());
    */
}

Balau::String c_cpu_x86_llvm::getTag() const
{
    return "igor_x86_llvm";
}

c_cpu_x86_llvm::c_cpu_x86_llvm(e_cpu_type cpuType)
{
    std::string error;
    std::string tripleName;

    Triple triple;

    switch (cpuType)
    {
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

    //IAssert(target == &TheX86_32Target, "We didn't get the proper target for x86 32");

    m_pMRI = m_pTarget->createMCRegInfo(tripleName);
    m_pMAI = m_pTarget->createMCAsmInfo(*m_pMRI, tripleName);
    m_pSTI = m_pTarget->createMCSubtargetInfo(tripleName, "", "");
    m_pMII = m_pTarget->createMCInstrInfo();

    m_pDisassembler = m_pTarget->createMCDisassembler(*m_pSTI);
    m_pPrinter = new IgorLLVMX86InstPrinter(*m_pMAI, *m_pMII, *m_pMRI);
}

c_cpu_x86_llvm::~c_cpu_x86_llvm()
{

}

igor_result c_cpu_x86_llvm::analyze(s_analyzeState* pState)
{
    pState->m_cpu_analyse_result->m_startOfInstruction = pState->m_PC;
    pState->m_cpu_analyse_result->m_instructionSize = 0;

    MCInst inst;
    uint64_t size;

    LLVMTestMemoryObject memoryObject(pState);
    std::string outStr1, cmtStr1;
    raw_string_ostream out1(outStr1), cmt1(cmtStr1);

    MCDisassembler::DecodeStatus result = m_pDisassembler->getInstruction(inst, size, memoryObject, pState->m_PC.offset, out1, cmt1);

    if (result != MCDisassembler::Success)
    {
        return IGOR_FAILURE;
    }

    pState->m_cpu_analyse_result->m_instructionSize = size;

    out1.flush();
    cmt1.flush();
    //Printer::log(M_STATUS, "get inst - out: %s", outStr1.c_str());
    //Printer::log(M_STATUS, "get inst - cmt: %s", cmtStr1.c_str());

    const MCInstrDesc & desc = m_pMII->get(inst.getOpcode());
    uint64_t tsflags = desc.TSFlags;

    //Printer::log(M_STATUS, "Instruction flags: %" PRIX64, tsflags);

    if (tsflags & X86II::FS)
        Printer::log(M_STATUS, "Has FS: prefix");

    if (tsflags & X86II::GS)
        Printer::log(M_STATUS, "Has GS: prefix");

    if (desc.isCall())
        Printer::log(M_STATUS, "Is a call");

    /*
    std::string outStr2, cmtStr2;
    raw_string_ostream out2(outStr2), cmt2(cmtStr2);
   
    m_pPrinter->setCommentStream(cmt2);
    m_pPrinter->printInst(&inst, out2, "");
    out2.flush();
    cmt2.flush();
    Printer::log(M_STATUS, "print inst - out: %s", outStr2.c_str());
    Printer::log(M_STATUS, "print inst - cmt: %s", cmtStr2.c_str());
    */
    pState->m_PC += pState->m_cpu_analyse_result->m_instructionSize;

    return IGOR_SUCCESS;
}

igor_result c_cpu_x86_llvm::printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor)
{
    MCInst inst;
    uint64_t size;

    LLVMTestMemoryObject memoryObject(pState);
    std::string outStr1, cmtStr1;
    raw_string_ostream out1(outStr1), cmt1(cmtStr1);

    MCDisassembler::DecodeStatus result = m_pDisassembler->getInstruction(inst, size, memoryObject, pState->m_cpu_analyse_result->m_startOfInstruction.offset, out1, cmt1);

    if (result != MCDisassembler::Success)
    {
        return IGOR_FAILURE;
    }

    out1.flush();
    cmt1.flush();
    //Printer::log(M_STATUS, "get inst - out: %s", outStr1.c_str());
    //Printer::log(M_STATUS, "get inst - cmt: %s", cmtStr1.c_str());

    const MCInstrDesc & desc = m_pMII->get(inst.getOpcode());
    uint64_t tsflags = desc.TSFlags;

    //Printer::log(M_STATUS, "Instruction flags: %" PRIX64, tsflags);

    if (tsflags & X86II::FS)
        Printer::log(M_STATUS, "Has FS: prefix");

    if (tsflags & X86II::GS)
        Printer::log(M_STATUS, "Has GS: prefix");

    if (desc.isCall())
        Printer::log(M_STATUS, "Is a call");

    std::string outStr2, cmtStr2;
    raw_string_ostream out2(outStr2), cmt2(cmtStr2);

    m_pPrinter->setCommentStream(cmt2);
    m_pPrinter->printInst(&inst, out2, "");
    out2.flush();
    cmt2.flush();
    //Printer::log(M_STATUS, "print inst - out: %s", outStr2.c_str());
    //Printer::log(M_STATUS, "print inst - cmt: %s", cmtStr2.c_str());

    outputString.append("%s", outStr2.c_str());

    while (outputString.strchr('\t') != -1)
    {
        outputString[outputString.strchr('\t')] = ' ';
    }

    return IGOR_SUCCESS;
}

igor_result c_cpu_x86_llvm::getMnemonic(s_analyzeState* pState, Balau::String& outputString)
{
    return IGOR_FAILURE;
}

int c_cpu_x86_llvm::getNumOperands(s_analyzeState* pState)
{
    return 0;
}

igor_result c_cpu_x86_llvm::getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor)
{
    return IGOR_FAILURE;
}

void c_cpu_x86_llvm::generateReferences(s_analyzeState* pState)
{

}
