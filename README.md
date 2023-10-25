
# Chip8 Emulator

## Overview

This is a simple Chip8 emulator implemented in C using the SDL library for graphics. Chip8 is a minimalist interpreted programming language used in the development of games for various retro computer systems. This emulator allows you to play classic Chip8 games on your computer.

## What is Chip8?

Chip8 is an interpreted programming language that was originally designed to create games for the COSMAC VIP and Telmac 1800 microcomputers. It uses a virtual machine and provides a set of opcodes that allow developers to create simple 2D games and applications. Chip8 programs are stored in a binary format and run on a virtual machine.

## Why Use a Chip8 Emulator?

The Chip8 system has a cult following among retro gaming enthusiasts. It's a simple yet fun way to experience the early days of computer gaming. Using this emulator, you can play classic games like Pong, Space Invaders, and Tetris, which were originally developed for the Chip8 platform.

## Acknowledgments

This emulator is a project created for educational purposes and serves as a simplified implementation of a Chip8 emulator. It may not support all ROMs or features found in more advanced emulators. Feel free to use, modify, or expand upon the code for your own projects.

The ROMS and test ROMS used in this repository is taken from :
        - https://github.com/Timendus/chip8-test-suite
        - https://github.com/cj1128/chip8-emulator/tree/master
        
## Implementation

This emulator is implemented in C, leveraging the SDL (Simple DirectMedia Layer) library for graphics and input handling. The project structure consists of the following key components:

- `chip8.c`: The core emulator code.
- `ROMS`: A directory for storing Chip8 ROMs. The roms included in this are taken from various sources and are not written by me
- `tests`: This folder contain test ROMS to check functionaing of codes


The emulator uses the Chip8 virtual machine to interpret and execute Chip8 programs. It emulates the Chip8 CPU, memory, display, and input systems. It renders graphics and handles user input through SDL.

- ** ROMS ** - The roms included in this are taken from various and are not written by me
  
## Features

- **Classic Chip8 Support**: The emulator is designed to run classic Chip8 games, preserving the authentic gaming experience.

- **Customizable Configuration**: You can configure various aspects of the emulator, such as window dimensions, colors, scaling, and clock speed, by modifying the `config_t` struct in the `main.c` file.

- **Keyboard Input**: The emulator maps Chip8 keypad keys to your computer's keyboard. You can customize key mappings as needed in the `handle_input` function in the `main.c` file.

## Getting Started

- Pull the repo to your local directory and run `$ make` in the directory to create the executable
- run `$ ./chip8.c ROM/<name of rom>` to get started
-  **Hint** - Tetris uses W and E for left and righ moevement and Q rotate

-  

