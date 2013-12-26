#pragma once

#include <Task.h>
#include "Igor.h"
#include "IgorDatabase.h"

class IgorAnalysis : public Balau::Task {
public:
    void igor_add_code_analysis_task(u64 PC);
    void stop() { igor_add_code_analysis_task((u64)-1); }
    void Do();
    const char * getName() const { return "IgorAnalysis"; }
    bool isRunning() { return m_status == RUNNING; }
    igor_result igor_flag_address_as_u32(u64 virtualAddress);
    igor_result igor_flag_address_as_instruction(u64 virtualAddress, u8 instructionSize);
    igor_result igor_is_address_flagged_as_code(u64 virtualAddress);
    void setDB(s_igorDatabase * db) { AAssert(m_pDatabase == NULL, "Can only set database once"); m_pDatabase = db; }
    s_igorDatabase * getDB() { return m_pDatabase; }
private:
    enum {
        IDLE,
        RUNNING,
        STOPPING,
    } m_status;
    s_igorDatabase * m_pDatabase = NULL;
};
