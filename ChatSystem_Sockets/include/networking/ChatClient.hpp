#pragma once

#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

class ChatClient {
public:
    ChatClient();
    ~ChatClient();

    bool connect(const std::string& host, int port);
    void disconnect();
    bool is_connected() const;

    bool send_message(const std::string& message);
    std::string receive_message();
    bool has_message() const;

private:
    SOCKET socket_;
    bool connected_;
};
