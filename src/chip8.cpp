#include "chip8.h"

#include <ios>
#include <iostream>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>

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
        static constexpr unsigned char ZERO = 0;
        ranges::fill(buffer, ZERO);
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

    register_t get_value_from_opcode_nn(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;
        register_t value = static_cast<register_t>(opcode & 0x00FF);
        std::cout << "Value from opcode " << static_cast<int>(value) << std::endl;
        return value;
    }

    register_t get_value_from_opcode_n(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;
        register_t value = static_cast<register_t>(opcode & 0x000F);
        std::cout << "Value from opcode " << static_cast<int>(value) << std::endl;
        return value;
    }

    unsigned short get_value_from_opcode_nnn(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        unsigned short value = static_cast<unsigned short>(opcode & 0x0FFF);
        std::cout << "Value from opcode " << static_cast<int>(value) << std::endl;
        return value;
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

        const auto reg   = get_first_register_from_opcode(opcode); // Extract register index
        const auto value = get_value_from_opcode_nn(opcode); // Extract value

        if (reg != value)
        {
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void jump_if_not_equal(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const auto reg   = get_first_register_from_opcode(opcode); // Extract register index
        const auto value = get_value_from_opcode_nn(opcode); // Extract value

        if (reg == value)
        {
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void jump_if_registers_equal(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const auto register_1 = get_first_register_from_opcode(opcode); // Extract register index 1
        const auto register_2 = get_second_register_from_opcode(opcode); // Extract register index 2

        if (register_1 != register_2)
        {
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void set_register_to_value(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        register_t& reg =  get_first_register_from_opcode(opcode); // Extract register index
        register_t new_value  = get_value_from_opcode_nn(opcode); // Extract value

        reg = new_value;

        next_instruction();
    }

    static void add_assign_register_to_value(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        auto& reg = get_first_register_from_opcode(opcode);
        
        reg+= get_value_from_opcode_nn(opcode);

        next_instruction();
    }

    static void assign_to_register(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        switch(opcode & 0x000F)
        {
            case 0x0:
            {
                register_t& register_1      = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                register_1 = register_2;
                return;
            }
            case 0x1:
            {
                register_t& register_1      = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                register_1 |= register_2;
                return;
            }
            case 0x2:
            {
                register_t& register_1      = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                register_1 &= register_2;
                return;
            }
            case 0x3:
            {
                register_t& register_1       = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                register_1 ^= register_2;
                return;
            }
            case 0x4:
            {
                register_t& register_1       = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
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
                const register_t register_2 = get_second_register_from_opcode(opcode);
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
    
    static void jump_if_registers_not_equal(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const auto register_1 = get_first_register_from_opcode(opcode); // Extract register index 1
        const auto register_2 = get_second_register_from_opcode(opcode); // Extract register index 2

        if (register_1 == register_2)
        {
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void assign_address_register(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;
        I = get_memory_address_from_opcode(opcode);
        next_instruction();
    }

    static void jump_to_address(const opcode_t opcode)
    {
        const auto register_0 = static_cast<unsigned short>(V[0]);
        const auto value = get_value_from_opcode_nnn(opcode);
        pc += register_0;
        pc += value;
    }

    static void set_register_to_bitwise_and_of_random(const opcode_t opcode)
    {
        static std::random_device rd;
        static std::uniform_int_distribution<int> dist(0, 255);

        register_t& reg   = get_first_register_from_opcode(opcode);
        register_t value = get_value_from_opcode_nn(opcode); 

        register_t rand = static_cast<register_t>(dist(rd));

        reg = static_cast<register_t>(rand & value);
    }

    static void draw_sprite(const opcode_t /*opcode*/)
    {
        std::cout << __FUNCTION__ << std::endl;
        // const register_t x      = get_first_register_from_opcode(opcode);
        // const register_t y      = get_second_register_from_opcode(opcode);
        // const register_t height = get_value_from_opcode_n(opcode);
        throw std::invalid_argument("Unsupported operation");
    }

    static void skip_by_key(const opcode_t /*opcode*/)
    {
        throw std::invalid_argument("Unsupported operation");
    }

    static void funcF(const opcode_t /*opcode*/)
    {
        throw std::invalid_argument("Unsupported operation");
    }

    static std::function<void(const opcode_t)> funcs[] = { 
                                       clear_screen_and_return, jump_to, call_func, jump_if_equal, 
                                       jump_if_not_equal, jump_if_registers_equal, set_register_to_value, add_assign_register_to_value, 
                                       assign_to_register, jump_if_registers_not_equal, assign_address_register, jump_to_address, 
                                       set_register_to_bitwise_and_of_random, draw_sprite, skip_by_key, funcF
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
        std::cout << "############## UPDATE ##############" << std::endl;

        // Fetch Opcode
        auto first_half_opcode = memory[pc];
        auto second_half_opcode = memory[pc+1];

        opcode_t opcode = static_cast<opcode_t>(first_half_opcode << 8 | second_half_opcode);
        auto f_index = first_half_opcode >> 4;
        std::cout <<"opcode: " << std::hex << opcode << std::endl;
        funcs[f_index](opcode);
        std::cout << "pc: " << pc << std::endl;
        std::cout << "I: " << I << std::endl;
        for (int i = 0; i != 15; i++)
        {
            std::cout << "register" << i << ": " << static_cast<int>(V[i]) << std::endl;
        }
        std::cout << "flag_register: " << static_cast<int>(flag_register) << std::endl;
        std::cout << std::endl;

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