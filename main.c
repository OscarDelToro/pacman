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
#define CELLWIDTH 16
#define MILLISDELAY 20

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
void buildMap();
int countCoins();


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

double mapDimY=CELLWIDTH*NUMCELLSY;
double mapDimX=CELLWIDTH*NUMCELLSX;

double npcStartingX;
double npcStartingY;

double pacmanStartingX=NUMCELLSX/2;
double pacmanStartingY=25;

bool pacmanIsAlive=true;

int score=0;
int combo=0;

    
// Window dimensions
static const int width = CELLWIDTH*NUMCELLSX;
static const int height = CELLWIDTH*NUMCELLSY+34;
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
int remainingCoins;


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
    buildMap();
    remainingCoins=countCoins();

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
    destWindowRect.y=NUMCELLSY*CELLWIDTH;


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
                windowRectCell.y=i*CELLWIDTH;
                for(int j=0; j<NUMCELLSX;j++){
                    textureRectCell.x=mapX[cells[i*NUMCELLSX+j].resType];
                    windowRectCell.x=j*CELLWIDTH;
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
            if(remainingCoins==0){
                text = TTF_RenderText_Solid( font,"Ganaste el juego", white );
            }
            else{
                text = TTF_RenderText_Solid( font,textScore, white );
            }
            
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
                            playerX=pacmanStartingX*CELLWIDTH;
                            playerY=pacmanStartingY*CELLWIDTH;
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
                    msleep(MILLISDELAY);
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
        currentPowerUp-=MILLISDELAY;
        if(currentPowerUp<=POWERUPDURATION/4){
            endingPowerUp=true;
        }
        if(currentPowerUp<=MILLISDELAY){//restore things to before power up
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
        npcs[index].x=round(npcs[index].x/CELLWIDTH)*CELLWIDTH+1;
        break;
    
    case BOTTOM:
        npcs[index].direction=BOTTOM;
        npcs[index].x=round(npcs[index].x/CELLWIDTH)*CELLWIDTH+1;
        break;

    case LEFT:
        npcs[index].direction=LEFT;
        npcs[index].y=round(npcs[index].y/CELLWIDTH)*CELLWIDTH+2;
        
        break;
    
    case RIGHT:
        npcs[index].direction=RIGHT;
        npcs[index].y=round(npcs[index].y/CELLWIDTH)*CELLWIDTH+2;
        
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
        return !(cells[getIndexByXY(npcs[index].x,npcs[index].y+CELLWIDTH)].resType);
        break;
    case LEFT:
        return !(cells[getIndexByXY(npcs[index].x-9,npcs[index].y)].resType);
        break;
    case RIGHT:
        return !(cells[getIndexByXY(npcs[index].x+CELLWIDTH,npcs[index].y)].resType||playerX/CELLWIDTH>=NUMCELLSX-1);
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
                nextDirChange=randomInRange(50,150)*MILLISDELAY;
            }
            if(isInCollision(index)){
                
                do{
                    changeNPCDirection(index,randomInRange(0,3));
                }while(isInCollision(index));
                nextDirChange=randomInRange(50,150)*MILLISDELAY;

            }
            nextDirChange-=MILLISDELAY;
            if(!npcs[index].isAlive){
                delaySpawn-=MILLISDELAY;
                if(delaySpawn<=MILLISDELAY){
                    npcs[index].isAlive=true;
                    delaySpawn=5000;
                }

            }
            moveNPC(index);
            msleep(MILLISDELAY);
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
        if(cells[getIndexByXY(npcs[index].x,npcs[index].y+CELLWIDTH)].resType)
            npcs[index].y=getMod(npcs[index].y+stepNPC,mapDimY);
    }
    if(dir==LEFT){
        if(cells[getIndexByXY(npcs[index].x-9,npcs[index].y)].resType)
            npcs[index].x=getMod(npcs[index].x-stepNPC,mapDimX);
    }
    if(dir==RIGHT){
        if(cells[getIndexByXY(npcs[index].x+CELLWIDTH,npcs[index].y)].resType||playerX/CELLWIDTH>=NUMCELLSX-1)
            npcs[index].x=getMod(npcs[index].x+stepNPC,mapDimX);
    }
}
void checkCollision(){
    int tmpxP,
        tmpyP,
        tmpxNP,
        tmpyNP;
    tmpxP=playerX/CELLWIDTH;
    tmpyP=playerY/CELLWIDTH;
    for(int i=0; i<numNPCS;i++){
        tmpxNP=npcs[i].x/CELLWIDTH;
        tmpyNP=npcs[i].y/CELLWIDTH;
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
        return (x/CELLWIDTH)+(y/CELLWIDTH)*NUMCELLSX;
    }
    return ((x+8)/CELLWIDTH)+((y+8)/CELLWIDTH)*NUMCELLSX;
}
void checkMapForPoints(){
    int tmpx,
        tmpy,
        type;
    if(playerDirection==BOTTOM||playerDirection==RIGHT){
        tmpx=playerX/CELLWIDTH;
        tmpy=playerY/CELLWIDTH;
    }
    else{
        tmpx=(playerX+8)/CELLWIDTH;
        tmpy=(playerY+8)/CELLWIDTH;
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
        if(cells[getIndexByXY(playerX,playerY+CELLWIDTH)].resType)
            playerY=getMod(playerY+step,mapDimY);
    }
    if(playerDirection==LEFT){
        if(cells[getIndexByXY(playerX-9,playerY)].resType)
            playerX=getMod(playerX-step,mapDimX);
    }
    if(playerDirection==RIGHT){
        if(cells[getIndexByXY(playerX+CELLWIDTH,playerY)].resType||playerX/CELLWIDTH>=NUMCELLSX-1)
            playerX=getMod(playerX+step,mapDimX);
    }

}
int countCoins(){
    int res=0;
    for(int i=0; i<NUMCELLSX*NUMCELLSY;i++){
        if(cells[i].resType==COIN){
            res++;
        }
    }
    return res;
}
void collectCoin(int index){
    printf("Coin collected!\n");
    score+=10;
    remainingCoins--;
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
        npcs[i].x=CELLWIDTH*i+(npcStartingX*CELLWIDTH-numNPCS*CELLWIDTH/2);
        npcs[i].y=npcStartingY*CELLWIDTH;
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
    
    int ina=NUMCELLSX*(NUMCELLSY/2);
    int inb=ina+NUMCELLSX;
    cells[ina].resType=PATH;
    cells[ina+NUMCELLSX-1].resType=PATH;
    cells[inb].resType=PATH;
    cells[inb+NUMCELLSX-1].resType=PATH;
    
    windowRectCell.x=0;
    windowRectCell.y=0;
    windowRectCell.w=CELLWIDTH;
    windowRectCell.h=CELLWIDTH;

    textureRectCell.x=0;
    textureRectCell.y=199;
    textureRectCell.w=CELLWIDTH;
    textureRectCell.h=CELLWIDTH;
}
void initPlayerResources(){
    playerDirection=RIGHT;
    
    playerX=pacmanStartingX*CELLWIDTH;
    playerY=pacmanStartingY*CELLWIDTH;
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
        playerY=round(playerY/CELLWIDTH)*CELLWIDTH+2;
    } 
    else if(strcmp(key, "S") == 0) {
        printf("Key S pressed \n");
        playerDirection=BOTTOM;  
        setDirectionPlayerRender();
        playerX=round(playerX/CELLWIDTH)*CELLWIDTH+1;
    }
    else if(strcmp(key, "D") == 0) {
        printf("Key D pressed \n");
        playerDirection=RIGHT;
        setDirectionPlayerRender();
        playerY=round(playerY/CELLWIDTH)*CELLWIDTH+2;
    }
    else if(strcmp(key, "W") == 0) {
        printf("Key W pressed \n");
        playerDirection=TOP;
        setDirectionPlayerRender();
        playerX=round(playerX/CELLWIDTH)*CELLWIDTH+1;
    } 
    else{
        printf("Pressed %s key\n",key);
    }  
}
void killPacman(){
    pacmanIsAlive=false;
}
void buildMap(){
    cells[NUMCELLSX*10+8].resType=BARRIER;
    cells[NUMCELLSX*9+8].resType=BARRIER;
    cells[NUMCELLSX*8+8].resType=BARRIER;
    cells[NUMCELLSX*10+7].resType=BARRIER;
    cells[NUMCELLSX*9+7].resType=BARRIER;
    cells[NUMCELLSX*8+7].resType=BARRIER;
    cells[NUMCELLSX*10+6].resType=BARRIER;
    cells[NUMCELLSX*9+6].resType=BARRIER;
    cells[NUMCELLSX*8+6].resType=BARRIER;
    cells[NUMCELLSX*10+9].resType=BARRIER;
    cells[NUMCELLSX*9+9].resType=BARRIER;
    cells[NUMCELLSX*8+9].resType=BARRIER;

    cells[NUMCELLSX*10+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*9+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*8+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*10+23].resType=BARRIER;
    cells[NUMCELLSX*9+23].resType=BARRIER;
    cells[NUMCELLSX*8+23].resType=BARRIER;
    cells[NUMCELLSX*10+22].resType=BARRIER;
    cells[NUMCELLSX*9+22].resType=BARRIER;
    cells[NUMCELLSX*8+22].resType=BARRIER;
    cells[NUMCELLSX*10+21].resType=BARRIER;
    cells[NUMCELLSX*9+21].resType=BARRIER;
    cells[NUMCELLSX*8+21].resType=BARRIER;

    cells[NUMCELLSX*31+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*30+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*29+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*31+23].resType=BARRIER;
    cells[NUMCELLSX*30+23].resType=BARRIER;
    cells[NUMCELLSX*29+23].resType=BARRIER;
    cells[NUMCELLSX*31+22].resType=BARRIER;
    cells[NUMCELLSX*30+22].resType=BARRIER;
    cells[NUMCELLSX*29+22].resType=BARRIER;
    cells[NUMCELLSX*31+21].resType=BARRIER;
    cells[NUMCELLSX*30+21].resType=BARRIER;
    cells[NUMCELLSX*29+21].resType=BARRIER;

    cells[NUMCELLSX*31+8].resType=BARRIER;
    cells[NUMCELLSX*30+8].resType=BARRIER;
    cells[NUMCELLSX*29+8].resType=BARRIER;
    cells[NUMCELLSX*31+7].resType=BARRIER;
    cells[NUMCELLSX*30+7].resType=BARRIER;
    cells[NUMCELLSX*29+7].resType=BARRIER;
    cells[NUMCELLSX*31+6].resType=BARRIER;
    cells[NUMCELLSX*30+6].resType=BARRIER;
    cells[NUMCELLSX*29+6].resType=BARRIER;
    cells[NUMCELLSX*31+9].resType=BARRIER;
    cells[NUMCELLSX*30+9].resType=BARRIER;
    cells[NUMCELLSX*29+9].resType=BARRIER;

    cells[NUMCELLSX*24+10].resType=BARRIER;
    cells[NUMCELLSX*24+11].resType=BARRIER;
    cells[NUMCELLSX*24+12].resType=BARRIER;
    cells[NUMCELLSX*24+13].resType=BARRIER;
    cells[NUMCELLSX*24+14].resType=BARRIER;
    cells[NUMCELLSX*24+15].resType=BARRIER;
    cells[NUMCELLSX*24+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*24+17].resType=BARRIER;
    cells[NUMCELLSX*24+18].resType=BARRIER;
    cells[NUMCELLSX*24+19].resType=BARRIER;

    cells[NUMCELLSX*26+10].resType=BARRIER;
    cells[NUMCELLSX*26+11].resType=BARRIER;
    cells[NUMCELLSX*26+12].resType=BARRIER;
    cells[NUMCELLSX*26+13].resType=BARRIER;
    cells[NUMCELLSX*26+14].resType=BARRIER;
    cells[NUMCELLSX*26+15].resType=BARRIER;
    cells[NUMCELLSX*26+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*26+17].resType=BARRIER;
    cells[NUMCELLSX*26+18].resType=BARRIER;
    cells[NUMCELLSX*26+19].resType=BARRIER;
    cells[NUMCELLSX*27+14].resType=BARRIER;
    cells[NUMCELLSX*27+15].resType=BARRIER;
    cells[NUMCELLSX*28+14].resType=BARRIER;
    cells[NUMCELLSX*28+15].resType=BARRIER;
    cells[NUMCELLSX*29+14].resType=BARRIER;
    cells[NUMCELLSX*29+15].resType=BARRIER;
    cells[NUMCELLSX*30+14].resType=BARRIER;
    cells[NUMCELLSX*30+15].resType=BARRIER;
    cells[NUMCELLSX*31+14].resType=BARRIER;
    cells[NUMCELLSX*31+15].resType=BARRIER;
    cells[NUMCELLSX*31+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*31+17].resType=BARRIER;
    cells[NUMCELLSX*31+12].resType=BARRIER;
    cells[NUMCELLSX*31+13].resType=BARRIER;

    cells[NUMCELLSX*18+7].resType=BARRIER;
    cells[NUMCELLSX*19+7].resType=BARRIER;
    cells[NUMCELLSX*MILLISDELAY+7].resType=BARRIER;
    cells[NUMCELLSX*21+7].resType=BARRIER;
    cells[NUMCELLSX*21+8].resType=BARRIER;
    cells[NUMCELLSX*21+9].resType=BARRIER;
    cells[NUMCELLSX*21+10].resType=BARRIER;
    cells[NUMCELLSX*21+19].resType=BARRIER;
    cells[NUMCELLSX*21+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*21+21].resType=BARRIER;
    cells[NUMCELLSX*21+22].resType=BARRIER;
    cells[NUMCELLSX*18+22].resType=BARRIER;
    cells[NUMCELLSX*19+22].resType=BARRIER;
    cells[NUMCELLSX*MILLISDELAY+22].resType=BARRIER;

    cells[NUMCELLSX*22+1].resType=BARRIER;
    cells[NUMCELLSX*22+2].resType=BARRIER;
    cells[NUMCELLSX*22+3].resType=BARRIER;
    cells[NUMCELLSX*19+1].resType=BARRIER;
    cells[NUMCELLSX*19+2].resType=BARRIER;
    cells[NUMCELLSX*19+3].resType=BARRIER;

    cells[NUMCELLSX*22+26].resType=BARRIER;
    cells[NUMCELLSX*22+27].resType=BARRIER;
    cells[NUMCELLSX*22+28].resType=BARRIER;
    cells[NUMCELLSX*19+26].resType=BARRIER;
    cells[NUMCELLSX*19+27].resType=BARRIER;
    cells[NUMCELLSX*19+28].resType=BARRIER;


    cells[NUMCELLSX*25+1].resType=BARRIER;
    cells[NUMCELLSX*25+2].resType=BARRIER;
    cells[NUMCELLSX*25+3].resType=BARRIER;

    cells[NUMCELLSX*25+6].resType=BARRIER;
    cells[NUMCELLSX*25+7].resType=BARRIER;
    cells[NUMCELLSX*25+5].resType=BARRIER;
    cells[NUMCELLSX*24+6].resType=BARRIER;
    cells[NUMCELLSX*26+6].resType=BARRIER;

    cells[NUMCELLSX*25+23].resType=BARRIER;
    cells[NUMCELLSX*25+22].resType=BARRIER;
    cells[NUMCELLSX*25+24].resType=BARRIER;
    cells[NUMCELLSX*24+23].resType=BARRIER;
    cells[NUMCELLSX*26+23].resType=BARRIER;

    cells[NUMCELLSX*28+1].resType=BARRIER;
    cells[NUMCELLSX*28+2].resType=BARRIER;
    cells[NUMCELLSX*28+3].resType=BARRIER;

    cells[NUMCELLSX*31+1].resType=BARRIER;
    cells[NUMCELLSX*31+2].resType=BARRIER;
    cells[NUMCELLSX*31+3].resType=BARRIER;

    cells[NUMCELLSX*34+1].resType=BARRIER;
    cells[NUMCELLSX*34+2].resType=BARRIER;
    cells[NUMCELLSX*34+3].resType=BARRIER;

    cells[NUMCELLSX*25+26].resType=BARRIER;
    cells[NUMCELLSX*25+27].resType=BARRIER;
    cells[NUMCELLSX*25+28].resType=BARRIER;

    cells[NUMCELLSX*28+26].resType=BARRIER;
    cells[NUMCELLSX*28+27].resType=BARRIER;
    cells[NUMCELLSX*28+28].resType=BARRIER;

    cells[NUMCELLSX*31+26].resType=BARRIER;
    cells[NUMCELLSX*31+27].resType=BARRIER;
    cells[NUMCELLSX*31+28].resType=BARRIER;

    cells[NUMCELLSX*34+26].resType=BARRIER;
    cells[NUMCELLSX*34+27].resType=BARRIER;
    cells[NUMCELLSX*34+28].resType=BARRIER;

    cells[NUMCELLSX*35+8].resType=BARRIER;
    cells[NUMCELLSX*36+8].resType=BARRIER;
    cells[NUMCELLSX*37+8].resType=BARRIER;
    cells[NUMCELLSX*34+8].resType=BARRIER;
    cells[NUMCELLSX*34+9].resType=BARRIER;
    cells[NUMCELLSX*34+10].resType=BARRIER;
    cells[NUMCELLSX*34+11].resType=BARRIER;
    cells[NUMCELLSX*34+12].resType=BARRIER;
    cells[NUMCELLSX*34+13].resType=BARRIER;
    cells[NUMCELLSX*34+14].resType=BARRIER;
    cells[NUMCELLSX*34+15].resType=BARRIER;
    cells[NUMCELLSX*34+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*34+17].resType=BARRIER;
    cells[NUMCELLSX*34+18].resType=BARRIER;
    cells[NUMCELLSX*34+19].resType=BARRIER;
    cells[NUMCELLSX*34+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*34+21].resType=BARRIER;
    cells[NUMCELLSX*35+21].resType=BARRIER;
    cells[NUMCELLSX*36+21].resType=BARRIER;
    cells[NUMCELLSX*37+21].resType=BARRIER;
    cells[NUMCELLSX*37+8].resType=BARRIER;
    cells[NUMCELLSX*37+9].resType=BARRIER;
    cells[NUMCELLSX*37+10].resType=BARRIER;
    cells[NUMCELLSX*37+11].resType=BARRIER;
    cells[NUMCELLSX*37+12].resType=BARRIER;
    cells[NUMCELLSX*37+13].resType=BARRIER;
    cells[NUMCELLSX*37+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*37+17].resType=BARRIER;
    cells[NUMCELLSX*37+18].resType=BARRIER;
    cells[NUMCELLSX*37+19].resType=BARRIER;
    cells[NUMCELLSX*37+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*37+21].resType=BARRIER;

    cells[NUMCELLSX*36+26].resType=BARRIER;
    cells[NUMCELLSX*36+25].resType=BARRIER;
    cells[NUMCELLSX*36+24].resType=BARRIER;
    cells[NUMCELLSX*37+26].resType=BARRIER;
    cells[NUMCELLSX*37+25].resType=BARRIER;
    cells[NUMCELLSX*37+24].resType=BARRIER;

    cells[NUMCELLSX*36+3].resType=BARRIER;
    cells[NUMCELLSX*36+5].resType=BARRIER;
    cells[NUMCELLSX*36+4].resType=BARRIER;
    cells[NUMCELLSX*37+3].resType=BARRIER;
    cells[NUMCELLSX*37+5].resType=BARRIER;
    cells[NUMCELLSX*37+4].resType=BARRIER;
    
    cells[NUMCELLSX*33+6].resType=BARRIER;

    cells[NUMCELLSX*33+23].resType=BARRIER;


    cells[NUMCELLSX*13+14].resType=BARRIER;
    cells[NUMCELLSX*13+15].resType=BARRIER;
    cells[NUMCELLSX*12+14].resType=BARRIER;
    cells[NUMCELLSX*12+15].resType=BARRIER;
    cells[NUMCELLSX*11+14].resType=BARRIER;
    cells[NUMCELLSX*11+15].resType=BARRIER;
    cells[NUMCELLSX*10+14].resType=BARRIER;
    cells[NUMCELLSX*10+15].resType=BARRIER;
    cells[NUMCELLSX*9+14].resType=BARRIER;
    cells[NUMCELLSX*9+15].resType=BARRIER;
    cells[NUMCELLSX*8+14].resType=BARRIER;
    cells[NUMCELLSX*8+15].resType=BARRIER;
    cells[NUMCELLSX*7+14].resType=BARRIER;
    cells[NUMCELLSX*7+15].resType=BARRIER;
    cells[NUMCELLSX*6+14].resType=BARRIER;
    cells[NUMCELLSX*6+15].resType=BARRIER;
    cells[NUMCELLSX*5+14].resType=BARRIER;
    cells[NUMCELLSX*5+15].resType=BARRIER;
    cells[NUMCELLSX*4+14].resType=BARRIER;
    cells[NUMCELLSX*4+15].resType=BARRIER;

    cells[NUMCELLSX*4+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*4+17].resType=BARRIER;
    cells[NUMCELLSX*4+18].resType=BARRIER;
    cells[NUMCELLSX*4+19].resType=BARRIER;
    cells[NUMCELLSX*4+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*4+21].resType=BARRIER;
    cells[NUMCELLSX*4+22].resType=BARRIER;
    cells[NUMCELLSX*4+23].resType=BARRIER;
    cells[NUMCELLSX*3+23].resType=BARRIER;
    cells[NUMCELLSX*2+23].resType=BARRIER;
    cells[NUMCELLSX*2+24].resType=BARRIER;
    cells[NUMCELLSX*2+25].resType=BARRIER;
    cells[NUMCELLSX*2+26].resType=BARRIER;

    cells[NUMCELLSX*4+13].resType=BARRIER;
    cells[NUMCELLSX*4+12].resType=BARRIER;
    cells[NUMCELLSX*4+11].resType=BARRIER;
    cells[NUMCELLSX*4+10].resType=BARRIER;
    cells[NUMCELLSX*4+9].resType=BARRIER;
    cells[NUMCELLSX*4+8].resType=BARRIER;
    cells[NUMCELLSX*4+7].resType=BARRIER;
    cells[NUMCELLSX*4+6].resType=BARRIER;
    cells[NUMCELLSX*3+6].resType=BARRIER;
    cells[NUMCELLSX*2+6].resType=BARRIER;
    cells[NUMCELLSX*2+5].resType=BARRIER;
    cells[NUMCELLSX*2+4].resType=BARRIER;
    cells[NUMCELLSX*2+3].resType=BARRIER;

    cells[NUMCELLSX*17+2].resType=BARRIER;
    cells[NUMCELLSX*17+3].resType=BARRIER;
    cells[NUMCELLSX*17+4].resType=BARRIER;

    cells[NUMCELLSX*15+1].resType=BARRIER;
    cells[NUMCELLSX*15+2].resType=BARRIER;
    cells[NUMCELLSX*15+3].resType=BARRIER;

    cells[NUMCELLSX*13+2].resType=BARRIER;
    cells[NUMCELLSX*13+3].resType=BARRIER;
    cells[NUMCELLSX*13+4].resType=BARRIER;
    
    cells[NUMCELLSX*11+1].resType=BARRIER;
    cells[NUMCELLSX*11+2].resType=BARRIER;
    cells[NUMCELLSX*11+3].resType=BARRIER;
    
    cells[NUMCELLSX*9+2].resType=BARRIER;
    cells[NUMCELLSX*9+3].resType=BARRIER;
    cells[NUMCELLSX*9+4].resType=BARRIER;

    cells[NUMCELLSX*7+1].resType=BARRIER;
    cells[NUMCELLSX*7+2].resType=BARRIER;
    cells[NUMCELLSX*7+3].resType=BARRIER;


    cells[NUMCELLSX*17+27].resType=BARRIER;
    cells[NUMCELLSX*17+26].resType=BARRIER;
    cells[NUMCELLSX*17+25].resType=BARRIER;

    cells[NUMCELLSX*15+28].resType=BARRIER;
    cells[NUMCELLSX*15+27].resType=BARRIER;
    cells[NUMCELLSX*15+26].resType=BARRIER;

    cells[NUMCELLSX*13+27].resType=BARRIER;
    cells[NUMCELLSX*13+26].resType=BARRIER;
    cells[NUMCELLSX*13+25].resType=BARRIER;
    
    cells[NUMCELLSX*11+28].resType=BARRIER;
    cells[NUMCELLSX*11+27].resType=BARRIER;
    cells[NUMCELLSX*11+26].resType=BARRIER;
    
    cells[NUMCELLSX*9+27].resType=BARRIER;
    cells[NUMCELLSX*9+26].resType=BARRIER;
    cells[NUMCELLSX*9+25].resType=BARRIER;

    cells[NUMCELLSX*7+28].resType=BARRIER;
    cells[NUMCELLSX*7+27].resType=BARRIER;
    cells[NUMCELLSX*7+26].resType=BARRIER;

    cells[NUMCELLSX*5+27].resType=BARRIER;
    cells[NUMCELLSX*5+26].resType=BARRIER;
    cells[NUMCELLSX*5+25].resType=BARRIER;
    cells[NUMCELLSX*4+27].resType=BARRIER;
    cells[NUMCELLSX*4+26].resType=BARRIER;
    cells[NUMCELLSX*4+25].resType=BARRIER;

    cells[NUMCELLSX*5+2].resType=BARRIER;
    cells[NUMCELLSX*5+3].resType=BARRIER;
    cells[NUMCELLSX*5+4].resType=BARRIER;
    cells[NUMCELLSX*4+2].resType=BARRIER;
    cells[NUMCELLSX*4+3].resType=BARRIER;
    cells[NUMCELLSX*4+4].resType=BARRIER;

    cells[NUMCELLSX*6+6].resType=BARRIER;
    cells[NUMCELLSX*6+7].resType=BARRIER;
    cells[NUMCELLSX*6+8].resType=BARRIER;
    cells[NUMCELLSX*6+9].resType=BARRIER;
    cells[NUMCELLSX*6+10].resType=BARRIER;
    cells[NUMCELLSX*6+11].resType=BARRIER;
    cells[NUMCELLSX*6+12].resType=BARRIER;


    cells[NUMCELLSX*7+12].resType=BARRIER;
    cells[NUMCELLSX*8+12].resType=BARRIER;
    cells[NUMCELLSX*9+12].resType=BARRIER;
    cells[NUMCELLSX*10+12].resType=BARRIER;
    cells[NUMCELLSX*11+12].resType=BARRIER;
    cells[NUMCELLSX*12+12].resType=BARRIER;
    cells[NUMCELLSX*13+12].resType=BARRIER;

    
    cells[NUMCELLSX*6+23].resType=BARRIER;
    cells[NUMCELLSX*6+22].resType=BARRIER;
    cells[NUMCELLSX*6+21].resType=BARRIER;
    cells[NUMCELLSX*6+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*6+19].resType=BARRIER;
    cells[NUMCELLSX*6+18].resType=BARRIER;
    cells[NUMCELLSX*6+17].resType=BARRIER;
    
    cells[NUMCELLSX*7+17].resType=BARRIER;
    cells[NUMCELLSX*8+17].resType=BARRIER;
    cells[NUMCELLSX*9+17].resType=BARRIER;
    cells[NUMCELLSX*10+17].resType=BARRIER;
    cells[NUMCELLSX*11+17].resType=BARRIER;
    cells[NUMCELLSX*12+17].resType=BARRIER;
    cells[NUMCELLSX*13+17].resType=BARRIER;

    cells[NUMCELLSX*2+8].resType=BARRIER;
    cells[NUMCELLSX*2+9].resType=BARRIER;
    cells[NUMCELLSX*2+10].resType=BARRIER;
    cells[NUMCELLSX*2+11].resType=BARRIER;
    cells[NUMCELLSX*2+12].resType=BARRIER;
    cells[NUMCELLSX*2+13].resType=BARRIER;
    cells[NUMCELLSX*2+14].resType=BARRIER;
    cells[NUMCELLSX*2+15].resType=BARRIER;
    cells[NUMCELLSX*2+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*2+17].resType=BARRIER;
    cells[NUMCELLSX*2+18].resType=BARRIER;
    cells[NUMCELLSX*2+19].resType=BARRIER;
    cells[NUMCELLSX*2+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*2+21].resType=BARRIER;

    cells[NUMCELLSX*13+6].resType=BARRIER;
    cells[NUMCELLSX*13+7].resType=BARRIER;
    cells[NUMCELLSX*13+8].resType=BARRIER;
    cells[NUMCELLSX*13+9].resType=BARRIER;
    cells[NUMCELLSX*12+6].resType=BARRIER;
    cells[NUMCELLSX*12+7].resType=BARRIER;
    cells[NUMCELLSX*12+8].resType=BARRIER;
    cells[NUMCELLSX*12+9].resType=BARRIER;

    cells[NUMCELLSX*13+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*13+21].resType=BARRIER;
    cells[NUMCELLSX*13+22].resType=BARRIER;
    cells[NUMCELLSX*13+23].resType=BARRIER;
    cells[NUMCELLSX*12+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*12+21].resType=BARRIER;
    cells[NUMCELLSX*12+22].resType=BARRIER;
    cells[NUMCELLSX*12+23].resType=BARRIER;

    cells[NUMCELLSX*15+8].resType=BARRIER;
    cells[NUMCELLSX*15+9].resType=BARRIER;
    cells[NUMCELLSX*15+10].resType=BARRIER;
    cells[NUMCELLSX*15+11].resType=BARRIER;
    cells[NUMCELLSX*15+12].resType=BARRIER;
    cells[NUMCELLSX*15+13].resType=BARRIER;
    cells[NUMCELLSX*15+14].resType=BARRIER;
    cells[NUMCELLSX*15+15].resType=BARRIER;
    cells[NUMCELLSX*15+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*15+17].resType=BARRIER;
    cells[NUMCELLSX*15+18].resType=BARRIER;
    cells[NUMCELLSX*15+19].resType=BARRIER;
    cells[NUMCELLSX*15+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*15+21].resType=BARRIER;
    
    
    cells[NUMCELLSX*17+13].resType=BARRIER;
    cells[NUMCELLSX*17+14].resType=BARRIER;
    cells[NUMCELLSX*17+15].resType=BARRIER;
    cells[NUMCELLSX*17+CELLWIDTH].resType=BARRIER;
    cells[NUMCELLSX*18+13].resType=BARRIER;
    cells[NUMCELLSX*18+14].resType=BARRIER;
    cells[NUMCELLSX*18+15].resType=BARRIER;
    cells[NUMCELLSX*18+CELLWIDTH].resType=BARRIER;

    cells[NUMCELLSX*17+9].resType=BARRIER;
    cells[NUMCELLSX*17+10].resType=BARRIER;
    cells[NUMCELLSX*17+11].resType=BARRIER;
    cells[NUMCELLSX*18+9].resType=BARRIER;
    cells[NUMCELLSX*18+10].resType=BARRIER;
    cells[NUMCELLSX*18+11].resType=BARRIER;

    cells[NUMCELLSX*17+18].resType=BARRIER;
    cells[NUMCELLSX*17+19].resType=BARRIER;
    cells[NUMCELLSX*17+MILLISDELAY].resType=BARRIER;
    cells[NUMCELLSX*18+18].resType=BARRIER;
    cells[NUMCELLSX*18+19].resType=BARRIER;
    cells[NUMCELLSX*18+MILLISDELAY].resType=BARRIER;

    cells[NUMCELLSX*1+1].resType=POWERUP;
    cells[NUMCELLSX*38+1].resType=POWERUP;
    cells[NUMCELLSX*1+28].resType=POWERUP;
    cells[NUMCELLSX*38+28].resType=POWERUP;

    cells[NUMCELLSX*17+12].resType=POWERUP;
    cells[NUMCELLSX*17+17].resType=POWERUP;


}
