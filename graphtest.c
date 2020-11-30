#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
int main(int argc, char** args)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    SDL_Window* window = SDL_CreateWindow("Animating using Sprite Sheets", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Event input;
    bool quit = false;

    SDL_Texture* spriteSheet = NULL;
    SDL_Surface* temp = IMG_Load("Sprites/capguy-walk.png");
    spriteSheet = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    //'windowRect' defines the dimensions of the rendering sprite on window
    SDL_Rect windowRect;
    windowRect.x = 300;
    windowRect.y = 0;
    windowRect.w = 140;
    windowRect.h = 200;

    //'textureRect' defines the dimensions of the rendering sprite on texture
    SDL_Rect textureRect;
    textureRect.x = 0;
    textureRect.y = 0;

    //SDL_QueryTexture() method gets the width and height of the texture
    SDL_QueryTexture(spriteSheet, NULL, NULL, &textureRect.w, &textureRect.h);
    //Now, textureRect.w and textureRect.h are filled
    //with respective dimensions of the image/texture

    //As there are 8 frames with same width, we simply
    //get the width of a frame by dividing with 8
    textureRect.w /= 8;
    //Height for each frame is the same as for the whole sheet/texture

    while (!quit)
    {
        while (SDL_PollEvent(&input) > 0)
        {
            if (input.type == SDL_QUIT) quit = true;
        }

        //Total number of frames of the animation
        int totalFrames = 8;

        //Amount of delay in milliseconds for each frame
        int delayPerFrame = 100;

        //SDL_GetTicks() method gives us the time in milliseconds

        //'frame' will give us the index of frame we want to render
        //For example, if 'frame' is 2, it will give us the third frame
        int frame = (SDL_GetTicks() / delayPerFrame) % totalFrames;

        //The index of frame is multiplied by the width of the frame
        //This will give us the appropriate
        //frame dimensions from the sprite sheet
        textureRect.x = frame * textureRect.w;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        //Copying the texture on to the window using
        //renderer, texture rectangle and window rectangle
        SDL_RenderCopy(renderer, spriteSheet, &textureRect, &windowRect);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(spriteSheet);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
