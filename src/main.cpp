#include <iostream>
#include "unistd.h"
#include "include/cpu.hpp"
#include "include/cartridge.hpp"

#include "include/SDL2/SDL.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 800


int main(int argc, char *argv[]) {
    std::cout << "the ROM we are using is " << argv[1] << std::endl;
    Cartridge::load(argv[1]);
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("facl");
    }
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    SDL_Surface * imageSurface = NULL;
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
    screenSurface = SDL_GetWindowSurface(window);
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
    imageSurface = SDL_LoadBMP("puppy.bmp");
    SDL_BlitSurface(imageSurface, NULL, screenSurface, NULL);
    SDL_UpdateWindowSurface(window);
    // A basic main loop to prevent blocking
    bool is_running = true;
    SDL_Event event;
    while (is_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                is_running = false;
            }
        }
        SDL_Delay(16);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    while (true) {
        CPU::run_frame();
    }
}