#pragma once

#include <Task.h>
#include "Igor.h"
#include "IgorDatabase.h"

class IgorAnalysis : public Balau::Task {
public:
    static void igor_add_code_analysis_task(u64 PC);
    static void stop() { igor_add_code_analysis_task((u64)-1); }
    void Do();
    const char * getName() const { return "IgorAnalysis"; }
    bool isRunning() { return m_status == RUNNING; }
private:
    enum {
        IDLE,
        RUNNING,
        STOPPING,
    } m_status;
};

extern IgorAnalysis * g_igorAnalysis;
