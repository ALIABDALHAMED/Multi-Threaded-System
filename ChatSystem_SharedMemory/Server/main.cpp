#include "../GUI/server_gui.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Starting Chat Server..." << std::endl;

    ServerGUI gui;

    if (!gui.Initialize()) {
        std::cerr << "Failed to initialize server GUI" << std::endl;
        return 1;
    }

    gui.Run();
    gui.Shutdown();

    std::cout << "Server shutdown complete" << std::endl;
    return 0;
}
