#pragma once

#include <atomic>
#include <list>
#include <Task.h>
#include <StacklessTask.h>
#include "Igor.h"
#include "IgorDatabase.h"
#include "cpu/cpuModule.h"
#include "IgorSession.h"

class c_cpu_module;
class c_cpu_state;

class IgorLocalSession : public Balau::Task, public IgorSession {
public:
    void add_code_analysis_task(igorAddress PC);
    void Do();
    void stop() { add_code_analysis_task(IGOR_INVALID_ADDRESS); }
    const char * getName() const { return "IgorAnalysisManagerLocal"; }
    bool isRunning() { return m_status == RUNNING; }
    void setDB(s_igorDatabase * db) { AAssert(m_pDatabase == NULL, "Can only set database once"); m_pDatabase = db; }
    s_igorDatabase * getDB() { return m_pDatabase; }
    const char * getStatusString();
    void add_instruction() { m_instructions++; }

    igor_result readS32(igorAddress address, s32& output);
    igor_result readU32(igorAddress address, u32& output);
    igor_result readS16(igorAddress address, s16& output);
    igor_result readU16(igorAddress address, u16& output);
    igor_result readS8(igorAddress address, s8& output);
    igor_result readU8(igorAddress address, u8& output);
    igorAddress findSymbol(const char* symbolName);
    int readString(igorAddress address, Balau::String& outputString);
    c_cpu_module* getCpuForAddress(igorAddress PC);
    c_cpu_state* getCpuStateForAddress(igorAddress PC);
    igor_result is_address_flagged_as_code(igorAddress virtualAddress);
    igorAddress get_next_valid_address_before(igorAddress virtualAddress);
    igorAddress get_next_valid_address_after(igorAddress virtualAddress);
    igor_result flag_address_as_u32(igorAddress virtualAddress);
    igor_result flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize);

    igorAddress getEntryPoint();
    igor_section_handle getSectionFromAddress(igorAddress virtualAddress);
    igorAddress getSectionStartVirtualAddress(igor_section_handle sectionHandle);
    u64 getSectionSize(igor_section_handle sectionHandle);

    virtual std::tuple<igorAddress, igorAddress, size_t> getRanges();
    virtual igorAddress linearToVirtual(u64);

    virtual bool getSymbolName(igorAddress, Balau::String& name);

private:
    enum {
        IDLE,
        RUNNING,
        STOPPING,
    } m_status = IDLE;
    s_igorDatabase * m_pDatabase = NULL;
    std::list<std::pair<Balau::Events::TaskEvent *, igorAddress>> m_evts;
    std::atomic<uint64_t> m_instructions;
};
