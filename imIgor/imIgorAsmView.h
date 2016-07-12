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
	IgorSession* m_pSession;
	igorAddress m_cursorPosition;

    std::stack<igorAddress> m_history;
};
