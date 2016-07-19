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

#include "imIgorAsmView.h"

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

        imIgorAsmView* pDisassemblyWindow = new imIgorAsmView(pIgorSession);
        m_pDisassemblyWindows.push_back(pDisassemblyWindow);
    }

    void process();

private:

    void drawDisassemblyWindow(imIgorAsmView* pDisassemblyWindow);
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

    std::vector<imIgorAsmView*> m_pDisassemblyWindows;
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

    ImGui::Begin("Segments", &m_segmentsListWindowOpen);

    ImGui::Columns(3);

    ImGui::Text("name");
    ImGui::NextColumn();
    ImGui::Text("Start");
    ImGui::NextColumn();
    ImGui::Text("End");
    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::Separator();

    int hoovertedSegmentId = -1;

    for (int i = 0; i < segments.size(); i++)
    {
        Balau::String name;
        m_pIgorSession->getSegmentName(segments[i], name);
        igorAddress segmentsStart = m_pIgorSession->getSegmentStartVirtualAddress(segments[i]);
        m_pIgorSession->getSegmentStartVirtualAddress(segments[i]);
        u64 segmentSize = m_pIgorSession->getSegmentSize(segments[i]);
        igorAddress segmentEnd = segmentsStart + segmentSize;

        ImGui::Selectable(name.to_charp(), false, ImGuiSelectableFlags_SpanAllColumns);

        if (ImGui::IsItemHovered())
            hoovertedSegmentId = i;

        ImGui::NextColumn();
        ImGui::Text("0x%llX", segmentsStart.offset);
        ImGui::NextColumn();
        ImGui::Text("0x%llX", segmentEnd.offset);
        ImGui::NextColumn();
        ImGui::Separator();

        // handle segment specific popup
        {
            ImGui::PushID(i);
            if (ImGui::IsMouseClicked(1))
            {
                if (hoovertedSegmentId != -1)
                    ImGui::OpenPopup("Edit segment");
            }

            bool bDeleteSegment = false;
            if (ImGui::BeginPopup("Edit segment"))
            {
                if (ImGui::Selectable("Delete", false))
                {
                    bDeleteSegment = true;

                }
                if (ImGui::Selectable("Edit", false))
                {

                }
                ImGui::EndPopup();
            }

            if (bDeleteSegment)
                ImGui::OpenPopup("Delete segment");

            if (ImGui::BeginPopupModal("Delete segment", 0, ImGuiWindowFlags_NoResize))
            {
                ImGui::Text("Are you sure?");
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
            ImGui::PopID();
        }
    }

    ImGui::Columns(1);

    // handle right click when not hovering a segment
    {
        if (ImGui::IsMouseClicked(1))
        {
            if (hoovertedSegmentId == -1)
                ImGui::OpenPopup("New Segment");
        }

        static char segmentName[256];
        static char startAddressString[256]; // needs to be string because 64bit pointers
        static char endAddressString[256];

        bool bCreateSegment = false;
        if (ImGui::BeginPopup("New Segment"))
        {
            if (ImGui::Selectable("Create New Segment", false))
            {
                bCreateSegment = true;
            }
            ImGui::EndPopup();
        }

        if (bCreateSegment)
        {
            strcpy(segmentName, "");
            strcpy(startAddressString, "0");
            strcpy(endAddressString, "0");
            ImGui::OpenPopup("Create New Segment");
        }

        if (ImGui::BeginPopupModal("Create New Segment", 0))
        {
            ImGui::InputText("Segment name", segmentName, sizeof(segmentName));

            ImGui::Text("0x"); ImGui::SameLine();
            ImGui::InputText("Start Address", startAddressString, sizeof(startAddressString), ImGuiInputTextFlags_CharsHexadecimal);

            ImGui::Text("0x"); ImGui::SameLine();
            ImGui::InputText("End Address", endAddressString, sizeof(endAddressString), ImGuiInputTextFlags_CharsHexadecimal);

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                // try to create new segment
                u64 startAddress = 0;
                u64 endAddress = 0;

                sscanf(startAddressString, "%llX", &startAddress);
                sscanf(endAddressString, "%llX", &endAddress);

                if (endAddress > startAddress)
                {
                    u64 size = endAddress - startAddress;
                    igor_segment_handle newHandle;
                    m_pIgorSession->create_segment(startAddress, size, newHandle);

                    if (newHandle != -1)
                    {
                        m_pIgorSession->setSegmentName(newHandle, Balau::String(segmentName));
                    }
                }

                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void ImSession::drawDisassemblyWindow(imIgorAsmView* pDisassemblyWindow)
{
    bool goToAddressPopup = false;

    ImGui::Begin(m_pIgorSession->getSessionName().to_charp(), 0, ImGuiWindowFlags_MenuBar);

    // menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Files"))
        {
            if (ImGui::MenuItem("Save database"))
            {
                m_pIgorSession->serialize("test.igor");
            }

            ImGui::EndMenu();
        }

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

    if (goToAddressPopup)
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

    pDisassemblyWindow->Update();

    // commands
    ImGui::Separator();

    ImGui::End();
}

void ImSession::process()
{
    ImGui::GetStyle().WindowRounding = 0.f;
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.95f;

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

                if (strstr(event.drop.file, ".igor"))
                {
                    std::tie(result, session, errorMsg1, errorMsg2) = IgorLocalSession::deserialize(event.drop.file);
                }
                else
                {
                    std::tie(result, session, errorMsg1, errorMsg2) = IgorAsyncLoadBinary(event.drop.file);
                }

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
                if (ImGui::MenuItem("New database"))
                {
                    IgorSession* pNewIgorSession = new IgorLocalSession();
                    ImSession* pNewSession = new ImSession(pNewIgorSession);
                    sessions.push_back(pNewSession);
                }
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
