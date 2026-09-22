#pragma once
#include <array>
#include <cstdint>
#include <string>

class Joypad;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class Display {
public:
    Display() = default;
    ~Display();
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    bool open();
    bool poll(Joypad& joypad);
    bool present(const std::array<uint16_t, 160 * 144>& framebuffer);
    std::string error() const;
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};
