BREW_LIBS = libomp sdl2 sdl2_image sdl2_mixer sdl2_ttf pkg-config
APP_NAME  =pacman
LIBS 	  = -fopenmp -lomp  -lSDL2_Image -lSDL2_ttf -Wall -pedantic
CC 		  =clang
CFLAGS 	  = -Xpreprocessor
PKG 	  = `pkg-config --libs sdl2`



build:
	$(CC) $(CFLAGS) $(LIBS) main.c -o ${APP_NAME} $(PKG)

dependencies:
	brew install $(BREW_LIBS)

run: 
	./$(APP_NAME) $(NUM_THREADS)


