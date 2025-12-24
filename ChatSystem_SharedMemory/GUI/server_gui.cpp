#include "server_gui.h"
#include <tchar.h>
#include <iostream>

ServerGUI* g_pServerGUI = NULL;

// Forward declaration
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ServerGUI::ServerGUI() : server_started(false), hwnd(NULL), g_pd3dDevice(NULL),
                        g_pd3dDeviceContext(NULL), g_pSwapChain(NULL), g_mainRenderTargetView(NULL) {
    memset(broadcast_text, 0, sizeof(broadcast_text));
}

ServerGUI::~ServerGUI() {
    Shutdown();
}

bool ServerGUI::Initialize() {
    // Store global pointer to this instance
    g_pServerGUI = this;
    
    // Create application window
    wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ChatServer"), NULL };
    ::RegisterClassEx(&wc);

    hwnd = ::CreateWindow(wc.lpszClassName, _T("Chat Server Control Panel"), WS_OVERLAPPEDWINDOW,
                         100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

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

void ServerGUI::Run() {
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
        const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }
}

void ServerGUI::Shutdown() {
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

    if (server_started) {
        server.stop();
    }
}

void ServerGUI::RenderGUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Chat Server Control Panel", NULL,
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

    // Server Control Section
    ImGui::Separator();
    ImGui::Text("Server Control");

    if (!server_started) {
        if (ImGui::Button("Start Server", ImVec2(120, 30))) {
            if (server.initialize()) {
                server.start();
                server_started = true;
                std::cout << "Server started from GUI" << std::endl;
            }
        }
    } else {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Server Running");
        if (ImGui::Button("Stop Server", ImVec2(120, 30))) {
            server.stop();
            server_started = false;
            std::cout << "Server stopped from GUI" << std::endl;
        }
    }

    ImGui::Separator();

    // Broadcast Section
    ImGui::Text("Broadcast Message");
    ImGui::InputTextMultiline("##broadcast", broadcast_text, IM_ARRAYSIZE(broadcast_text),
                             ImVec2(-1, 60), ImGuiInputTextFlags_EnterReturnsTrue);

    if (ImGui::Button("Send Broadcast", ImVec2(120, 30)) && server_started && strlen(broadcast_text) > 0) {
        if (server.broadcast_message(broadcast_text)) {
            memset(broadcast_text, 0, sizeof(broadcast_text)); // Clear after sending
        }
    }

    ImGui::Separator();

    // Connected Clients Section
    ImGui::Text("Connected Clients (%d)", (int)connected_clients.size());
    if (server_started) {
        UpdateServerStatus();
        ImGui::BeginChild("Clients", ImVec2(0, 100), true);
        for (const auto& client : connected_clients) {
            ImGui::Text("%s", client.c_str());
        }
        ImGui::EndChild();
    }

    ImGui::Separator();

    // Recent Messages Section
    ImGui::Text("Recent Messages");
    if (server_started) {
        recent_messages = server.get_recent_messages(20);
        ImGui::BeginChild("Messages", ImVec2(0, 200), true);
        for (const auto& msg : recent_messages) {
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
        ImGui::EndChild();
    }

    ImGui::End();
}

void ServerGUI::UpdateServerStatus() {
    if (server_started) {
        connected_clients = server.get_connected_clients();
    }
}

// Helper functions for DirectX setup
bool ServerGUI::CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
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
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2,
                                     D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void ServerGUI::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void ServerGUI::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void ServerGUI::CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pServerGUI && g_pServerGUI->g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            g_pServerGUI->CleanupRenderTarget();
            g_pServerGUI->g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            g_pServerGUI->CreateRenderTarget();
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
