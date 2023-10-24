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
    uint16_t opcode; // 16 BIT (2 byte) instruction
    uint16_t NNN;    // 12 bit instruction/constant
    uint8_t NN;      // 8 bit constant
    uint8_t N;       // 4 bit constant
    uint8_t X;       // 4 bit register identifier
    uint8_t Y;       // 4 bit register identifier

} instruction_t;

// Chip8 Machine
typedef struct {
    emulator_state_t state;
    uint8_t ram[4096];
    bool display[64 * 32]; // Original Chip8 resolution
    uint8_t V[16];         // Registers V0 to VF
    uint16_t stack[12];    // Sub routine stack
    u_int16_t *stack_ptr;  // Stack pointer
    uint16_t I;            // Index register
    uint8_t delay_timer;   // Decrements at 60Hz when > 0
    uint8_t sound_timer;   // Decrements at 60hz and plays tone when > 0
    bool keypad[16];       // Hexadecimal Keypad
    uint16_t PC;           // Program Counter
    char *rom_name;        // Currently running ROM
    instruction_t inst;    // Current Instruction
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
        .bg_colour = 0xF5FFC900,
        .fg_colour = 0x804674FF,
        .scaler = 20,

    };

    (void)argc;
    (void)argv;

    return true;
}

// Machine initializer
bool init_chip8(chip8_t *chip8, char rom_name[]) {

    const uint16_t entry_point = 0x200; // Strating point for ROM to be loaded
    chip8->state = RUNNING;
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    // Loading Font into memory
    memcpy(&chip8->ram[0], font, sizeof(font)); // Loading font

    // Opening ROM file
    FILE *rom = fopen(rom_name, "rb");
    if (!rom) {
        SDL_Log("Rom file %s invalid or does not exits\n", rom_name);
        return false;
    }
    // Finding size of ROM
    fseek(rom, 0, SEEK_END);
    const size_t max_size = sizeof chip8->ram - entry_point;
    const size_t rom_size = ftell(rom);
    rewind(rom);

    // Checking for rom size
    if (rom_size > max_size) {

        SDL_Log("Rom file %zu is too big for ram %zu \n", rom_size, max_size);
        return false;
    }

    // Loading Chip8 memory
    if (fread(&chip8->ram[entry_point], rom_size, 1, rom) != 1) {
        SDL_Log("Could not read ROM onto memory\n");
        return false;
    }
    fclose(rom);

    // Initiating PC
    chip8->stack_ptr = &chip8->stack[0];
    chip8->PC = entry_point;
    chip8->rom_name = rom_name;
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
            switch (event.key.keysym.sym) {

            case (SDLK_ESCAPE):
                // Escape key
                chip8->state = QUIT;
                break;

            case (SDLK_SPACE):
                // Spacebar key
                if (chip8->state == RUNNING) {
                    chip8->state = PAUSED; // Pause chip8
                    puts("===== PAUSED =====");
                } else {
                    chip8->state = RUNNING; // Resuming operation
                    puts("===== RESUMED =====");
                }
            }
            break;

        case SDL_KEYUP:

        default:
            // Invalid/unimplemented key
            break;
        }
    }
}

