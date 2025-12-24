#ifndef CLIENT_H
#define CLIENT_H

#include "../shared.h"
#include <thread>
#include <atomic>
#include <functional>

// Client class for participating in the chat system
class ChatClient {
private:
    SharedMemory* shared_mem;
    std::string username;
    std::atomic<bool> connected;
    std::atomic<int> last_read_index;
    std::thread message_thread;

    // Callback for new messages
    std::function<void(const Message&)> message_callback;

    void message_listener();
    bool wait_for_server();

public:
    ChatClient();
    ~ChatClient();

    bool connect(const std::string& username);
    void disconnect();
    bool is_connected() const;

    // Message handling
    bool send_message(const std::string& message);
    void set_message_callback(std::function<void(const Message&)> callback);

    // Getters
    std::string get_username() const;
    std::vector<std::string> get_connected_clients();
    std::vector<Message> get_message_history();
};

#endif // CLIENT_H
