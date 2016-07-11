#include "imIgorAsmView.h"

#include <vector>

#include <Igor.h>
#include <IgorAsyncActions.h>

#include <imgui.h>
#include <BString.h>

using namespace Balau;

imIgorAsmView::imIgorAsmView(IgorSession* pSession)
{
	m_pSession = pSession;
	m_cursorPosition = pSession->getEntryPoint();
}

void imIgorAsmView::Update()
{
	IgorSession* pSession = m_pSession;
	assert(pSession);

	ImGui::Begin(pSession->getSessionName().to_charp());

	struct s_textCacheEntry
	{
		igorAddress m_address;
		Balau::String m_text;
	};
	std::vector<s_textCacheEntry> m_textCache;

	ImVec2 displayStart = ImGui::GetCursorPos();
	float lineHeight = ImGui::GetTextLineHeight();

	// compute how many lines of text we can fit
	int itemStart = 0;
	int itemEnd = 0;
	ImGui::CalcListClipping(INT_MAX, lineHeight, &itemStart, &itemEnd);

	igorAddress destinationPC;
	destinationPC.makeInvalid();

	igorAddress currentPC = m_cursorPosition;
	int numLinesToDraw = itemEnd - itemStart; // should always be itemEnd

	IgorSession* m_pSession = pSession;

	{
		c_cpu_module* pCpu = m_pSession->getCpuForAddress(currentPC);

		s_analyzeState analyzeState;
		analyzeState.m_PC = currentPC;
		analyzeState.pCpu = pCpu;
		analyzeState.pCpuState = m_pSession->getCpuStateForAddress(currentPC);
		analyzeState.pSession = m_pSession;
		analyzeState.m_cpu_analyse_result = NULL;
		if (pCpu)
		{
			analyzeState.m_cpu_analyse_result = pCpu->allocateCpuSpecificAnalyseResult();
		}

		while (m_textCache.size() < numLinesToDraw)
		{
			s_textCacheEntry currentEntry;

			currentEntry.m_address = analyzeState.m_PC;

			igor_segment_handle hSection = m_pSession->getSegmentFromAddress(analyzeState.m_PC);
			if (hSection != 0xFFFF)
			{
				String sectionName;
				if (m_pSession->getSegmentName(hSection, sectionName))
				{
					currentEntry.m_text.append("%s:", sectionName.to_charp());
				}
			}

			currentEntry.m_text.append("%016llX: ", analyzeState.m_PC.offset);

			Balau::String symbolName;
			if (m_pSession->getSymbolName(analyzeState.m_PC, symbolName))
			{
				s_textCacheEntry symbolNameEntry;

				symbolNameEntry.m_address = analyzeState.m_PC;
				symbolNameEntry.m_text.append(currentEntry.m_text.to_charp());
				symbolNameEntry.m_text.append("%s%s%s:", c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL).to_charp(), symbolName.to_charp(), c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL).to_charp());

				m_textCache.push_back(symbolNameEntry);
			}

			// display cross references
			{
				std::vector<igorAddress> crossReferences;
				m_pSession->getReferences(analyzeState.m_PC, crossReferences);

				for (int i = 0; i < crossReferences.size(); i++)
				{
					s_textCacheEntry crossRefEntry;
					crossRefEntry.m_address = analyzeState.m_PC;
					crossRefEntry.m_text.append(currentEntry.m_text.to_charp());
					crossRefEntry.m_text.append("                            xref: "); // alignment of xref

					Balau::String symbolName;
					if (m_pSession->getSymbolName(crossReferences[i], symbolName))
					{
						crossRefEntry.m_text.append("%s%s 0x%08llX%s", c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL).to_charp(), symbolName.to_charp(), crossReferences[i].offset, c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL).to_charp());
					}
					else
					{
						crossRefEntry.m_text.append("%s0x%08llX%s", c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL).to_charp(), crossReferences[i].offset, c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL).to_charp());
					}

					m_textCache.push_back(crossRefEntry);
				}
			}

			currentEntry.m_text.append("         "); // alignment of code/data

			if (m_pSession->is_address_flagged_as_code(analyzeState.m_PC) && (pCpu->analyze(&analyzeState) == IGOR_SUCCESS))
			{
				/*
				Balau::String mnemonic;
				pCpu->getMnemonic(&analyzeState, mnemonic);

				const int numMaxOperand = 4;

				Balau::String operandStrings[numMaxOperand];

				int numOperandsInInstruction = pCpu->getNumOperands(&analyzeState);
				EAssert(numMaxOperand >= numOperandsInInstruction);

				for (int i = 0; i < numOperandsInInstruction; i++)
				{
				pCpu->getOperand(&analyzeState, i, operandStrings[i]);
				}
				*/

				pCpu->printInstruction(&analyzeState, currentEntry.m_text, true);

				m_textCache.push_back(currentEntry);

				analyzeState.m_PC = analyzeState.m_cpu_analyse_result->m_startOfInstruction + analyzeState.m_cpu_analyse_result->m_instructionSize;
			}
			else
			{
				currentEntry.m_text.append("db       0x%02X", m_pSession->readU8(analyzeState.m_PC));

				m_textCache.push_back(currentEntry);

				analyzeState.m_PC++;
			}
		}

		delete analyzeState.m_cpu_analyse_result;
	}

	// draw the text cache

	int currentLine = 0;

	while (currentLine < m_textCache.size())
	{
		// draw the text while honoring color changes
		float currentX = 0;
		Balau::String& currentText = m_textCache[currentLine].m_text;
		Balau::String::List stringList = currentText.split(';');

		//dc.SetTextForeground(*wxBLACK);

		ImVec4 currentColor = ImColor(1.f, 1.f, 1.f);

		c_cpu_module::e_colors currentType = c_cpu_module::DEFAULT;

		for (int i = 0; i < stringList.size(); i++)
		{
			if (strstr(stringList[i].to_charp(), "C="))
			{
				int blockType;
				int numParsedElements = stringList[i].scanf("C=%d", &blockType);
				assert(numParsedElements == 1);
				assert(blockType >= 0);
				assert(blockType < c_cpu_module::COLOR_MAX);

				currentType = (c_cpu_module::e_colors)blockType;

			}
			else
			{
				currentColor = ImColor(c_cpu_module::getColorForType(currentType));
				ImGui::SetCursorPosX(currentX);
				ImGui::SetCursorPosY(displayStart.y + currentLine * lineHeight);

				ImVec2 textSize = ImGui::CalcTextSize(stringList[i].to_charp());

				if ((currentType == c_cpu_module::KNOWN_SYMBOL) || (currentType == c_cpu_module::MEMORY_ADDRESS))
				{
					if (ImGui::Selectable(stringList[i].to_charp(), false, ImGuiSelectableFlags_AllowDoubleClick, textSize))
					{
						destinationPC = m_pSession->findSymbol(stringList[i].to_charp());

						if (destinationPC.isNotValid())
						{
							u64 offset;
							if (stringList[i].scanf("0x%016llX", &offset))
							{
								// TODO: need to find a section here
								destinationPC = igorAddress(m_pSession, offset, -1);
							}
						}
					}
				}

				ImGui::SetCursorPosX(currentX);
				ImGui::SetCursorPosY(displayStart.y + currentLine * lineHeight);
				ImGui::TextColored(currentColor, stringList[i].to_charp());

				currentX += textSize.x;
			}
		}

		currentLine++;
	}
	ImGui::End();

	if (destinationPC.isValid())
	{
		m_cursorPosition = destinationPC;
	}
}
