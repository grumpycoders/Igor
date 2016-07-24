#include <set>

#include "IgorAnalysis.h"
#include "IgorSession.h"

#ifdef _WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif
#include <Printer.h>
#include <BString.h>
#include <TaskMan.h>

#include "IgorMemory.h"

using namespace Balau;

namespace {

    template<int NUM_ENTRIES, typename storage_t = uint16_t>
    class FastSet {
    public:
        void clear() { m_n = 0; }
        bool isInSet(storage_t val) {
            AAssert((val < NUM_ENTRIES) && (val >= 0), "Invalid value");
            storage_t sparse = m_sparse[val];
            if ((sparse >= m_n) || (sparse < 0))
                return false;
            return m_dense[sparse] == val;
        }
        bool isEmpty() { return m_n == 0; }
        storage_t pop() {
            AAssert(!isEmpty(), "Can't pop an empty set");
            return m_dense[--m_n];
        }
        void insert(storage_t val) {
            if (isInSet(val))
                return;
            m_dense[m_n] = val;
            m_sparse[val] = m_n++;
        }
        void remove(storage_t val) {
            if (!isInSet(val))
                return;
            storage_t sparse = m_sparse[val];
            m_dense[sparse] = m_dense[--m_n];
        }
    private:
        int m_n = 0;
        storage_t m_dense[NUM_ENTRIES];
        storage_t m_sparse[NUM_ENTRIES];
    };

    class IdList : public Balau::AtStart {
    public:
        IdList() : AtStart(500) { }
        virtual void doStart() override {
            for (int i = NUM_IDS - 1; i != 0; i--)
                m_set.insert(i);
        }
        uint16_t getId() {
            Balau::ScopeLock sl(m_lock);
            return m_set.pop();
        }
        void returnId(uint16_t id) {
            Balau::ScopeLock sl(m_lock);
            IAssert(!m_set.isInSet(id), "Database %u has already been released", id);
            m_set.insert(id);
        }

    private:
        Balau::Lock m_lock;
        static const int NUM_IDS = 2048;
        FastSet<NUM_IDS> m_set;
    };

}

static IdList s_ids;

String IgorSession::generateUUID() {
#ifdef _WIN32
    UUID uuid;
    RPC_STATUS s;
#ifdef UNICODE
    RPC_WSTR uuidstr;
#else
    RPC_CSTR uuidstr;
#endif
    s = UuidCreate(&uuid);
    EAssert(s == RPC_S_OK, "UuidCreate didn't return RPC_S_OK but %i", s);

    s = UuidToString(&uuid, &uuidstr);
    EAssert(s == RPC_S_OK, "UuidToString didn't return RPC_S_OK but %i", s);

#ifdef UNICODE
    String r((const char *) uuidstr, wcslen((const wchar_t *) uuidstr) * 2);
    r.do_iconv("CP1200", "UTF-8");
#else
    String r = *uuidstr;
#endif

    RpcStringFree(&uuidstr);
#else
    uuid_t uuid;
    char uuidstr[37];

    uuid_generate(uuid);
    uuid_unparse(uuid, uuidstr);

    String r = uuidstr;
#endif

    return r.lower();
}

RWLock IgorSession::m_listLock;
IgorSession * IgorSession::m_head = NULL;

void IgorSession::activate() {
    ScopeLockW sl(m_listLock);
    m_active = true;

    addRef();
}

void IgorSession::deactivate() {
    ScopeLockW sl(m_listLock);

    m_active = false;

    release();
}

void IgorSession::assignNewUUID() {
    AAssert(m_uuid == "", "Can't assign UUID twice");
    m_uuid = generateUUID();
}

IgorSession::IgorSession() {
    m_refs = 1;
    m_id = s_ids.getId();
    ScopeLockW sl(m_listLock);

    m_next = m_head;
    m_prev = NULL;
    m_head = this;
}

IgorSession::~IgorSession() {
    s_ids.returnId(m_id);
    ScopeLockR sl(m_listLock);
    IAssert(!m_active, "Session still actively linked while being deleted.");

    if (m_head == this)
        m_head = m_next;

    if (m_next)
        m_next->m_prev = m_prev;

    if (m_prev)
        m_prev->m_next = m_next;

    m_next = m_prev = NULL;
}

void IgorSession::enumerate(std::function<void(IgorSession *)> cb) {
    ScopeLockR sl(m_listLock);

    for (IgorSession * ptr = m_head; ptr; ptr = ptr->m_next)
        if (ptr->m_active)
            cb(ptr);
}

IgorSession * IgorSession::find(const Balau::String & uuid) {
    ScopeLockR sl(m_listLock);

    for (IgorSession * ptr = m_head; ptr; ptr = ptr->m_next) {
        if (ptr->m_active && ptr->getUUID() == uuid) {
            ptr->addRef();
            return ptr;
        }
    }

    return NULL;
}

IgorSession * IgorSession::find(uint16_t id) {
    ScopeLockR sl(m_listLock);

    for (IgorSession * ptr = m_head; ptr; ptr = ptr->m_next) {
        if (ptr->m_id == id) {
            ptr->addRef();
            return ptr;
        }
    }

    return NULL;
}

igor_result IgorSession::getProperties(igorAddress address, s_IgorPropertyBag& outputPropertyBag)
{
    // does this address map a segment?
    igor_segment_handle hSegment = getSegmentFromAddress(address);
    if (hSegment == -1)
    {
        // no segment, so no properties?
        return IGOR_SUCCESS;
    }

    // is this code?
    if (is_address_flagged_as_code(address))
    {
        // is there a cpu declared for this address?
        c_cpu_module* pCpu = getCpuForAddress(address);
        if (pCpu)
        {
            s_analyzeState analyzeState;
            analyzeState.m_PC = address;
            analyzeState.pCpu = pCpu;
            analyzeState.pCpuState = getCpuStateForAddress(address);
            analyzeState.pSession = this;
            analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();
            if (pCpu->analyze(&analyzeState) == IGOR_SUCCESS)
            {
                s_IgorPropertyCode* pPropertyCode = new s_IgorPropertyCode();
                pCpu->printInstruction(&analyzeState, pPropertyCode->m_instruction, true);
                pPropertyCode->m_instructionSize = analyzeState.m_cpu_analyse_result->m_instructionSize;

                outputPropertyBag.addProperty(pPropertyCode);
            }
            delete analyzeState.m_cpu_analyse_result;
        }
    }
    else
    {
        // add one byte of data
        s_IgorPropertyData* pPropertyData = new s_IgorPropertyData();
        u8 data = readU8(address);
        pPropertyData->m_bytes.push_back(data);

        outputPropertyBag.addProperty(pPropertyData);
    }

    // is there a symbol?
    {
        String symbolName;
        if (getSymbolName(address, symbolName))
        {
            s_IgorPropertySymbol* pPropertySymbol = new s_IgorPropertySymbol();
            pPropertySymbol->m_symbol = symbolName;

            outputPropertyBag.addProperty(pPropertySymbol);
        }
    }

    // is there cross references?
    {
        std::vector<igorAddress> crossReferences;
        getReferences(address, crossReferences);

        if (crossReferences.size())
        {
            s_IgorPropertyCrossReference* pPropertyCrossReference = new s_IgorPropertyCrossReference();
            pPropertyCrossReference->m_crossReferences = crossReferences;

            outputPropertyBag.addProperty(pPropertyCrossReference);
        }
    }
    return IGOR_SUCCESS;
}

