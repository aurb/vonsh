DEB = vonsh_0.0-1.deb
PROGRAM = vonsh_0.0-1/usr/games/vonsh
OBJS = vonsh.o text_renderer.o pcg_basic.o
CC = gcc
CFLAGS = -Wall -O3
LDFLAGS = -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer
all: $(DEB)
clean:
	rm $(DEB) $(PROGRAM) $(OBJS) vonsh_0.0-1/usr/share/doc/vonsh/*
.PHONY: all clean
$(DEB): $(PROGRAM)
	cp LICENSE vonsh_0.0-1/usr/share/doc/vonsh
	cp README.md vonsh_0.0-1/usr/share/doc/vonsh
	dpkg-deb --build vonsh_0.0-1/
$(PROGRAM): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^
