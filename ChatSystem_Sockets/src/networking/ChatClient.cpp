#include "networking/ChatClient.hpp"
#include <iostream>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

ChatClient::ChatClient()
    : socket_(INVALID_SOCKET), connected_(false) {
}

ChatClient::~ChatClient() {
    disconnect();
}

bool ChatClient::connect(const std::string& host, int port) {
    if (connected_) return true;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return false;
    }

    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) != 1) {
        closesocket(socket_);
        WSACleanup();
        return false;
    }

    if (::connect(socket_, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(socket_);
        WSACleanup();
        return false;
    }

    // Set non-blocking mode
    u_long mode = 1;
    if (ioctlsocket(socket_, FIONBIO, &mode) == SOCKET_ERROR) {
        closesocket(socket_);
        WSACleanup();
        return false;
    }

    connected_ = true;
    return true;
}

void ChatClient::disconnect() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
    connected_ = false;
    WSACleanup();
}

bool ChatClient::is_connected() const {
    return connected_;
}

bool ChatClient::send_message(const std::string& message) {
    if (!connected_) return false;

    std::string msg = message;
    if (!msg.empty() && msg.back() != '\n') {
        msg.push_back('\n');
    }

    int sent = send(socket_, msg.c_str(), (int)msg.size(), 0);
    if (sent == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            connected_ = false;
        }
        return false;
    }

    return true;
}

std::string ChatClient::receive_message() {
    if (!connected_) return "";

    char buffer[4096];
    int n = recv(socket_, buffer, sizeof(buffer) - 1, 0);

    if (n > 0) {
        buffer[n] = '\0';
        return std::string(buffer);
    } else if (n == 0) {
        connected_ = false;
    } else {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINTR) {
            connected_ = false;
        }
    }

    return "";
}

bool ChatClient::has_message() const {
    if (!connected_) return false;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_, &readfds);

    timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int result = select(0, &readfds, nullptr, nullptr, &tv);
    return result > 0 && FD_ISSET(socket_, &readfds);
}
