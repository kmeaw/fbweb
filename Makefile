.PHONY:	build

SOURCE	:=	.
CFILES  :=	$(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES  :=	$(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.cpp)))
OFILES  :=	$(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
CFLAGS	:=	-g
CXXFLAGS	:=	-g

build: main

main: $(OFILES)
	$(CC) $(OFILES) -o main $(CFLAGS) $(LDFLAGS) $(shell pkg-config --libs sdl)

sdl.o: sdl.c
	$(CC) $(CFLAGS) -c $^ -o $@ $(shell pkg-config --cflags sdl)

test: main
	./main

clean:
	rm -f *.o
