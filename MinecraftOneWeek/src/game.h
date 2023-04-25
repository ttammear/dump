#include "Maths/maths.h"
#include "camera.h"
#include "Player/player.h"
#include "GameWorld/blockstore.h"

#include <SDL2/SDL.h>

class Game
{
public:

    enum Mode
    {
        Mode_FreeView,
        Mode_Player
    };

    ~Game();

    void simulate(class Renderer* renderer, float dt);
    void updateAndRender(class Renderer *renderer, float dt);
    void mouseClick(int button);
    void keyPress(SDL_Keycode key);
    void setMode(uint32_t mode);
    void mouseMotion(float x, float y);
    void mouseScroll(int delta);

    bool initialized = false;
    uint32_t mode = Mode::Mode_FreeView;

    Camera *activeCam;
    Player player;
    Camera freeCam;

    Vec2 mouseDelta;
    Vec2 camRot;

    BlockStore blockStore;

    // TODO: TEMPORARY
    SDL_Window* window;
    class Gui *gui = nullptr;
    class World* world;
    class TextureArray *atlas; // texture atlas
};
