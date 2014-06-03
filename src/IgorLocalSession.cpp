#include "IgorAnalysis.h"
#include "IgorLocalSession.h"
#include "IgorSqlite.h"

#include "google/protobuf/io/zero_copy_stream.h"
#include "protobufs/IgorProtoFile.pb.h"

#include <Printer.h>
#include <BString.h>
#include <TaskMan.h>
#include <MMap.h>
#include "Loaders/PE/PELoader.h"
#include "Loaders/Dmp/dmpLoader.h"
#include "Loaders/Elf/elfLoader.h"

#include "IgorMemory.h"

using namespace Balau;

static bool safeOperationLambda(std::function<bool()> lambda) {
    bool success = true;

    try {
        success = lambda();
    }
    catch (...) {
        success = false;
    }

    return success;
}

void IgorLocalSession::Do() {
    if (!m_pDatabase)
        return;

    waitFor(m_pDatabase->m_analysisRequests.getEvent());

    Printer::log(M_INFO, "AnalysisManager starting...");
    m_instructions = 0;

    for (;;) {
        for (auto i = m_evts.begin(); i != m_evts.end(); i++) {
            Events::TaskEvent * evt = i->first;
            if (evt->gotSignal()) {
                evt->ack();
                delete evt;
                m_evts.erase(i);
                i = m_evts.begin();
                if (m_evts.empty())
                    break;
            }
        }
        if (m_evts.size() == 0) {
            if (m_status == STOPPING)
                return;
            else if (m_pDatabase->m_analysisRequests.isEmpty())
                m_status = IDLE;
        }

        if (m_status == IDLE)
            Printer::log(M_INFO, "AnalysisManager going idle; analyzed %lli instructions...", m_instructions.load());

        m_pDatabase->m_analysisRequests.getEvent()->resetMaybe();
        if (m_pDatabase->m_analysisRequests.isEmpty()) {
            yield();
            continue;
        }
        while (!m_pDatabase->m_analysisRequests.isEmpty()) {
            s_analysisRequest* pRequest = m_pDatabase->m_analysisRequests.pop();
            igorAddress currentPC = pRequest->m_pc;
            delete pRequest;

            if (m_status == STOPPING || currentPC == IGOR_INVALID_ADDRESS) {
                m_status = STOPPING;
                continue;
            }

            m_status = RUNNING;

            Events::TaskEvent * evt = new Events::TaskEvent;
            m_evts.push_back(std::pair<Events::TaskEvent *, igorAddress>(evt, currentPC));
            TaskMan::registerTask(new IgorAnalysis(m_pDatabase, currentPC, this), evt);
            waitFor(evt);
            //Printer::log(M_INFO, "AnalysisManager spawned a task for %016llx", currentPC);
        }
        yieldNoWait();
    }
}

void IgorLocalSession::freeze() {

}

void IgorLocalSession::thaw() {

}

class IgorSessionSqlite : public IgorSqlite3 {
public:
    static const int CURRENT_VERSION = 1;
    int upgradeDB(int version);
};

int IgorSessionSqlite::upgradeDB(int version) {
    switch (version) {
    case 0:
        version = 1;
        safeWriteStmt("CREATE TABLE IF NOT EXISTS main.properties (name TEXT PRIMARY KEY, value);");
        safeWriteStmt("CREATE TABLE IF NOT EXISTS main.symbols (address, name TEXT PRIMARY KEY, type);");
        safeWriteStmt("CREATE TABLE IF NOT EXISTS main.'references' (addressSrc, addressDst);");
        safeWriteStmt("CREATE TABLE IF NOT EXISTS main.sections (virtualAddress PRIMARY KEY, option, rawData BLOB, instructionSize BLOB);");
        break;
    default:
        Failure("Upgrade case not supported");
    }
    return version;
}

