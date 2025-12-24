#include "shared.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

static SharedMemory* shared_mem = nullptr;
static bool is_creator = false;

#ifdef _WIN32
static HANDLE shared_mem_handle = NULL;
#endif

bool create_shared_memory() {
#ifdef _WIN32
    shared_mem_handle = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(SharedMemory),
        SHARED_MEMORY_NAME
    );

    if (shared_mem_handle == NULL) {
        std::cerr << "Failed to create shared memory: " << GetLastError() << std::endl;
        return false;
    }

    shared_mem = (SharedMemory*)MapViewOfFile(
        shared_mem_handle,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedMemory)
    );

    if (shared_mem == NULL) {
        std::cerr << "Failed to map shared memory: " << GetLastError() << std::endl;
        CloseHandle(shared_mem_handle);
        return false;
    }

    is_creator = (GetLastError() != ERROR_ALREADY_EXISTS);
    if (is_creator) {
        new (shared_mem) SharedMemory(); // Placement new to initialize
    }

#else
    int fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "Failed to create shared memory" << std::endl;
        return false;
    }

    if (ftruncate(fd, sizeof(SharedMemory)) == -1) {
        std::cerr << "Failed to set shared memory size" << std::endl;
        close(fd);
        return false;
    }

    shared_mem = (SharedMemory*)mmap(
        NULL,
        sizeof(SharedMemory),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    if (shared_mem == MAP_FAILED) {
        std::cerr << "Failed to map shared memory" << std::endl;
        close(fd);
        return false;
    }

    close(fd);

    // Check if we're the creator
    struct stat st;
    if (fstat(fd, &st) == 0 && st.st_size == 0) {
        is_creator = true;
        new (shared_mem) SharedMemory(); // Placement new to initialize
    }
#endif

    return true;
}

bool attach_shared_memory() {
#ifdef _WIN32
    shared_mem_handle = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHARED_MEMORY_NAME
    );

    if (shared_mem_handle == NULL) {
        std::cerr << "Failed to open shared memory: " << GetLastError() << std::endl;
        return false;
    }

    shared_mem = (SharedMemory*)MapViewOfFile(
        shared_mem_handle,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedMemory)
    );

    if (shared_mem == NULL) {
        std::cerr << "Failed to map shared memory: " << GetLastError() << std::endl;
        CloseHandle(shared_mem_handle);
        return false;
    }

#else
    int fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "Failed to open shared memory" << std::endl;
        return false;
    }

    shared_mem = (SharedMemory*)mmap(
        NULL,
        sizeof(SharedMemory),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    if (shared_mem == MAP_FAILED) {
        std::cerr << "Failed to map shared memory" << std::endl;
        close(fd);
        return false;
    }

    close(fd);
#endif

    return true;
}

void detach_shared_memory() {
    if (shared_mem) {
#ifdef _WIN32
        UnmapViewOfFile(shared_mem);
        if (shared_mem_handle) {
            CloseHandle(shared_mem_handle);
            shared_mem_handle = NULL;
        }
        if (is_creator) {
            // Note: On Windows, shared memory persists until system restart
            // or explicitly deleted. For this demo, we'll leave cleanup to OS.
        }
#else
        munmap(shared_mem, sizeof(SharedMemory));
        if (is_creator) {
            shm_unlink(SHARED_MEMORY_NAME);
        }
#endif
        shared_mem = nullptr;
    }
}

SharedMemory* get_shared_memory() {
    return shared_mem;
}
