#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    // Initialize SDL video and game controller subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("[Error] SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    // Create 1080p Window for PS4 output
    SDL_Window* window = SDL_CreateWindow("Wishwood: Kingdom 3030",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, 
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("[Error] Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create hardware-accelerated renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("[Error] Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Open first available controller (DualShock 4 / DualSense)
    SDL_GameController* controller = NULL;
    if (SDL_NumJoysticks() > 0) {
        controller = SDL_GameControllerOpen(0);
        if (controller) {
            printf("[Info] Controller connected: %s\n", SDL_GameControllerName(controller));
        }
    }

    // Player starting position and movement variables
    float player_x = SCREEN_WIDTH / 2.0f - 32.0f;
    float player_y = SCREEN_HEIGHT / 2.0f - 32.0f;
    float speed = 10.0f;

    bool running = true;
    SDL_Event event;

    // Main Game Loop
    while (running) {
        // Handle input events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Handle Keyboard / Digital D-Pad input mapping
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A])  player_x -= speed;
        if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) player_x += speed;
        if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W])     player_y -= speed;
        if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S])   player_y += speed;

        // Handle Controller Joystick / D-Pad input if connected
        if (controller) {
            // D-Pad buttons
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  player_x -= speed;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) player_x += speed;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    player_y -= speed;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  player_y -= speed;

            // Left Analog Stick axes (-32768 to 32767)
            Sint16 axis_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 axis_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
            
            float deadzone = 8000.0f;
            if (axis_x > deadzone)  player_x += speed * (axis_x / 32767.0f);
            if (axis_x < -deadzone) player_x += speed * (axis_x / 32767.0f);
            if (axis_y > deadzone)  player_y += speed * (axis_y / 32767.0f);
            if (axis_y < -deadzone) player_y += speed * (axis_y / 32767.0f);

            // Exit game if PS button or Special options combo pressed
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK)) {
                running = false;
            }
        }

        // Screen Boundaries check
        if (player_x < 0) player_x = 0;
        if (player_x > SCREEN_WIDTH - 64) player_x = SCREEN_WIDTH - 64;
        if (player_y < 0) player_y = 0;
        if (player_y > SCREEN_HEIGHT - 64) player_y = SCREEN_HEIGHT - 64;

        // Clear screen with background color (Dark Navy)
        SDL_SetRenderDrawColor(renderer, 20, 24, 35, 255);
        SDL_RenderClear(renderer);

        // Draw Player Sprite (Green Square placeholder)
        SDL_Rect player_rect = { (int)player_x, (int)player_y, 64, 64 };
        SDL_SetRenderDrawColor(renderer, 0, 230, 138, 255);
        SDL_RenderFillRect(renderer, &player_rect);

        // Present frame to screen
        SDL_RenderPresent(renderer);
        
        // Frame throttle ~60 FPS
        SDL_Delay(16);
    }

    // Cleanup resources
    if (controller) {
        SDL_GameControllerClose(controller);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
