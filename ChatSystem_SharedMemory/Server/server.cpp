#include "server.h"
#include <iostream>
#include <algorithm>
#include <cstring>

ChatServer::ChatServer() : shared_mem(nullptr), running(false) {}

ChatServer::~ChatServer() {
    stop();
}

bool ChatServer::initialize() {
    if (!create_shared_memory()) {
        std::cerr << "Failed to create shared memory" << std::endl;
        return false;
    }

    shared_mem = get_shared_memory();
    if (!shared_mem) {
        std::cerr << "Failed to get shared memory pointer" << std::endl;
        return false;
    }

    // Mark server as running
    shared_mem->server_running = true;

    std::cout << "Server initialized successfully" << std::endl;
    return true;
}

void ChatServer::start() {
    if (running) return;

    running = true;

    // Start cleanup thread
    cleanup_thread = std::thread([this]() {
        while (running) {
            cleanup_disconnected_clients();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });

    std::cout << "Server started" << std::endl;
}

void ChatServer::stop() {
    if (!running) return;

    running = false;

    if (cleanup_thread.joinable()) {
        cleanup_thread.join();
    }

    if (shared_mem) {
        shared_mem->server_running = false;
    }

    detach_shared_memory();
    std::cout << "Server stopped" << std::endl;
}

bool ChatServer::is_running() const {
    return running && shared_mem && shared_mem->server_running;
}

void ChatServer::cleanup_disconnected_clients() {
    if (!shared_mem) return;

    while (shared_mem->clients_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    auto now = std::chrono::system_clock::now();
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (shared_mem->clients[i].is_connected) {
            auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
                now - shared_mem->clients[i].last_activity
            );

            // Remove clients inactive for more than 30 seconds
            if (time_diff.count() > 30) {
                std::cout << "Removing inactive client: " << shared_mem->clients[i].username << std::endl;
                remove_client(i);
            }
        }
    }

    shared_mem->clients_lock.store(false, std::memory_order_release);
}

int ChatServer::find_available_client_slot() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!shared_mem->clients[i].is_connected) {
            return i;
        }
    }
    return -1;
}

void ChatServer::remove_client(int client_index) {
    if (client_index < 0 || client_index >= MAX_CLIENTS) return;

    std::string username = shared_mem->clients[client_index].username;

    // Clear client info
    shared_mem->clients[client_index] = ClientInfo();
    shared_mem->client_count--;

    // Notify about client leaving
    std::string leave_message = username + " has left the chat.";
    broadcast_message(leave_message);
}

bool ChatServer::register_client(const std::string& username) {
    if (!shared_mem || username.empty() || username.length() >= MAX_USERNAME_LENGTH) {
        return false;
    }

    while (shared_mem->clients_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Check if username already exists
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (shared_mem->clients[i].is_connected &&
            strcmp(shared_mem->clients[i].username, username.c_str()) == 0) {
            shared_mem->clients_lock.store(false, std::memory_order_release);
            return false; // Username already taken
        }
    }

    int slot = find_available_client_slot();
    if (slot == -1) {
        shared_mem->clients_lock.store(false, std::memory_order_release);
        return false; // No available slots
    }

    // Register client
    strncpy(shared_mem->clients[slot].username, username.c_str(), MAX_USERNAME_LENGTH - 1);
    shared_mem->clients[slot].username[MAX_USERNAME_LENGTH - 1] = '\0';
    shared_mem->clients[slot].is_connected = true;
    shared_mem->clients[slot].last_activity = std::chrono::system_clock::now();
    shared_mem->client_count++;

    shared_mem->clients_lock.store(false, std::memory_order_release);

    // Notify about new client
    std::string join_message = username + " has joined the chat.";
    broadcast_message(join_message);

    std::cout << "Client registered: " << username << std::endl;

    return true;
}

void ChatServer::unregister_client(const std::string& username) {
    if (!shared_mem || username.empty()) return;

    while (shared_mem->clients_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (shared_mem->clients[i].is_connected &&
            strcmp(shared_mem->clients[i].username, username.c_str()) == 0) {
            // Don't call remove_client as it tries to acquire lock again
            shared_mem->clients[i] = ClientInfo();
            shared_mem->client_count--;
            break;
        }
    }

    shared_mem->clients_lock.store(false, std::memory_order_release);
}

bool ChatServer::broadcast_message(const std::string& message) {
    if (!shared_mem || message.empty() || message.length() >= MAX_MESSAGE_LENGTH) {
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
    strncpy(shared_mem->messages[write_idx].username, "SERVER", MAX_USERNAME_LENGTH - 1);
    shared_mem->messages[write_idx].username[MAX_USERNAME_LENGTH - 1] = '\0';
    strncpy(shared_mem->messages[write_idx].content, message.c_str(), MAX_MESSAGE_LENGTH - 1);
    shared_mem->messages[write_idx].content[MAX_MESSAGE_LENGTH - 1] = '\0';
    shared_mem->messages[write_idx].timestamp = std::chrono::system_clock::now();
    shared_mem->messages[write_idx].is_broadcast = true;

    shared_mem->write_index = next_write_idx;
    shared_mem->message_count++;

    shared_mem->new_broadcast_available = true;
    shared_mem->messages_lock.store(false, std::memory_order_release);

    std::cout << "Broadcast message: " << message << std::endl;
    return true;
}

void ChatServer::add_client_message(const std::string& username, const std::string& message) {
    if (!shared_mem || username.empty() || message.empty() ||
        message.length() >= MAX_MESSAGE_LENGTH || username.length() >= MAX_USERNAME_LENGTH) {
        return;
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

    shared_mem->messages_lock.store(false, std::memory_order_release);

    std::cout << "Client message from " << username << ": " << message << std::endl;

    // Update client's last activity
    while (shared_mem->clients_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (shared_mem->clients[i].is_connected &&
            strcmp(shared_mem->clients[i].username, username.c_str()) == 0) {
            shared_mem->clients[i].last_activity = std::chrono::system_clock::now();
            break;
        }
    }
    shared_mem->clients_lock.store(false, std::memory_order_release);
}

std::vector<std::string> ChatServer::get_connected_clients() {
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

std::vector<Message> ChatServer::get_recent_messages(int count) {
    std::vector<Message> messages;

    if (!shared_mem || count <= 0) return messages;

    while (shared_mem->messages_lock.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    int available_messages = shared_mem->message_count.load();
    int messages_to_return = std::min(count, available_messages);

    int read_idx = shared_mem->read_index.load();

    for (int i = 0; i < messages_to_return; ++i) {
        int msg_idx = (read_idx + i) % MAX_MESSAGES;
        messages.push_back(shared_mem->messages[msg_idx]);
    }

    shared_mem->messages_lock.store(false, std::memory_order_release);

    return messages;
}
