#include "client.h"
#include <iostream>
#include <algorithm>
#include <cstring>

ChatClient::ChatClient() : shared_mem(nullptr), connected(false), last_read_index(0) {}

ChatClient::~ChatClient() {
    disconnect();
}

bool ChatClient::connect(const std::string& username) {
    if (connected || username.empty() || username.length() >= MAX_USERNAME_LENGTH) {
        return false;
    }

    this->username = username;

    // Try to attach to existing shared memory
    if (!attach_shared_memory()) {
        std::cerr << "Failed to attach to shared memory. Is the server running?" << std::endl;
        return false;
    }

    shared_mem = get_shared_memory();
    if (!shared_mem) {
        std::cerr << "Failed to get shared memory pointer" << std::endl;
        return false;
    }

    // Wait for server to be running
    if (!wait_for_server()) {
        std::cerr << "Server is not running" << std::endl;
        detach_shared_memory();
        return false;
    }

    // Create a temporary server instance to register
    // Note: In a real implementation, this would be handled differently
    // For now, we'll assume the server logic is accessible
    // This is a simplified approach - in production, you'd have IPC for registration

    std::cout << "Attempting to register as: " << username << std::endl;

    // For this demo, we'll simulate registration by directly accessing shared memory
    // In a real system, you'd use proper IPC mechanisms
    if (!shared_mem) {
        std::cerr << "Critical error: shared_mem is null after successful attachment" << std::endl;
        return false;
    }

    // Acquire spinlock for client access
    while (shared_mem->clients_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Check if username already exists
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (shared_mem->clients[i].is_connected &&
            strcmp(shared_mem->clients[i].username, username.c_str()) == 0) {
            std::cerr << "Username already taken: " << username << std::endl;
            detach_shared_memory();
            return false;
        }
    }

    // Find available slot
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!shared_mem->clients[i].is_connected) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        std::cerr << "No available client slots" << std::endl;
        detach_shared_memory();
        return false;
    }

    // Register client
    strncpy(shared_mem->clients[slot].username, username.c_str(), MAX_USERNAME_LENGTH - 1);
    shared_mem->clients[slot].username[MAX_USERNAME_LENGTH - 1] = '\0';
    shared_mem->clients[slot].is_connected = true;
    shared_mem->clients[slot].last_activity = std::chrono::system_clock::now();
    shared_mem->client_count++;

    // Release spinlock
    shared_mem->clients_lock.store(false, std::memory_order_release);

    connected = true;

    // Start message listener thread
    message_thread = std::thread([this]() {
        message_listener();
    });

    // Add join message
    std::string join_message = username + " has joined the chat.";
    
    // Acquire spinlock for message access
    while (shared_mem->messages_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    int write_idx = shared_mem->write_index.load();
    strncpy(shared_mem->messages[write_idx].username, "SERVER", MAX_USERNAME_LENGTH - 1);
    shared_mem->messages[write_idx].username[MAX_USERNAME_LENGTH - 1] = '\0';
    strncpy(shared_mem->messages[write_idx].content, join_message.c_str(), MAX_MESSAGE_LENGTH - 1);
    shared_mem->messages[write_idx].content[MAX_MESSAGE_LENGTH - 1] = '\0';
    shared_mem->messages[write_idx].timestamp = std::chrono::system_clock::now();
    shared_mem->messages[write_idx].is_broadcast = true;

    shared_mem->write_index = (write_idx + 1) % MAX_MESSAGES;
    shared_mem->message_count++;

    // Release spinlock
    shared_mem->messages_lock.store(false, std::memory_order_release);

    std::cout << "Client connected as: " << username << std::endl;
    return true;
}

