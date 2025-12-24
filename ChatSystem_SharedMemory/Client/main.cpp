#include "../GUI/client_gui.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Starting Chat Client..." << std::endl;

    ClientGUI gui;

    if (!gui.Initialize()) {
        std::cerr << "Failed to initialize client GUI" << std::endl;
        return 1;
    }

    gui.Run();
    gui.Shutdown();

    std::cout << "Client shutdown complete" << std::endl;
    return 0;
}
