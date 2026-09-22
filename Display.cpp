#include "Display.h"
#include "Joypad.h"
#ifdef GB_WITH_SDL
#include <SDL3/SDL.h>
#include <cstring>

Display::~Display(){
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
bool Display::open(){
    if(!SDL_Init(SDL_INIT_VIDEO)) return false;
    window = SDL_CreateWindow("Game Boy Emulator", 640, 576, SDL_WINDOW_RESIZABLE);
    if(!window) return false;
    renderer = SDL_CreateRenderer(window, nullptr);
    if(!renderer) return false;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if(!texture) return false;
    return SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) &&
           SDL_SetRenderLogicalPresentation(renderer, 160, 144, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}
bool Display::poll(Joypad& joypad){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_EVENT_WINDOW_FOCUS_LOST){
            for(int i = 0; i < 8; ++i) joypad.release(static_cast<Joypad::Button>(i));
        }
        if((event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) && !event.key.repeat){
            int button = -1;
            switch(event.key.scancode){
                case SDL_SCANCODE_RIGHT: button = Joypad::RIGHT; break;
                case SDL_SCANCODE_LEFT: button = Joypad::LEFT; break;
                case SDL_SCANCODE_UP: button = Joypad::UP; break;
                case SDL_SCANCODE_DOWN: button = Joypad::DOWN; break;
                case SDL_SCANCODE_Z: button = Joypad::A; break;
                case SDL_SCANCODE_X: button = Joypad::B; break;
                case SDL_SCANCODE_BACKSPACE: button = Joypad::SELECT; break;
                case SDL_SCANCODE_RETURN: button = Joypad::START; break;
                default: break;
            }
            if(button >= 0) joypad.setButtonState(static_cast<Joypad::Button>(button),
                                                 event.type == SDL_EVENT_KEY_DOWN);
        }
        if(event.type == SDL_EVENT_QUIT ||
           (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) return false;
    }
    return true;
}
bool Display::present(const std::array<uint16_t, 160 * 144>& framebuffer){
    void* pixels = nullptr;
    int pitch = 0;
    if(!SDL_LockTexture(texture, nullptr, &pixels, &pitch)) return false;
    for(int y = 0; y < 144; ++y){
        for(int x = 0; x < 160; ++x){
            uint16_t color = framebuffer[y * 160 + x];
            auto expand = [](unsigned c){ return (c << 3) | (c >> 2); };
            uint32_t rgb = (expand((color >> 10) & 31) << 16) |
                           (expand((color >> 5) & 31) << 8) | expand(color & 31);
            std::memcpy(static_cast<uint8_t*>(pixels) + y * pitch + x * 4, &rgb, 4);
        }
    }
    SDL_UnlockTexture(texture);
    return SDL_RenderClear(renderer) && SDL_RenderTexture(renderer, texture, nullptr, nullptr) &&
           SDL_RenderPresent(renderer);
}
std::string Display::error() const { return SDL_GetError(); }
#else
Display::~Display() = default;
bool Display::open(){ return false; }
bool Display::poll(Joypad&){ return false; }
bool Display::present(const std::array<uint16_t, 160 * 144>&){ return false; }
std::string Display::error() const {
    return "Built without SDL3. Build with make, or use --headless.";
}
#endif
