#pragma once

#include <list>
#include <Task.h>
#include "Igor.h"
#include "IgorDatabase.h"

class IgorAnalysisManager : public Balau::Task {
public:
    void add_code_analysis_task(u64 PC);
    void Do();
    void stop() { add_code_analysis_task((u64) -1); }
    const char * getName() const { return "IgorAnalysisManager"; }
    bool isRunning() { return m_status == RUNNING; }
    void setDB(s_igorDatabase * db) { AAssert(m_pDatabase == NULL, "Can only set database once"); m_pDatabase = db; }
    s_igorDatabase * getDB() { return m_pDatabase; }
    const char * getStatusString();
private:
    enum {
        IDLE,
        RUNNING,
        STOPPING,
    } m_status = IDLE;
    s_igorDatabase * m_pDatabase = NULL;
    std::list<std::pair<Balau::Events::TaskEvent *, u64>> m_evts;
};

class IgorAnalysis : public Balau::Task {
public:
    IgorAnalysis(s_igorDatabase * db, u64 PC, IgorAnalysisManager * parent) : m_pDatabase(db), m_PC(PC), m_parent(parent) { m_name.set("IgorAnalysis for %016llx", PC); }
    void Do();
    const char * getName() const { return m_name.to_charp(); }
private:
    s_igorDatabase * m_pDatabase = NULL;
    IgorAnalysisManager * m_parent;
    u64 m_PC = 0;
    Balau::String m_name;
};
