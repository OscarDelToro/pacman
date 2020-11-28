dependencies:
	brew install libomp sdl2 sdl2_image sdl2_mixer sdl2_ttf pkg-config

build:
	clang -Xpreprocessor -fopenmp -lomp main.c -o main

buildtest:
	clang -Xpreprocessor -fopenmp -lomp  -Wall -pedantic testfile.c -o test `pkg-config --libs sdl2`

runtest:
	./test

run: 
	./main