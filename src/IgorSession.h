#pragma once

#include <Task.h>
#include "Igor.h"
#include "IgorDatabase.h"
#include "cpu/cpuModule.h"

class c_cpu_module;
class c_cpu_state;

class IgorSession
{
  public:
      virtual ~IgorSession();

    void setSessionName(const Balau::String & name) { m_name = name; }
    void setSessionName(const char * name) { m_name = name; }

    const Balau::String & getSessionName() { return m_name; }
    const Balau::String & getUUID() { return m_uuid; }

    static void enumerate(std::function<bool(IgorSession *)>);

	virtual igor_result readS64(igorAddress address, s64& output) = 0;
	virtual igor_result readU64(igorAddress address, u64& output) = 0;
    virtual igor_result readS32(igorAddress address, s32& output) = 0;
    virtual igor_result readU32(igorAddress address, u32& output) = 0;
    virtual igor_result readS16(igorAddress address, s16& output) = 0;
    virtual igor_result readU16(igorAddress address, u16& output) = 0;
    virtual igor_result readS8(igorAddress address, s8& output) = 0;
    virtual igor_result readU8(igorAddress address, u8& output) = 0;

    virtual igorAddress findSymbol(const char* symbolName) = 0;

	s64 readS64(igorAddress address)
	{
		s64 output;
		readS64(address, output);
		return output;
	}

	u64 readU64(igorAddress address)
	{
		u64 output;
		readU64(address, output);
		return output;
	}

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

    virtual int readString(igorAddress address, Balau::String& outputString) = 0;
    virtual c_cpu_module* getCpuForAddress(igorAddress PC) = 0;
    virtual c_cpu_state* getCpuStateForAddress(igorAddress PC) = 0;
    virtual igor_result is_address_flagged_as_code(igorAddress virtualAddress) = 0;
    virtual igorAddress get_next_valid_address_before(igorAddress virtualAddress) = 0;
    virtual igorAddress get_next_valid_address_after(igorAddress virtualAddress) = 0;

    virtual void add_code_analysis_task(igorAddress PC) = 0;

    virtual igor_result flag_address_as_u32(igorAddress virtualAddress) = 0;
    virtual igor_result flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize) = 0;

    virtual igorAddress getEntryPoint() = 0;
    virtual igor_section_handle getSectionFromAddress(igorAddress virtualAddress) = 0;
    virtual igorAddress getSectionStartVirtualAddress(igor_section_handle sectionHandle) = 0;
    virtual u64 getSectionSize(igor_section_handle sectionHandle) = 0;

    virtual std::tuple<igorAddress, igorAddress, size_t> getRanges() = 0;
    virtual igorAddress linearToVirtual(u64) = 0;

    virtual bool getSymbolName(igorAddress, Balau::String& name) = 0;

    virtual void addReference(igorAddress referencedAddress, igorAddress referencedFrom) = 0;
    virtual void getReferences(igorAddress referencedAddress, std::vector<igorAddress>& referencedFrom) = 0;

  protected:
    void assignNewUUID();
    void linkMe();

  private:
    Balau::String m_uuid, m_name;
    static Balau::RWLock m_listLock;
    static IgorSession * m_head;
    IgorSession * m_next, * m_prev;
};
