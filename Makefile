PROGRAM = vonsh
OBJS = vonsh.o text_renderer.o
CC = gcc
CFLAGS = -Wall -O3
LDFLAGS = -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer
all: $(PROGRAM)
clean:
	rm $(PROGRAM) $(OBJS)
.PHONY: all clean
$(PROGRAM): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^
