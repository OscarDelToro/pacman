#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <omp.h>

#define LEFT 0
#define RIGTH 1
#define TOP 2
#define BOTTOM 3

#define COIN 2
#define POWERUP 3
#define PATH 1
#define BARRIER 0


#define NUMCELLSX 50
#define NUMCELLSY 40
void setDirectionPlayerRender();
void keyboardHandler(const char *);
void initPlayerResources();
void initMap();
void initNPCS();
double getMod(double,double);
int msleep(unsigned int tms);
void collectCoin(int);
void collectPowerUp(int);
void movePlayer();
void checkMapForPoints();
int getIndexByXY(int, int);

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
    bool isEdible;
    int direction;
    double x;
    double y;
} NPC;

double playerX,
       playerY;
int playerDirection;

Cell cells[NUMCELLSX*NUMCELLSY];
NPC  *npcs;
int numNPCS;
double step=1;

double mapDimY=16*NUMCELLSY;
double mapDimX=16*NUMCELLSX;

    
// Window dimensions
static const int width = 16*NUMCELLSX;
static const int height = 16*NUMCELLSY+34;
int npcY[]={87,107,127,147};
int npcX[]={7,27,47,67,87,107,127,147};
int mapX[]={0,17,34,51};


//PLAYER

SDL_Rect windowRectPlayer,
    textureRectPlayer,
    windowRectCell,
    textureRectCell,
    windowRectNPC,
    textureRectNPC;

int main(int argc, char **argv) {
    printf("test: %d\n",-1%127);
    if (argc>1){
        numNPCS=atoi(argv[1]);
    }
    else{
        numNPCS=6;
    }

    // Initialize  cells and things
    initNPCS();
    initMap();

    // Initialize SDL
    CHECK_ERROR(SDL_Init(SDL_INIT_VIDEO) != 0, SDL_GetError());
    CHECK_ERROR(IMG_Init(IMG_INIT_PNG)== NULL, SDL_GetError());
    

    // Create an SDL window
    SDL_Window *window = SDL_CreateWindow("Pacman", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    CHECK_ERROR(window == NULL, SDL_GetError());

    // Create a renderer (accelerated and in sync with the display refresh rate)
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);    
    CHECK_ERROR(renderer == NULL, SDL_GetError());

    SDL_Texture* spriteSheet = NULL;
    SDL_Surface* temp = IMG_Load("Sprites/gamesprites.png");
    spriteSheet = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    initPlayerResources();


    

    int totalPlayerFrames = 3;
    int delayPerFrame = 100;

    int playerFrame;
    int framesFour;
    int framesEight;
    int framesTwo;
    // Initial renderer color
    bool running = true;
    bool endingPowerUp = true;//This flag indicates if the powerup is ending
setDirectionPlayerRender();


    #pragma omp parallel section
    {
        
        #pragma omp master task //should be the rendering and events processor 
        while(running) {
            // Process events and rendering
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type == SDL_QUIT) {
                    running = false;
                } 
                else if(event.type == SDL_KEYDOWN) {
                    const char *key = SDL_GetKeyName(event.key.keysym.sym);
                    keyboardHandler(key);                                          
                }
            }
            playerFrame = (SDL_GetTicks() / delayPerFrame) % totalPlayerFrames;
            framesFour = (SDL_GetTicks() / delayPerFrame) % 4;
            framesEight = (SDL_GetTicks() / delayPerFrame) % 8;
            framesTwo = (SDL_GetTicks() / delayPerFrame) % 2;

            textureRectPlayer.x = playerFrame * textureRectPlayer.w +playerFrame*7+7;

            SDL_SetRenderDrawColor(renderer, 0, 120, 0, 255);
            
            // Clear screen
            SDL_RenderClear(renderer);
            
            
            //RENDER MAP
            for(int i=0;i<NUMCELLSY;i++){
                windowRectCell.y=i*16;
                for(int j=0; j<NUMCELLSX;j++){
                    textureRectCell.x=mapX[cells[i*NUMCELLSX+j].resType];
                    windowRectCell.x=j*16;
                    SDL_RenderCopy(renderer, spriteSheet, &textureRectCell, &windowRectCell);
                }
            }

            //RENDER NPCS
            for(int i=0; i<numNPCS; i++){
                windowRectNPC.x=npcs[i].x;
                windowRectNPC.y=npcs[i].y;
                if(npcs[i].isEdible){
                    textureRectNPC.y=167;
                    
                    if(endingPowerUp){
                        textureRectNPC.x=npcX[framesFour]; 
                    }
                    else
                        textureRectNPC.x=npcX[framesTwo]; 
                    
                       
                }
                else{
                    if(npcs[i].isAlive){
                        textureRectNPC.y=npcY[i%4];
                        textureRectNPC.x=npcX[framesEight];
                    }
                    else{
                        textureRectNPC.y=184;
                        textureRectNPC.x=npcX[framesFour];
                    }
                }
                SDL_RenderCopy(renderer, spriteSheet, &textureRectNPC, &windowRectNPC);
                
            }
            

            //RENDER PLAYER   
            windowRectPlayer.x=playerX;
            windowRectPlayer.y=playerY;  
            SDL_RenderCopy(renderer, spriteSheet, &textureRectPlayer, &windowRectPlayer);


            // Draw

            // Show what was drawn
            SDL_RenderPresent(renderer);
        }
        #pragma omp single //inertial movement on player
            {
                while(running){
                    movePlayer();
                    checkMapForPoints();
                    msleep(20);
                }
                

            }
        
    }
    // Release resources
    SDL_DestroyTexture(spriteSheet);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
