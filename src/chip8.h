#pragma once

namespace chip8
{
    using draw_buffer = unsigned char[64 * 32];

    void init();
    void update();
    void load(const char* path);
    const draw_buffer& gfx_buffer();
};