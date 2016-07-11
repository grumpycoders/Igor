// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <Igor.h>
#include <IgorAsyncActions.h>

#include <BString.h>

#include <vector>

#include <imgui.h>
#include "imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <GL/gl3w.h>
#include <SDL.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "Opengl32.lib")

using namespace Balau;

class ImSession
{
public:
    ImSession(IgorSession* pIgorSession) :
        m_pIgorSession(pIgorSession),
        m_commandWindowOpen(false),
        m_symbolListWindowOpen(false),
        m_segmentsListWindowOpen(false)
    {
        m_commandBuffer[0] = 0;

        s_disassemblyWindow* pDisassemblyWindow = new s_disassemblyWindow;
        pDisassemblyWindow->m_currentPC = pIgorSession->getEntryPoint();

        m_pDisassemblyWindows.push_back(pDisassemblyWindow);
    }

    void process();

private:

    struct s_disassemblyWindow
    {
        igorAddress m_currentPC;
    };

    void drawDisassemblyWindow(s_disassemblyWindow* pDisassemblyWindow);
    void drawCommandWindow();
    void drawSymbolListWindow();
    void drawSegmentWindow();

    bool m_commandWindowOpen;
    bool m_symbolListWindowOpen;
    bool m_segmentsListWindowOpen;

    IgorSession* m_pIgorSession;

    enum
    {
        commandBufferSize = 256,
    };
    char m_commandBuffer[commandBufferSize];

    std::vector<s_disassemblyWindow*> m_pDisassemblyWindows;
};

void ImSession::drawCommandWindow()
{
    if (!m_commandWindowOpen)
        return;

    ImGui::Begin("CommandWindow", &m_commandWindowOpen);

    if (ImGui::InputText("Command", m_commandBuffer, commandBufferSize, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        m_pIgorSession->executeCommand(Balau::String(m_commandBuffer));
        m_commandBuffer[0] = 0;
    }

    ImGui::End();
}

void ImSession::drawSymbolListWindow()
{
    s_igorDatabase::t_symbolMap::iterator start;
    s_igorDatabase::t_symbolMap::iterator end;
    m_pIgorSession->lock();
    m_pIgorSession->getSymbolsIterator(start, end);

    int itemId = 0;

    while (start != end)
    {
        Balau::String addressString;
        addressString.append("0x%08llX", start->first.offset);
/*        wxListItem newItem;
        newItem.SetId(itemId);
        newItem.SetText(addressString.to_charp());
        int itemIndex = m_symbolList->InsertItem(newItem);


        m_symbolList->SetItem(itemIndex, 2, start->second.m_name.to_charp());*/

        itemId++;
        start++;
    }

    m_pIgorSession->unlock();
}

void ImSession::drawSegmentWindow()
{
    if (!m_segmentsListWindowOpen)
        return;

    std::vector<igor_segment_handle> segments;
    m_pIgorSession->getSegments(segments);

}

void ImSession::drawDisassemblyWindow(s_disassemblyWindow* pDisassemblyWindow)
{
    IgorSession* pIgorSession = m_pIgorSession;

    ImGui::Begin(pIgorSession->getSessionName().to_charp(), 0, ImGuiWindowFlags_MenuBar);

    bool goToAddressPopup = false;

    // menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Windows"))
        {
            if (ImGui::MenuItem("Commands")) m_commandWindowOpen = true;
            if (ImGui::MenuItem("Symbols")) m_symbolListWindowOpen = true;
            if (ImGui::MenuItem("Segments")) m_segmentsListWindowOpen = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Actions"))
        {
            if (ImGui::MenuItem("Go to address"))
            {
                goToAddressPopup = true;
                
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    if(goToAddressPopup)
        ImGui::OpenPopup("Go to address");

    if (ImGui::BeginPopupModal("Go to address"))
    {
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }


    ImGui::BeginChild("Assembly", ImVec2(0, -30));

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

    igorAddress currentPC = pDisassemblyWindow->m_currentPC;
    int numLinesToDraw = itemEnd - itemStart; // should always be itemEnd

    IgorSession* m_pSession = pIgorSession;

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
                        currentType = currentType;
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

    ImGui::EndChild();

    // commands
    ImGui::Separator();

    ImGui::End();
}

void ImSession::process()
{
    IgorSession* pIgorSession = m_pIgorSession;

    ImGui::PushID(pIgorSession->getSessionName().to_charp());

    for (int i = 0; i < m_pDisassemblyWindows.size(); i++)
    {
        ImGui::PushID(i);
        drawDisassemblyWindow(m_pDisassemblyWindows[i]);
        ImGui::PopID();
    }

    drawSymbolListWindow();
    drawCommandWindow();
    drawSegmentWindow();
    ImGui::PopID();
}

void* initImIgor(void*)
{
	startIgorAsyncWorker();

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return NULL;
	}

	// Setup window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	SDL_Window *window = SDL_CreateWindow("ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	gl3wInit();

	// enable file dropping
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

	// Setup ImGui binding
	ImGui_ImplSdlGL3_Init(window);

	// Load Fonts
	// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
	//ImGuiIO& io = ImGui::GetIO();
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

	bool show_test_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImColor(114, 144, 154);

	std::vector<ImSession*> sessions;

	// Main loop
	bool done = false;
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_DROPFILE)
			{
				igor_result result;
				String errorMsg1, errorMsg2;
				IgorSession * session = NULL;

				std::tie(result, session, errorMsg1, errorMsg2) = IgorAsyncLoadBinary(event.drop.file);

				if (result == IGOR_SUCCESS)
				{
                    ImSession* pNewSession = new ImSession(session);
					sessions.push_back(pNewSession);
				}
			}
		}
		ImGui_ImplSdlGL3_NewFrame(window);

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// sessions
		for (int i = 0; i < sessions.size(); i++)
		{
            sessions[i]->process();
		}

		// Rendering
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplSdlGL3_Shutdown();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

pthread_t imGuiThread;

bool wxIgorStartup(int& argc, char** argv)
{
	pthread_create(&imGuiThread, NULL, initImIgor, NULL);

	return true;
}

void wxIgorExit()
{

}
