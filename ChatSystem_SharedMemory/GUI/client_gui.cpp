#include "client_gui.h"
#include <tchar.h>
#include <iostream>

// Global pointer to the ClientGUI instance for WndProc callback
ClientGUI* g_pClientGUI = NULL;

// Forward declaration
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ClientGUI::ClientGUI() : state(ClientState::ENTERING_USERNAME), show_history_on_connect(false),
                        hwnd(NULL), g_pd3dDevice(NULL), g_pd3dDeviceContext(NULL),
                        g_pSwapChain(NULL), g_mainRenderTargetView(NULL) {
    memset(username_input, 0, sizeof(username_input));
    memset(message_input, 0, sizeof(message_input));

    // Set message callback
    client.set_message_callback([this](const Message& msg) {
        this->HandleNewMessage(msg);
    });
}

ClientGUI::~ClientGUI() {
    Shutdown();
}

bool ClientGUI::Initialize() {
    // Store global pointer to this instance
    g_pClientGUI = this;
    
    // Create application window
    wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ChatClient"), NULL };
    ::RegisterClassEx(&wc);

    hwnd = ::CreateWindow(wc.lpszClassName, _T("Chat Client"), WS_OVERLAPPEDWINDOW,
                         150, 150, 900, 700, NULL, NULL, wc.hInstance, NULL);

    if (!hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    return true;
}

void ClientGUI::Run() {
    if (!hwnd) return;

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT) {
        // Poll and handle messages (inputs, window resize, etc.)
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render GUI
        RenderGUI();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.35f, 0.45f, 0.50f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }
}

void ClientGUI::Shutdown() {
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    if (hwnd) {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        hwnd = NULL;
    }

    client.disconnect();
}

void ClientGUI::RenderGUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Chat Client", NULL,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    switch (state) {
    case ClientState::ENTERING_USERNAME: {
        ImGui::Separator();
        ImGui::Text("Welcome to the Chat System");
        ImGui::Spacing();
        ImGui::Text("Please enter your username to connect:");

        ImGui::SetNextItemWidth(200);
        bool enter_pressed = ImGui::InputText("Username", username_input, IM_ARRAYSIZE(username_input),
                                             ImGuiInputTextFlags_EnterReturnsTrue);

        if ((ImGui::Button("Connect", ImVec2(120, 30)) || enter_pressed) && strlen(username_input) > 0) {
            state = ClientState::CONNECTING;
            std::cout << "Attempting to connect as: " << username_input << std::endl;
        }

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                          "Note: Make sure the server is running before connecting.");
        break;
    }

    case ClientState::CONNECTING: {
        ImGui::Separator();
        ImGui::Text("Connecting to server...");
        ImGui::Text("Username: %s", username_input);

        // Try to connect
        if (client.connect(username_input)) {
            state = ClientState::CHATTING;
            show_history_on_connect = true;
            std::cout << "Successfully connected!" << std::endl;
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Connection failed!");
            ImGui::Text("Possible reasons:");
            ImGui::BulletText("Server is not running");
            ImGui::BulletText("Username already taken");
            ImGui::BulletText("Maximum clients reached");

            if (ImGui::Button("Back", ImVec2(120, 30))) {
                state = ClientState::ENTERING_USERNAME;
            }
        }
        break;
    }

    case ClientState::CHATTING: {
        UpdateClientStatus();

        // Connection status
        ImGui::Separator();
        if (client.is_connected()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected as: %s", client.get_username().c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");
            state = ClientState::DISCONNECTED;
            break;
        }

        // Connected clients
        ImGui::Text("Connected Users (%d):", (int)connected_clients.size());
        ImGui::SameLine();
        std::string client_list;
        for (size_t i = 0; i < connected_clients.size(); ++i) {
            if (i > 0) client_list += ", ";
            client_list += connected_clients[i];
        }
        ImGui::TextWrapped("%s", client_list.c_str());

        ImGui::Separator();

        // Chat messages
        ImGui::Text("Chat Messages:");
        ImGui::BeginChild("ChatMessages", ImVec2(0, 400), true);

        if (show_history_on_connect) {
            // Show message history when first connecting
            auto history = client.get_message_history();
            for (const auto& msg : history) {
                chat_messages.push_back(msg);
            }
            show_history_on_connect = false;
        }

        for (const auto& msg : chat_messages) {
            char time_str[32];
            auto time_t = std::chrono::system_clock::to_time_t(msg.timestamp);
            strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&time_t));

            if (msg.is_broadcast) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "[%s] %s: %s",
                                  time_str, msg.username, msg.content);
            } else {
                ImGui::Text("[%s] %s: %s", time_str, msg.username, msg.content);
            }
        }

        // Auto-scroll to bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();

        ImGui::Separator();

        // Message input
        ImGui::Text("Your Message:");
        ImGui::InputTextMultiline("##message", message_input, IM_ARRAYSIZE(message_input),
                                 ImVec2(-1, 60), ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImGui::Button("Send Message", ImVec2(120, 30)) && strlen(message_input) > 0) {
            if (client.send_message(message_input)) {
                memset(message_input, 0, sizeof(message_input)); // Clear after sending
                ImGui::SetKeyboardFocusHere(-1); // Refocus input
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Disconnect", ImVec2(120, 30))) {
            client.disconnect();
            state = ClientState::ENTERING_USERNAME;
            chat_messages.clear();
            connected_clients.clear();
            memset(username_input, 0, sizeof(username_input));
            memset(message_input, 0, sizeof(message_input));
        }

        break;
    }

    case ClientState::DISCONNECTED: {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected from server");

        if (ImGui::Button("Reconnect", ImVec2(120, 30))) {
            state = ClientState::ENTERING_USERNAME;
            chat_messages.clear();
            connected_clients.clear();
        }
        break;
    }
    }

    ImGui::End();
}

void ClientGUI::UpdateClientStatus() {
    if (client.is_connected()) {
        connected_clients = client.get_connected_clients();
    }
}

void ClientGUI::HandleNewMessage(const Message& msg) {
    chat_messages.push_back(msg);

    // Keep only last 1000 messages to prevent memory issues
    if (chat_messages.size() > 1000) {
        chat_messages.erase(chat_messages.begin());
    }
}

// Helper functions (same as server GUI)
bool ClientGUI::CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2,
                                     D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void ClientGUI::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void ClientGUI::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void ClientGUI::CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler (static global)
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pClientGUI && g_pClientGUI->g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            g_pClientGUI->CleanupRenderTarget();
            g_pClientGUI->g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            g_pClientGUI->CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
