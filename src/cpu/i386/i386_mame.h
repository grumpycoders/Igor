#pragma once

#include "cpu/cpuModule.h"

class c_i386_mame : public c_cpu_module
{
public:
	static c_cpu_module* create(s_cpuConstructionFlags*)
	{
		return new c_i386_mame();
	}

	static void registerCpuModule(std::vector<const s_cpuInfo*>& cpuList);

	c_i386_mame();
	virtual Balau::String getTag() const override { return "i386"; }
	virtual igor_result analyze(s_analyzeState* pState) override;
	//virtual igor_result printInstruction(s_analyzeState* pState, Balau::String& outputString, bool bUseColor = false) override;


private:

};