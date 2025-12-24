#include "networking/ChatServer.hpp"
#include <iostream>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

ChatServer::ChatServer(int port)
    : port_(port), listen_socket_(INVALID_SOCKET), running_(false) {
}

ChatServer::~ChatServer() {
    stop();
}

bool ChatServer::start() {
    if (running_) return true;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "WSA startup failed\n";
        return false;
    }

    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) {
        std::cout << "Socket creation failed\n";
        WSACleanup();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listen_socket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cout << "Bind failed\n";
        closesocket(listen_socket_);
        WSACleanup();
        return false;
    }

    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed\n";
        closesocket(listen_socket_);
        WSACleanup();
        return false;
    }

    running_ = true;
    std::cout << "Server started on port " << port_ << "\n";
    
    accept_thread_ = std::thread(&ChatServer::accept_clients, this);
    accept_thread_.detach();

    return true;
}

void ChatServer::stop() {
    if (!running_) return;

    running_ = false;

    if (listen_socket_ != INVALID_SOCKET) {
        closesocket(listen_socket_);
        listen_socket_ = INVALID_SOCKET;
    }

    for (SOCKET s : clients_) {
        closesocket(s);
    }
    clients_.clear();

    WSACleanup();
    std::cout << "Server stopped\n";
}

bool ChatServer::is_running() const {
    return running_;
}

int ChatServer::get_client_count() const {
    return (int)clients_.size();
}

void ChatServer::accept_clients() {
    while (running_) {
        SOCKET client = accept(listen_socket_, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            if (running_) {
                std::cout << "Accept failed\n";
            }
            break;
        }

        clients_.push_back(client);
        std::cout << "Client connected. Total: " << clients_.size() << "\n";
        
        std::thread(&ChatServer::handle_client, this, client).detach();
    }
}

void ChatServer::handle_client(SOCKET client) {
    char buffer[1024];

    while (running_) {
        int n = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (n <= 0) {
            break;
        }

        buffer[n] = '\0';
        std::string msg(buffer);

        // Broadcast to other clients (peer-to-peer communication via server)
        broadcast(msg, client);

        // Don't queue client messages for server GUI display
        // Server only displays its own broadcast messages
    }

    remove_client(client);
}

void ChatServer::broadcast(const std::string& msg, SOCKET sender) {
    for (SOCKET s : clients_) {
        if (s == sender) continue;
        send(s, msg.c_str(), (int)msg.size(), 0);
    }
}

void ChatServer::remove_client(SOCKET client) {
    closesocket(client);
    
    auto it = std::remove(clients_.begin(), clients_.end(), client);
    if (it != clients_.end()) {
        clients_.erase(it, clients_.end());
        std::cout << "Client disconnected. Total: " << clients_.size() << "\n";
    }
}

void ChatServer::queue_message(const std::string& msg) {
    std::lock_guard<std::mutex> lock(msg_mutex_);
    message_queue_.push(msg);
}

bool ChatServer::has_message() const {
    std::lock_guard<std::mutex> lock(msg_mutex_);
    return !message_queue_.empty();
}

std::string ChatServer::receive_message() {
    std::lock_guard<std::mutex> lock(msg_mutex_);
    if (message_queue_.empty()) return "";
    
    std::string msg = message_queue_.front();
    message_queue_.pop();
    return msg;
}
