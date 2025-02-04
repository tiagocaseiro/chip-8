#include "chip8.h"

#include <ios>
#include <iostream>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace chip8
{
    namespace ranges = std::ranges;

    using opcode_t      = unsigned short;
    using register_t    = unsigned char;
    using stack_entry_t = unsigned short;

    static opcode_t opcode;

    static unsigned char memory[4096];

    static register_t V[16];

    static unsigned short I;
    static unsigned short pc;

    static unsigned char gfx [64 * 32];

    static unsigned char delay_timer;
    static unsigned char sound_timer;

    static constexpr auto STACK_SIZE = 16;

    static stack_entry_t stack[STACK_SIZE];
    static stack_entry_t* sp = nullptr;

    static unsigned char key[16];

    static constexpr auto PROGRAM_OFFSET = 0x200;
    
    static void clear_buffer(auto& buffer)
    {
        ranges::fill(buffer, 0);
    }

    static void clear_screen_and_return(opcode_t opcode)
    {
        switch (opcode)
        {
        case 0x00E0: // Clear screen
            clear_buffer(gfx);
            return;
        case 0x00EE: // Return from subroutine
            sp--; // Go back in the stack to previous valid entry
            if (sp < &stack[0])
            {
                std::out_of_range("Stack pointer decremented to outside the range of the stack");
            }
            pc = *sp; // Point pc to saved memory address
            return;
        }
    }

    static void jump_to(opcode_t opcode)
    {
        auto memory_address = opcode & 0x0FFF; // Extract memory address from opcode
        pc = memory_address; // Point pc to new memory address
    }

    static void call_func(opcode_t)
    {
        auto memory_address = opcode & 0x0FFF; // Extract memory address from opcode

        *sp = pc; // Assign current sp to current pc address
        sp++; // Go up next available entry stack
        if (sp > &stack[STACK_SIZE-1])
        {
            std::out_of_range("Stack pointer incremented to outside the range of the stack");
        }
        pc = memory_address; // Have pc point to new memory address
    }

    static void jump_if_equal(opcode_t opcode)
    {
        auto register_index = (opcode & 0x0F00) >> 8; // Extract register index
        auto value          = opcode & 0x00FF; // Extract value

        auto register_value = V[register_index];

        if (register_value != value)
        {
            return;
        }

        pc+=2; // Jump a whole instruction
    }

    static void jump_if_not_equal(opcode_t opcode)
    {
        auto register_index = (opcode & 0x0F00) >> 8; // Extract register index
        auto value          = opcode & 0x00FF; // Extract value

        auto register_value = V[register_index];

        if (register_value == value)
        {
            return;
        }

        pc+=2; // Jump a whole instruction
    }

    static void jump_if_registers_equal(opcode_t opcode)
    {
        auto register_index_1 = (opcode & 0x0F00) >> 8; // Extract register index 1
        auto register_index_2 = (opcode & 0x00F0) >> 4; // Extract register index 2

        auto register_value_1 = V[register_index_1];
        auto register_value_2 = V[register_index_2];

        if (register_value_1 != register_value_2)
        {
            return;
        }

        pc+=2; // Jump a whole instruction
    }

    static void set_register_to_value(opcode_t opcode)
    {
        auto register_index = (opcode & 0x0F00) >> 8; // Extract register index
        auto new_register_value  = (opcode & 0x00FF); // Extract value

        V[register_index] = new_register_value;
    }

    static void func7(opcode_t)
    {
        return;
    }

    static void func8(opcode_t)
    {
        return;
    }
    
    static void func9(opcode_t)
    {
        return;
    }

    static void funcA(opcode_t)
    {
        return;
    }

    static void funcB(opcode_t)
    {
        return;
    }

    static void funcC(opcode_t)
    {
        return;
    }

    static void funcD(opcode_t)
    {
        return;
    }

    static void funcE(opcode_t)
    {
        return;
    }

    static void funcF(opcode_t)
    {
        return;
    }

    static std::function<void(opcode_t)> funcs[] = { 
                                       clear_screen_and_return, jump_to, call_func, jump_if_equal, 
                                       jump_if_not_equal, jump_if_registers_equal, set_register_to_value, func7, 
                                       func8, func9, funcA, funcB, 
                                       funcC, funcD, funcE, funcF
                                    };

    void initialize()
    {   
        pc     = PROGRAM_OFFSET; // Program counter starts at 0x200
        opcode = 0;              // Reset current opcode
        I      = 0;              // Reset index register
        sp     = &stack[0];      // Reset stack pointer

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
        std::streamsize max_count = sizeof(memory) - PROGRAM_OFFSET;
        is.read(reinterpret_cast<char*>(begin), max_count);
    }
}