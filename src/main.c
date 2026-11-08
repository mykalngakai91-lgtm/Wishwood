#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("[Error] SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Wishwood: Kingdom 3030",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, 
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("[Error] Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("[Error] Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Load generated BMP assets
    SDL_Surface* bg_surface = SDL_LoadBMP("assets/bg.bmp");
    SDL_Texture* bg_texture = bg_surface ? SDL_CreateTextureFromSurface(renderer, bg_surface) : NULL;
    if (bg_surface) SDL_FreeSurface(bg_surface);

    SDL_Surface* player_surface = SDL_LoadBMP("assets/player.bmp");
    SDL_Texture* player_texture = player_surface ? SDL_CreateTextureFromSurface(renderer, player_surface) : NULL;
    if (player_surface) SDL_FreeSurface(player_surface);

    // Open controller if available
    SDL_GameController* controller = NULL;
    if (SDL_NumJoysticks() > 0) {
        controller = SDL_GameControllerOpen(0);
    }

    float player_x = SCREEN_WIDTH / 2.0f - 32.0f;
    float player_y = SCREEN_HEIGHT / 2.0f - 32.0f;
    float speed = 10.0f;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Input handling
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A])  player_x -= speed;
        if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) player_x += speed;
        if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W])     player_y -= speed;
        if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S])   player_y += speed;

        if (controller) {
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  player_x -= speed;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) player_x += speed;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    player_y -= speed;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  player_y += speed;

            Sint16 axis_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 axis_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
            float deadzone = 8000.0f;
            if (axis_x > deadzone)  player_x += speed * (axis_x / 32767.0f);
            if (axis_x < -deadzone) player_x += speed * (axis_x / 32767.0f);
            if (axis_y > deadzone)  player_y += speed * (axis_y / 32767.0f);
            if (axis_y < -deadzone) player_y += speed * (axis_y / 32767.0f);

            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK)) {
                running = false;
            }
        }

        // Boundaries
        if (player_x < 0) player_x = 0;
        if (player_x > SCREEN_WIDTH - 64) player_x = SCREEN_WIDTH - 64;
        if (player_y < 0) player_y = 0;
        if (player_y > SCREEN_HEIGHT - 64) player_y = SCREEN_HEIGHT - 64;

        // Render frame
        SDL_RenderClear(renderer);

        // Draw background texture if loaded, otherwise fallback to color
        if (bg_texture) {
            SDL_RenderCopy(renderer, bg_texture, NULL, NULL);
        } else {
            SDL_SetRenderDrawColor(renderer, 20, 24, 35, 255);
            SDL_RenderFillRect(renderer, NULL);
        }

        // Draw player texture or fallback rect
        SDL_Rect player_rect = { (int)player_x, (int)player_y, 64, 64 };
        if (player_texture) {
            SDL_RenderCopy(renderer, player_texture, NULL, &player_rect);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 230, 138, 255);
            SDL_RenderFillRect(renderer, &player_rect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (bg_texture) SDL_DestroyTexture(bg_texture);
    if (player_texture) SDL_DestroyTexture(player_texture);
    if (controller) SDL_GameControllerClose(controller);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
CC = clang
CFLAGS = -O3 -Wall -Wextra -I$(OO_PS4_TOOLCHAIN)/include -Isrc
LDFLAGS = -L$(OO_PS4_TOOLCHAIN)/lib -lSDL2 -lm -lpthread

all: WISH30300.pkg

assets_gen:
	python3 tools/generate_assets.py

eboot.bin: src/main.c assets_gen
	$(CC) $(CFLAGS) src/main.c -o eboot.bin $(LDFLAGS)

PARAM.SFO:
	python3 tools/gen_sfo.py

project.gp4: eboot.bin PARAM.SFO
	python3 tools/make_gp4.py

WISH30300.pkg: project.gp4
	python3 tools/pkg_builder.py

clean:
	rm -f eboot.bin PARAM.SFO project.gp4 WISH30300.pkg
	rm -rf pkg_root sc0 assets
