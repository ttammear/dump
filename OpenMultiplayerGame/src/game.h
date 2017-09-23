#include "Maths/maths.h"
#include "camera.h"
#include "Player/player.h"
#include "Player/spectator.h"
#include "GameWorld/blockstore.h"

#include <SDL2/SDL_keyboard.h>

class Game
{
public:

    enum Mode
    {
        Mode_FreeView,
        Mode_Player
    };

    ~Game();

    void simulate(class Renderer* renderer, double dt);
    void updateAndRender(class Renderer *renderer, double dt);
    void mouseClick(int button);
    void keyPress(SDL_Keycode key);
    void setMode(uint32_t mode);
    void mouseScroll(int delta);

    bool initialized = false;
    uint32_t mode = Mode::Mode_FreeView;

    Camera *activeCam;
    Player player;
    Spectator spectator;

    Vec2 mousePosLast;
    Vec2 mouseDelta;
    Vec2 camRot;

    BlockStore blockStore;

    // TODO: TEMPORARY
    struct SDL_Window *window;
    class Gui *gui = nullptr;
    class World* world;
    class TextureArray *atlas; // texture atlas
};