// Only compile in Debug mode
#ifdef DEBUG
void debug_info(chip8_t *chip8) {
    printf("Address: 0x%04X, Opcode: 0x%04X Desc: ", chip8->PC - 2,
           chip8->inst.opcode);

    switch ((chip8->inst.opcode >> 12) & 0x0F) {
    case 0x00:
        if (chip8->inst.NN == 0xE0) {
            // 0x00E0: Clear the screen
            printf("Clear screen\n");

        } else if (chip8->inst.NN == 0xEE) {
            // 0x00EE: Return from subroutine
            // Set program counter to last address on subroutine stack ("pop" it
            // off the stack)
            //   so that next opcode will be gotten from that address.
            printf("Return from subroutine to address 0x%04X\n",
                   *(chip8->stack_ptr - 1));
        } else {
            printf("Unimplemented Opcode.\n");
        }
        break;

    case 0x01:
        // 0x1NNN: Jump to address NNN
        printf("Jump to address NNN (0x%04X)\n", chip8->inst.NNN);
        break;

    case 0x02:
        // 0x2NNN: Call subroutine at NNN
        // Store current address to return to on subroutine stack ("push" it on
        // the stack)
        //   and set program counter to subroutine address so that the next
        //   opcode is gotten from there.
        printf("Call subroutine at NNN (0x%04X)\n", chip8->inst.NNN);
        break;

    case 0x03:
        // 0x3XNN: Check if VX == NN, if so, skip the next instruction
        printf("Check if V%X (0x%02X) == NN (0x%02X), skip next instruction if "
               "true\n",
               chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.NN);
        break;

    case 0x04:
        // 0x4XNN: Check if VX != NN, if so, skip the next instruction
        printf("Check if V%X (0x%02X) != NN (0x%02X), skip next instruction if "
               "true\n",
               chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.NN);
        break;

    case 0x05:
        // 0x5XY0: Check if VX == VY, if so, skip the next instruction
        printf("Check if V%X (0x%02X) == V%X (0x%02X), skip next instruction "
               "if true\n",
               chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y,
               chip8->V[chip8->inst.Y]);
        break;

    case 0x06:
        // 0x6XNN: Set register VX to NN
        printf("Set register V%X = NN (0x%02X)\n", chip8->inst.X,
               chip8->inst.NN);
        break;

    case 0x07:
        // 0x7XNN: Set register VX += NN
        printf("Set register V%X (0x%02X) += NN (0x%02X). Result: 0x%02X\n",
               chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.NN,
               chip8->V[chip8->inst.X] + chip8->inst.NN);
        break;

    case 0x08:
        switch (chip8->inst.N) {
        case 0:
            // 0x8XY0: Set register VX = VY
            printf("Set register V%X = V%X (0x%02X)\n", chip8->inst.X,
                   chip8->inst.Y, chip8->V[chip8->inst.Y]);
            break;

        case 1:
            // 0x8XY1: Set register VX |= VY
            printf(
                "Set register V%X (0x%02X) |= V%X (0x%02X); Result: 0x%02X\n",
                chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y,
                chip8->V[chip8->inst.Y],
                chip8->V[chip8->inst.X] | chip8->V[chip8->inst.Y]);
            break;

        case 2:
            // 0x8XY2: Set register VX &= VY
            printf(
                "Set register V%X (0x%02X) &= V%X (0x%02X); Result: 0x%02X\n",
                chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y,
                chip8->V[chip8->inst.Y],
                chip8->V[chip8->inst.X] & chip8->V[chip8->inst.Y]);
            break;

        case 3:
            // 0x8XY3: Set register VX ^= VY
            printf(
                "Set register V%X (0x%02X) ^= V%X (0x%02X); Result: 0x%02X\n",
                chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y,
                chip8->V[chip8->inst.Y],
                chip8->V[chip8->inst.X] ^ chip8->V[chip8->inst.Y]);
            break;

        case 4:
            // 0x8XY4: Set register VX += VY, set VF to 1 if carry
            printf("Set register V%X (0x%02X) += V%X (0x%02X), VF = 1 if "
                   "carry; Result: 0x%02X, VF = %X\n",
                   chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y,
                   chip8->V[chip8->inst.Y],
                   chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y],
                   ((uint16_t)(chip8->V[chip8->inst.X] +
                               chip8->V[chip8->inst.Y]) > 255));
            break;

        case 5:
            // 0x8XY5: Set register VX -= VY, set VF to 1 if there is not a
            // borrow (result is positive/0)
            printf("Set register V%X (0x%02X) -= V%X (0x%02X), VF = 1 if no "
                   "borrow; Result: 0x%02X, VF = %X\n",
                   chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y,
                   chip8->V[chip8->inst.Y],
                   chip8->V[chip8->inst.X] - chip8->V[chip8->inst.Y],
                   (chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]));
            break;

        case 6:
            // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
            printf("Set register V%X (0x%02X) >>= 1, VF = shifted off bit "
                   "(%X); Result: 0x%02X\n",
                   chip8->inst.X, chip8->V[chip8->inst.X],
                   chip8->V[chip8->inst.X] & 1, chip8->V[chip8->inst.X] >> 1);
            break;

        case 7:
            // 0x8XY7: Set register VX = VY - VX, set VF to 1 if there is not a
            // borrow (result is positive/0)
            printf("Set register V%X = V%X (0x%02X) - V%X (0x%02X), VF = 1 if "
                   "no borrow; Result: 0x%02X, VF = %X\n",
                   chip8->inst.X, chip8->inst.Y, chip8->V[chip8->inst.Y],
                   chip8->inst.X, chip8->V[chip8->inst.X],
                   chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X],
                   (chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]));
            break;

        case 0xE:
            // 0x8XYE: Set register VX <<= 1, store shifted off bit in VF
            printf("Set register V%X (0x%02X) <<= 1, VF = shifted off bit "
                   "(%X); Result: 0x%02X\n",
                   chip8->inst.X, chip8->V[chip8->inst.X],
                   (chip8->V[chip8->inst.X] & 0x80) >> 7,
                   chip8->V[chip8->inst.X] << 1);
            break;

        default:
            // Wrong/unimplemented opcode
            break;
        }
        break;

    default:
        printf("Unimplemented Opcode.\n");
        break; // Unimplemented or invalid opcode

    case 0x0A:
        // 0xANNN: Set index register I to NNN
        printf("Set I to NNN (0x%04X)\n", chip8->inst.NNN);
        break;

    case 0x0D:
        // 0xDXYN: Draw N-height sprite at coords X,Y; Read from memory location
        // I;
        //   Screen pixels are XOR'd with sprite bits,
        //   VF (Carry flag) is set if any screen pixels are set off; This is
        //   useful for collision detection or other reasons.
        printf("Draw N (%u) height sprite at coords V%X (0x%02X), V%X (0x%02X) "
               "from memory location I (0x%04X). Set VF = 1 if any pixels are "
               "turned off.\n",
               chip8->inst.N, chip8->inst.X, chip8->V[chip8->inst.X],
               chip8->inst.Y, chip8->V[chip8->inst.Y], chip8->I);
        break;
    }
}
#endif

