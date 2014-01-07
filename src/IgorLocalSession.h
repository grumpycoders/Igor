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
      IgorLocalSession() : m_pDatabase(new s_igorDatabase) { }
      ~IgorLocalSession() { delete m_pDatabase; }

    void loaded(const char * filename);

    void serialize(const char * filename);
    static IgorLocalSession * deserialize(const char * filename);

    void add_code_analysis_task(igorAddress PC);
    virtual void Do() override;
    void stop() { add_code_analysis_task(IGOR_INVALID_ADDRESS); }
    virtual const char * getName() const override { return "IgorAnalysisManagerLocal"; }
    bool isRunning() { return m_status == RUNNING; }
    void setDB(s_igorDatabase * db) { AAssert(m_pDatabase == NULL, "Can only set database once"); m_pDatabase = db; }
    s_igorDatabase * getDB() { return m_pDatabase; }
    const char * getStatusString();
    void add_instruction() { m_instructions++; }

    virtual igor_result readS32(igorAddress address, s32& output) override;
    virtual igor_result readU32(igorAddress address, u32& output) override;
    virtual igor_result readS16(igorAddress address, s16& output) override;
    virtual igor_result readU16(igorAddress address, u16& output) override;
    virtual igor_result readS8(igorAddress address, s8& output) override;
    virtual igor_result readU8(igorAddress address, u8& output) override;
    virtual igorAddress findSymbol(const char* symbolName) override;
    virtual int readString(igorAddress address, Balau::String& outputString) override;
    virtual c_cpu_module* getCpuForAddress(igorAddress PC) override;
    virtual c_cpu_state* getCpuStateForAddress(igorAddress PC) override;
    virtual igor_result is_address_flagged_as_code(igorAddress virtualAddress) override;
    virtual igorAddress get_next_valid_address_before(igorAddress virtualAddress) override;
    virtual igorAddress get_next_valid_address_after(igorAddress virtualAddress) override;
    virtual igor_result flag_address_as_u32(igorAddress virtualAddress) override;
    virtual igor_result flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize) override;

    virtual igorAddress getEntryPoint() override;
    virtual igor_section_handle getSectionFromAddress(igorAddress virtualAddress) override;
    virtual igorAddress getSectionStartVirtualAddress(igor_section_handle sectionHandle) override;
    virtual u64 getSectionSize(igor_section_handle sectionHandle) override;

    virtual std::tuple<igorAddress, igorAddress, size_t> getRanges() override;
    virtual igorAddress linearToVirtual(u64) override;

    virtual bool getSymbolName(igorAddress, Balau::String& name) override;

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
