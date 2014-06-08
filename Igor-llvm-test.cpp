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

#include <Printer.h>

using namespace llvm;
using namespace Balau;

class LLVMTestMemoryObject : public MemoryObject {
    virtual uint64_t getBase() const override { return 0; }
    virtual uint64_t getExtent() const override { return static_cast<uint64_t>(-1); }
    virtual int readByte(uint64_t address, uint8_t * ptr) const override { *ptr = 0x90; return 0; }
    virtual int readBytes(uint64_t address, uint64_t size, uint8_t *buf) const override { memset(buf, 0x90, size); return 0; }
};

void test_llvm() {
    // Initialize targets and assembly printers/parsers.
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllDisassemblers();

    std::string error;
    std::string tripleName;

    Triple triple;
    triple.setTriple(Triple::normalize("i686-pc-win32"));
    const Target * target = TargetRegistry::lookupTarget("", triple, error);
    tripleName = triple.getTriple();

    OwningPtr<const MCRegisterInfo> MRI(target->createMCRegInfo(tripleName));
    OwningPtr<const MCAsmInfo> AsmInfo(target->createMCAsmInfo(*MRI, tripleName));
    OwningPtr<const MCSubtargetInfo> STI(target->createMCSubtargetInfo(tripleName, "", ""));
    OwningPtr<const MCInstrInfo> MII(target->createMCInstrInfo());
    OwningPtr<MCDisassembler> DisAsm(target->createMCDisassembler(*STI));

    int AsmPrinterVariant = AsmInfo->getAssemblerDialect();
    OwningPtr<MCInstPrinter> IP(target->createMCInstPrinter(AsmPrinterVariant, *AsmInfo, *MII, *MRI, *STI));

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

    std::string outStr2, cmtStr2;
    raw_string_ostream out2(outStr2), cmt2(cmtStr2);

    IP->setCommentStream(cmt2);
    IP->printInst(&inst, out2, "");
    out2.flush();
    cmt2.flush();
    Printer::log(M_STATUS, "print inst - out: %s", outStr2.c_str());
    Printer::log(M_STATUS, "print inst - cmt: %s", cmtStr2.c_str());
}
