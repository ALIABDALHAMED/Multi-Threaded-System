#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <thread>
#include <queue>
#include <mutex>

class ChatServer {
public:
    ChatServer(int port);
    ~ChatServer();

    bool start();
    void stop();
    bool is_running() const;
    int get_client_count() const;
    void broadcast(const std::string& msg, SOCKET sender = INVALID_SOCKET);
    
    // Message queue for incoming messages
    bool has_message() const;
    std::string receive_message();

private:
    void accept_clients();
    void handle_client(SOCKET client);
    void remove_client(SOCKET client);
    void queue_message(const std::string& msg);

    int port_;
    SOCKET listen_socket_;
    bool running_;
    
    std::vector<SOCKET> clients_;
    std::thread accept_thread_;
    
    // Thread-safe message queue
    mutable std::mutex msg_mutex_;
    std::queue<std::string> message_queue_;
};
