#include <stdint.h>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAnalysis/MCAtom.h"
#include "llvm/MC/MCAnalysis/MCFunction.h"
#include "llvm/MC/MCAnalysis/MCModule.h"
#include "llvm/MC/MCAnalysis/MCModuleYAML.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
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
    {
		PrintImmHex = true;
	}
    void setStatus(LLVMStatus * pStatus) { m_pStatus = pStatus; }
    virtual void printInst(const MCInst *MI, raw_ostream &OS, StringRef Annot)
	{
		const MCInstrDesc &Desc = MII.get(MI->getOpcode());
		bool isFlowControl = Desc.mayAffectControlFlow(*MI, MRI);

		if(isFlowControl)
			OS << c_cpu_module::startColor(c_cpu_module::MNEMONIC_FLOW_CONTROL, true);

        X86IntelInstPrinter::printInst(MI, OS, Annot);

		if (isFlowControl)
			OS << c_cpu_module::finishColor(c_cpu_module::MNEMONIC_FLOW_CONTROL, true);

    }
    virtual void printRegName(raw_ostream &OS, unsigned RegNo) const {
        X86IntelInstPrinter::printRegName(OS, RegNo);
    }
    virtual void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
        X86IntelInstPrinter::printOperand(MI, OpNo, O);
    }

    // tweaked version to handle references relatives to program counter
    virtual void printMemReference(const MCInst *MI, unsigned Op, raw_ostream &O) {
        const MCOperand &BaseReg = MI->getOperand(Op);
        int64_t ScaleVal = MI->getOperand(Op + 1).getImm();
        const MCOperand &IndexReg = MI->getOperand(Op + 2);
        const MCOperand &DispSpec = MI->getOperand(Op + 3);
        const MCOperand &SegReg = MI->getOperand(Op + 4);

        // If this has a segment register, print it.
        if (SegReg.getReg()) {
            printOperand(MI, Op + 4, O);
            O << ':';
        }

        O << '[';

        bool NeedPlus = false;

        if (BaseReg.getReg() == X86::RIP)
        {
            assert(!IndexReg.getReg());
            assert(DispSpec.isImm());

            igorAddress abs = m_pStatus->PCAfter + DispSpec.getImm();

            Balau::String symbolName;
            if (m_pSession->getSymbolName(abs, symbolName))
            {
                O << c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL, true);
                O << symbolName.to_charp();
                O << c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL, true);
            }
            else
            {
                O << formatHex(abs.offset);
            }
        }
        else
        {
            if (BaseReg.getReg()) {
                printOperand(MI, Op, O);
                NeedPlus = true;
            }

            if (IndexReg.getReg()) {
                if (NeedPlus) O << " + ";
                if (ScaleVal != 1)
                    O << ScaleVal << '*';
                printOperand(MI, Op + 2, O);
                NeedPlus = true;
            }

            if (!DispSpec.isImm()) {
                if (NeedPlus) O << " + ";
                assert(DispSpec.isExpr() && "non-immediate displacement for LEA?");
                O << *DispSpec.getExpr();
            }
            else {
                int64_t DispVal = DispSpec.getImm();
                if (DispVal || (!IndexReg.getReg() && !BaseReg.getReg())) {
                    if (NeedPlus) {
                        if (DispVal > 0)
                            O << " + ";
                        else {
                            O << " - ";
                            DispVal = -DispVal;
                        }
                    }

					{
						igorAddress abs(m_pSession, DispVal, -1);

						Balau::String symbolName;
						if (m_pSession->getSymbolName(abs, symbolName))
						{
							O << c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL, true);
							O << symbolName.to_charp();
							O << c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL, true);
						}
						else
						{
							O << formatImm(DispVal);
						}
					}
                }
            }
        }

        O << ']';
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

			Balau::String symbolName;
			if (m_pSession->getSymbolName(abs, symbolName))
			{
				O << c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL, true);
				O << symbolName.to_charp();
				O << c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL, true);
			}
			else
			{
				O << formatHex(abs.offset);
			}
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
    virtual void printMemOffset(const MCInst *MI, unsigned Op, raw_ostream &O) {
 		const MCOperand &DispSpec = MI->getOperand(Op);
		const MCOperand &SegReg = MI->getOperand(Op + 1);

		// If this has a segment register, print it.
		if (SegReg.getReg()) {
			printOperand(MI, Op + 1, O);
			O << ':';
		}

		O << '[';

		if (DispSpec.isImm())
		{
			int64_t imm = DispSpec.getImm();
			igorAddress abs(m_pSession, imm, -1);

			Balau::String symbolName;
			if (m_pSession->getSymbolName(abs, symbolName))
			{
				O << c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL, true);
				O << symbolName.to_charp();
				O << c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL, true);
			}
			else
			{
				O << formatImm(DispSpec.getImm());
			}
		}
		else {
			assert(DispSpec.isExpr() && "non-immediate displacement?");
			O << *DispSpec.getExpr();
		}

		O << ']';
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

    void setSession(IgorSession* pSession)
    {
        m_pSession = pSession;
    }

private:
    struct LLVMStatus * m_pStatus;
    IgorSession* m_pSession;
};

