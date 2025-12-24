#ifndef SERVER_H
#define SERVER_H

#include "../shared.h"
#include <thread>
#include <atomic>

// Server class for managing the chat system
class ChatServer {
private:
    SharedMemory* shared_mem;
    std::atomic<bool> running;
    std::thread cleanup_thread;

    void cleanup_disconnected_clients();
    int find_available_client_slot();
    void remove_client(int client_index);

public:
    ChatServer();
    ~ChatServer();

    bool initialize();
    void start();
    void stop();
    bool is_running() const;

    // Message handling
    bool broadcast_message(const std::string& message);
    void add_client_message(const std::string& username, const std::string& message);

    // Client management
    bool register_client(const std::string& username);
    void unregister_client(const std::string& username);

    // Getters for GUI
    std::vector<std::string> get_connected_clients();
    std::vector<Message> get_recent_messages(int count = 50);
};

#endif // SERVER_H
