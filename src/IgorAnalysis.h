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
	virtual igor_result readS32(u64 address, s32& output) = 0;
	virtual igor_result readU32(u64 address, u32& output) = 0;
	virtual igor_result readS16(u64 address, s16& output) = 0;
	virtual igor_result readU16(u64 address, u16& output) = 0;
	virtual igor_result readS8(u64 address, s8& output) = 0;
	virtual igor_result readU8(u64 address, u8& output) = 0;

	virtual u64 findSymbol(const char* symbolName) = 0;

	s32 readS32(u64 address)
	{
		s32 output;
		readS32(address, output);
		return output;
	}

	u32 readU32(u64 address)
	{
		u32 output;
		readU32(address, output);
		return output;
	}

	s16 readS16(u64 address)
	{
		s16 output;
		readS16(address, output);
		return output;
	}

	u16 readU16(u64 address)
	{
		u16 output;
		readU16(address, output);
		return output;
	}

	s8 readS8(u64 address)
	{
		s8 output;
		readS8(address, output);
		return output;
	}

	u8 readU8(u64 address)
	{
		u8 output;
		readU8(address, output);
		return output;
	}

	virtual int readString(u64 address, Balau::String& outputString) = 0;
	virtual s_igorSection* findSectionFromAddress(u64 address) = 0;
	virtual c_cpu_module* getCpuForAddress(u64 PC) = 0;
	virtual c_cpu_state* getCpuStateForAddress(u64 PC) = 0;
	virtual igor_result is_address_flagged_as_code(u64 virtualAddress) = 0;
	virtual u64 get_next_valid_address_before(u64 virtualAddress) = 0;
	virtual u64 get_next_valid_address_after(u64 virtualAddress) = 0;

	virtual void add_code_analysis_task(u64 PC) = 0;

	virtual igor_result flag_address_as_u32(u64 virtualAddress) = 0;
	virtual igor_result flag_address_as_instruction(u64 virtualAddress, u8 instructionSize) = 0;
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

	igor_result readS32(u64 address, s32& output);
	igor_result readU32(u64 address, u32& output);
	igor_result readS16(u64 address, s16& output);
	igor_result readU16(u64 address, u16& output);
	igor_result readS8(u64 address, s8& output);
	igor_result readU8(u64 address, u8& output);
	u64 findSymbol(const char* symbolName);
	int readString(u64 address, Balau::String& outputString);
	s_igorSection* findSectionFromAddress(u64 address);
	c_cpu_module* getCpuForAddress(u64 PC);
	c_cpu_state* getCpuStateForAddress(u64 PC);
	igor_result is_address_flagged_as_code(u64 virtualAddress);
	u64 get_next_valid_address_before(u64 virtualAddress);
	u64 get_next_valid_address_after(u64 virtualAddress);
	igor_result flag_address_as_u32(u64 virtualAddress);
	igor_result flag_address_as_instruction(u64 virtualAddress, u8 instructionSize);

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
    IgorAnalysis(s_igorDatabase * db, u64 PC, IgorLocalSession * parent) : m_pDatabase(db), m_PC(PC), m_parent(parent) { m_name.set("IgorAnalysis for %016llx", PC); }
    void Do();
    const char * getName() const { return m_name.to_charp(); }
private:
    s_igorDatabase * m_pDatabase = NULL;
    IgorLocalSession * m_parent;
    u64 m_PC = 0;
    Balau::String m_name;
    c_cpu_module * m_pCpu;
    c_cpu_state * m_pCpuState;
};
