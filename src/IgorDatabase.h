#pragma once

#include <vector>
#include <map>

#include <Task.h>

#include "Igor.h"

#include "IgorAPI.h"
#include "IgorSection.h"
#include "cpu/cpuModule.h"
#include "Threads.h"

typedef int igor_cpu_handle;

struct s_analysisRequest
{
    s_analysisRequest(const igorAddress & PC) : m_pc(PC) { }
    igorAddress m_pc;
};

class s_IgorProperty
{
public:
    enum e_property
    {
        Code,
        SymbolNamed,
        OperandOffset,
    };

    e_property m_type;
    int m_propertySize;
};

// TODO: ideally, all that stuff would be stack allocated on the calling site
class s_IgorPropertyBag
{
public:
    int getNumProperties();
    s_IgorProperty* getProperty(int index);
private:
    std::vector<s_IgorProperty*> m_properties;
};

struct s_igorDatabase
{
    s_igorDatabase();
    ~s_igorDatabase();
    // Not sure about that stuff yet. Kind of making it up as I go
    enum e_baseTypes
    {
        TYPE_U8,
        TYPE_S8,
        TYPE_U16,
        TYPE_S16,
        TYPE_U32,
        TYPE_S32,
        TYPE_U64,
        TYPE_S64,

        TYPE_STRUCT,
    };

    struct s_variableType
    {
        e_baseTypes m_baseType;
        u32 m_arraySize; // if 0, not an array. What would array of 1 mean?
        u32 m_structHandle; // get you to the structure definition;

        void initAs(e_baseTypes baseType)
        {
            m_baseType = baseType;
            m_arraySize = 0;
            m_structHandle = -1;
        }
    };

    enum e_symbolType
    {
        SYMBOL_UNKOWN, // Default "unknown" state

        SYMBOL_VARIABLE,

        SYMBOL_FUNCTION,
        SYMBOL_IMPORT_FUNCTION,
        SYMBOL_STRING,
    };

    struct s_symbolDefinition
    {
        e_symbolType m_type;
        Balau::String m_name;
        // Balau::String m_comment;
        union
        {
            s_variableType m_variable;
        };

        s_symbolDefinition() :
            m_type(SYMBOL_UNKOWN)
        {

        }
    };

    struct igorAddressCompare : public std::binary_function<const igorAddress, const igorAddress, bool>
    {
    public:
        bool operator()(const igorAddress x, const igorAddress y) const // returns x>y
        {
            return (x.m_offset > y.m_offset);
        }
    };

    typedef std::map<igorAddress, s_symbolDefinition, igorAddressCompare> t_symbolMap;
    t_symbolMap m_symbolMap;
    void getSymbolsIterator(s_igorDatabase::t_symbolMap::iterator& start, s_igorDatabase::t_symbolMap::iterator& end)
    {
        start = m_symbolMap.begin();
        end = m_symbolMap.end();
    }

    // should the references be implicit instead of explicit?
    typedef std::multimap<igorAddress, igorAddress> t_references;
    t_references m_references;

    void addReference(igorAddress referencedAddress, igorAddress referencedFrom);
    void getReferences(igorAddress referencedAddress, std::vector<igorAddress>& referencedFrom);

    // a map of the name of all symbols. Should the name of a symbol be included in the global m_symbolTypeMap?
    // Technically, only symbol could have name...
    //std::map<u64, Balau::String> m_stringMap;

    // How do we define a structure in Igor?
    struct s_structure
    {
        struct s_element
        {
            u32 offset;
            s_variableType m_type;
        };
        Balau::String m_name;
        std::vector<s_element> m_elements;
    };
    std::vector<s_structure> m_structures;

    std::vector<c_cpu_module*> m_cpu_modules;
    Balau::TQueue<s_analysisRequest> m_analysisRequests;
    std::vector<s_igorSegment*> m_segments;

    uint16_t m_sessionId;

    igor_result igor_add_cpu(c_cpu_module* pCpuModule, igor_cpu_handle& outputCpuHandle);
    
    c_cpu_module* getCpuForAddress(igorAddress PC);
    c_cpu_state* getCpuStateForAddress(igorAddress PC);

    igor_result readS64(igorAddress address, s64& output);
    igor_result readU64(igorAddress address, u64& output);
    igor_result readS32(igorAddress address, s32& output);
    igor_result readU32(igorAddress address, u32& output);
    igor_result readS16(igorAddress address, s16& output);
    igor_result readU16(igorAddress address, u16& output);
    igor_result readS8(igorAddress address, s8& output);
    igor_result readU8(igorAddress address, u8& output);

    igorAddress findSymbol(const char* symbolName);

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

    int readString(igorAddress address, Balau::String& outputString);

    igor_result create_segment(igorLinearAddress virtualAddress, u64 size, igor_segment_handle& outputsegmentHandle);
    igor_result set_segment_name(const Balau::String& sectionName);
    igor_result set_segment_option(igor_segment_handle segmentHandle, e_igor_section_option option);
    igor_result load_segment_data(igor_segment_handle segmentHandle, BFile reader, u64 size);
    igor_result load_segment_data(igor_segment_handle segmentHandle, const void* data, u64 size);

    igor_result declare_name(igorAddress virtualAddress, Balau::String name);
    igor_result declare_symbolType(igorAddress virtualAddress, e_symbolType type);
    igor_result declare_variable(igorAddress virtualAddress, e_baseTypes type);

    igor_result flag_address_as_u32(igorAddress virtualAddress);
    igor_result flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize);


    igor_result is_address_flagged_as_code(igorAddress virtualAddress);
    igorAddress get_next_valid_address_before(igorAddress virtualAddress);
    igorAddress get_next_valid_address_after(igorAddress virtualAddress);

    igorAddress getEntryPoint();
    
    igor_result getSegments(std::vector<igor_segment_handle>& outputSegments);
    s_igorSegment* getSegment(igor_segment_handle segmentHandle);
    igor_segment_handle getSegmentFromAddress(igorAddress virtualAddress);
    igorAddress getSegmentStartVirtualAddress(igor_segment_handle segmentHandle);
    u64 getSegmentSize(igor_segment_handle segmentHandle);
    igor_result setSegmentName(igor_segment_handle segmentHandle, Balau::String& name);
    igor_result getSegmentName(igor_segment_handle segmentHandle, Balau::String& name);

    igorAddress m_entryPoint;

    std::tuple<igorAddress, igorAddress, size_t> getRanges();
    igorAddress linearToVirtual(u64);

    igor_result executeCommand(Balau::String& command);

    bool getSymbolName(igorAddress address, Balau::String& name);

    void lock() { m_lock.enter(); }
    void unlock() { m_lock.leave(); }

    // give all the properties of a given address
    void getProperties(igorAddress address, s_IgorPropertyBag& outputPropertyBag);

private:
    s_igorSegment* findSectionFromAddress(igorAddress address);
    s_symbolDefinition* get_Symbol(igorAddress virtualAddress);
    Balau::Lock m_lock;
};
