#include "gui/ChatClientGui.hpp"

int main() {
    ChatClientGui client_gui;

    if (!client_gui.init("Client", 1000, 700)) {
        return 1;
    }

    // Main render loop
    while (client_gui.is_running()) {
        client_gui.render();
    }

    client_gui.shutdown();
    return 0;
}
