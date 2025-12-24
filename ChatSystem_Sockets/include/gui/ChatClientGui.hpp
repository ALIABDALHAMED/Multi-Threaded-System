#include "gui/ChatGui.hpp"

// Client-only GUI application
class ChatClientGui {
public:
    ChatClientGui();
    ~ChatClientGui();

    bool init(const std::string& title, int width, int height);
    void render();
    bool is_running() const;
    void shutdown();

private:
    enum class State {
        CONNECTING,
        CONNECTED,
        DISCONNECTED
    };

    std::unique_ptr<ChatClient> client_;
    std::vector<std::string> messages_;
    char ip_buffer_[64];
    int port_;
    char input_buffer_[512];
    State state_;
    bool connected_;
};
