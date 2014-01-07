#include "IgorAnalysis.h"
#include "IgorLocalSession.h"

#include <Printer.h>
#include <BString.h>
#include <TaskMan.h>

using namespace Balau;

void IgorLocalSession::Do()
{
    if (!m_pDatabase)
        return;

    waitFor(m_pDatabase->m_analysisRequests.getEvent());

    Printer::log(M_INFO, "AnalysisManager starting...");
    m_instructions = 0;

    for (;;)
    {
        for (auto i = m_evts.begin(); i != m_evts.end(); i++)
        {
            Events::TaskEvent * evt = i->first;
            if (evt->gotSignal())
            {
                evt->ack();
                delete evt;
                m_evts.erase(i);
                i = m_evts.begin();
                if (m_evts.empty())
                    break;
            }
        }
        if (m_evts.size() == 0)
        {
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

            if (m_status == STOPPING || currentPC == IGOR_INVALID_ADDRESS)
            {
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

const char * IgorLocalSession::getStatusString() {
    switch (m_status) {
        case IDLE: return "IDLE"; break;
        case RUNNING: return "RUNNING"; break;
        case STOPPING: return "STOPPING"; break;
    }
    return "ERROR";
}

void IgorLocalSession::add_code_analysis_task(igorAddress PC)
{
    if (!m_pDatabase->is_address_flagged_as_code(PC))
    {
        s_analysisRequest * newAnalysisRequest = new s_analysisRequest(PC);

        m_pDatabase->m_analysisRequests.push(newAnalysisRequest);
    }
}

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
