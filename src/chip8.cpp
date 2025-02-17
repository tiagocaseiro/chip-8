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

    static register_t& flag_register = V[15];

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
    
    static inline void clear_buffer(auto& buffer)
    {
        unsigned char zero = 0;
        ranges::fill(buffer, zero);
    }

    static inline unsigned short get_memory_address_from_opcode(const opcode_t opcode)
    {
        return static_cast<unsigned short>(opcode & 0x0FFF);
    }
   
    static inline register_t& get_first_register_from_opcode(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        auto index = opcode & 0x0F00;
        index >>= 8;

        std::cout << "index " << index << std::endl;

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
    
    register_t get_value_from_opcode(const opcode_t opcode)
    {
        return static_cast<register_t>(opcode & 0x00FF);
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
        std::cout << __FUNCTION__ << std::endl;

        sp--; // Go back in the stack to previous valid entry
        if (sp < &stack[0])
        {
            std::out_of_range("Stack pointer decremented to outside the range of the stack");
        }
        pc = *sp; // Point pc to saved memory address
    }
    
    static void clear_screen_and_return(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        switch (opcode)
        {
        case 0xE0: // Clear screen
            clear_buffer(gfx);
            return;
        case 0xEE: // Return from subroutine
            return_from_subroutine();
            return;
        }
    }

    static void jump_to(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        pc = get_memory_address_from_opcode(opcode); 
    }

    static void call_func(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

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
        std::cout << __FUNCTION__ << std::endl;

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
        std::cout << __FUNCTION__ << std::endl;

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
        std::cout << __FUNCTION__ << std::endl;

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
        std::cout << __FUNCTION__  << 1 << std::endl;

        auto& reg =  get_first_register_from_opcode(opcode); // Extract register index
        auto new_value  = get_value_from_opcode(opcode); // Extract value

        reg = new_value;
        std::cout << __FUNCTION__  << 2 << std::endl;

        next_instruction();
    }

    static void add_assign_register_to_value(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        auto& reg = get_first_register_from_opcode(opcode);
        
        reg+= get_value_from_opcode(opcode);

        next_instruction();
    }

    static void assign_to_register(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        switch(opcode & 0x000F)
        {
            case 0x0:
            {
                register_t& register_1       = get_first_register_from_opcode(opcode);
                const register_t& register_2 = get_second_register_from_opcode(opcode);
                register_1 = register_2;
                return;
            }
            case 0x1:
            {
                register_t& register_1       = get_first_register_from_opcode(opcode);
                const register_t& register_2 = get_second_register_from_opcode(opcode);
                register_1 |= register_2;
                return;
            }
            case 0x2:
            {
                register_t& register_1       = get_first_register_from_opcode(opcode);
                const register_t& register_2 = get_second_register_from_opcode(opcode);
                register_1 &= register_2;
                return;
            }
            case 0x3:
            {
                register_t& register_1       = get_first_register_from_opcode(opcode);
                const register_t& register_2 = get_second_register_from_opcode(opcode);
                register_1 ^= register_2;
                return;
            }
            case 0x4:
            {
                register_t& register_1       = get_first_register_from_opcode(opcode);
                const register_t& register_2 = get_second_register_from_opcode(opcode);
                int temp = register_1;
                temp+= register_2;
                register_1 = static_cast<register_t>(temp);

                // Check if digits passed char size were flipped
                auto is_overflow = temp >> 8 != 0;
                if (is_overflow) 
                {
                    flag_register = 1;
                    return;                
                }
                flag_register = 0;
                return;      
            }
            case 0x5:
            {
                register_t& register_1       = get_first_register_from_opcode(opcode);
                const register_t& register_2 = get_second_register_from_opcode(opcode);
                auto is_underflow = register_2 > register_1;
                register_1 -= register_2;

                // Check if digits passed char size were flipped
                if (is_underflow) 
                {
                    flag_register = 0;
                    return;                
                }
                flag_register = 1;
                return;      
            }
            case 0x6:
            {
                throw std::invalid_argument("Unsupported operation");
            }
            case 0x7:
            {
                throw std::invalid_argument("Unsupported operation");
            }
            case 0xE:
            {
                throw std::invalid_argument("Unsupported operation");
            }        
        }

        return;
    }
    
    static void func9(const opcode_t)
    {
        throw std::invalid_argument("Unsupported operation");
    }

    static void funcA(const opcode_t)
    {
        throw std::invalid_argument("Unsupported operation");
    }

    static void funcB(const opcode_t)
    {
        throw std::invalid_argument("Unsupported operation");
    }

    static void funcC(const opcode_t)
    {
        throw std::invalid_argument("Unsupported operation");
    }

    static void funcD(const opcode_t)
    {
        throw std::invalid_argument("Unsupported operation");
    }

    static void funcE(const opcode_t)
    {
        throw std::invalid_argument("Unsupported operation");
    }

    static void funcF(const opcode_t)
    {
        throw std::invalid_argument("Unsupported operation");
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

        opcode_t opcode = static_cast<opcode_t>(first_half_opcode << 8 | second_half_opcode);
        
        std::cout << __FUNCTION__ << 1 << " "<< std::hex << opcode << std::endl;

        auto f_index = first_half_opcode >> 4;
        funcs[f_index](opcode);
 
        std::cout << __FUNCTION__ << 2 << " "<< std::hex << opcode << std::endl;
        
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