std::tuple<igor_result, String, String> IgorLocalSession::serialize(const char * name) {
    igor_result result = IGOR_SUCCESS;
    String errorMsg1, errorMsg2;

    if (!m_pDatabase) {
        result = IGOR_FAILURE;
        errorMsg1 = "No database";
        return std::tie(result, errorMsg1, errorMsg2);
    }

    try {
        IgorSessionSqlite db;
        db.openDB(name);
        db.safeWriteStmt("BEGIN EXCLUSIVE TRANSACTION;");
        db.createVersionnedDB([&](int version) { return db.upgradeDB(version); }, IgorSessionSqlite::CURRENT_VERSION);
        db.safeWriteStmt("DELETE FROM main.properties;");
        db.safeWriteStmt("DELETE FROM main.symbols;");
        db.safeWriteStmt("DELETE FROM main.'references';");
        db.safeWriteStmt("DELETE FROM main.sections;");
        sqlite3_stmt * stmt = NULL;

        stmt = db.safeStmt("INSERT INTO main.properties (name, value) VALUES(?1, ?2);");
        db.safeBind(stmt, 1, "CPU");
        db.safeBind(stmt, 2, m_pDatabase->m_cpu_modules[0]->getTag());
        db.safeWriteStep(stmt);
        db.safeFinalize(stmt);

        stmt = db.safeStmt("INSERT INTO main.symbols (address, name, type) VALUES(?1, ?2, ?3);");
        for (auto & symbol : m_pDatabase->m_symbolMap) {
            db.safeBind(stmt, 1, (sqlite3_int64)symbol.first.offset);
            db.safeBind(stmt, 2, symbol.second.m_name);
            db.safeBind(stmt, 3, symbol.second.m_type);
            db.safeWriteStep(stmt);
            db.safeReset(stmt);
        }
        db.safeFinalize(stmt);

        stmt = db.safeStmt("INSERT INTO main.'references' (addressSrc, addressDst) VALUES(?1, ?2);");
        for (auto & ref : m_pDatabase->m_references) {
            db.safeBind(stmt, 1, (sqlite3_int64)ref.first.offset);
            db.safeBind(stmt, 2, (sqlite3_int64)ref.second.offset);
            db.safeWriteStep(stmt);
            db.safeReset(stmt);
        }
        db.safeFinalize(stmt);

        stmt = db.safeStmt("INSERT INTO main.sections (virtualAddress, option, rawData, instructionSize) VALUES(?1, ?2, ?3, ?4);");
        for (auto & section : m_pDatabase->m_sections) {
            db.safeBind(stmt, 1, (sqlite3_int64)section->m_virtualAddress);
            db.safeBind(stmt, 2, (sqlite3_int64)section->m_option);
            if (section->m_rawData)
                db.safeBindBlob(stmt, 3, section->m_rawDataSize);
            if (section->m_instructionSize)
                db.safeBindBlob(stmt, 4, section->m_size);
            db.safeWriteStep(stmt);
            sqlite3_int64 rowid = db.lastInsertRowID();
            if (section->m_rawData)
                db.safeWriteWholeBlob("main", "sections", "rawData", rowid, section->m_rawData, section->m_rawDataSize);
            if (section->m_instructionSize)
                db.safeWriteWholeBlob("main", "sections", "instructionSize", rowid, section->m_instructionSize, section->m_size);
            db.safeReset(stmt);
        }
        db.safeFinalize(stmt);

        db.safeWriteStmt("COMMIT TRANSACTION;");

        db.closeDB();
    }
    catch (GeneralException & e) {
        result = IGOR_FAILURE;
        errorMsg1 = e.getMsg();
        errorMsg2 = e.getDetails();
    }
    catch (...) {
        result = IGOR_FAILURE;
        errorMsg1 = "Unknown error";
        errorMsg2 = "Uncatched exception";
    }

    return std::tie(result, errorMsg1, errorMsg2);
}

std::tuple<igor_result, IgorLocalSession *, String, String> IgorLocalSession::deserialize(const char * name) {
    igor_result result = IGOR_FAILURE;
    IgorLocalSession * session = NULL;
    String errorMsg1 = "Not yet implemented";
    String errorMsg2;

    return std::tie(result, session, errorMsg1, errorMsg2);
}

std::tuple<igor_result, IgorLocalSession *, String, String> IgorLocalSession::loadBinary(const char * name) {
    IgorLocalSession * session = new IgorLocalSession();
    igor_result r = IGOR_SUCCESS;
    String errorMsg1, errorMsg2;

    String fileName = name;

    try {
        IO<MMap> reader(new MMap(name));
        reader->open();

        if (fileName.strstr(".exe") != -1) {
            c_PELoader PELoader;
            // Note: having the session here is actually useful not just for the entry point,
            // but for all the possible hints the file might have for us.
            r = PELoader.loadPE(reader, session);
            // Note: destroying the object from the stack would do the same, but
            // as this might trigger a context switch, it's better to do it explicitly
            // than from a destructor, as a general good practice.
        } else if (fileName.strstr(".dmp") != -1) {
            c_dmpLoader dmpLoader;
            r = dmpLoader.load(reader, session);
        } else if (fileName.strstr(".elf") != -1) {
            c_elfLoader elfLoader;
            r = elfLoader.load(reader, session);
        }
        reader->close();

        session->loaded(name);
    }
    catch (GeneralException & e) {
        r = IGOR_FAILURE;
        errorMsg1 = e.getMsg();
        errorMsg2 = e.getDetails();
    }
    catch (...) {
        r = IGOR_FAILURE;
        errorMsg1 = "Unknown error";
        errorMsg2 = "Uncatched exception";
    }

    // Add the task even in case of failure, so it can properly clean itself out.
    TaskMan::registerTask(session);

    return std::tie(r, session, errorMsg1, errorMsg2);
}