void emulate_instruct(chip8_t *chip8, config_t *config) {

    // Grabbing opcode from ram
    chip8->inst.opcode =
        (chip8->ram[chip8->PC] << 8) | (chip8->ram[chip8->PC + 1]);
    chip8->PC += 2; // incrementing PC

    // Filling instruction format
    // Filling instruction format
    chip8->inst.NNN = chip8->inst.opcode & 0x0FFF;
    chip8->inst.NN = chip8->inst.opcode & 0x00FF; // Corrected line
    chip8->inst.N = chip8->inst.opcode & 0x000F;

    chip8->inst.X = (chip8->inst.opcode >> 8) & 0x0F;
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0F;

// Only called in debug mode
#ifdef DEBUG
    debug_info(chip8);
#endif

    switch ((chip8->inst.opcode >> 12) & 0x0F) {
    case 0x0:

        if (chip8->inst.NN == 0xE0) {
            // 0x00E0 : Clears the screen
            memset(&chip8->display[0], false, sizeof chip8->display);
        } else if (chip8->inst.NN == 0xEE) {
            // 0x00EE : Returns from a subroutine
            chip8->PC = *--chip8->stack_ptr;
        }

        break;

    case 0x01:
        // 0x1NNN : Jumps to address NNN.
        chip8->PC = chip8->inst.NNN;
        break;

    case 0x02:
        // 0x2NNN : Calls subroutine at NNN
        *chip8->stack_ptr++ = chip8->PC; // Storing current address and push ptr
        chip8->PC = chip8->inst.NNN;     // Jump to NNN
        break;

    case 0x03:
        // 0x3XNN : Skips the next instruction if VX equals NN
        if (chip8->V[chip8->inst.X] == chip8->inst.NN)
            chip8->PC += 2; // Skipping next instruction

        break;

    case 0x04:
        // 0x4XNN : Skips the next instruction if VX does not equal NN
        if (chip8->V[chip8->inst.X] != chip8->inst.NN)
            chip8->PC += 2; // Skipping next instruction
        break;

    case 0x05:
        // 0x5XY0 : Skips the next instruction if VX equals VY
        if (chip8->V[chip8->inst.X] == chip8->V[chip8->inst.Y])
            chip8->PC += 2; // Skipping next instruction
        break;

    case 0x06:
        // 0x6XNN : Sets VX to NN
        chip8->V[chip8->inst.X] = chip8->inst.NN;
        break;

    case 0x07:
        // 0x7XNN : Adds NN to VX (carry flag is not changed).
        chip8->V[chip8->inst.X] += chip8->inst.NN;
        break;

    case 0x08:
        // ALU Operations
        switch (chip8->inst.N) {

        case 0:
            // 0x8XY0 : Sets VX to the value of VY
            chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y];
            break;

        case 1:
            // 0x8XY1 : Sets VX to VX or VY.
            chip8->V[chip8->inst.X] |= chip8->V[chip8->inst.Y];
            break;

        case 2:
            // 0x8XY1: Sets VX to VX and VY
            chip8->V[chip8->inst.X] &= chip8->V[chip8->inst.Y];
            break;

        case 3:
            // 0x8XY3 : Sets VX to VX xor VY.
            chip8->V[chip8->inst.X] ^= chip8->V[chip8->inst.Y];
            break;

        case 4:
            // 0x8XY4 : Adds VY to VX. VF is set to 1 when there's a carry, and
            // to 0 when there is not.
            chip8->V[0x0F] = ((uint16_t)(chip8->V[chip8->inst.X] +
                                         chip8->V[chip8->inst.Y]) > 255);
            chip8->V[chip8->inst.X] += chip8->V[chip8->inst.Y];
            break;

        case 5:
            // 0x8XY5 : VY is subtracted from VX. VF is set to 0 when there's a
            // borrow, and 1 when there is not.
            chip8->V[0x0F] =
                (chip8->V[chip8->inst.X] >= chip8->V[chip8->inst.Y]);
            chip8->V[chip8->inst.X] -= chip8->V[chip8->inst.Y];
            break;

        case 6:
            // 0x8XY6 : Stores the least significant bit of VX in VF and then
            // shifts VX to the right by 1.

            // TODO : Might have to do jank according to wiki note
            chip8->V[0x0F] = (chip8->V[chip8->inst.X] & 1);
            chip8->V[chip8->inst.X] >>= 1;
            break;

        case 7:
            // c0x8XY7 : Sets VX to VY minus VX. VF is set to 0 when there's a
            // borrow, and 1 when there is not.
            chip8->V[0x0F] =
                (chip8->V[chip8->inst.Y] >= chip8->V[chip8->inst.X]);
            chip8->V[chip8->inst.X] =
                chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X];
            break;

        case 0x0E:
            // 0x8XYE : Stores the most significant bit of VX in VF and then
            // shifts VX to the left by 1.
            chip8->V[0x0F] = ((chip8->V[chip8->inst.X] & 0x80) >> 7) & 1;
            chip8->V[chip8->inst.X] <<= 1;
            break;

        default:
            break;
        }
        break;

    case 0x0A:
        // 0xANNN : Sets I to the address NNN
        chip8->I = chip8->inst.NNN;
        break;

    case 0x0D:
        // 0xDXYN: Draw N-height sprite at coords X,Y; Read from memory
        // location I;
        //   Screen pixels are XOR'd with sprite bits,
        //   VF (Carry flag) is set if any screen pixels are set off; This
        //   is useful for collision detection or other reasons.
        uint8_t X_coord = chip8->V[chip8->inst.X] % config->window_width;
        uint8_t Y_coord = chip8->V[chip8->inst.Y] % config->window_height;
        const uint8_t orig_X = X_coord; // Original X value

        chip8->V[0xF] = 0; // Initialize carry flag to 0

        // Loop over all N rows of the sprite
        for (uint8_t i = 0; i < chip8->inst.N; i++) {
            // Get next byte/row of sprite data
            const uint8_t sprite_data = chip8->ram[chip8->I + i];
            X_coord = orig_X; // Reset X for the next row to draw

            for (int8_t j = 7; j >= 0; j--) {
                // If sprite pixel/bit is on and display pixel is on, set
                // carry flag
                bool *pixel =
                    &chip8->display[Y_coord * config->window_width + X_coord];
                const bool sprite_bit = (sprite_data & (1 << j));

                if (sprite_bit && *pixel) {
                    chip8->V[0xF] = 1;
                }

                // XOR display pixel with sprite pixel/bit to set it on or
                // off
                *pixel ^= sprite_bit;

                // Move to the next column (pixel)
                X_coord = (X_coord + 1) % config->window_width;
            }

            // Move to the next row (Y-coordinate)
            Y_coord = (Y_coord + 1) % config->window_height;
        }
        break;

    default:
        break;
    }
}

