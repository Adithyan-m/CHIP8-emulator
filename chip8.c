#include "SDL.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//====================== DATA TYPES ======================//

typedef struct {
    SDL_Window *window;
    SDL_Renderer *rend;
} sdl_t;

typedef struct {
    uint32_t window_width;
    uint32_t window_height;
    uint32_t fg_colour; // RRBBGGAA
    uint32_t bg_colour; // RRGGBBAA
    uint16_t scaler;
} config_t;

// Emulator states
typedef enum { QUIT, RUNNING, PAUSED } emulator_state_t;

typedef struct {
    emulator_state_t state;
} chip8_t;

//====================== INITIALIZER FUNCTIONS ======================//

// SDL Initializer
bool init_sdl(sdl_t *sdl, config_t *config) {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Could not initialize SDL %s\n", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   config->window_width * config->scaler,
                                   config->window_height * config->scaler, 0);

    if (!sdl->window) {
        SDL_Log("Could not initialize window %s\n", SDL_GetError());
        return false;
    }

    sdl->rend = SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_ACCELERATED);
    if (!sdl->rend) {
        SDL_Log("Could not initialize renderer %s\n", SDL_GetError());
        return false;
    }

    return true;
}

// Set up initial configues to default or from command line
bool init_config(config_t *config, int argc, char **argv) {

    // Defaults
    *config = (config_t){
        .window_width = 64,  // CHIP8 X resolution
        .window_height = 32, // CHIP8 Y resoultions
        .bg_colour = 0xD291BC00,
        .fg_colour = 0x957DADFF,
        .scaler = 10,

    };

    (void)argc;
    (void)argv;

    return true;
}

// Machine initializer
bool init_chip8(chip8_t *chip8) {
    chip8->state = RUNNING;
    return true;
}

//====================== RUNTIME FUNCTIONS ======================//

// Handling Key presses
void handle_input(chip8_t *chip8) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        // Exiting Window
        case SDL_QUIT:
            // Kill Machine
            chip8->state = QUIT;
            return;
        case SDL_KEYDOWN:
            // Check for a specific key to exit (e.g., Escape key)
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                chip8->state = QUIT;
            }
            break;

        case SDL_KEYUP:

        default:
            // Invalid/unimplemented key
            break;
        }
    }
}

// Updates screen with updated values
void update_screen(sdl_t *sdl) { SDL_RenderPresent(sdl->rend); }

// Clears screen to background colour set in config
void clear_screen(sdl_t *sdl, config_t *config) {

    const uint8_t r = (config->bg_colour >> 24) & 0xFF;
    const uint8_t g = (config->bg_colour >> 16) & 0xFF;
    const uint8_t b = (config->bg_colour >> 8) & 0xFF;
    const uint8_t a = (config->bg_colour >> 0) & 0xFF;

    SDL_SetRenderDrawColor(sdl->rend, r, g, b, a);
    SDL_RenderClear(sdl->rend);
}

// Cleanup Function
void final_cleanup(sdl_t *sdl) {
    SDL_DestroyRenderer(sdl->rend);
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

//====================== MAIN ======================//

int main(int argc, char **argv) {

    // initialize configurations
    config_t config = {0};
    if (!init_config(&config, argc, argv))
        exit(EXIT_FAILURE);

    // Initialize SDL
    sdl_t sdl = {0};
    if (!init_sdl(&sdl, &config))
        exit(EXIT_FAILURE);

    // Initial clear screen
    clear_screen(&sdl, &config);

    // Initiazlie chip8 machine
    chip8_t chip8 = {0};
    if (!init_chip8(&chip8))
        exit(EXIT_FAILURE);

    // Runtime loop
    while (chip8.state != QUIT) {

        handle_input(&chip8);

        SDL_Delay(16);
        update_screen(&sdl);
    }

    final_cleanup(&sdl);
}