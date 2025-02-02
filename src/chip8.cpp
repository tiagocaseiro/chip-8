#include "chip8.h"

#include <ios>
#include <iostream>
#include <functional>
#include <algorithm>
#include <fstream>

namespace chip8
{
    namespace ranges = std::ranges;

    using opcode_t = unsigned short;
    using register_t = unsigned char;

    static opcode_t opcode;

    static unsigned char memory[4096];

    static register_t V[16];

    static unsigned short I;
    static unsigned short pc;

    static unsigned char gfx [64 * 32];

    static unsigned char delay_timer;
    static unsigned char sound_timer;

    static unsigned short stack[16];
    static unsigned short sp;

    static unsigned char key[16];

    static constexpr auto PROGRAM_OFFSET = 0x200;

    static void func0(opcode_t)
    {

    }

    static void func1(opcode_t)
    {

    }

    static void func2(opcode_t)
    {

    }

    static void func3(opcode_t)
    {

    }

    static void func4(opcode_t)
    {

    }

    static void func5(opcode_t)
    {

    }

    static void func6(opcode_t)
    {

    }

    static void func7(opcode_t)
    {

    }

    static void func8(opcode_t)
    {

    }
    
    static void func9(opcode_t)
    {

    }

    static void funcA(opcode_t)
    {

    }

    static void funcB(opcode_t)
    {

    }

    static void funcC(opcode_t)
    {

    }

    static void funcD(opcode_t)
    {

    }

    static void funcE(opcode_t)
    {

    }

    static void funcF(opcode_t)
    {

    }

    static std::function<void(opcode_t)> funcs[] = { 
                                       func0, func1, func2, func3, 
                                       func4, func5, func6, func7, 
                                       func8, func9, funcA, funcB, 
                                       funcC, funcD, funcE, funcF
                                    };

    void clear_buffer(auto& buffer)
    {
        ranges::fill(buffer, 0);
    }

    void initialize()
    {   
        pc     = PROGRAM_OFFSET; // Program counter starts at 0x200
        opcode = 0;              // Reset current opcode
        I      = 0;              // Reset index register
        sp     = 0;              // Reset stack pointer

        // Clear display
        clear_buffer(gfx);
        // Clear stack
        clear_buffer(stack);
        // Clear registers V0-VF
        clear_buffer(V);
        // Clear memory
        clear_buffer(memory);

        for(int i = 0; i < 80; i++)
        {
            // memory[i] = chip8_fontset[i];
        }

        // Reset timers
        delay_timer = 0;
        sound_timer = 0;
    }

    void update()
    {
        // Fetch Opcode
        auto first_half_opcode = memory[pc];
        auto second_half_opcode = memory[pc+1];

        opcode_t opcode = first_half_opcode << 8 | second_half_opcode;

        auto f_index = first_half_opcode >> 4;
        funcs[f_index](opcode);

        // Update timers
        if (delay_timer > 0)
        {
            delay_timer--;
        }

        if (sound_timer > 0)
        {
            if (sound_timer == 1)
            {
                std::cout << "Beep" << std::endl;
            }
            sound_timer--;
        }
    }

    void load(const char* program)
    {
        // Load program into memory
        std::ifstream is(program, std::ios::binary);
        auto begin = &memory[0];
        begin+=PROGRAM_OFFSET;
        std::streamsize count = sizeof(memory) - PROGRAM_OFFSET;
        is.read(reinterpret_cast<char*>(begin), count);
    }
}