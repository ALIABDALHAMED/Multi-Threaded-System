#include "gui/ChatGui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <iostream>

static GLFWwindow* g_window = nullptr;

ChatGui::ChatGui()
    : current_mode_(AppMode::SERVER), 
      port_(5000), 
      server_running_(false),
      client_connected_(false) {
    std::memset(ip_buffer_, 0, sizeof(ip_buffer_));
    std::strcpy(ip_buffer_, "127.0.0.1");
    std::memset(input_buffer_, 0, sizeof(input_buffer_));
    
    client_ = std::make_unique<ChatClient>();
    server_ = std::make_unique<ChatServer>(port_);
}

ChatGui::~ChatGui() {
    shutdown();
}

bool ChatGui::init(const std::string& title, int width, int height) {
    if (!glfwInit()) return false;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    g_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!g_window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Set basic dark theme
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    messages_.push_back("Chat System Started");
    
    // Auto-start server
    server_running_ = server_->start();
    if (server_running_) {
        messages_.push_back("[System] Server started on port 5000");
    } else {
        messages_.push_back("[System] Failed to start server");
    }

    return true;
}

void ChatGui::shutdown() {
    if (server_running_) {
        server_->stop();
    }
    if (client_connected_) {
        client_->disconnect();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (g_window) {
        glfwDestroyWindow(g_window);
    }
    glfwTerminate();
}

bool ChatGui::is_running() const {
    return g_window && !glfwWindowShouldClose(g_window);
}

void ChatGui::render() {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int display_w, display_h;
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));
    
    ImGui::Begin("Chat System", nullptr, 
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Check for incoming messages from clients (server mode)
    if (current_mode_ == AppMode::SERVER && server_running_) {
        while (server_->has_message()) {
            std::string msg = server_->receive_message();
            if (!msg.empty()) {
                messages_.push_back(msg);
            }
        }
    }

    // Render current mode
    if (current_mode_ == AppMode::SELECTION) {
        render_selection_screen();
    } else if (current_mode_ == AppMode::CLIENT) {
        render_client_view();
    } else if (current_mode_ == AppMode::SERVER) {
        render_server_view();
    }

    ImGui::End();

    // Render ImGui
    ImGui::Render();
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(g_window);
}

void ChatGui::render_selection_screen() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetCursorPos(ImVec2(center.x - 150, center.y - 80));

    ImGui::Text("Chat System - Select Mode");
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Start as Server", ImVec2(300, 50))) {
        server_running_ = server_->start();
        if (server_running_) {
            current_mode_ = AppMode::SERVER;
            messages_.push_back("[System] Server started on port 5000");
        } else {
            messages_.push_back("[System] Failed to start server");
        }
    }

    ImGui::Spacing();

    if (ImGui::Button("Connect as Client", ImVec2(300, 50))) {
        current_mode_ = AppMode::CLIENT;
    }
}

void ChatGui::render_server_view() {
    ImGui::Text("Server Mode - Port 5000");
    ImGui::Separator();

    // Display messages with colors
    ImGui::BeginChild("Messages", ImVec2(0, -80), true);
    for (const auto& msg : messages_) {
        if (msg.find("[System]") != std::string::npos) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", msg.c_str());
        } else if (msg.find("[Server]") != std::string::npos) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", msg.c_str());
        } else if (msg.find("[Client]") != std::string::npos) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", msg.c_str());
        } else {
            ImGui::Text("%s", msg.c_str());
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Message input - Server can broadcast messages to all clients
    bool send_msg = ImGui::InputText("##input", input_buffer_, sizeof(input_buffer_),
                                     ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Broadcast", ImVec2(100, 0)) || send_msg) {
        if (input_buffer_[0] != '\0') {
            std::string msg = std::string(input_buffer_);
            messages_.push_back("[Server] " + msg);
            // Broadcast to all connected clients
            server_->broadcast(msg);
            std::memset(input_buffer_, 0, sizeof(input_buffer_));
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop Server", ImVec2(150, 30))) {
        server_->stop();
        server_running_ = false;
        current_mode_ = AppMode::SELECTION;
        messages_.clear();
        messages_.push_back("Chat System Started");
    }
}

void ChatGui::render_client_view() {
    ImGui::Text("Client Mode");
    ImGui::Separator();

    // Connection UI
    if (!client_connected_) {
        ImGui::Text("Connect to Server");
        ImGui::InputText("Server IP", ip_buffer_, sizeof(ip_buffer_));
        ImGui::InputInt("Port", &port_);

        if (ImGui::Button("Connect", ImVec2(100, 30))) {
            if (client_->connect(ip_buffer_, port_)) {
                client_connected_ = true;
                messages_.push_back("[System] Connected to server");
            } else {
                messages_.push_back("[System] Connection failed");
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Back to Selection")) {
            current_mode_ = AppMode::SELECTION;
            messages_.clear();
            messages_.push_back("Chat System Started");
        }
    } else {
        // Connected - show chat
        ImGui::Text("Connected to %s:%d", ip_buffer_, port_);
        ImGui::Separator();

        // Display messages with colors
        ImGui::BeginChild("Messages", ImVec2(0, -80), true);
        for (const auto& msg : messages_) {
            if (msg.find("[System]") != std::string::npos) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", msg.c_str());
            } else if (msg.find("[You]") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", msg.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", msg.c_str());
            }
        }
        ImGui::EndChild();

        ImGui::Separator();

        // Message input
        bool send_msg = ImGui::InputText("##input", input_buffer_, sizeof(input_buffer_),
                                         ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("Send", ImVec2(100, 0)) || send_msg) {
            if (input_buffer_[0] != '\0') {
                if (client_->send_message(input_buffer_)) {
                    messages_.push_back("[You] " + std::string(input_buffer_));
                }
                std::memset(input_buffer_, 0, sizeof(input_buffer_));
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Disconnect")) {
            client_->disconnect();
            client_connected_ = false;
            current_mode_ = AppMode::SELECTION;
            messages_.clear();
            messages_.push_back("Chat System Started");
        }
    }
}
