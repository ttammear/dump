#include <chrono>

class Application
{
public:
    Application();
    ~Application();
    void start();
    void doEvents();
    void doFrame();
    void exit();
    bool isRunning();

    struct SDL_Window *window;
private:
    bool initialized;
    class Renderer *mainRenderer;
    class Game *game;

    std::chrono::steady_clock::time_point startTime;
};