int getIndexByXY(int x, int y){
    if(playerDirection==BOTTOM||playerDirection==RIGTH){
        return (x/16)+(y/16)*NUMCELLSX;
    }
    return ((x+8)/16)+((y+8)/16)*NUMCELLSX;
    
}
void checkMapForPoints(){
    int tmpx,
        tmpy,
        type;
    if(playerDirection==BOTTOM||playerDirection==RIGTH){
        tmpx=playerX/16;
        tmpy=playerY/16;
    }
    else{
        tmpx=(playerX+8)/16;
        tmpy=(playerY+8)/16;

    }

    
    type=cells[tmpy*NUMCELLSX+tmpx].resType;
    
    if (type==COIN){
        collectCoin(tmpy*NUMCELLSX+tmpx);

    }
    else if(type==POWERUP){
        collectPowerUp(tmpy*NUMCELLSX+tmpx);

    }

}
void movePlayer(){
    if(playerDirection==TOP){
        if(cells[getIndexByXY(playerX,playerY-9)].resType)
            playerY=getMod(playerY-step,mapDimY);
    }
    if(playerDirection==BOTTOM){
        if(cells[getIndexByXY(playerX,playerY+16)].resType)
            playerY=getMod(playerY+step,mapDimY);
    }
    if(playerDirection==LEFT){
        if(cells[getIndexByXY(playerX-9,playerY)].resType)
            playerX=getMod(playerX-step,mapDimX);
    }
    if(playerDirection==RIGTH){
        if(cells[getIndexByXY(playerX+16,playerY)].resType||playerX/16>=NUMCELLSX-1)
            playerX=getMod(playerX+step,mapDimX);
    }

}
void collectCoin(int index){
    printf("Coin collected!\n");
    cells[index].resType=PATH;

}
void collectPowerUp(int index){
    printf("Power-up collected!\n");
    cells[index].resType=PATH;

}
int msleep(unsigned int tms) {
  return usleep(tms * 1000);
}
double getMod(double x,double mod){
    if(x<-13){
        return mod;
    }
    else if(x>mod){
        return 0;
    }
    return x;

}
void initNPCS(){
    npcs=malloc(numNPCS* sizeof *npcs);
    for(int i=0; i<numNPCS; i++){
        npcs[i].isAlive=true;
        npcs[i].direction=TOP;
        npcs[i].isEdible=false;
        npcs[i].x=20*i;
        npcs[i].y=0;
    }
    windowRectNPC.x=0;
    windowRectNPC.y=0;
    windowRectNPC.w=14;
    windowRectNPC.h=14;
    textureRectNPC.w=14;
    textureRectNPC.h=14;
}
void initMap(){
    for(int i=0; i< NUMCELLSX*NUMCELLSY; i++){
        cells[i].isPath=true;
        cells[i].resType=COIN;
        cells[i].hasPoints=true;
        if(i<NUMCELLSX||i%NUMCELLSX==0||
            i%NUMCELLSX==NUMCELLSX-1||i>NUMCELLSX*NUMCELLSY-NUMCELLSX){
            cells[i].resType=BARRIER;
            
        }
    }
    cells[20*3].resType=POWERUP;
    cells[20*4].resType=POWERUP;
    int ina=NUMCELLSX*(NUMCELLSY/2);
    int inb=ina+NUMCELLSX;
    cells[ina].resType=PATH;
    cells[ina+NUMCELLSX-1].resType=PATH;
    cells[inb].resType=PATH;
    cells[inb+NUMCELLSX-1].resType=PATH;
    
    
    windowRectCell.x=0;
    windowRectCell.y=0;
    windowRectCell.w=16;
    windowRectCell.h=16;

    textureRectCell.x=0;
    textureRectCell.y=199;
    textureRectCell.w=16;
    textureRectCell.h=16;
}
void initPlayerResources(){
    playerDirection=RIGTH;
    
    playerX=17;
    playerY=17;
    windowRectPlayer.x = 0;
    windowRectPlayer.y = 0;
    windowRectPlayer.w = 13;
    windowRectPlayer.h = 13;

    textureRectPlayer.x = 7;
    textureRectPlayer.y = 7;
    textureRectPlayer.w = 13;
    textureRectPlayer.h = 13;

}
void setDirectionPlayerRender(){
    if(playerDirection==LEFT){
                textureRectPlayer.y = 7;
            }
            else if(playerDirection==RIGTH){
                textureRectPlayer.y = 27;
            }
            else if(playerDirection==TOP){
                textureRectPlayer.y = 47;
            }
            else if(playerDirection==BOTTOM){
                textureRectPlayer.y = 66;
            }

}

void keyboardHandler(const char *key){
    if(strcmp(key, "C") == 0) {
        printf("Key C pressed \n");

    } 
    else if(strcmp(key, "A") == 0) {
        printf("Key A pressed \n");
        playerDirection=LEFT;
        setDirectionPlayerRender();
        playerY=round(playerY/16)*16+2;

        
    } 
    else if(strcmp(key, "S") == 0) {
        printf("Key S pressed \n");
        playerDirection=BOTTOM;
        setDirectionPlayerRender();
        playerX=round(playerX/16)*16+1;
    }
    else if(strcmp(key, "D") == 0) {
        printf("Key D pressed \n");
        playerDirection=RIGTH;
        setDirectionPlayerRender();
        playerY=round(playerY/16)*16+2;
    }
    else if(strcmp(key, "W") == 0) {
        printf("Key W pressed \n");
        playerDirection=TOP;
        setDirectionPlayerRender();
        playerX=round(playerX/16)*16+1;
    } 
    else{
        printf("Pressed %s key\n",key);
    }  
}

