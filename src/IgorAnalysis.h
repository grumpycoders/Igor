#pragma once

#include <atomic>
#include <list>
#include <Task.h>
#include <StacklessTask.h>
#include "Igor.h"
#include "IgorDatabase.h"
#include "cpu/cpuModule.h"

class c_cpu_module;
class c_cpu_state;
class IgorLocalSession;

class IgorAnalysis : public Balau::StacklessTask {
public:
    IgorAnalysis(s_igorDatabase * db, igorAddress PC, IgorLocalSession * parent, class IgorAnalysisManagerLocal *);
    ~IgorAnalysis();
    void Do();
    const char * getName() const { return m_name.to_charp(); }
private:
	Balau::String resultString;
    s_analyzeState m_analyzeState;
    s_igorDatabase * m_pDatabase = NULL;
    IgorLocalSession * m_session;
    IgorAnalysisManagerLocal * m_analysisManager;
    igorAddress m_PC;
    Balau::String m_name;
    c_cpu_module * m_pCpu;
    c_cpu_state * m_pCpuState;
    uint64_t m_instructionsCounter = 0;
};
