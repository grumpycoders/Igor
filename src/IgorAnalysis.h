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

igor_result igor_flag_address_as_u32(u64 virtualAddress);
igor_result igor_flag_address_as_instruction(u64 virtualAddress, u8 instructionSize);
igor_result igor_is_address_flagged_as_code(u64 virtualAddress);