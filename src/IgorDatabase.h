#pragma once

#include <vector>

#include <Task.h>

#include "Igor.h"

#include "yVector.h"

#include "IgorAPI.h"
#include "IgorSection.h"
#include "cpuModule.h"

typedef int igor_cpu_handle;
igor_result igor_add_cpu(c_cpu_module* pCpuModule, igor_cpu_handle& outputCpuHandle);

struct s_analysisRequest
{
    s_analysisRequest(u64 PC) : m_pc(PC) { }
	u64 m_pc;
};

struct s_igorDatabase
{
	std::vector<c_cpu_module*> m_cpu_modules;
	Balau::TQueue<s_analysisRequest> m_analysisRequests;
	VECTOR<s_igorSection> m_sections;

	c_cpu_module* getCpuForAddress(u64 PC);
	c_cpu_state* getCpuStateForAddress(u64 PC);

	s_igorSection* findSectionFromAddress(u64 address);
	igor_result readByte(u64 address, u8& outputByte);
	igor_result readS32(u64 address, s32& output);
	igor_result readU32(u64 address, u32& output);
};

s_igorDatabase* getCurrentIgorDatabase();
