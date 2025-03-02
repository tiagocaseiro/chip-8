#include "chip8.h"

#include <algorithm>
#include <bitset>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <random>
#include <sstream>
#include <utility>

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

    static bool key_state[16];

    static bool draw_this_frame = false;

    unsigned char chip8_fontset[80] = {
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

    static constexpr auto PROGRAM_OFFSET = 0x200;

    static inline void clear_buffer(auto& buffer)
    {
        static constexpr unsigned char ZERO = 0;
        ranges::fill(buffer, ZERO);
    }

    static unsigned char get_key_from_register(const register_t reg)
    {
        return static_cast<unsigned char>(reg & 0xF);
    }

    static bool paint_row_pixels_at(const int x, const int y, unsigned char memory_row)
    {
        static constexpr auto ROW_WIDTH = 8;

        bool flipped = false;

        for(auto i = 0; i != ROW_WIDTH; i++)
        {
            static constexpr auto FIRST_PIXEL_POSITION = 0x80;

            // Only entries with 1 require action
            if((memory_row & (FIRST_PIXEL_POSITION >> i)) == 0)
            {
                continue;
            }

            const int index = y * 64 + x + i;
            flipped |= gfx[index] == 1;
            gfx[index] ^= 1;
        }

        return flipped;
    }

    static inline unsigned short get_memory_address_from_opcode(const opcode_t opcode)
    {
        return static_cast<unsigned short>(opcode & 0x0FFF);
    }

    static register_t& get_first_register_from_opcode(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        auto index = opcode & 0x0F00;
        index >>= 8;

        std::cout << std::hex << "register " << index << " value " << static_cast<int>(V[index]) << std::endl;

        return V[index];
    }

    register_t& get_second_register_from_opcode(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        auto index = opcode & 0x00F0;
        index >>= 4;

        std::cout << std::hex << "register " << index << " value " << static_cast<int>(V[index]) << std::endl;

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
        std::cout << __FUNCTION__ << std::endl;

        pc += 2;
    }

    static void fill_registers_with_memory(const opcode_t opcode)
    {

        register_t& end = get_first_register_from_opcode(opcode);

        auto i = 0;
        for(register_t* it = &V[0]; it <= &end; it++)
        {
            *it = memory[I + i];
            i++;
        }

        next_instruction();
    }

    static void fill_memory_with_registers(const opcode_t opcode)
    {

        const register_t& end = get_first_register_from_opcode(opcode);

        auto i = 0;
        for(register_t* it = &V[0]; it <= &end; it++)
        {
            memory[I + i] = *it;
            i++;
        }

        next_instruction();
    }

    static inline void store_bcd(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const register_t reg = get_first_register_from_opcode(opcode);
        memory[I]            = static_cast<unsigned char>(reg / 100); // hundreds digit
        memory[I + 1]        = static_cast<unsigned char>((reg - memory[I]) / 10);
        memory[I + 2]        = static_cast<unsigned char>(reg - memory[I] - memory[I + 1]);

        next_instruction();
    }

    static inline void set_memory_address_to_character_sprite_address(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const register_t reg = get_first_register_from_opcode(opcode);

        const unsigned char key = get_key_from_register(reg);

        std::cout << "key " << std::hex << static_cast<int>(key) << " " << &memory[0] << " " << &memory[key * 5]
                  << std::endl;

        I = static_cast<unsigned short>(std::distance(&memory[0], &memory[key * 5]));

        next_instruction();
    }

    void jump_next_instruction()
    {
        std::cout << __FUNCTION__ << std::endl;

        pc += 4;
    }

    static void return_from_subroutine()
    {
        std::cout << __FUNCTION__ << std::endl;

        sp--; // Go back in the stack to previous valid entry
        if(sp < &stack[0])
        {
            std::out_of_range("Stack pointer decremented to outside the range of the stack");
        }
        pc = *sp; // Point pc to saved memory address

        next_instruction();
    }

    static void clear_screen_and_return(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        switch(opcode)
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
        sp++;     // Go up next available entry stack
        if(sp > &stack[STACK_SIZE - 1])
        {
            std::out_of_range("Stack pointer incremented to outside the range of the stack");
        }
        pc = memory_address; // Have pc point to new memory address
    }

    static void jump_if_equal(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const auto reg   = get_first_register_from_opcode(opcode); // Extract register index
        const auto value = get_value_from_opcode_nn(opcode);       // Extract value

        if(reg != value)
        {
            next_instruction();
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void jump_if_not_equal(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const auto reg   = get_first_register_from_opcode(opcode); // Extract register index
        const auto value = get_value_from_opcode_nn(opcode);       // Extract value

        if(reg == value)
        {
            next_instruction();
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void jump_if_registers_equal(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const auto register_1 = get_first_register_from_opcode(opcode);  // Extract register index 1
        const auto register_2 = get_second_register_from_opcode(opcode); // Extract register index 2

        if(register_1 != register_2)
        {
            next_instruction();
            return;
        }

        jump_next_instruction(); // Jump a whole instruction
    }

    static void set_register_to_value(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        register_t& reg      = get_first_register_from_opcode(opcode); // Extract register index
        register_t new_value = get_value_from_opcode_nn(opcode);       // Extract value

        reg = new_value;

        next_instruction();
    }

    static void add_assign_register_to_value(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        auto& reg = get_first_register_from_opcode(opcode);

        reg += get_value_from_opcode_nn(opcode);

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
                register_1                  = register_2;
                break;
            }
            case 0x1:
            {
                register_t& register_1      = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                register_1 |= register_2;
                break;
            }
            case 0x2:
            {
                register_t& register_1      = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                register_1 &= register_2;
                break;
            }
            case 0x3:
            {
                register_t& register_1      = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                register_1 ^= register_2;
                break;
            }
            case 0x4:
            {
                register_t& register_1      = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                int temp                    = register_1;
                temp += register_2;
                register_1 = static_cast<register_t>(temp);

                // Check if digits passed char size were flipped
                auto is_overflow = temp >> 8 != 0;
                if(is_overflow)
                {
                    flag_register = 1;
                }
                else
                {
                    flag_register = 0;
                }
                break;
            }
            case 0x5:
            {
                register_t& register_1      = get_first_register_from_opcode(opcode);
                const register_t register_2 = get_second_register_from_opcode(opcode);
                auto is_underflow           = register_2 > register_1;
                register_1 -= register_2;

                // Check if digits passed char size were flipped
                if(is_underflow)
                {
                    flag_register = 0;
                }
                else
                {
                    flag_register = 1;
                }
                break;
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
        next_instruction();
    }

    static void jump_if_registers_not_equal(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const auto register_1 = get_first_register_from_opcode(opcode);  // Extract register index 1
        const auto register_2 = get_second_register_from_opcode(opcode); // Extract register index 2

        if(register_1 == register_2)
        {
            next_instruction();
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
        const auto value      = get_value_from_opcode_nnn(opcode);
        pc += register_0;
        pc += value;
    }

    static void set_register_to_bitwise_and_of_random(const opcode_t opcode)
    {
        static std::random_device rd;
        static std::uniform_int_distribution<int> dist(0, 255);

        register_t& reg  = get_first_register_from_opcode(opcode);
        register_t value = get_value_from_opcode_nn(opcode);

        register_t rand = static_cast<register_t>(dist(rd));

        reg = static_cast<register_t>(rand & value);

        next_instruction();
    }

    static void draw_sprite(const opcode_t opcode)
    {
        draw_this_frame = true;
        std::cout << __FUNCTION__ << std::endl;
        const int x      = get_first_register_from_opcode(opcode);
        int y            = get_second_register_from_opcode(opcode);
        const int height = get_value_from_opcode_n(opcode);
        std::cout << std::dec << "x: " << x << " y: " << y << " h: " << height << std::endl;

        bool pixel_flipped = false;
        for(auto i = 0; i != height; i++)
        {
            const int index = I + i;
            if(index >= static_cast<int>(sizeof memory))
            {
                std::out_of_range("Pointing out of memory range");
            }
            pixel_flipped |= paint_row_pixels_at(x, y, memory[index]);
            y++;
        }

        flag_register = pixel_flipped;
        // auto k        = 0;
        // for(int i = 0; i != 32; i++)
        // {
        //     std::cout << 10000000 + k << "      ";
        //     for(int j = 0; j != 64; j++)
        //     {
        //         // if(int(gfx[k]) == 1)
        //         // {

        //         std::cout << int(gfx[k]) << " ";
        //         // }
        //         // else
        //         // {
        //         //     std::cout << " " << " ";
        //         // }
        //         k++;
        //     }
        //     std::cout << std::endl;
        // }

        next_instruction();
    }

    static bool key_is_pressed(unsigned char key)
    {
        return key_state[key];
    }

    static void jump_if_key_pressed(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const register_t reg    = get_first_register_from_opcode(opcode);
        const unsigned char key = get_key_from_register(reg);

        if(key_is_pressed(key))
        {
            jump_next_instruction();
            return;
        }
        next_instruction();
    }

    static void jump_if_key_not_pressed(const opcode_t opcode)
    {
        std::cout << __FUNCTION__ << std::endl;

        const register_t reg    = get_first_register_from_opcode(opcode);
        const unsigned char key = get_key_from_register(reg);

        if(key_is_pressed(key) == false)
        {
            jump_next_instruction();
            return;
        }
        next_instruction();
    }

    static void jump_by_key(const opcode_t opcode)
    {
        switch(get_value_from_opcode_nn(opcode))
        {
            case 0x9E:
                jump_if_key_pressed(opcode);
                break;
            case 0xA1:
                jump_if_key_not_pressed(opcode);
                break;
            default:
                break;
        }
    }
    static inline void set_sound_timer_to_register(const opcode_t opcode)
    {
        sound_timer = get_first_register_from_opcode(opcode);
        next_instruction();
    }

    static inline void set_delay_timer_to_register(const opcode_t opcode)
    {
        delay_timer = get_first_register_from_opcode(opcode);
        next_instruction();
    }

    static inline void set_register_to_delay_timer(const opcode_t opcode)
    {
        register_t& reg = get_first_register_from_opcode(opcode);
        reg             = delay_timer;
        next_instruction();
    }

    static void misc(const opcode_t opcode)
    {
        switch(get_value_from_opcode_nn(opcode))
        {
            case 0x07:
            {
                set_register_to_delay_timer(opcode);
                return;
            }
            case 0x0A:
            {
                throw std::invalid_argument("Unsupported operation");
                return;
            }
            case 0x15:
            {
                set_delay_timer_to_register(opcode);
                return;
            }
            case 0x18:
            {
                set_sound_timer_to_register(opcode);
                return;
            }
            case 0x1E:
            {
                throw std::invalid_argument("Unsupported operation");
                return;
            }
            case 0x29:
            {
                set_memory_address_to_character_sprite_address(opcode);
                return;
            }
            case 0x33:
            {
                store_bcd(opcode);
                return;
            }
            case 0x55:
            {
                fill_memory_with_registers(opcode);
                return;
            }
            case 0x65:
            {
                fill_registers_with_memory(opcode);
                return;
            }
        }
    }

    static std::function<void(const opcode_t)> funcs[] = {
        clear_screen_and_return,               // 0
        jump_to,                               // 1
        call_func,                             // 2
        jump_if_equal,                         // 3
        jump_if_not_equal,                     // 4
        jump_if_registers_equal,               // 5
        set_register_to_value,                 // 6
        add_assign_register_to_value,          // 7
        assign_to_register,                    // 8
        jump_if_registers_not_equal,           // 9
        assign_address_register,               // A
        jump_to_address,                       // B
        set_register_to_bitwise_and_of_random, // C
        draw_sprite,                           // D
        jump_by_key,                           // E
        misc                                   // F
    };

    void init()
    {
        pc = PROGRAM_OFFSET; // Program counter starts at 0x200
        I  = 0;              // Reset index register
        sp = &stack[0];      // Reset stack pointer

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
            memory[i] = chip8_fontset[i];

            std::memcpy(&memory, &chip8_fontset, sizeof chip8_fontset); // OK
        }

        // Reset timers
        delay_timer = 0;
        sound_timer = 0;
    }

    void update()
    {
        std::cout << "############## UPDATE ##############" << std::endl;

        draw_this_frame = false;
        // Fetch Opcode
        auto first_half_opcode  = memory[pc];
        auto second_half_opcode = memory[pc + 1];

        opcode_t opcode = static_cast<opcode_t>(first_half_opcode << 8 | second_half_opcode);
        auto f_index    = first_half_opcode >> 4;
        std::cout << "opcode: " << std::hex << opcode << std::endl;
        if(f_index >= static_cast<int>(sizeof funcs))
        {
            throw std::invalid_argument("Unsupported operation");
        }
        funcs[f_index](opcode);
        std::cout << "pc: " << pc << std::endl;
        std::cout << "I: " << I << std::endl;
        for(int i = 0; i != 15; i++)
        {
            std::cout << "register " << i << ": " << static_cast<int>(V[i]) << std::endl;
        }
        std::cout << "flag_register: " << static_cast<int>(flag_register) << std::endl;
        std::cout << std::endl;

        if(delay_timer > 0)
        {
            delay_timer--;
        }

        if(sound_timer > 0)
        {
            if(sound_timer == 1)
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
        begin += PROGRAM_OFFSET;
        std::streamsize max_count = sizeof(memory) - PROGRAM_OFFSET;
        is.read(reinterpret_cast<char*>(begin), max_count);
    }

    bool draw_triggered()
    {
        return draw_this_frame;
    }

    const draw_buffer& gfx_buffer()
    {
        return gfx;
    }
} // namespace chip8