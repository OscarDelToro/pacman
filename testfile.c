#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <omp.h>

#define LEFT 0
#define RIGHT 1
#define TOP 2
#define BOTTOM 3

#define COIN 2
#define POWERUP 3
#define PATH 1
#define BARRIER 0


#define NUMCELLSX 30
#define NUMCELLSY 40
#define POWERUPDURATION 10000 //milliseconds

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
void checkCollision();
void NPCController(int);
void moveNPC(int);
int randomInRange(int,int);
void changeNPCDirection(int,int);
bool isInCollision(int);
void powerUpDriver();
void killPacman();

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
double stepNPC=1;

double mapDimY=16*NUMCELLSY;
double mapDimX=16*NUMCELLSX;

double npcStartingX;
double npcStartingY;

double pacmanStartingX=NUMCELLSX/2;
double pacmanStartingY=25;

bool pacmanIsAlive=true;

int score=0;
int combo=0;

    
// Window dimensions
static const int width = 16*NUMCELLSX;
static const int height = 16*NUMCELLSY+34;
int npcY[]={87,107,127,147};
int npcX[]={7,27,47,67,87,107,127,147};
int mapX[]={0,17,34,51};
int deadframes[]={7,26,46,66,86,106,126,146,166,186,206,220};//si la bolita final no jala es 208
int currentPowerUp = 0;


//PLAYER

SDL_Rect windowRectPlayer,
    textureRectPlayer,
    windowRectCell,
    textureRectCell,
    windowRectNPC,
    textureRectNPC,
    destWindowRect;

TTF_Font* font;
SDL_Surface* text;
SDL_Color white = { 255, 255, 255 };

SDL_Texture* textTexture;



bool running = true;
bool endingPowerUp = true;//This flag indicates if the powerup is ending

