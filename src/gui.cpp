//
// Created by Brian Bonafilia on 6/3/21.
//
#include <iostream>
#include "include/cpu.hpp"
#include "include/cartridge.hpp"

#include "include/SDL2/SDL.h"
#include "include/SDL2/SDL_timer.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 640

#define PIXEL_WIDTH 256
#define PIXEL_HEIGHT 240

namespace GUI {

    SDL_Renderer* renderer = NULL;
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    SDL_Surface * imageSurface = NULL;

    SDL_Texture* gamePixels;

    void update_frame(u32* pixels) {
        SDL_UpdateTexture(gamePixels, NULL, pixels, PIXEL_WIDTH * sizeof(u32));
    }

    void render() {
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, gamePixels, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    int init() {
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            printf("failed to init video");
            return -1;
        }
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
            return 1;
        }
        window = SDL_CreateWindow(
                "hello_sdl2",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                SCREEN_WIDTH, SCREEN_HEIGHT,
                SDL_WINDOW_SHOWN
        );
        if (window == NULL) {
            fprintf(stderr, "could not create window: %s\n", SDL_GetError());
            return 1;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        SDL_RenderSetLogicalSize(renderer, PIXEL_WIDTH, PIXEL_HEIGHT);
        gamePixels = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                       PIXEL_WIDTH, PIXEL_HEIGHT);

        u32 startFrame, endFrame, timeToRunFrame;
        const int frameRate = 60;
        int delay = 1000 / frameRate;
        bool is_running = true;
        SDL_Event event;
        while (is_running) {
            startFrame = SDL_GetTicks();
            CPU::run_frame();
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    is_running = false;
                }
            }
            endFrame = SDL_GetTicks();
            timeToRunFrame = endFrame - startFrame;
            std::cout << "time to tick " << std::to_string(delay) << std::endl;
            std::cout << "start time: " << std::to_string(startFrame) << std::endl;
            std::cout << "end time: " << std::to_string(endFrame) << std::endl;
            if (timeToRunFrame < delay) {
                SDL_Delay(delay - timeToRunFrame);
            }
            render();
        }
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }
}