.PHONY:	build

SOURCE	:=	.
CFILES  :=	$(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES  :=	$(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.cpp)))
OFILES  :=	$(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
CFLAGS	:=	-g
CXXFLAGS	:=	-g -std=c++0x

build: main

main: $(OFILES)
	$(CC) $(OFILES) -o main $(CFLAGS) $(LDFLAGS) $(shell pkg-config --libs sdl freetype2 libpng)

sdl.o: sdl.c
	$(CC) $(CFLAGS) -c $^ -o $@ $(shell pkg-config --cflags sdl)

png.o: png.c
	$(CC) $(CFLAGS) -c $^ -o $@ $(shell pkg-config --cflags libpng)

main.o: main.cpp xml.h view.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o $@ $(shell pkg-config --cflags freetype2)

test: main
	./main

clean:
	rm -f *.o
