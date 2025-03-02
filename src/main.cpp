#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <cmath>

#include "chip8.h"

constexpr uint32_t windowStartWidth  = 1280;
constexpr uint32_t windowStartHeight = 640;

bool show_demo_window    = true;
bool show_another_window = false;

struct AppContext
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_AppResult app_quit = SDL_APP_CONTINUE;
};

SDL_AppResult SDL_Fail()
{
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    // create a window
    SDL_Window* window =
        SDL_CreateWindow("SDL Minimal Sample", windowStartWidth, windowStartHeight, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if(not window)
    {
        return SDL_Fail();
    }

    // create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if(renderer == nullptr)
    {
        return SDL_Fail();
    }

    // print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        SDL_Log("Window size: %ix%i", width, height);
        SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
        if(width != bbwidth)
        {
            SDL_Log("This is a highdpi environment.");
        }
    }

    // set up the application data
    *appstate = new AppContext{
        .window   = window,
        .renderer = renderer,
    };

    SDL_SetRenderVSync(renderer, -1); // enable vysnc

    SDL_Log("Application started successfully!");

    chip8::init();
    chip8::load("C:/Users/tiago.ferreira/Downloads/Pong.ch8");
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* app = (AppContext*)appstate;

    if(event->type == SDL_EVENT_QUIT)
    {
        app->app_quit = SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    chip8::update();

    auto* app = reinterpret_cast<AppContext*>(appstate);

    if(chip8::draw_triggered() == false)
    {
        return app->app_quit;
    }

    auto renderer = app->renderer;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    auto _draw_buffer = chip8::gfx_buffer();
    for(auto i = 0; i != chip8::DRAW_BUFFER_HEIGHT; i++)
    {
        for(auto j = 0; j != chip8::DRAW_BUFFER_WIDTH; j++)
        {
            if(_draw_buffer[i * chip8::DRAW_BUFFER_WIDTH + j])
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

                SDL_FRect pixel = {static_cast<float>(j * 20), static_cast<float>(i * 20), 20, 20};
                // SDL_RenderFillRect(renderer, &pixel);
                SDL_RenderRect(renderer, &pixel);
            }
        }
    }

    SDL_RenderPresent(app->renderer);

    return app->app_quit;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    if(auto* app = reinterpret_cast<AppContext*>(appstate))
    {
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);

        delete app;
    }

    SDL_Log("Application quit successfully!");
    SDL_Quit();
}