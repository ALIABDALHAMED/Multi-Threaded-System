# Chat System - Multi-Implementation Client/Server with ImGui GUI

A modern C++17 chat application with multiple inter-process communication implementations:

## Overview

This project contains two different implementations of a chat system:
1. **Socket-based** - Uses Windows Sockets for network communication
2. **Shared Memory** - Uses Windows Shared Memory for IPC communication

Both implementations feature:
- **Server**: Multi-client broadcast server
- **GUI Client**: ImGui + GLFW interactive chat interface
- **Modern C++17**: Smart pointers, thread safety, RAII patterns

## Project Structure

```
OS Project/
├── ChatSystem_Sockets/           # Socket-based implementation
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── gui/
│   │   │   └── ChatGui.hpp
│   │   └── networking/
│   │       └── ChatClient.hpp
│   ├── src/
│   │   ├── client.cpp            # CLI client
│   │   ├── server.cpp            # Server
│   │   ├── gui/
│   │   │   └── ChatGui.cpp
│   │   └── networking/
│   │       └── ChatClient.cpp
│   ├── gui/
│   │   ├── main_gui.cpp          # GUI client entry point
│   │   └── imgui/                # ImGui + backends
│   └── cmake-build-debug/
│
├── ChatSystem_SharedMemory/      # Shared Memory implementation
│   ├── CMakeLists.txt
│   ├── shared.h                  # Shared memory structures
│   ├── shared.cpp
│   ├── Client/
│   │   ├── client.cpp
│   │   ├── client.h
│   │   └── main.cpp              # CLI client
│   ├── Server/
│   │   ├── main.cpp              # Server
│   │   ├── server.cpp
│   │   └── server.h
│   ├── GUI/
│   │   ├── client_gui.cpp        # GUI client implementation
│   │   ├── client_gui.h
│   │   ├── server_gui.cpp
│   │   ├── server_gui.h
│   │   └── imgui/                # ImGui + backends
│   └── cmake-build-debug/
│
└── README.md
```

## Implementations

### 1. Socket-Based Chat System (ChatSystem_Sockets/)

**Communication Method**: Windows Sockets (Winsock2) for network-based communication

**Key Features**:
- Network-based client/server communication
- Multi-threaded server with per-client threads
- Non-blocking socket operations
- Cross-machine communication support
- Default port: 54000

**Networking Architecture**:
- `ChatClient` class: Thread-safe message queue, non-blocking receive
- Proper resource cleanup with RAII patterns
- Better error handling and connection tracking

### 2. Shared Memory Chat System (ChatSystem_SharedMemory/)

**Communication Method**: Windows Shared Memory for inter-process communication

**Key Features**:
- Fast IPC using shared memory segments
- Mutex-based synchronization for thread safety
- Local machine communication only
- Lower latency for same-machine messaging
- Structured message passing with fixed-size buffers

**Architecture**:
- `shared.h/cpp`: Shared memory structures and management
- `Client/`: Client-side IPC implementation
- `Server/`: Server-side shared memory management
- `GUI/`: GUI implementation with shared memory backend

## Building & Running

### Building Socket-Based Implementation

```bash
cd ChatSystem_Sockets
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Debug
```

**Running**:
```bash
# Terminal 1: Start server
./build/Debug/server

# Terminal 2: Run CLI client (optional)
./build/Debug/client

# Terminal 3: Run GUI client
./build/Debug/ChatGUI
```

### Building Shared Memory Implementation

```bash
cd ChatSystem_SharedMemory
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Debug
```

**Running**:
```bash
# Terminal 1: Start server
./build/Debug/server

# Terminal 2: Run CLI client (optional)
./build/Debug/client

# Terminal 3: Run GUI client
./build/Debug/ChatGUI
```

## Dependencies

### Required for Both Implementations:
- **CMake** 3.10+
- **C++17 compiler** (MSVC, GCC, or Clang)
- **Windows SDK** (for Winsock2 and Shared Memory APIs)
- **GLFW3** (for GUI client)
- **OpenGL 3.0+** (for GUI rendering)

### Installation (vcpkg):
```powershell
vcpkg install glfw3:x64-windows
vcpkg integrate install
```

### Socket-Based Specific:
- Winsock2 (included in Windows SDK)

### Shared Memory Specific:
- Windows Named Shared Memory APIs (included in Windows SDK)
- Synchronization primitives (Mutex, Events)

## Features

### GUI Client (Both Implementations)
- **Real-time messaging**: Send and receive messages instantly
- **Connection status**: Visual indicator (disconnected/connected states)
- **Menu bar**: Connect/Disconnect options
- **Auto-scroll**: Chat log follows new messages
- **Input validation**: Enter key sends messages
- **Message formatting**: Shows sender and timestamp for each message
- **Scrollable history**: Browse past conversations

### Server (Both Implementations)
- **Multi-client support**: Broadcasts to all connected clients
- **Thread-safe operations**: Safe concurrent access to shared data
- **Clean shutdown**: Graceful client disconnection handling
- **Real-time relay**: Instant message distribution

### Socket-Based Advantages
- Network communication across machines
- Scalable to distributed systems
- Standard network protocols

### Shared Memory Advantages
- Minimal latency for local IPC
- Efficient for single-machine deployments
- Lower CPU overhead than sockets

## Code Quality & Architecture

### Design Patterns
- **Modern C++17**: Smart pointers, RAII, const-correctness
- **Thread safety**: Mutex-protected shared resources
- **Error handling**: Comprehensive error checking and reporting
- **Separation of concerns**: Decoupled networking, GUI, and core logic
- **Documentation**: Inline documentation in headers

### Implementation Comparison

| Feature | Socket-Based | Shared Memory |
|---------|-------------|---------------|
| **Range** | Network/Distributed | Local Only |
| **Latency** | Higher | Minimal |
| **Setup** | Port binding required | Shared memory segment |
| **Scalability** | Across machines | Single machine |
| **Use Case** | Chat servers, remote comms | Local service IPC |
| **Firewall** | Requires configuration | N/A |

## Troubleshooting

### Socket-Based Issues
- **"Failed to connect"**: Ensure server is running and port 54000 is not blocked
- **"Connection refused"**: Server may not be listening, restart it
- **Firewall blocks**: Add exception for the port in Windows Firewall

### Shared Memory Issues
- **"Cannot attach to shared memory"**: Server must be running first
- **"Permission denied"**: Run both with same privilege level (Admin/User)
- **"Already in use"**: Previous instance may not have cleaned up; restart server

### Both Implementations
- **GUI freezes**: Check Windows Event Viewer for crashes
- **Messages not appearing**: Verify all programs are running
- **Compilation errors**: Ensure C++17 is enabled in CMake settings

## Comparison & When to Use

### Use Socket-Based When:
- Building distributed chat systems
- Scaling across multiple machines
- Planning for internet deployment
- Need standard network protocols

### Use Shared Memory When:
- Building local service communication
- Minimizing latency is critical
- All processes run on same machine
- Reducing CPU overhead matters

## Testing

Both implementations can be tested with:
1. Start the server
2. Launch multiple GUI clients
3. Send messages from each client
4. Verify broadcast delivery to all connected clients
5. Test disconnection and reconnection

## Future Enhancements

- UDP support for lower latency
- SSL/TLS encryption
- User authentication
- Persistent message history (database)
- Cross-platform file transfer
- Dark/Light theme toggle
- Emoji support
- Per-user private messaging
- Chat room/channel support

---

**Project**: Chat System - Operating Systems Project  
**C++ Version**: 17  
**Team Members**: See Team Members.ahk
