#pragma once

#include "Igor.h"

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

    // output
    c_cpu_analyse_result* m_cpu_analyse_result;

    e_analyzeResult m_analyzeResult;
};

class c_cpu_module
{
public:
    virtual Balau::String getTag() const = 0;
    virtual igor_result analyze(s_analyzeState* pState) = 0;
    virtual igor_result printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor = false) = 0;

    virtual igor_result getMnemonic(s_analyzeState* pState, Balau::String& outputString) = 0;
    virtual int getNumOperands(s_analyzeState* pState) = 0;
    virtual igor_result getOperand(s_analyzeState* pState, int operandIndex, Balau::String& outputString, bool bUseColor = false) = 0;

    virtual c_cpu_analyse_result* allocateCpuSpecificAnalyseResult() = 0;

    virtual void generateReferences(s_analyzeState* pState) = 0;

    enum e_colors
    {
        RESET_COLOR,
        KNOWN_SYMBOL,
        MNEMONIC_DEFAULT,
        MNEMONIC_FLOW_CONTROL,
    };

    static const char* startColor(e_colors, bool bUseColor = true);
    static const char* finishColor(e_colors, bool bUseColor = true);
};

class c_cpu_factory
{
public:
    static c_cpu_module* createCpuFromString(const Balau::String &);
protected:
    c_cpu_factory() { m_list.push_back(this); }
    virtual c_cpu_module* maybeCreateCpu(const Balau::String &) = 0;
private:
    static std::vector<c_cpu_factory*> m_list;
};
