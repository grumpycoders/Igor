#pragma once

#include <Igor.h>

class imIgorAsmView
{
public:
	imIgorAsmView(IgorSession* pSession);

	void Update();

private:
	IgorSession* m_pSession;
	igorAddress m_cursorPosition;
};