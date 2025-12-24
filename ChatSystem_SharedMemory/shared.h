#ifndef SHARED_H
#define SHARED_H

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

// Maximum number of messages to keep in history
#define MAX_MESSAGES 1000

// Maximum number of clients
#define MAX_CLIENTS 50

// Maximum message length
#define MAX_MESSAGE_LENGTH 512

// Maximum username length
#define MAX_USERNAME_LENGTH 32

// Shared memory key/name
#define SHARED_MEMORY_NAME "ChatSystem_SharedMemory"

// Message structure
struct Message {
    char username[MAX_USERNAME_LENGTH];
    char content[MAX_MESSAGE_LENGTH];
    std::chrono::system_clock::time_point timestamp;
    bool is_broadcast; // true if from server, false if from client

    Message() : timestamp(std::chrono::system_clock::now()), is_broadcast(false) {
        username[0] = '\0';
        content[0] = '\0';
    }
};

// Client information
struct ClientInfo {
    char username[MAX_USERNAME_LENGTH];
    bool is_connected;
    std::chrono::system_clock::time_point last_activity;

    ClientInfo() : is_connected(false), last_activity(std::chrono::system_clock::now()) {
        username[0] = '\0';
    }
};

// Shared memory structure
struct SharedMemory {
    // Message buffer (circular buffer)
    Message messages[MAX_MESSAGES];
    std::atomic<int> message_count;
    std::atomic<int> read_index;
    std::atomic<int> write_index;
    std::atomic<bool> messages_lock; // Simple spinlock for messages

    // Client management
    ClientInfo clients[MAX_CLIENTS];
    std::atomic<bool> clients_lock;  // Simple spinlock for clients
    std::atomic<int> client_count;

    // Server control
    std::atomic<bool> server_running;
    std::atomic<bool> new_broadcast_available;

    SharedMemory() :
        message_count(0),
        read_index(0),
        write_index(0),
        messages_lock(false),
        clients_lock(false),
        client_count(0),
        server_running(false),
        new_broadcast_available(false) {}
};

// Helper functions for shared memory management
bool create_shared_memory();
bool attach_shared_memory();
void detach_shared_memory();
SharedMemory* get_shared_memory();

#endif // SHARED_H
