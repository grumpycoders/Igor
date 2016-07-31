#pragma once

#include <atomic>
#include <list>
#include <Task.h>
#include <StacklessTask.h>
#include <Handle.h>
#include "Igor.h"
#include "IgorDatabase.h"
#include "cpu/cpuModule.h"
#include "IgorSession.h"

class c_cpu_module;
class c_cpu_state;
class IgorLocalSession;
class IgorAnalysis;

class IgorAnalysisManagerLocal : public Balau::Task {
  public:
    friend class IgorAnalysis;
      IgorAnalysisManagerLocal(IgorLocalSession * session);
      ~IgorAnalysisManagerLocal();
    virtual void Do() override;
    virtual const char * getName() const override { return "IgorAnalysisManagerLocal"; }
  private:
    IgorLocalSession * m_session;
    Balau::Events::Async m_gotOneStop;
    ev_tstamp m_lastUpdate;
};

class IgorLocalSession : public IgorSession {
    friend class IgorAnalysisManagerLocal;
    friend class IgorAnalysis;
  public:
      IgorLocalSession();
      IgorLocalSession(const IgorLocalSession &) = delete;
      ~IgorLocalSession() { delete m_pDatabase; }

    void loaded(const char * filename);

    virtual std::tuple<igor_result, Balau::String, Balau::String> serialize(const char * name) override;
    static std::tuple<igor_result, IgorLocalSession *, Balau::String, Balau::String> deserialize(const char * name);
    static std::tuple<igor_result, IgorLocalSession *, Balau::String, Balau::String> loadBinary(const char * name);

    void add_code_analysis_task(igorAddress PC);
    virtual void stop() override { igorAddress invalid; add_code_analysis_task(invalid); }
    bool isRunning() { return m_status == RUNNING; }
    void setDB(s_igorDatabase * db) { AAssert(m_pDatabase == NULL, "Can only set database once"); m_pDatabase = db; }
    s_igorDatabase * getDB() { return m_pDatabase; }
    virtual const char * getStatusString() override;
    void add_instruction() { m_instructions++; }

    void freeze();
    void thaw();

    virtual igor_result readS64(igorAddress address, s64& output) override;
    virtual igor_result readU64(igorAddress address, u64& output) override;
    virtual igor_result readS32(igorAddress address, s32& output) override;
    virtual igor_result readU32(igorAddress address, u32& output) override;
    virtual igor_result readS16(igorAddress address, s16& output) override;
    virtual igor_result readU16(igorAddress address, u16& output) override;
    virtual igor_result readS8(igorAddress address, s8& output) override;
    virtual igor_result readU8(igorAddress address, u8& output) override;
    virtual igorAddress findSymbol(const char* symbolName) override;
    virtual void getSymbolsIterator(s_igorDatabase::t_symbolMap::iterator& start, s_igorDatabase::t_symbolMap::iterator& end) override;
    virtual int readString(igorAddress address, Balau::String& outputString) override;
    virtual c_cpu_module* getCpuForAddress(igorAddress PC) override;
    virtual c_cpu_state* getCpuStateForAddress(igorAddress PC) override;
    virtual bool is_address_flagged_as_code(igorAddress virtualAddress) override;
    virtual igorAddress get_next_valid_address_before(igorAddress virtualAddress) override;
    virtual igorAddress get_next_valid_address_after(igorAddress virtualAddress) override;
    virtual igor_result flag_address_as_u32(igorAddress virtualAddress) override;
    virtual igor_result flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize) override;
    
    virtual igorAddress getEntryPoint() override;

    virtual igor_result create_segment(igorLinearAddress virtualAddress, u64 size, igor_segment_handle& outputsegmentHandle) override;
    virtual igor_result getSegments(std::vector<igor_segment_handle>& outputSegments) override;
    virtual igor_segment_handle getSegmentFromAddress(igorAddress virtualAddress) override;
    virtual igorAddress getSegmentStartVirtualAddress(igor_segment_handle segmentHandle) override;
    virtual u64 getSegmentSize(igor_segment_handle segmentHandle) override;
    virtual igor_result setSegmentName(igor_segment_handle segmentHandle, Balau::String& name) override;
    virtual igor_result getSegmentName(igor_segment_handle segmentHandle, Balau::String& name) override;
    virtual igor_result setSegmentCPU(igor_segment_handle segmentHandle, Balau::String& cpuName) override;

    virtual std::tuple<igorAddress, igorAddress, size_t> getRanges() override;
    virtual igorAddress linearToVirtual(u64) override;

    virtual igor_result executeCommand(Balau::String& command) override;

    virtual bool getSymbolName(igorAddress, Balau::String& name) override;

    virtual void addReference(igorAddress referencedAddress, igorAddress referencedFrom);
    virtual void getReferences(igorAddress referencedAddress, std::vector<igorAddress>& referencedFrom);

    virtual igor_result loadAdditionalBinary(igorAddress address, BFile& file);

    virtual void lock() { m_pDatabase->lock(); }
    virtual void unlock() { m_pDatabase->unlock(); }

  private:
    enum {
        IDLE,
        RUNNING,
        STOPPING,
    } m_status = IDLE;
    s_igorDatabase * m_pDatabase = NULL;
    std::atomic<uint64_t> m_instructions;
    std::atomic<uint64_t> m_nTasks;
};
