#include <iostream>

#include "chip8.h" 

int main()
{
    // Set up render system and register input callbacks
    // setupGraphics();
    // setupInput();

    // Initialize the Chip8 system and load the game into the memory
    chip8::initialize();
    chip8::load("C:\\Users\\tiago.ferreira\\Downloads\\Pong.ch8");

    // // Emulation loop
    // while(true)
    // {
    //     // Emulate one cycle
    //     chip8::emulateCycle();

    //     // If the draw flag is set, update the screen
    //     // if(myChip8.drawFlag)
    //     // {
    //     //     //drawGraphics();
    //     // }

    //     // Story key press state (Press and Release)
    //     // myChip8.setKeys();
    // }

    return 0;
}