dependencies:
	brew install libomp sdl2 sdl2_image sdl2_mixer sdl2_ttf pkg-config

build:
	clang -Xpreprocessor -fopenmp -lomp main.c -o main

buildtest:
	clang -Xpreprocessor -fopenmp -lomp  -lSDL2_Image -Wall -pedantic testfile.c -o test `pkg-config --libs sdl2`

runtest:
	./test

run: 
	./main

buildgraph:
	clang  -Wall -pedantic -lSDL2_Image graphtest.c -o testgraph `pkg-config --libs sdl2`

rungra:
	./testgraph
