PROGRAM = vonsh
OBJS = vonsh.o
CC = gcc
CFLAGS = -Wall
LDFLAGS = -lSDL2 -lSDL2main -lSDL2_image
all: $(PROGRAM)
clean:
	rm $(PROGRAM) $(OBJS)
.PHONY: all clean
$(PROGRAM): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^