void ChatClient::disconnect() {
    if (!connected) return;

    connected = false;

    if (message_thread.joinable()) {
        message_thread.join();
    }

    if (shared_mem) {
        // Remove client from shared memory
        while (shared_mem->clients_lock.exchange(true, std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (shared_mem->clients[i].is_connected &&
                strcmp(shared_mem->clients[i].username, username.c_str()) == 0) {
                shared_mem->clients[i] = ClientInfo();
                shared_mem->client_count--;
                break;
            }
        }

        // Add leave message
        std::string leave_message = username + " has left the chat.";
        while (shared_mem->messages_lock.exchange(true, std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        int write_idx = shared_mem->write_index.load();
        strncpy(shared_mem->messages[write_idx].username, "SERVER", MAX_USERNAME_LENGTH - 1);
        shared_mem->messages[write_idx].username[MAX_USERNAME_LENGTH - 1] = '\0';
        strncpy(shared_mem->messages[write_idx].content, leave_message.c_str(), MAX_MESSAGE_LENGTH - 1);
        shared_mem->messages[write_idx].content[MAX_MESSAGE_LENGTH - 1] = '\0';
        shared_mem->messages[write_idx].timestamp = std::chrono::system_clock::now();
        shared_mem->messages[write_idx].is_broadcast = true;

        shared_mem->write_index = (write_idx + 1) % MAX_MESSAGES;
        shared_mem->message_count++;

        // Release spinlock
        shared_mem->messages_lock.store(false, std::memory_order_release);
    }

    detach_shared_memory();
    shared_mem = nullptr;

    std::cout << "Client disconnected: " << username << std::endl;
}

bool ChatClient::is_connected() const {
    return connected && shared_mem && shared_mem->server_running;
}

bool ChatClient::wait_for_server() {
    if (!shared_mem) return false;

    // Wait up to 5 seconds for server to start
    for (int i = 0; i < 50; ++i) {
        if (shared_mem->server_running) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

void ChatClient::message_listener() {
    while (connected) {
        if (!shared_mem) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!connected || !shared_mem) break;

        // Acquire spinlock
        while (shared_mem->messages_lock.exchange(true, std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // Check for new messages
        int current_write = shared_mem->write_index.load();
        int message_count = shared_mem->message_count.load();

        while (last_read_index != current_write && message_count > 0) {
            int msg_idx = last_read_index % MAX_MESSAGES;

            if (message_callback) {
                message_callback(shared_mem->messages[msg_idx]);
            }

            last_read_index = (last_read_index + 1) % MAX_MESSAGES;
            message_count--;
        }

        // Release spinlock
        shared_mem->messages_lock.store(false, std::memory_order_release);

        // Update last activity
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (shared_mem->clients[i].is_connected &&
                strcmp(shared_mem->clients[i].username, username.c_str()) == 0) {
                shared_mem->clients[i].last_activity = std::chrono::system_clock::now();
                break;
            }
        }
    }
}

bool ChatClient::send_message(const std::string& message) {
    if (!connected || !shared_mem || message.empty() || message.length() >= MAX_MESSAGE_LENGTH) {
        return false;
    }

    while (shared_mem->messages_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    int write_idx = shared_mem->write_index.load();
    int next_write_idx = (write_idx + 1) % MAX_MESSAGES;

    // Check if buffer is full
    if (next_write_idx == shared_mem->read_index.load() && shared_mem->message_count.load() > 0) {
        // Overwrite oldest message
        shared_mem->read_index = (shared_mem->read_index.load() + 1) % MAX_MESSAGES;
        shared_mem->message_count--;
    }

    // Add message
    strncpy(shared_mem->messages[write_idx].username, username.c_str(), MAX_USERNAME_LENGTH - 1);
    shared_mem->messages[write_idx].username[MAX_USERNAME_LENGTH - 1] = '\0';
    strncpy(shared_mem->messages[write_idx].content, message.c_str(), MAX_MESSAGE_LENGTH - 1);
    shared_mem->messages[write_idx].content[MAX_MESSAGE_LENGTH - 1] = '\0';
    shared_mem->messages[write_idx].timestamp = std::chrono::system_clock::now();
    shared_mem->messages[write_idx].is_broadcast = false;

    shared_mem->write_index = next_write_idx;
    shared_mem->message_count++;

    // Release spinlock
    shared_mem->messages_lock.store(false, std::memory_order_release);

    std::cout << "Message sent: " << message << std::endl;
    return true;
}

void ChatClient::set_message_callback(std::function<void(const Message&)> callback) {
    message_callback = callback;
}

std::string ChatClient::get_username() const {
    return username;
}

std::vector<std::string> ChatClient::get_connected_clients() {
    std::vector<std::string> clients;

    if (!shared_mem) return clients;

    while (shared_mem->clients_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (shared_mem->clients[i].is_connected) {
            clients.push_back(shared_mem->clients[i].username);
        }
    }

    shared_mem->clients_lock.store(false, std::memory_order_release);

    return clients;
}

std::vector<Message> ChatClient::get_message_history() {
    std::vector<Message> messages;

    if (!shared_mem) return messages;

    while (shared_mem->messages_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    int available_messages = shared_mem->message_count.load();
    int messages_to_return = std::min(50, available_messages);

    int read_idx = shared_mem->read_index.load();

    for (int i = 0; i < messages_to_return; ++i) {
        int msg_idx = (read_idx + i) % MAX_MESSAGES;
        messages.push_back(shared_mem->messages[msg_idx]);
    }

    shared_mem->messages_lock.store(false, std::memory_order_release);

    return messages;
}