const char * IgorLocalSession::getStatusString() {
    switch (m_status) {
        case IDLE: return "IDLE"; break;
        case RUNNING: return "RUNNING"; break;
        case STOPPING: return "STOPPING"; break;
    }
    return "ERROR";
}

void IgorLocalSession::loaded(const char * filename) {
    assignNewUUID();
    setSessionName(filename);
    linkMe();
}

void IgorLocalSession::add_code_analysis_task(igorAddress PC)
{
    if (!m_pDatabase->is_address_flagged_as_code(PC))
    {
        s_analysisRequest * newAnalysisRequest = new s_analysisRequest(PC);

        m_pDatabase->m_analysisRequests.push(newAnalysisRequest);
    }
}

igor_result IgorLocalSession::readS64(igorAddress address, s64& output) { return m_pDatabase->readS64(address, output); }
igor_result IgorLocalSession::readU64(igorAddress address, u64& output) { return m_pDatabase->readU64(address, output); }
igor_result IgorLocalSession::readS32(igorAddress address, s32& output) { return m_pDatabase->readS32(address, output); }
igor_result IgorLocalSession::readU32(igorAddress address, u32& output) { return m_pDatabase->readU32(address, output); }
igor_result IgorLocalSession::readS16(igorAddress address, s16& output) { return m_pDatabase->readS16(address, output); }
igor_result IgorLocalSession::readU16(igorAddress address, u16& output) { return m_pDatabase->readU16(address, output); }
igor_result IgorLocalSession::readS8(igorAddress address, s8& output) { return m_pDatabase->readS8(address, output); }
igor_result IgorLocalSession::readU8(igorAddress address, u8& output) { return m_pDatabase->readU8(address, output); }

igorAddress IgorLocalSession::findSymbol(const char* symbolName) { return m_pDatabase->findSymbol(symbolName); }

int IgorLocalSession::readString(igorAddress address, Balau::String& outputString) { return m_pDatabase->readString(address, outputString); }
c_cpu_module* IgorLocalSession::getCpuForAddress(igorAddress PC) { return m_pDatabase->getCpuForAddress(PC); }
c_cpu_state* IgorLocalSession::getCpuStateForAddress(igorAddress PC) { return m_pDatabase->getCpuStateForAddress(PC);  }
igor_result IgorLocalSession::is_address_flagged_as_code(igorAddress virtualAddress) { return m_pDatabase->is_address_flagged_as_code(virtualAddress); }
igorAddress IgorLocalSession::get_next_valid_address_before(igorAddress virtualAddress) { return m_pDatabase->get_next_valid_address_before(virtualAddress); }
igorAddress IgorLocalSession::get_next_valid_address_after(igorAddress virtualAddress) { return m_pDatabase->get_next_valid_address_after(virtualAddress); }
igor_result IgorLocalSession::flag_address_as_u32(igorAddress virtualAddress) { return m_pDatabase->flag_address_as_u32(virtualAddress); }
igor_result IgorLocalSession::flag_address_as_instruction(igorAddress virtualAddress, u8 instructionSize) { return m_pDatabase->flag_address_as_instruction(virtualAddress, instructionSize); }

igorAddress IgorLocalSession::getEntryPoint() { return m_pDatabase->getEntryPoint(); }
igor_section_handle IgorLocalSession::getSectionFromAddress(igorAddress virtualAddress) { return m_pDatabase->getSectionFromAddress(virtualAddress); }
igorAddress IgorLocalSession::getSectionStartVirtualAddress(igor_section_handle sectionHandle) { return m_pDatabase->getSectionStartVirtualAddress(sectionHandle); }
u64 IgorLocalSession::getSectionSize(igor_section_handle sectionHandle) { return m_pDatabase->getSectionSize(sectionHandle); }
std::tuple<igorAddress, igorAddress, size_t> IgorLocalSession::getRanges() { return m_pDatabase->getRanges(); }
igorAddress IgorLocalSession::linearToVirtual(u64 linear) { return m_pDatabase->linearToVirtual(linear); }

bool IgorLocalSession::getSymbolName(igorAddress address, Balau::String& name) { return m_pDatabase->getSymbolName(address, name); }

void IgorLocalSession::addReference(igorAddress referencedAddress, igorAddress referencedFrom) { m_pDatabase->addReference(referencedAddress, referencedFrom); }
void IgorLocalSession::getReferences(igorAddress referencedAddress, std::vector<igorAddress>& referencedFrom) { m_pDatabase->getReferences(referencedAddress, referencedFrom); }
