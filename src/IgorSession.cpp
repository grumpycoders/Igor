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

void IgorSession::linkMe() {
    ScopeLockW sl(m_listLock);

    m_next = m_head;
    m_prev = NULL;
    m_head = this;
}

void IgorSession::assignNewUUID() {
    AAssert(m_uuid == "", "Can't assign UUID twice");
    m_uuid = generateUUID();
}

IgorSession::~IgorSession() {
    ScopeLockW sl(m_listLock);

    if (m_head == this)
        m_head = m_next;

    if (m_next)
        m_next->m_prev = m_prev;

    if (m_prev)
        m_prev->m_next = m_next;
}

void IgorSession::enumerate(std::function<bool(IgorSession *)> cb) {
    ScopeLockR sl(m_listLock);

    for (IgorSession * ptr = m_head; ptr; ptr = ptr->m_next)
        if (!cb(ptr))
            break;
}
