#pragma once

#include "Igor.h"

#include <vector>

class c_cpu_module;

class c_cpu_state
{
public:
};

// Holds the result of one analyze step (ie, one instruction).
// Each CPU must derive from it
// It is passed back to the CPU for query
class c_cpu_analyse_result
{
public:
    igorAddress m_startOfInstruction;
    u8 m_instructionSize;

};

struct s_igorDatabase;

enum e_analyzeResult
{
    stop_analysis,
    continue_analysis,
};

class IgorSession;

struct s_analyzeState
{
    // input
    igorAddress m_PC;
    c_cpu_module* pCpu;
    c_cpu_state* pCpuState;
    IgorSession* pSession;
	bool m_useColor;

    // output
    c_cpu_analyse_result* m_cpu_analyse_result;

	Balau::String m_disassembly; // raw disassembly
	std::vector<Balau::String> m_operands;

    e_analyzeResult m_analyzeResult;
};

class c_cpu_module
{
public:
    virtual ~c_cpu_module() { }
    virtual Balau::String getTag() const = 0;
    virtual igor_result analyze(s_analyzeState* pState) = 0;

	virtual igor_result splitOperands(Balau::String& instruction, Balau::String::List& splitOperands);
	virtual igorLinearAddress getAsAddress(Balau::String& operand);

    virtual c_cpu_analyse_result* allocateCpuSpecificAnalyseResult()
    {
        return new c_cpu_analyse_result;
    }

    virtual void generateReferences(s_analyzeState* pState)
    {

    }

    enum e_colors
    {
        DEFAULT = 0,
        KNOWN_SYMBOL,
        MNEMONIC_DEFAULT,
        MNEMONIC_FLOW_CONTROL,
        MEMORY_ADDRESS,

        OPERAND_REGISTER,
        OPERAND_IMMEDIATE,

        COLOR_MAX
    };

    static Balau::String startColor(e_colors color, bool bUseColor = true)
    {
        if (!bUseColor)
        {
            return "";
        }

        Balau::String colorString;
        colorString.append(";C=%d;", color);

        return colorString;
    }

    static Balau::String finishColor(e_colors, bool bUseColor = true)
    {
        return startColor(DEFAULT, bUseColor);
    }

    static uint32_t getColorForType(c_cpu_module::e_colors blockType);
};

struct s_cpuConstructionFlags
{
    Balau::String m_name;
};

struct s_cpuInfo
{
    const char* m_moduleName;
    const char** m_supportedCpuList;
    c_cpu_module*(*m_cpuContructionFunc)(s_cpuConstructionFlags* pFlags);
};

class c_cpu_factory
{
public:
    static void initialize();
    static c_cpu_module* createCpuFromString(const Balau::String &);
    static void getCpuList(std::vector<Balau::String>& cpuList);

private:
    static std::vector<const s_cpuInfo*> m_list;
};
