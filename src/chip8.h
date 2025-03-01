#pragma once

namespace chip8
{
    static constexpr auto DRAW_BUFFER_WIDTH  = 64;
    static constexpr auto DRAW_BUFFER_HEIGHT = 32;

    using draw_buffer = unsigned char[DRAW_BUFFER_WIDTH * DRAW_BUFFER_HEIGHT];

    void init();
    void update();
    void load(const char* path);
    const draw_buffer& gfx_buffer();
}; // namespace chip8