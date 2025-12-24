cmake_minimum_required(VERSION 3.10)
project(ChatSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/GUI)
include_directories(${CMAKE_SOURCE_DIR}/GUI/imgui)

# Source files
set(SHARED_SOURCES
    shared.h
    shared.cpp
)

set(SERVER_SOURCES
    Server/server.h
    Server/server.cpp
    Server/main.cpp
)

set(CLIENT_LIBRARY_SOURCES
    Client/client.h
    Client/client.cpp
)

set(CLIENT_SOURCES
    Client/client.h
    Client/client.cpp
    Client/main.cpp
)

set(GUI_SOURCES
    GUI/imgui/imgui.cpp
    GUI/imgui/imgui_demo.cpp
    GUI/imgui/imgui_draw.cpp
    GUI/imgui/imgui_tables.cpp
    GUI/imgui/imgui_widgets.cpp
    GUI/imgui/imgui_impl_win32.cpp
    GUI/imgui/imgui_impl_dx11.cpp
)

set(SERVER_GUI_SOURCES
    GUI/server_gui.h
    GUI/server_gui.cpp
)

set(CLIENT_GUI_SOURCES
    GUI/client_gui.h
    GUI/client_gui.cpp
)

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBS
        d3d11.lib
        d3dcompiler.lib
        dxgi.lib
        dwmapi.lib
        user32.lib
        kernel32.lib
        gdi32.lib
    )
else()
    # For Linux/Unix systems
    find_package(X11 REQUIRED)
    set(PLATFORM_LIBS
        ${X11_LIBRARIES}
        rt  # for shared memory
    )
endif()

# Server executable
add_executable(ChatServer
    ${SHARED_SOURCES}
    ${SERVER_SOURCES}
    ${CLIENT_LIBRARY_SOURCES}
    ${GUI_SOURCES}
    ${SERVER_GUI_SOURCES}
)

target_link_libraries(ChatServer
    Threads::Threads
    ${PLATFORM_LIBS}
)

# Client executable
add_executable(ChatClient
    ${SHARED_SOURCES}
    ${CLIENT_SOURCES}
    ${GUI_SOURCES}
    ${CLIENT_GUI_SOURCES}
)

target_link_libraries(ChatClient
    Threads::Threads
    ${PLATFORM_LIBS}
)

# Set output directories
set_target_properties(ChatServer ChatClient
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Compiler flags
if(MSVC)
    # Windows specific flags
    target_compile_options(ChatServer PRIVATE /W4 /permissive-)
    target_compile_options(ChatClient PRIVATE /W4 /permissive-)
else()
    # GCC/Clang flags
    target_compile_options(ChatServer PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(ChatClient PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation
install(TARGETS ChatServer ChatClient
    RUNTIME DESTINATION bin
)
