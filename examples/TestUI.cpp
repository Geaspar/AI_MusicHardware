#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>

// Simple SDL-based test program to demonstrate custom UI capabilities
// This is a simplified version that directly uses SDL instead of our UI framework

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "AI Music Hardware UI Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Main loop
    bool quit = false;
    float playheadPosition = 0.0f;
    
    while (!quit) {
        // Process events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            }
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderClear(renderer);
        
        // Draw title text area
        SDL_Rect titleRect = { 10, 10, 780, 40 };
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &titleRect);
        
        // Draw buttons
        SDL_Rect button1 = { 20, 60, 100, 40 };
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        SDL_RenderFillRect(renderer, &button1);
        
        SDL_Rect button2 = { 140, 60, 100, 40 };
        SDL_RenderFillRect(renderer, &button2);
        
        SDL_Rect button3 = { 260, 60, 100, 40 };
        SDL_RenderFillRect(renderer, &button3);
        
        // Draw waveform display
        SDL_Rect waveformRect = { 20, 120, 760, 150 };
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderFillRect(renderer, &waveformRect);
        
        // Draw waveform
        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
        for (int x = 0; x < 758; x++) {
            float time = static_cast<float>(x) / 758.0f;
            float sampleValue = 0.8f * sin(2.0f * M_PI * 3.0f * time);
            int y = 120 + 75 + static_cast<int>(sampleValue * 70.0f);
            SDL_RenderDrawPoint(renderer, 21 + x, y);
        }
        
        // Draw grid lines
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        // Horizontal center line
        SDL_RenderDrawLine(renderer, 20, 120 + 75, 780, 120 + 75);
        // Vertical lines (10% intervals)
        for (int i = 1; i < 10; i++) {
            int x = 20 + (i * 760) / 10;
            SDL_RenderDrawLine(renderer, x, 120, x, 270);
        }
        
        // Draw envelope editor
        SDL_Rect envelopeRect = { 20, 300, 350, 100 };
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderFillRect(renderer, &envelopeRect);
        
        // Draw ADSR envelope
        SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
        SDL_RenderDrawLine(renderer, 20, 400, 60, 300); // Attack
        SDL_RenderDrawLine(renderer, 60, 300, 120, 340); // Decay
        SDL_RenderDrawLine(renderer, 120, 340, 250, 340); // Sustain
        SDL_RenderDrawLine(renderer, 250, 340, 370, 400); // Release
        
        // Draw handles
        SDL_Rect handle1 = { 55, 295, 10, 10 };
        SDL_Rect handle2 = { 115, 335, 10, 10 };
        SDL_Rect handle3 = { 245, 335, 10, 10 };
        SDL_Rect handle4 = { 365, 395, 10, 10 };
        SDL_SetRenderDrawColor(renderer, 255, 100, 0, 255);
        SDL_RenderFillRect(renderer, &handle1);
        SDL_RenderFillRect(renderer, &handle2);
        SDL_RenderFillRect(renderer, &handle3);
        SDL_RenderFillRect(renderer, &handle4);
        
        // Draw sequencer grid
        SDL_Rect seqRect = { 400, 300, 380, 200 };
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &seqRect);
        
        // Draw grid cells
        int cellWidth = 380 / 16;
        int cellHeight = 200 / 8;
        
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 16; col++) {
                SDL_Rect cell = { 
                    400 + col * cellWidth, 
                    300 + row * cellHeight, 
                    cellWidth, 
                    cellHeight 
                };
                
                // Active cell if row + col is even (just for demonstration)
                if ((row + col) % 2 == 0) {
                    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
                    SDL_RenderFillRect(renderer, &cell);
                }
                
                // Draw cell border
                SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
                SDL_RenderDrawRect(renderer, &cell);
            }
        }
        
        // Animate playhead
        playheadPosition += 0.03f;
        if (playheadPosition >= 16.0f) {
            playheadPosition = 0.0f;
        }
        
        int playheadCol = static_cast<int>(playheadPosition);
        SDL_Rect playhead = { 
            400 + playheadCol * cellWidth, 
            300, 
            cellWidth, 
            200 
        };
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 80);
        SDL_RenderFillRect(renderer, &playhead);
        
        // Draw knob
        SDL_Rect knobRect = { 100, 450, 60, 60 };
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &knobRect);
        
        // Draw knob circle
        int knobCenterX = 100 + 30;
        int knobCenterY = 450 + 30;
        int knobRadius = 25;
        
        // Draw knob outline
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        for (int w = -knobRadius; w < knobRadius; w++) {
            for (int h = -knobRadius; h < knobRadius; h++) {
                if (w*w + h*h > (knobRadius-3)*(knobRadius-3) && 
                    w*w + h*h <= knobRadius*knobRadius) {
                    SDL_RenderDrawPoint(renderer, knobCenterX + w, knobCenterY + h);
                }
            }
        }
        
        // Draw knob indicator line
        float knobValue = 0.7f;
        float knobAngle = (knobValue * 270.0f - 135.0f) * M_PI / 180.0f;
        int indicatorX = knobCenterX + static_cast<int>(cos(knobAngle) * knobRadius * 0.8f);
        int indicatorY = knobCenterY + static_cast<int>(sin(knobAngle) * knobRadius * 0.8f);
        SDL_RenderDrawLine(renderer, knobCenterX, knobCenterY, indicatorX, indicatorY);
        
        // Present renderer
        SDL_RenderPresent(renderer);
        
        // Cap frame rate
        SDL_Delay(16); // ~60fps
    }
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}