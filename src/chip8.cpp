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

    static unsigned char memory[4096];

    static register_t V[16];

    static unsigned short I;
    static unsigned short pc;

    static draw_buffer gfx;

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

    int get_memory_address_from_opcode(const opcode_t opcode)
    {
        return opcode & 0x0FFF;
    }
   
    register_t& get_first_register_from_opcode(const opcode_t opcode)
    {
        auto index = opcode & 0x0F00;
        index >>= 8;
        return V[index];
    }

    register_t& get_second_register_from_opcode(const opcode_t opcode)
    {
        auto index = opcode & 0x00F0;
        index >>= 4;
        return V[index];
    }

    int get_second_register_index_from_opcode(const opcode_t opcode)
    {
        auto second_register = opcode & 0x00F0;
        second_register = second_register >> 4;
        return second_register;
    }
    
    int get_value_from_opcode(const opcode_t opcode)
    {
        return opcode & 0x00FF;
    }

    void next_instruction()
    {
        pc+=2;
    }

    void jump_next_instruction()
    {
        pc+=4;
    }

    static void return_from_subroutine()
    {
        sp--; // Go back in the stack to previous valid entry
        if (sp < &stack[0])
        {
            std::out_of_range("Stack pointer decremented to outside the range of the stack");
        }
        pc = *sp; // Point pc to saved memory address
    }
    
    static void clear_screen_and_return(const opcode_t opcode)
    {
        switch (opcode)
        {
        case 0x00E0: // Clear screen
            clear_buffer(gfx);
            return;
        case 0x00EE: // Return from subroutine
            return_from_subroutine();
            return;
        }
    }

    static void jump_to(const opcode_t opcode)
    {
        pc = get_memory_address_from_opcode(opcode); 
    }

    static void return_from_subroutine(const opcode_t opcode)
    {
        sp--; // Go back in the stack to previous valid entry
        if (sp < &stack[0])
        {
            std::out_of_range("Stack pointer decremented to outside the range of the stack");
        }
        pc = *sp; // Point pc to saved memory address
    }
    
    static void call_func(const opcode_t opcode)
    {
        auto memory_address = get_memory_address_from_opcode(opcode); // Extract memory address from opcode

        *sp = pc; // Assign current sp to current pc address
        sp++; // Go up next available entry stack
        if (sp > &stack[STACK_SIZE-1])
        {
            std::out_of_range("Stack pointer incremented to outside the range of the stack");
        }
        pc = memory_address; // Have pc point to new memory address
    }

    static void jump_if_equal(const opcode_t opcode)
    {
        auto& reg = get_first_register_from_opcode(opcode); // Extract register index
        auto value          = get_value_from_opcode(opcode); // Extract value

        if (reg != value)
        {
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void jump_if_not_equal(const opcode_t opcode)
    {
        auto& reg   = get_first_register_from_opcode(opcode); // Extract register index
        auto value  = get_value_from_opcode(opcode); // Extract value

        if (reg == value)
        {
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void jump_if_registers_equal(const opcode_t opcode)
    {
        auto& register_1 = get_first_register_from_opcode(opcode); // Extract register index 1
        auto& register_2 = get_second_register_from_opcode(opcode); // Extract register index 2

        if (register_1 != register_2)
        {
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void set_register_to_value(const opcode_t opcode)
    {
        auto& reg =  get_first_register_from_opcode(opcode); // Extract register index
        auto new_value  = get_value_from_opcode(opcode); // Extract value

        reg = new_value;

        next_instruction();
    }

    static void add_assign_register_to_value(const opcode_t opcode)
    {
        auto& reg = get_first_register_from_opcode(opcode);
        
        reg+= get_value_from_opcode(opcode);

        next_instruction();
    }

    static void assign_to_register(const opcode_t opcode)
    {
        auto& register_1 = get_first_register_from_opcode(opcode);
        auto& register_2 = get_second_register_from_opcode(opcode);

        switch(opcode & 0x000F)
        {
            case 0:
                register_1 = register_2;
                return;
            case 1:
                register_1 |= register_2;
                return;
            case 2:
                register_1 &= register_2;
                return;
            case 3:
                register_1 ^= register_2;
                return;
        }

        return;
    }
    
    static void func9(const opcode_t)
    {
        return;
    }

    static void funcA(const opcode_t)
    {
        return;
    }

    static void funcB(const opcode_t)
    {
        return;
    }

    static void funcC(const opcode_t)
    {
        return;
    }

    static void funcD(const opcode_t)
    {
        return;
    }

    static void funcE(const opcode_t)
    {
        return;
    }

    static void funcF(const opcode_t)
    {
        return;
    }

    static std::function<void(const opcode_t)> funcs[] = { 
                                       clear_screen_and_return, jump_to, call_func, jump_if_equal, 
                                       jump_if_not_equal, jump_if_registers_equal, set_register_to_value, add_assign_register_to_value, 
                                       assign_to_register, func9, funcA, funcB, 
                                       funcC, funcD, funcE, funcF
                                    };

    void init()
    {   
        pc     = PROGRAM_OFFSET; // Program counter starts at 0x200
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
 
        std::cout << std::hex << opcode << std::endl;
        
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

    void load(const char* path)
    {
        // Load program into memory
        std::ifstream is(path, std::ios::binary);
        auto begin = &memory[0];
        begin+=PROGRAM_OFFSET;
        std::streamsize max_count = sizeof(memory) - PROGRAM_OFFSET;
        is.read(reinterpret_cast<char*>(begin), max_count);
    }

    const draw_buffer& gfx_buffer()
    {
        return gfx;
    }
}