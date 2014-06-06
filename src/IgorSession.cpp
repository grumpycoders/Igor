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

    class IdList : public Balau::AtStart {
    public:
        IdList() : AtStart(500) { }
        virtual void doStart() override {
            for (int i = 1; i < static_cast<uint16_t>(-1); i++)
                m_set.insert(i);
        }
        uint16_t getId() {
            Balau::ScopeLock sl(m_lock);
            auto iter = m_set.begin();
            IAssert(iter != m_set.end(), "Too many open databases");
            uint16_t ret = *iter;
            m_set.erase(iter);

            return ret;
        }
        void returnId(uint16_t id) {
            Balau::ScopeLock sl(m_lock);
            auto iter = m_set.find(id);
            IAssert(iter != m_set.end(), "Database %u has already been released", id);
            m_set.insert(id);
        }

    private:
        Balau::Lock m_lock;
        std::set<uint16_t> m_set;
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
    r.do_iconv("UNICODELITTLE", "UTF-8");
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
