#include "game.h"
#include "Renderer/renderer.h"
#include "Renderer/texture.h"
#include "GameWorld/world.h"
#include "GameWorld/chunk.h"
#include "Player/gui.h"
#include "Networking/server.h"
#include "macros.h"

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


Game::~Game()
{
    if(this->gui != nullptr)
    {
        delete this->gui;
    }
}

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
            player.world = world;
            break;
    }
}

Dotnet dotnet;

Server server;

void Game::simulate(Renderer *renderer, float dt)
{
    if(!initialized)
    {
        server.Init(8888);
        server.Deinit();

        dotnet.loadClrLib();
        dotnet.startHost();
        dotnet.test();
        dotnet.stopHost();

        initialized = true;

        player.transform.position = Vec3(0.0f, 10.0f, 0.0f);

        sf::Vector2i globalPosition = sf::Mouse::getPosition();
        mousePosLast = Vec2((float)globalPosition.x, (float)globalPosition.y);
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

        blockStore.createBlock(1, {"Stone", {2, 2, 2, 2, 2, 2}});
        blockStore.createBlock(2, {"Grass", {4, 4, 1, 3, 4, 4}});
        blockStore.createBlock(3, {"Dirt", {3, 3, 3, 3, 3, 3}});
        blockStore.createBlock(4, {"Cobblestone", {17, 17, 17, 17, 17, 17}});
        blockStore.createBlock(9, {"Water", {206, 206, 206, 206, 206, 206}});
        blockStore.createBlock(12, {"Sand", {177, 177, 177, 177, 177, 177}});
        blockStore.createBlock(13, {"Gravel", {20, 20, 20, 20, 20, 20}});
        blockStore.createBlock(17, {"Wood", {21, 21, 22, 22, 21, 21}});
        blockStore.createBlock(18, {"Leaves", {53, 53, 53, 53, 53, 53}});
        blockStore.createBlock(45, {"Brick", {8, 8, 8, 8, 8, 8}});

        world = new World(renderer, &blockStore);
        world->cameras.push_back(&player.camera);
        world->cameras.push_back(&spectator.camera);
        setMode(Mode::Mode_Player);

        this->gui = new Gui(renderer, &this->player, &blockStore);
    }

    // get the global mouse position (relative to the desktop)
    sf::Vector2i globalPosition = sf::Mouse::getPosition();
    this->mouseDelta = Vec2(globalPosition.x - mousePosLast.x, globalPosition.y - mousePosLast.y);
    
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
    {
        sf::Mouse::setPosition(sf::Vector2i(renderer->width / 2.0f, renderer->height / 2.0f), *window);
        globalPosition = sf::Mouse::getPosition();
    }
    mousePosLast = Vec2((float)globalPosition.x, (float)globalPosition.y);

    if(mode == Mode::Mode_FreeView)
    {
        spectator.update(dt, mouseDelta);
    }
    else
    {
        player.update(dt, mouseDelta);
    }

    world->update(activeCam);
}

void Game::keyPress(sf::Keyboard::Key key)
{
    if(key == sf::Keyboard::F1)
        setMode(Mode::Mode_Player);
    else if(key == sf::Keyboard::F2)
        setMode(Mode::Mode_FreeView);
    else if(key == sf::Keyboard::E)
    {
        for(int i = 0; i < ARRAY_COUNT(player.inventory.mainSlots); i++)
        {
            Inventory::Slot slot = player.inventory.mainSlots[i];
            if(slot.blockId != 0 && slot.stacks != 0)
            {
                printf("%s: %d\n", blockStore.blocks[slot.blockId].name, slot.stacks);
            }
        }
    }
    else if(key == sf::Keyboard::Add)
    {
        player.inventory.nextSlot();
    }
    else if(key == sf::Keyboard::Subtract)
    {
        player.inventory.prevSlot();
    }
}

void Game::mouseClick(int button)
{
    if(mode == Mode::Mode_Player)
    {
        RaycastHit hit;
        if(world->lineCast(hit, activeCam->transform.position, activeCam->transform.position + 10.0f*activeCam->transform.forward()))
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

void Game::updateAndRender(Renderer *renderer, float dt)
{    
    simulate(renderer, dt);

    this->activeCam->targetWidth = renderer->width;
    this->activeCam->targetHeight = renderer->height;

    renderer->clearScreen(Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    world->render();
    if(mode == Mode::Mode_Player)
        gui->render(dt);
}
