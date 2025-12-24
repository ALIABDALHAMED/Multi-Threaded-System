#include "gui/ChatGui.hpp"

int main() {
    ChatGui gui;

    if (!gui.init("Server", 1000, 700)) {
        return 1;
    }

    // Main render loop
    while (gui.is_running()) {
        gui.render();
    }

    gui.shutdown();
    return 0;
}
