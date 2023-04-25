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

    struct SDL_Renderer* displayRenderer;
    struct SDL_Window* displayWindow;
private:
    bool initialized;
    bool running;
    unsigned int startTime;
    class Renderer *mainRenderer;
    class Game *game;
};
