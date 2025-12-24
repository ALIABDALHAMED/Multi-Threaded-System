cmake_minimum_required(VERSION 3.10)
project(ChatSystem_Sockets)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ====================================================================
# Find required packages
# ====================================================================
find_package(Threads REQUIRED)
find_package(OpenGL QUIET)
find_package(glfw3 CONFIG QUIET)

# ====================================================================
# Common include directories
# ====================================================================
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ====================================================================
# Check for GLFW3 and OpenGL availability
# ====================================================================
if(NOT glfw3_FOUND)
    if(EXISTS "C:/libs/glfw/include/GLFW/glfw3.h")
        set(GLFW_FOUND TRUE)
        set(GLFW_INCLUDE_DIR "C:/libs/glfw/include")
        set(GLFW_LIB_DIR "C:/libs/glfw/lib")
    else()
        set(GLFW_FOUND FALSE)
    endif()
else()
    set(GLFW_FOUND TRUE)
endif()

# ====================================================================
# Unified GUI Chat Application (Server + Client)
# ====================================================================

# Build ChatApp and ChatClient if both GLFW and OpenGL are available
if(GLFW_FOUND AND OPENGL_FOUND)
    message(STATUS "Building ChatApp target (server with GUI)")
    message(STATUS "Building ChatClient target (client-only GUI)")

    # Server application (Server.exe)
    add_executable(Server
        gui/main_gui.cpp
        src/gui/ChatGui.cpp
        src/networking/ChatClient.cpp
        src/networking/ChatServer.cpp
        gui/imgui/imgui.cpp
        gui/imgui/imgui_draw.cpp
        gui/imgui/imgui_tables.cpp
        gui/imgui/imgui_widgets.cpp
        gui/imgui/imgui_impl_glfw.cpp
        gui/imgui/imgui_impl_opengl3.cpp
    )

    # Client application (Client.exe)
    add_executable(Client
        gui/main_client.cpp
        src/gui/ChatClientGui.cpp
        src/networking/ChatClient.cpp
        gui/imgui/imgui.cpp
        gui/imgui/imgui_draw.cpp
        gui/imgui/imgui_tables.cpp
        gui/imgui/imgui_widgets.cpp
        gui/imgui/imgui_impl_glfw.cpp
        gui/imgui/imgui_impl_opengl3.cpp
    )

    # Common target includes
    foreach(TARGET Server Client)
        target_include_directories(${TARGET}
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/gui/imgui
        )

        if(glfw3_FOUND)
            target_link_libraries(${TARGET}
                PRIVATE
                glfw
                OpenGL::OpenGL
                Threads::Threads
                ws2_32
            )
        else()
            target_include_directories(${TARGET} PRIVATE ${GLFW_INCLUDE_DIR})
            target_link_libraries(${TARGET}
                PRIVATE
                "${GLFW_LIB_DIR}/libglfw3.a"
                opengl32
                Threads::Threads
                ws2_32
            )
        endif()
    endforeach()

else()
    message(STATUS "Server and Client targets will NOT be built")
    message(STATUS "  Reason:")
    if(NOT OPENGL_FOUND)
        message(STATUS "    - OpenGL not found (should be available on Windows)")
    endif()
    if(NOT GLFW_FOUND)
        message(STATUS "    - GLFW3 not found")
        message(STATUS "    - Option 1: Install via vcpkg:")
        message(STATUS "      vcpkg install glfw3:x64-windows")
        message(STATUS "    - Option 2: Download GLFW and build manually")
        message(STATUS "    - Option 3: Place GLFW in C:/libs/glfw/")
    endif()
endif()

# ====================================================================
# Compiler-specific settings
# ====================================================================
if(MSVC)
    # Visual Studio
    if(TARGET Server)
        target_compile_options(Server PRIVATE /W4)
    endif()
    if(TARGET Client)
        target_compile_options(Client PRIVATE /W4)
    endif()
else()
    # GCC/Clang (MinGW)
    if(TARGET Server)
        target_compile_options(Server PRIVATE -Wall -Wextra)
    endif()
    if(TARGET Client)
        target_compile_options(Client PRIVATE -Wall -Wextra)
    endif()
endif()
