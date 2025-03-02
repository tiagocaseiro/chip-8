#pragma once

namespace chip8
{
    static constexpr auto DRAW_BUFFER_WIDTH  = 64;
    static constexpr auto DRAW_BUFFER_HEIGHT = 32;

    using draw_buffer = unsigned char[DRAW_BUFFER_WIDTH * DRAW_BUFFER_HEIGHT];

    void init();
    void update();
    void load(const char* path);
    void on_key_down(const int key);
    void on_key_up(const int key_index);
    bool draw_triggered();
    const draw_buffer& gfx_buffer();
}; // namespace chip8