class c_x86_llvm_analyse_result : public c_cpu_analyse_result
{
public:
    MCInst m_inst;
};

c_cpu_analyse_result* c_cpu_x86_llvm::allocateCpuSpecificAnalyseResult()
{
    return new c_x86_llvm_analyse_result;
}

Balau::String c_cpu_x86_llvm::getTag() const
{
    return "igor_x86_llvm";
}

c_cpu_x86_llvm::c_cpu_x86_llvm(e_cpu_type cpuType)
{
    m_tls.setConstructor([cpuType]() -> TLS * {
        TLS * tls = new TLS;
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

        tls->m_pTarget = TargetRegistry::lookupTarget("", triple, error);
        tripleName = triple.getTriple();

        switch (cpuType) {
        case c_cpu_x86_llvm::X86:
            IAssert(tls->m_pTarget == &TheX86_32Target, "We didn't get the proper target for x86 32");
            break;
        case c_cpu_x86_llvm::X64:
            IAssert(tls->m_pTarget == &TheX86_64Target, "We didn't get the proper target for x86 64");
            break;
        }

        tls->m_pMRI = tls->m_pTarget->createMCRegInfo(tripleName);
        tls->m_pMAI = tls->m_pTarget->createMCAsmInfo(*tls->m_pMRI, tripleName);
        tls->m_pSTI = tls->m_pTarget->createMCSubtargetInfo(tripleName, "", "");
        tls->m_pMII = tls->m_pTarget->createMCInstrInfo();
        tls->m_pMOFI = new llvm::MCObjectFileInfo();
        tls->m_pCtx = new llvm::MCContext(tls->m_pMAI, tls->m_pMRI, tls->m_pMOFI);

        tls->m_pDisassembler = tls->m_pTarget->createMCDisassembler(*tls->m_pSTI, *tls->m_pCtx);
        tls->m_pAnalyzer = new IgorLLVMX86InstAnalyzer(*tls->m_pMAI, *tls->m_pMII, *tls->m_pMRI);
        tls->m_pPrinter = new IgorLLVMX86InstPrinter(*tls->m_pMAI, *tls->m_pMII, *tls->m_pMRI);

        return tls;
    });
}

c_cpu_x86_llvm::~c_cpu_x86_llvm()
{
}

c_cpu_x86_llvm::TLS::~TLS()
{
    delete m_pMRI;
    delete m_pMAI;
    delete m_pSTI;
    delete m_pMII;
    delete m_pDisassembler;
    delete m_pAnalyzer;
    delete m_pPrinter;
    delete m_pCtx;
    delete m_pMOFI;
}

igor_result c_cpu_x86_llvm::analyze(s_analyzeState * pState)
{
    pState->m_cpu_analyse_result->m_startOfInstruction = pState->m_PC;
    pState->m_cpu_analyse_result->m_instructionSize = 0;

    c_x86_llvm_analyse_result* pAnalyseResult = (c_x86_llvm_analyse_result*)pState->m_cpu_analyse_result;
    pAnalyseResult->m_inst.clear();

    LLVMMemoryObject memoryObject(pState);
    raw_null_ostream ns1, ns2;

    uint64_t size;
    MCDisassembler::DecodeStatus result = m_tls.get()->m_pDisassembler->getInstruction(pAnalyseResult->m_inst, size, memoryObject, pState->m_PC.offset, ns1, ns2);

    if (result != MCDisassembler::Success)
        return IGOR_FAILURE;

    IAssert(size < 256, "Instruction's too big! (%" PRIu64 " bytes)", size);
    pState->m_cpu_analyse_result->m_instructionSize = (u8) size;

    MCInst& inst = pAnalyseResult->m_inst;
    const MCInstrDesc & desc = m_tls.get()->m_pMII->get(inst.getOpcode());
    uint64_t tsflags = desc.TSFlags;

    if (desc.isBarrier())
        pState->m_analyzeResult = stop_analysis;

    LLVMStatus llvmStatus;

    m_tls.get()->m_pAnalyzer->setStatus(&llvmStatus);
    m_tls.get()->m_pAnalyzer->printInst(&inst, ns1, "");
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
    c_x86_llvm_analyse_result* pAnalyseResult = (c_x86_llvm_analyse_result*)pState->m_cpu_analyse_result;
    MCInst& inst = pAnalyseResult->m_inst;

    const MCInstrDesc & desc = m_tls.get()->m_pMII->get(inst.getOpcode());
    uint64_t tsflags = desc.TSFlags;

    std::string outStr;
    raw_string_ostream out(outStr);

    LLVMStatus llvmStatus;
    llvmStatus.PCBefore = pState->m_cpu_analyse_result->m_startOfInstruction;
    llvmStatus.PCAfter = llvmStatus.PCBefore + pState->m_cpu_analyse_result->m_instructionSize;

    m_tls.get()->m_pPrinter->setSession(pState->pSession);
    m_tls.get()->m_pPrinter->setStatus(&llvmStatus);
    m_tls.get()->m_pPrinter->printInst(&inst, out, "");
    m_tls.get()->m_pPrinter->setSession(NULL);

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
