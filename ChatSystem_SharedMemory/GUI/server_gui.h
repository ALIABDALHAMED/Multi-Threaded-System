#ifndef SERVER_GUI_H
#define SERVER_GUI_H

#include "../Server/server.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <string>
#include <vector>

class ServerGUI {
private:
    ChatServer server;
    bool server_started;
    char broadcast_text[512];
    std::vector<std::string> connected_clients;
    std::vector<Message> recent_messages;

public:
    // Win32/DX11 variables (public for static WndProc access)
    WNDCLASSEX wc;
    HWND hwnd;
    ID3D11Device* g_pd3dDevice;
    ID3D11DeviceContext* g_pd3dDeviceContext;
    IDXGISwapChain* g_pSwapChain;
    ID3D11RenderTargetView* g_mainRenderTargetView;

private:
    void RenderGUI();
    void UpdateServerStatus();
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();

public:
    // Public render target methods (called from WndProc)
    void CreateRenderTarget();
    void CleanupRenderTarget();

    ServerGUI();
    ~ServerGUI();

    bool Initialize();
    void Run();
    void Shutdown();
};

#endif // SERVER_GUI_H