void update_screen(const sdl_t *sdl, const config_t *config, chip8_t *chip8) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = config->scaler, .h = config->scaler};

    // Getting Background and Foreground colours
    const uint8_t bg_r = (config->bg_colour >> 24) & 0xFF;
    const uint8_t fg_r = (config->fg_colour >> 24) & 0xFF;
    const uint8_t bg_g = (config->bg_colour >> 16) & 0xFF;
    const uint8_t fg_g = (config->fg_colour >> 16) & 0xFF;
    const uint8_t bg_b = (config->bg_colour >> 8) & 0xFF;
    const uint8_t fg_b = (config->fg_colour >> 8) & 0xFF;
    const uint8_t bg_a = (config->bg_colour >> 0) & 0xFF;
    const uint8_t fg_a = (config->fg_colour >> 0) & 0xFF;

    // Loop through display pixel, draw a reactagle per pixel
    for (uint32_t i = 0; i < sizeof chip8->display; i++) {

        // Translating 1D index i value to 2D X/Y coordinate
        rect.x = i % (config->window_width) * config->scaler;
        rect.y = i / (config->window_width) * config->scaler;

        if (chip8->display[i]) {
            SDL_SetRenderDrawColor(sdl->rend, fg_r, fg_g, fg_b, fg_a);
            SDL_RenderFillRect(sdl->rend, &rect);

            // if (config.pixel_outlines) {
            // If user requested drawing pixel outlines, draw those here
            SDL_SetRenderDrawColor(sdl->rend, bg_r, bg_g, bg_b, bg_a);
            SDL_RenderDrawRect(sdl->rend, &rect);

        } else {
            SDL_SetRenderDrawColor(sdl->rend, bg_r, bg_g, bg_b, bg_a);
            SDL_RenderFillRect(sdl->rend, &rect);
        }
    }
    SDL_RenderPresent(sdl->rend);
}

// Clears screen to background colour set in config
void clear_screen(const sdl_t *sdl, const config_t *config) {
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

    // Default startup message
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom_name> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

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
    char *rom_name = argv[1];
    chip8_t chip8 = {0};
    if (!init_chip8(&chip8, rom_name))
        exit(EXIT_FAILURE);

    // Runtime loop
    while (chip8.state != QUIT) {

        handle_input(&chip8);

        // Pause for debugging
        if (chip8.state == PAUSED)
            continue;

        emulate_instruct(&chip8, &config);
        // Chip8 60 Hz refresh
        SDL_Delay(16);
        update_screen(&sdl, &config, &chip8);
    }

    final_cleanup(&sdl);
}