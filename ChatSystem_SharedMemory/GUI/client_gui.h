#ifndef CLIENT_GUI_H
#define CLIENT_GUI_H

#include "../Client/client.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <string>
#include <vector>

enum class ClientState {
    ENTERING_USERNAME,
    CONNECTING,
    CHATTING,
    DISCONNECTED
};

class ClientGUI {
private:
    ChatClient client;
    ClientState state;
    bool show_history_on_connect;
    char username_input[32];
    char message_input[512];
    std::vector<Message> chat_messages;
    std::vector<std::string> connected_clients;
    
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void RenderGUI();
    void UpdateClientStatus();
    void HandleNewMessage(const Message& msg);

public:
    // Win32/DX11 variables (public for static WndProc access)
    WNDCLASSEX wc;
    HWND hwnd;
    ID3D11Device* g_pd3dDevice;
    ID3D11DeviceContext* g_pd3dDeviceContext;
    IDXGISwapChain* g_pSwapChain;
    ID3D11RenderTargetView* g_mainRenderTargetView;

    // Public render target methods (called from WndProc)
    void CreateRenderTarget();
    void CleanupRenderTarget();

    ClientGUI();
    ~ClientGUI();

    bool Initialize();
    void Run();
    void Shutdown();
};

#endif // CLIENT_GUI_H
