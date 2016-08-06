#pragma once

#include <Igor.h>
#include <stack>

class imIgorAsmView
{
public:
    imIgorAsmView(IgorSession* pSession);

    void goToAddress(igorAddress newAddress);
    void popAddress();
    void Update();

private:
    struct s_textCacheEntry
    {
        igorAddress m_address;
        Balau::String m_text;
    };
    std::vector<s_textCacheEntry> m_textCache;

    igorAddress generateTextForAddress(igorAddress address);

    IgorSession* m_pSession;

    std::stack<igorAddress> m_history;
};
