#include "game.h"
#include "Renderer/renderer.h"
#include "Renderer/texture.h"
#include "GameWorld/world.h"
#include "Player/gui.h"
#include "Networking/server.h"
#include "Networking/client.h"
#include "macros.h"

#include <SDL2/SDL.h>
#include "Dotnet/dotnet.h"

#include <cstdio>

#define        STBI_NO_JPEG
#define        STBI_NO_BMP
#define        STBI_NO_PSD
#define        STBI_NO_TGA
#define        STBI_NO_GIF
#define        STBI_NO_HDR
#define        STBI_NO_PIC
#define        STBI_NO_PNM 
#define STB_IMAGE_IMPLEMENTATION
#include "Libs/stb_image.h"


void Game::setMode(uint32_t mode)
{
    this->mode = mode;

    switch(mode)
    {
        case Mode::Mode_FreeView:
            activeCam = &spectator.camera;
            CLEARFLAG(spectator.camera.flags, Camera::Flags::Disabled);
            SETFLAG(player.camera.flags, Camera::Flags::Disabled);
            break;
        case Mode::Mode_Player:
            activeCam = &player.camera;
            SETFLAG(spectator.camera.flags, Camera::Flags::Disabled);
            CLEARFLAG(player.camera.flags, Camera::Flags::Disabled);
            break;
    }
}

Dotnet dotnet;

#ifdef SERVER
Server *server;
#else
Client client;
#endif

Transform testTrans;

Game::~Game()
{
    if(this->gui != nullptr)
    {
        delete this->gui;
    }
    dotnet.stopHost();
#ifdef SERVER
    server->deinit();
    delete server;
#else
    client.deinit();
#endif
}

void worldTick(void *ptr)
{
    ((Game*)ptr)->onTick();
}

void Game::onTick()
{
#ifdef CLIENT
    client.sendFrameInputs(0);
#endif
}

void Game::simulate(Renderer *renderer, double dt)
{
    if(!initialized)
    {
        dotnet.loadClrLib();
        dotnet.startHost();
        testTrans.position = Vec3(999.0f, 1337.0f, 8888.69f);

        initialized = true;

        player.transform.position = Vec3(0.0f, 10.0f, 0.0f);

        int mx,my;
        SDL_GetMouseState(&mx, &my);
        mousePosLast = Vec2((float)mx, (float)my);
        camRot = Vec2(0.0f, 0.0f);

        // texture
        atlas = new TextureArray(16, 16, 4, 257);

        int width, height, comps;
        unsigned char *data;

        data = stbi_load("Resources/mcatlas.png", &width, &height, &comps, 0);
        uint8_t texData[16*16*4];
        memset(texData, 0xFF, 16*16*4);
        atlas->copyLayer(texData, 16, 16, 4, 0); // first layer white
        for(int i = 0; i < 16; i++)
        for(int j = 0; j < 16; j++)
        {
            for(int x = 0; x < 16; x++)
            {
                memcpy(&texData[x*16*4], &data[(i*16+x)*width*4 + (j*16)*4], 64);
            }
            atlas->copyLayer(texData, 16, 16, 4, i*16+j+1);
        }
        stbi_image_free(data);

        Renderer::defaultMaterial->addTextureArray("texArr", atlas);
        Renderer::solidMaterial->addTextureArray("texArr", atlas);

        world = new World(renderer);
        world->cameras.push_back(&player.camera);
        world->cameras.push_back(&spectator.camera);
        setMode(Mode::Mode_Player);
        player.init(world);
        world->onTickUserPtr = this;
        world->onTick = worldTick;
#ifdef SERVER
        server = new Server(world);
        server->init(8888);
#else
        client.entities = world->entities;
        client.init();
        client.connect("127.0.0.1", 8888);
#endif

        this->gui = new Gui(renderer, &this->player);

        dotnet.test(&testTrans, world);
    }
    dotnet.update();

    // get the global mouse position (relative to the desktop)
    bool lctrlDown = (SDL_GetModState() & KMOD_LCTRL) != 0;
    bool hasFocus = (SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS) != 0;
    if(hasFocus && !lctrlDown)
    {
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        this->mouseDelta = Vec2(mx - mousePosLast.x, my - mousePosLast.y);
        
        if (!lctrlDown)
        {
            SDL_WarpMouseInWindow(window, renderer->width / 2.0f, renderer->height / 2.0f);
            SDL_GetMouseState(&mx, &my);
            mousePosLast = Vec2((int)renderer->width / 2, (int)renderer->height / 2);
        }
        else
            mousePosLast = Vec2((float)mx, (float)my);
    }
    else
    {
        this->mouseDelta = Vec2(0.0f, 0.0f);
    }
    camRot += mouseDelta;

    if(mode == Mode::Mode_FreeView)
    {
        spectator.update(dt, mouseDelta);
    }
    else
    {
        const Uint8 *state = SDL_GetKeyboardState(NULL);

        PlayerInput pinput;
        pinput.moveForward = state[SDL_SCANCODE_W];
        pinput.moveBackward = state[SDL_SCANCODE_S];
        pinput.moveRight = state[SDL_SCANCODE_D];
        pinput.moveLeft = state[SDL_SCANCODE_A];
        pinput.jump = state[SDL_SCANCODE_SPACE];
        auto rot = Quaternion::AngleAxis(camRot.x, Vec3(0.0f, 1.0f, 0.0f)) * Quaternion::AngleAxis(camRot.y, Vec3(1.0f, 0.0f, 0.0f));
        pinput.rotation = rot;
        player.update(dt, mouseDelta, pinput);
    }

    world->update(dt, activeCam);
}

void Game::keyPress(SDL_Keycode key)
{
    if(key == SDLK_F1)
        setMode(Mode::Mode_Player);
    else if(key == SDLK_F2)
        setMode(Mode::Mode_FreeView);
    else if(key == SDLK_PLUS)
    {
        player.inventory.nextSlot();
    }
    else if(key == SDLK_MINUS)
    {
        player.inventory.prevSlot();
    }
}

void Game::mouseClick(int button)
{
    printf("mouseclick\n");
    if(mode == Mode::Mode_Player)
    {
        RaycastHit hit;
/*        if(world->lineCast(hit, activeCam->transform.position, activeCam->transform.position + 10.0f*activeCam->transform.forward()))
        {
            if (button == 0)
            {
                uint8_t gotBlock = world->setBlockId(hit.block, 0);
                if(gotBlock == 2) // grass changes to dirt when broken
                    gotBlock = 3;
                player.inventory.tryStoreBlock(gotBlock);
            }
            else if(button == 1)
            {
                uint8_t gotBlock = player.inventory.tryRemoveBlock(player.inventory.activeSlot);
                if(gotBlock != 0)
                {
                    world->setBlockId(hit.block + hit.faceDirection, gotBlock);
                }
            }
        }
*/
    }
}

void Game::mouseScroll(int delta)
{
    if(delta > 0)
    {
        player.inventory.nextSlot();
    }
    else if(delta < 0)
    {
        player.inventory.prevSlot();
    }
}

void Game::updateAndRender(Renderer *renderer, double dt)
{        
    if(initialized)
    {
#ifdef SERVER
    server->doEvents();
#else
    client.doEvents();
#endif
    }

    simulate(renderer, dt);
#ifdef SERVER
    server->takeSnapshot(world);
#else
    client.updateTransforms();
#endif

    this->activeCam->targetWidth = renderer->width;
    this->activeCam->targetHeight = renderer->height;

    renderer->clearScreen(Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    world->render();
    if(mode == Mode::Mode_Player)
        gui->render(dt);
}
