#include "imIgorAsmView.h"

#include <vector>

#include <Igor.h>
#include <IgorAsyncActions.h>

#include <imgui.h>
#include <BString.h>
#include <SDL.h>

using namespace Balau;

imIgorAsmView::imIgorAsmView(IgorSession* pSession)
{
    m_pSession = pSession;
    m_cursorPosition = pSession->getEntryPoint();
}

void imIgorAsmView::goToAddress(igorAddress newAddress)
{
    if (newAddress.isValid())
    {
        m_history.push(m_cursorPosition);
        m_cursorPosition = newAddress;
    }
}

void imIgorAsmView::popAddress()
{
    if (!m_history.empty())
    {
        m_cursorPosition = m_history.top();
        m_history.pop();
    }
}

igorAddress imIgorAsmView::generateTextForAddress(igorAddress address)
{
    s_textCacheEntry currentEntry;

    s_IgorPropertyBag propertyBag;
    m_pSession->getProperties(address, propertyBag);

    igor_segment_handle hSection = m_pSession->getSegmentFromAddress(address);
    if (hSection != 0xFFFF)
    {
        String sectionName;
        if (m_pSession->getSegmentName(hSection, sectionName))
        {
            currentEntry.m_text.append("%s:", sectionName.to_charp());
        }
    }

    currentEntry.m_text.append("%016llX: ", address.offset);

    if (s_IgorPropertySymbol* pPropertySymbol = (s_IgorPropertySymbol*)propertyBag.findProperty(s_IgorProperty::Symbol))
    {
        s_textCacheEntry symbolNameEntry;

        symbolNameEntry.m_address = address;
        symbolNameEntry.m_text.append(currentEntry.m_text.to_charp());
        symbolNameEntry.m_text.append("%s%s%s:", c_cpu_module::startColor(c_cpu_module::KNOWN_SYMBOL).to_charp(), pPropertySymbol->m_symbol.to_charp(), c_cpu_module::finishColor(c_cpu_module::KNOWN_SYMBOL).to_charp());

        m_textCache.push_back(symbolNameEntry);
    }

    // display cross references
    {
        std::vector<igorAddress> crossReferences;
        m_pSession->getReferences(address, crossReferences);

        for (int i = 0; i < crossReferences.size(); i++)
        {
            s_textCacheEntry crossRefEntry;
            crossRefEntry.m_address = address;
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

    if (s_IgorPropertyCode* pPropertyCode = (s_IgorPropertyCode*)propertyBag.findProperty(s_IgorProperty::Code))
    {
        currentEntry.m_text.append("%s", pPropertyCode->m_instruction.to_charp());
        m_textCache.push_back(currentEntry);

        address += pPropertyCode->m_instructionSize;
    }
    else
    {
        currentEntry.m_text.append("db       0x%02X", m_pSession->readU8(address));

        m_textCache.push_back(currentEntry);

        address++;
    }

    return address;
}

void imIgorAsmView::Update()
{
    IgorSession* pIgorSession = m_pSession;

    ImGui::BeginChild("Assembly", ImVec2(0, -30), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (ImGui::IsKeyPressed(SDLK_UP& ~SDLK_SCANCODE_MASK, true) || (ImGui::GetIO().MouseWheel >= 1))
    {
        m_cursorPosition = m_pSession->get_next_valid_address_before(m_cursorPosition - 1);        
    }
    if (ImGui::IsKeyPressed(SDLK_DOWN& ~SDLK_SCANCODE_MASK, true) || (ImGui::GetIO().MouseWheel <= -1))
    {
        m_cursorPosition = m_pSession->get_next_valid_address_after(m_cursorPosition + 1);
    }
    if (ImGui::IsKeyPressed(SDLK_PAGEUP& ~SDLK_SCANCODE_MASK, true))
    {
        m_cursorPosition = m_pSession->get_next_valid_address_before(m_cursorPosition - 10);
    }
    if (ImGui::IsKeyPressed(SDLK_PAGEDOWN& ~SDLK_SCANCODE_MASK, true))
    {
        m_cursorPosition = m_pSession->get_next_valid_address_after(m_cursorPosition + 10);
    }
    if (ImGui::IsKeyPressed(SDLK_ESCAPE& ~SDLK_SCANCODE_MASK, true))
    {
        popAddress();
    }

    m_textCache.clear();

    ImVec2 displayStart = ImGui::GetCursorPos();
    float lineHeight = ImGui::GetTextLineHeight();

    // compute how many lines of text we can fit
    int itemStart = 0;
    int itemEnd = 0;
    ImGui::CalcListClipping(INT_MAX, lineHeight, &itemStart, &itemEnd);

    igorAddress currentPC = m_cursorPosition;
    if (currentPC.isValid())
    {
        int numLinesToDraw = itemEnd - itemStart; // should always be itemEnd
        while (m_textCache.size() < numLinesToDraw)
        {
            if (!currentPC.isValid())
                break;

            currentPC = generateTextForAddress(currentPC);
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
                            if (ImGui::IsMouseDoubleClicked(0))
                            {
                                igorAddress address = m_pSession->findSymbol(stringList[i].to_charp());

                                if (address.isValid())
                                {
                                    goToAddress(address);
                                }
                                else
                                {
                                    u64 offset;
                                    if (stringList[i].scanf("0x%016llX", &offset))
                                    {
                                        // TODO: need to find a section here
                                        goToAddress(igorAddress(m_pSession, offset, -1));
                                    }
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
    }

    ImGui::EndChild();
}
