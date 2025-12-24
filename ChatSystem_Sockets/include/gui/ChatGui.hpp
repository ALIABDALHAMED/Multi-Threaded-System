#pragma once

#include <string>
#include <vector>
#include <memory>
#include "networking/ChatClient.hpp"
#include "networking/ChatServer.hpp"

class ChatGui {
public:
    ChatGui();
    ~ChatGui();

    bool init(const std::string& title, int width, int height);
    void render();
    bool is_running() const;
    void shutdown();

private:
    enum class AppMode {
        SELECTION,      // Choose server or client
        CLIENT,         // Client connection mode
        SERVER          // Server listening mode
    };

    // UI rendering methods
    void render_selection_screen();
    void render_client_view();
    void render_server_view();

    AppMode current_mode_;
    
    // Networking objects
    std::unique_ptr<ChatClient> client_;
    std::unique_ptr<ChatServer> server_;

    // UI state
    char ip_buffer_[64];        // Input: server IP for client
    int port_;                  // Server port (5000)
    char input_buffer_[512];    // Message input
    
    std::vector<std::string> messages_;  // Chat messages with colors
    bool server_running_;
    bool client_connected_;
};