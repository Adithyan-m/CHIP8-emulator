#include "SDL.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//====================== DATA TYPES ======================//

typedef struct {
    SDL_Window *window;
} sdl_t;

typedef struct {
    uint32_t window_width;
    uint32_t window_height;
} config_t;

//====================== FUNCTIONS ======================//

// SDL Initializer
bool init_sdl(sdl_t *sdl, config_t *config) {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) ! = 0) {
        SDL_Log("Could not initialize SDL %s\n", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED, config->window_width,
                                   config->window_height, 0);

    if (!sdl->window) {
        SDL_Log("Could not initialize window %s\n", SDL_GetError);
        return false;
    }

    return true;
}

// Cleanup Function
final_cleanup(void) { SDL_Quit(); }

int main(int argc, char **argv) {

    (void)args;
    (void)argv;

    // Initialize SDL
    sdl_t sdl = {0};
    if (init_sdl(&sdl))
        exit(EXIT_FAILURE);

    final_cleanup();
}