int main(int argc, char **argv) {
    

    npcStartingX = NUMCELLSX*0.5;
    npcStartingY = NUMCELLSY*0.5;
    printf("%f, %f",npcStartingX,npcStartingY);
    srand(time(0)); 
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
    CHECK_ERROR(IMG_Init(IMG_INIT_PNG)<0, SDL_GetError());
    CHECK_ERROR(TTF_Init()<0,SDL_GetError());
    font = TTF_OpenFont("font/8-bit-pusab.ttf", 10);
    
    

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
    int pointer=0;
    // Initial renderer color

    setDirectionPlayerRender();

    destWindowRect.x=0;
    destWindowRect.y=NUMCELLSY*16;


    #pragma omp parallel num_threads(numNPCS+2)
    {
        
        #pragma omp master  //should be the rendering and events processor 
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

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            
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
                    if(!npcs[i].isAlive){
                        textureRectNPC.y=184;
                        textureRectNPC.x=npcX[framesFour];
                    }
                    else if(endingPowerUp){
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
            if(!pacmanIsAlive){
                textureRectPlayer.y= 249;
                textureRectPlayer.x = deadframes[pointer%12];
                
            }
            windowRectPlayer.x=playerX;
            windowRectPlayer.y=playerY;  
            
            SDL_RenderCopy(renderer, spriteSheet, &textureRectPlayer, &windowRectPlayer);
            //RENDER TEXT
            char textScore[100];
            sprintf(textScore,"Score: %d",score);
            text = TTF_RenderText_Solid( font,textScore, white );
            textTexture = SDL_CreateTextureFromSurface( renderer, text );
            SDL_QueryTexture(textTexture, NULL, NULL, &destWindowRect.w, &destWindowRect.h);
            
            
            SDL_RenderCopy(renderer,textTexture,NULL,&destWindowRect);
            
            // Draw
            // Show what was drawn
            SDL_RenderPresent(renderer);
        }
        #pragma omp task
        {
            NPCController(omp_get_thread_num());
        }
        #pragma omp single //inertial movement on player
            {
                while(running){
                    while(!pacmanIsAlive){
                        if(pointer>10){
                            pacmanIsAlive=true;
                            playerX=pacmanStartingX*16;
                            playerY=pacmanStartingY*16;
                            pointer=0;
                            setDirectionPlayerRender();
                            break;
                        }
                        pointer++;
                        msleep(100);
                    }
                    movePlayer();
                    checkCollision();
                    powerUpDriver();
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
void powerUpDriver(){
    if(currentPowerUp>0){
        currentPowerUp-=20;
        if(currentPowerUp<=POWERUPDURATION/4){
            endingPowerUp=true;
        }
        if(currentPowerUp<=20){//restore things to before power up
            for(int i=0; i< numNPCS; i++){
                npcs[i].isEdible=false;
                //npcs[i].isAlive=true;
            }
            stepNPC=step;   
            currentPowerUp = 0;
        }
    }

}
void changeNPCDirection(int index,int dir){
    switch (dir)
    {
    case TOP:
        npcs[index].direction=TOP;
        npcs[index].x=round(npcs[index].x/16)*16+1;
        break;
    
    case BOTTOM:
        npcs[index].direction=BOTTOM;
        npcs[index].x=round(npcs[index].x/16)*16+1;
        break;

    case LEFT:
        npcs[index].direction=LEFT;
        npcs[index].y=round(npcs[index].y/16)*16+2;
        
        break;
    
    case RIGHT:
        npcs[index].direction=RIGHT;
        npcs[index].y=round(npcs[index].y/16)*16+2;
        
        break;
    
    default:
        break;
    }

}
bool isInCollision(int index){
    switch (npcs[index].direction)
    {
    case TOP:
        return !(cells[getIndexByXY(npcs[index].x,npcs[index].y-9)].resType);
        break;
    case BOTTOM:
        return !(cells[getIndexByXY(npcs[index].x,npcs[index].y+16)].resType);
        break;
    case LEFT:
        return !(cells[getIndexByXY(npcs[index].x-9,npcs[index].y)].resType);
        break;
    case RIGHT:
        return !(cells[getIndexByXY(npcs[index].x+16,npcs[index].y)].resType||playerX/16>=NUMCELLSX-1);
        break;
    
    default:
        return false;
        break;
    }

}
int randomInRange(int lower, int upper){
    return (rand() % (upper - lower + 1)) + lower;
}
void NPCController(int ind){
    if(ind>1){
        int index=ind-2;
        int nextDirChange=0;
        int delaySpawn=5000;
        while(running){
            if(nextDirChange==0){
                changeNPCDirection(index,randomInRange(0,3));
                nextDirChange=randomInRange(50,150)*20;
            }
            if(isInCollision(index)){
                
                do{
                    changeNPCDirection(index,randomInRange(0,3));
                }while(isInCollision(index));
                nextDirChange=randomInRange(50,150)*20;

            }
            nextDirChange-=20;
            if(!npcs[index].isAlive){
                delaySpawn-=20;
                if(delaySpawn<=20){
                    npcs[index].isAlive=true;
                    delaySpawn=5000;
                }

            }
            moveNPC(index);
            msleep(20);
        }

    }
    
}
void moveNPC(int index){
    int dir=npcs[index].direction;
    if(dir==TOP){
        if(cells[getIndexByXY(npcs[index].x,npcs[index].y-9)].resType)
            npcs[index].y=getMod(npcs[index].y-stepNPC,mapDimY);
    }
    if(dir==BOTTOM){
        if(cells[getIndexByXY(npcs[index].x,npcs[index].y+16)].resType)
            npcs[index].y=getMod(npcs[index].y+stepNPC,mapDimY);
    }
    if(dir==LEFT){
        if(cells[getIndexByXY(npcs[index].x-9,npcs[index].y)].resType)
            npcs[index].x=getMod(npcs[index].x-stepNPC,mapDimX);
    }
    if(dir==RIGHT){
        if(cells[getIndexByXY(npcs[index].x+16,npcs[index].y)].resType||playerX/16>=NUMCELLSX-1)
            npcs[index].x=getMod(npcs[index].x+stepNPC,mapDimX);
    }
}
void checkCollision(){
    int tmpxP,
        tmpyP,
        tmpxNP,
        tmpyNP;
    tmpxP=playerX/16;
    tmpyP=playerY/16;
    for(int i=0; i<numNPCS;i++){
        tmpxNP=npcs[i].x/16;
        tmpyNP=npcs[i].y/16;
        if(tmpxNP==tmpxP&&tmpyNP==tmpyP){
            if(npcs[i].isAlive&&npcs[i].isEdible){
                npcs[i].isAlive=false;
                score+=pow(2,combo)*100;
                combo++;
                printf("Pacman smashed NPC number %d\n",i);
            }
            else if(npcs[i].isAlive&&!npcs[i].isEdible){
                killPacman();
                printf("Pacman slayed by NPC number %d\n",i);
            }
        }

    }


}
int getIndexByXY(int x, int y){
    if(playerDirection==BOTTOM||playerDirection==RIGHT){
        return (x/16)+(y/16)*NUMCELLSX;
    }
    return ((x+8)/16)+((y+8)/16)*NUMCELLSX;
}
void checkMapForPoints(){
    int tmpx,
        tmpy,
        type;
    if(playerDirection==BOTTOM||playerDirection==RIGHT){
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
    if(playerDirection==RIGHT){
        if(cells[getIndexByXY(playerX+16,playerY)].resType||playerX/16>=NUMCELLSX-1)
            playerX=getMod(playerX+step,mapDimX);
    }

}
void collectCoin(int index){
    printf("Coin collected!\n");
    score+=10;
    cells[index].resType=PATH;

}
void collectPowerUp(int index){
    printf("Power-up collected!\n");
    for(int i=0; i< numNPCS; i++){
        npcs[i].isEdible=true;
    }
    stepNPC=0.5;
    endingPowerUp=false;
    currentPowerUp = POWERUPDURATION;
    combo=0;
    
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
        npcs[i].direction=BOTTOM;
        npcs[i].isEdible=false;
        npcs[i].x=16*i+(npcStartingX*16-numNPCS*16/2);
        npcs[i].y=npcStartingY*16;
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
    
    cells[20*4].resType=POWERUP;
    int ina=NUMCELLSX*(NUMCELLSY/2);
    int inb=ina+NUMCELLSX;
    cells[ina].resType=PATH;
    cells[ina+NUMCELLSX-1].resType=PATH;
    cells[inb].resType=PATH;
    cells[inb+NUMCELLSX-1].resType=PATH;

    cells[130].resType=BARRIER;
    cells[131].resType=BARRIER;
    cells[132].resType=BARRIER;
    cells[133].resType=BARRIER;
    cells[134].resType=BARRIER;
    
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
    playerDirection=RIGHT;
    
    playerX=pacmanStartingX*16;
    playerY=pacmanStartingY*16;
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
            else if(playerDirection==RIGHT){
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
        playerDirection=RIGHT;
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
void killPacman(){
    pacmanIsAlive=false;
}

