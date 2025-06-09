#include <iostream>
#include <SDL2/SDL.h>
#include <memory>
#include <chrono>
#include <thread>

int main() {
    std::cout << "Simple UI Test" << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "UI Test Window",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    std::cout << "Window and renderer created successfully" << std::endl;
    
    // Main loop
    bool running = true;
    int frame = 0;
    
    while (running) {
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }
        
        // Clear screen with dark blue
        SDL_SetRenderDrawColor(renderer, 10, 10, 50, 255);
        SDL_RenderClear(renderer);
        
        // Draw a pulsing red square
        int pulse = (int)(128 + 127 * sin(frame * 0.05));
        SDL_SetRenderDrawColor(renderer, pulse, 0, 0, 255);
        SDL_Rect rect = {300, 200, 200, 200};
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw white border
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
        
        // Draw diagonal line
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawLine(renderer, 0, 0, 800, 600);
        
        // Present
        SDL_RenderPresent(renderer);
        
        frame++;
        
        // Small delay
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "Shutting down..." << std::endl;
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Clean shutdown complete" << std::endl;
    return 0;
}