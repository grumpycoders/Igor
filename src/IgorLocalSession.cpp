#include "IgorAnalysis.h"
#include "IgorLocalSession.h"

#include "google/protobuf/io/zero_copy_stream.h"
#include "protobufs/IgorProtoFile.pb.h"

#include <Printer.h>
#include <BString.h>
#include <TaskMan.h>

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

class gprotInput : public google::protobuf::io::ZeroCopyInputStream {
  public:
      gprotInput(IO<Handle> h) : m_h(h) { }

  private:
    virtual bool Next(const void ** data, int * size) override;
    virtual void BackUp(int count) override { m_availBytes += count; }
    virtual bool Skip(int count) override;
    virtual google::protobuf::int64 ByteCount() const override { return m_availBytes; }

    char m_data[1024];
    IO<Handle> m_h;
    int m_availBytes = 0;

    google::protobuf::int64 m_size = 0;
};

bool gprotInput::Next(const void ** data, int * size) {
    return safeOperationLambda([&]() -> bool {
        if (!m_availBytes) {
            ssize_t r = m_h->read(m_data, sizeof(m_data));
            if (r < 0)
                return false;
            m_availBytes = r;
            m_size += r;
        }
        *data = m_data;
        *size = m_availBytes;

        return true;
    });
}

bool gprotInput::Skip(int count) {
    bool success = safeOperationLambda([&]() -> bool {
        if (m_availBytes < count) {
            count -= m_availBytes;
            m_availBytes = 0;
            if (m_h->canSeek()) {
                m_h->seek(count, SEEK_CUR);
            }
            else {
                void * buf;

                if (count >= 1024)
                    buf = malloc(count);
                else
                    buf = alloca(count);

                m_h->forceRead(buf, count);

                if (count >= 1024)
                    free(buf);
            }
        }
        else {
            m_availBytes -= count;
        }

        return true;
    });

    return success || !m_h->isEOF();
}

class gprotOutput : public google::protobuf::io::ZeroCopyOutputStream {
  public:
      gprotOutput(IO<Handle> h) : m_h(h) { }
      virtual ~gprotOutput() { maybeFlush(); }

    bool maybeFlush();

  private:
    virtual bool Next(void ** data, int * size) override;
    virtual void BackUp(int count) override { m_bufSize -= count; }
    virtual google::protobuf::int64 ByteCount() const override { return m_size; }

    char m_data[1024];
    IO<Handle> m_h;
    int m_bufSize = 0;
    google::protobuf::int64 m_size = 0;
};

bool gprotOutput::Next(void ** data, int * size) {
    return safeOperationLambda([&]() {
        if (!maybeFlush())
            return false;
        m_bufSize = *size = sizeof(m_data);
        *data = m_data;
        return true;
    });
}

bool gprotOutput::maybeFlush() {
    ssize_t r = 0;
    if (m_bufSize)
        r = m_h->forceWrite(m_data, m_bufSize);
    if (r < 0)
        return false;
    m_size += r;
    m_bufSize = 0;
    return true;
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

void IgorLocalSession::serialize(IO<Handle> file) {
    IgorProtoFile::IgorFile protoFile;

    s_igorDatabase * db = getDB();

    protoFile.set_uuid(getUUID().to_charp());
    const char * name = getSessionName().to_charp();
    if (name && name[0])
        protoFile.set_name(name);
    protoFile.mutable_architecture()->set_manufacturer(IgorProtoFile::IgorFile_CPUManufacturer_INTEL);
    auto protoSymbols = protoFile.mutable_symbols();
    protoSymbols->Reserve(db->m_symbolMap.size());
    for (auto & i : db->m_symbolMap) {
        auto fileSymbol = protoSymbols->Add();
        const igorAddress & addr = i.first;
        const s_igorDatabase::s_symbolDefinition & symbol = i.second;

    }

    gprotOutput gpFile(file);
    protoFile.SerializeToZeroCopyStream(&gpFile);

    gpFile.maybeFlush();
}

IgorLocalSession * IgorLocalSession::deserialize(IO<Handle> file) {
    return NULL;
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