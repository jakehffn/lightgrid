#include <string>
#include <iostream>

#include <SDL.h>
#include <SDL_ttf.h>

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT (WINDOW_WIDTH)

/*
- x, y: upper left corner.
- texture, rect: outputs.
*/
void get_text_and_rect(SDL_Renderer *renderer, int x, int y, char *text,
        TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect) {
    int text_width;
    int text_height;
    SDL_Surface *surface;
    SDL_Color textColor = {200, 200, 180, 0};

    surface = TTF_RenderText_Solid(font, text, textColor);
    *texture = SDL_CreateTextureFromSurface(renderer, surface);
    text_width = surface->w;
    text_height = surface->h;
    SDL_FreeSurface(surface);
    rect->x = x;
    rect->y = y;
    rect->w = text_width;
    rect->h = text_height;
}

int main(int argc, char **argv) {

    SDL_Event event;
    SDL_Rect rect1, rect2;
    SDL_Renderer *renderer;
    SDL_Texture *texture1, *texture2;
    SDL_Window *window;
    std::string font_path{"test/example/cozette/cozette_bitmap.ttf"};
    bool quit;

    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_WIDTH, 0, &window, &renderer);
    TTF_Init();
    TTF_Font *font = TTF_OpenFont(font_path.c_str(), 24);

    if (font == NULL) {
        std::cout << "error: font not found\n";
    }
    get_text_and_rect(renderer, 0, 0, "Light", font, &texture1, &rect1);
    get_text_and_rect(renderer, 0, rect1.y + rect1.h, "Grid", font, &texture2, &rect2);

    quit = false;
    while (!quit) {

        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }
        SDL_SetRenderDrawColor(renderer, 40, 35, 30, 0);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, texture1, NULL, &rect1);
        SDL_RenderCopy(renderer, texture2, NULL, &rect2);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture1);
    SDL_DestroyTexture(texture2);
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}