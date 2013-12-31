#pragma once

#include <atomic>
#include <list>
#include <Task.h>
#include <StacklessTask.h>
#include "Igor.h"
#include "IgorDatabase.h"

class c_cpu_module;
class c_cpu_state;

class IgorSession
{
public:
    IgorSession();
    IgorSession(const Balau::String & uuid);
    ~IgorSession();

    const Balau::String & getUUID() { return m_uuid; }

    static void enumerate(std::function<bool(IgorSession *)>);

	virtual igor_result readS32(igorAddress address, s32& output) = 0;
	virtual igor_result readU32(igorAddress address, u32& output) = 0;
	virtual igor_result readS16(igorAddress address, s16& output) = 0;
	virtual igor_result readU16(igorAddress address, u16& output) = 0;
	virtual igor_result readS8(igorAddress address, s8& output) = 0;
	virtual igor_result readU8(igorAddress address, u8& output) = 0;

	virtual igorAddress findSymbol(const char* symbolName) = 0;

	s32 readS32(igorAddress address)
	{
		s32 output;
		readS32(address, output);
		return output;
	}

	u32 readU32(igorAddress address)
	{
		u32 output;
		readU32(address, output);
		return output;
	}

	s16 readS16(igorAddress address)
	{
		s16 output;
		readS16(address, output);
		return output;
	}

	u16 readU16(igorAddress address)
	{
		u16 output;
		readU16(address, output);
		return output;
	}

	s8 readS8(igorAddress address)
	{
		s8 output;
		readS8(address, output);
		return output;
	}

	u8 readU8(igorAddress address)
	{
		u8 output;
		readU8(address, output);
		return output;
	}

	virtual int readString(u64 address, Balau::String& outputString) = 0;
	virtual c_cpu_module* getCpuForAddress(igorAddress PC) = 0;
	virtual c_cpu_state* getCpuStateForAddress(igorAddress PC) = 0;
	virtual igor_result is_address_flagged_as_code(igorAddress virtualAddress) = 0;
	virtual igorAddress get_next_valid_address_before(igorAddress virtualAddress) = 0;
	virtual igorAddress get_next_valid_address_after(igorAddress virtualAddress) = 0;

    virtual void add_code_analysis_task(igorAddress PC) = 0;

	virtual igor_result flag_address_as_u32(u64 virtualAddress) = 0;
	virtual igor_result flag_address_as_instruction(u64 virtualAddress, u8 instructionSize) = 0;

	virtual igorAddress getEntryPoint() = 0;
	virtual igor_section_handle getSectionFromAddress(igorAddress virtualAddress) = 0;
	virtual igorAddress getSectionStartVirtualAddress(igor_section_handle sectionHandle) = 0;
	virtual u64 getSectionSize(igor_section_handle sectionHandle) = 0;

    virtual std::tuple<igorAddress, igorAddress, size_t> getRanges() = 0;
    virtual igorAddress linearToVirtual(igorAddress) = 0;

public:
    Balau::String m_uuid;
    static Balau::RWLock m_listLock;
    static IgorSession * m_head;
    IgorSession * m_next, * m_prev;
};

class IgorLocalSession : public Balau::Task, public IgorSession {
public:
    void add_code_analysis_task(u64 PC);
    void Do();
    void stop() { add_code_analysis_task((u64) -1); }
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
	int readString(u64 address, Balau::String& outputString);
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
    virtual igorAddress linearToVirtual(igorAddress);

private:
    enum {
        IDLE,
        RUNNING,
        STOPPING,
    } m_status = IDLE;
    s_igorDatabase * m_pDatabase = NULL;
    std::list<std::pair<Balau::Events::TaskEvent *, u64>> m_evts;
    std::atomic<uint64_t> m_instructions;
};

class c_cpu_module;
class c_cpu_state;

class IgorAnalysis : public Balau::StacklessTask {
public:
    IgorAnalysis(s_igorDatabase * db, u64 PC, IgorLocalSession * parent) : m_pDatabase(db), m_PC(PC), m_session(parent) { m_name.set("IgorAnalysis for %016llx", PC); }
    void Do();
    const char * getName() const { return m_name.to_charp(); }
private:
    s_igorDatabase * m_pDatabase = NULL;
    IgorLocalSession * m_session;
    u64 m_PC = 0;
    Balau::String m_name;
    c_cpu_module * m_pCpu;
    c_cpu_state * m_pCpuState;
};
