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
      IgorSession();
      virtual ~IgorSession();

    void setSessionName(const Balau::String & name) { m_name = name; }
    void setSessionName(const char * name) { m_name = name; }

    const Balau::String & getSessionName() { return m_name; }
    const Balau::String & getUUID() { return m_uuid; }
    uint16_t getId() { return m_id; }

    void addRef() {
        m_refs++;
    }
    void release() {
        if (--m_refs == 0)
            delete this;
    }

    static void enumerate(std::function<void(IgorSession *)>);
    static IgorSession * find(const Balau::String &);
    static IgorSession * find(uint16_t id);
    static Balau::String generateUUID();

    virtual std::tuple<igor_result, Balau::String, Balau::String> serialize(const char * name) { igor_result r = IGOR_FAILURE; Balau::String m1 = "Can't serialize this", m2; return std::tie(r, m1, m2); }

    // temporary; this needs to take care of the section limits, and switch section if necessary.
    igorAddress incrementAddress(const igorAddress & a, igorLinearAddress offset) {
        igorAddress r = a;
        r.m_offset += offset;
        return r;
    }

    igorAddress decrementAddress(const igorAddress & a, igorLinearAddress offset) {
        igorAddress r = a;
        r.m_offset -= offset;
        return r;
    }

    void do_incrementAddress(igorAddress & a, igorLinearAddress offset) {
        a.m_offset += offset;
    }

    void do_decrementAddress(igorAddress & a, igorLinearAddress offset) {
        a.m_offset -= offset;
    }

    virtual const char * getStatusString() { return "Unknown"; }
    virtual void stop() { }

    virtual igor_result readS64(igorAddress address, s64& output) = 0;
    virtual igor_result readU64(igorAddress address, u64& output) = 0;
    virtual igor_result readS32(igorAddress address, s32& output) = 0;
    virtual igor_result readU32(igorAddress address, u32& output) = 0;
    virtual igor_result readS16(igorAddress address, s16& output) = 0;
    virtual igor_result readU16(igorAddress address, u16& output) = 0;
    virtual igor_result readS8(igorAddress address, s8& output) = 0;
    virtual igor_result readU8(igorAddress address, u8& output) = 0;

    virtual igorAddress findSymbol(const char* symbolName) = 0;
    virtual void getSymbolsIterator(s_igorDatabase::t_symbolMap::iterator& start, s_igorDatabase::t_symbolMap::iterator& end) = 0;

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
    virtual bool is_address_flagged_as_code(igorAddress virtualAddress) = 0;
    virtual igorAddress get_next_valid_address_before(igorAddress virtualAddress) = 0;
    virtual igorAddress get_next_valid_address_after(igorAddress virtualAddress) = 0;

    virtual void add_code_analysis_task(igorAddress PC) = 0;

    virtual igor_result flag_address_as_u32(igorAddress virtualAddress) = 0;
    virtual igor_result flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize) = 0;

    virtual igorAddress getEntryPoint() = 0;

    virtual igor_result create_segment(igorLinearAddress virtualAddress, u64 size, igor_segment_handle& outputsegmentHandle) = 0;
    virtual igor_result getSegments(std::vector<igor_segment_handle>& outputSegments) = 0;
    virtual igor_segment_handle getSegmentFromAddress(igorAddress virtualAddress) = 0;
    virtual igorAddress getSegmentStartVirtualAddress(igor_segment_handle segmentHandle) = 0;
    virtual u64 getSegmentSize(igor_segment_handle segmentHandle) = 0;
    virtual igor_result setSegmentName(igor_segment_handle segmentHandle, Balau::String& name) = 0;
    virtual igor_result getSegmentName(igor_segment_handle segmentHandle, Balau::String& name) = 0;
    virtual igor_result setSegmentCPU(igor_segment_handle segmentHandle, Balau::String& cpuName) = 0;

    virtual std::tuple<igorAddress, igorAddress, size_t> getRanges() = 0;
    virtual igorAddress linearToVirtual(u64) = 0;

    virtual igor_result executeCommand(Balau::String& command) = 0;

    virtual bool getSymbolName(igorAddress, Balau::String& name) = 0;

    virtual void addReference(igorAddress referencedAddress, igorAddress referencedFrom) = 0;
    virtual void getReferences(igorAddress referencedAddress, std::vector<igorAddress>& referencedFrom) = 0;

    virtual igor_result loadAdditionalBinary(igorAddress address, BFile& file) = 0;

    void deactivate();

    virtual void lock() = 0;
    virtual void unlock() = 0;

    virtual igor_result getProperties(igorAddress address, s_IgorPropertyBag& outputPropertyBag);

    igorAddress m_hexViewAddress;

  protected:
    void assignNewUUID();
    void activate();

  private:
    uint16_t m_id; // this is local to the process, don't save it
    
    Balau::String m_uuid, m_name;
    static Balau::RWLock m_listLock;
    static IgorSession * m_head;
    IgorSession * m_next, * m_prev;
    bool m_active = false;
    std::atomic<int> m_refs;
};
