#pragma once

#include <vector>

#include <Task.h>

#include "Igor.h"

#include "IgorAPI.h"
#include "IgorSection.h"
#include "cpu/cpuModule.h"

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
	std::vector<s_igorSection*> m_sections;

	c_cpu_module* getCpuForAddress(u64 PC);
	c_cpu_state* getCpuStateForAddress(u64 PC);

	s_igorSection* findSectionFromAddress(u64 address);
	igor_result readByte(u64 address, u8& outputByte);
	igor_result readS32(u64 address, s32& output);
	igor_result readU32(u64 address, u32& output);
	igor_result readS16(u64 address, s16& output);
	igor_result readU16(u64 address, u16& output);
	igor_result readS8(u64 address, s8& output);
	igor_result readU8(u64 address, u8& output);


};

s_igorDatabase* getCurrentIgorDatabase();
