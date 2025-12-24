#include "gui/ChatClientGui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <iostream>

static GLFWwindow* g_client_window = nullptr;

ChatClientGui::ChatClientGui()
    : port_(5000), state_(State::DISCONNECTED), connected_(false) {
    std::memset(ip_buffer_, 0, sizeof(ip_buffer_));
    std::strcpy(ip_buffer_, "127.0.0.1");
    std::memset(input_buffer_, 0, sizeof(input_buffer_));
    client_ = std::make_unique<ChatClient>();
}

ChatClientGui::~ChatClientGui() {
    shutdown();
}

bool ChatClientGui::init(const std::string& title, int width, int height) {
    if (!glfwInit()) return false;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    g_client_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!g_client_window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(g_client_window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(g_client_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    messages_.push_back("Chat Client - Ready to connect");

    return true;
}

void ChatClientGui::shutdown() {
    if (connected_) {
        client_->disconnect();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (g_client_window) {
        glfwDestroyWindow(g_client_window);
    }
    glfwTerminate();
}

bool ChatClientGui::is_running() const {
    return g_client_window && !glfwWindowShouldClose(g_client_window);
}

void ChatClientGui::render() {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int display_w, display_h;
    glfwGetFramebufferSize(g_client_window, &display_w, &display_h);
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));
    
    ImGui::Begin("Chat Client", nullptr, 
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (state_ == State::DISCONNECTED) {
        // Connection screen
        ImGui::Text("Connect to Chat Server");
        ImGui::Separator();
        
        ImGui::InputText("Server IP", ip_buffer_, sizeof(ip_buffer_));
        ImGui::InputInt("Port", &port_);

        if (ImGui::Button("Connect", ImVec2(100, 30))) {
            if (client_->connect(ip_buffer_, port_)) {
                connected_ = true;
                state_ = State::CONNECTED;
                messages_.clear();
                messages_.push_back("[System] Connected to server");
            } else {
                messages_.clear();
                messages_.push_back("[System] Connection failed - check IP and port");
            }
        }

        ImGui::Separator();
        ImGui::Text("Connection Status:");
        for (const auto& msg : messages_) {
            if (msg.find("[System]") != std::string::npos) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", msg.c_str());
            } else {
                ImGui::Text("%s", msg.c_str());
            }
        }
    } else if (state_ == State::CONNECTED) {
        // Chat screen
        ImGui::Text("Connected to %s:%d", ip_buffer_, port_);
        ImGui::Separator();

        // Messages display
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

        // Input area
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
            connected_ = false;
            state_ = State::DISCONNECTED;
            messages_.clear();
            messages_.push_back("Chat Client - Ready to connect");
        }

        // Check for incoming messages
        while (client_->has_message()) {
            std::string msg = client_->receive_message();
            if (!msg.empty()) {
                messages_.push_back("[Server] " + msg);
            }
        }
    }

    ImGui::End();

    ImGui::Render();
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(g_client_window);
}
