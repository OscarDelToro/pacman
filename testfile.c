#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <omp.h>

#define LEFT 0
#define RIGTH 1
#define TOP 2
#define BOTTOM 3

#define NUMCELLSX 20
#define NUMCELLSY 20

// Utility macros
#define CHECK_ERROR(test, message) \
    do { \
        if((test)) { \
            fprintf(stderr, "%s\n", (message)); \
            exit(1); \
        } \
    } while(0)


typedef struct {
    bool isPath;
    int resType;
    bool hasPoints;
} Cell;
typedef struct {
    bool isAlive;
    int direction;
    double x;
    double y;
} NPC;

double playerX,
       playerY;
int playerDirection;

Cell cells[NUMCELLSX*NUMCELLSY];
NPC  npcs[5];


// Get a random number from 0 to 255
int randInt(int rmin, int rmax) {
    return rand() % rmax + rmin;
}
    
// Window dimensions
static const int width = 800;
static const int height = 600;

int main(int argc, char **argv) {
    // Initialize  cells and things
    for(int i=0; i< NUMCELLSX*NUMCELLSY; i++){
        cells[i].isPath=true;
        cells[i].resType=1;
        cells[i].hasPoints=true;
    }

    
    srand((unsigned int)time(NULL));
    
    // Initialize SDL
    CHECK_ERROR(SDL_Init(SDL_INIT_VIDEO) != 0, SDL_GetError());

    // Create an SDL window
    SDL_Window *window = SDL_CreateWindow("Hello, SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    CHECK_ERROR(window == NULL, SDL_GetError());

    // Create a renderer (accelerated and in sync with the display refresh rate)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);    
    CHECK_ERROR(renderer == NULL, SDL_GetError());

    // Initial renderer color
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    bool running = true;
    #pragma omp parallel section
    {
        
        #pragma omp master task //should be the rendering
        while(running) {
            // Process events and rendering
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type == SDL_QUIT) {
                    running = false;
                } 
                else if(event.type == SDL_KEYDOWN) {
                    const char *key = SDL_GetKeyName(event.key.keysym.sym);
                    if(strcmp(key, "C") == 0) {
                        printf("Key C pressed \n");
                    } 
                    else if(strcmp(key, "A") == 0) {
                        printf("Key A pressed \n");
                        playerDirection=TOP;
                    } 
                    else if(strcmp(key, "S") == 0) {
                        printf("Key S pressed \n");
                        playerDirection=BOTTOM;
                    }
                    else if(strcmp(key, "D") == 0) {
                        printf("Key D pressed \n");
                        playerDirection=RIGTH;
                    }
                    else if(strcmp(key, "W") == 0) {
                        printf("Key W pressed \n");
                        playerDirection=LEFT;
                    } 
                    else{
                        printf("Pressed %c key\n",key);
                    }                         
                }
            }
            

            // Clear screen
            SDL_RenderClear(renderer);

            // Draw

            // Show what was drawn
            SDL_RenderPresent(renderer);
        }
        #pragma omp single task
            {
                for(int i=0; i<30;i++){
                    printf("Hola, desde la iteración %d\n",i);
                    sleep(2);
                }
                printf("hello world from thread %d\n",omp_get_thread_num());

            }

    }
    // Release resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

