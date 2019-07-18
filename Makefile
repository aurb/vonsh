DEB = vonsh_0.0-1.deb
EXE = vonsh_0.0-1/usr/games/vonsh
SRC_DIR = src
OBJ_DIR = obj
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CFLAGS = -Iinclude -Wall -O3
LDFLAGS =
LDLIBS = -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer
.PHONY: all clean
all: $(EXE) $(DEB)
$(DEB): $(EXE)
	cp LICENSE vonsh_0.0-1/usr/share/doc/vonsh/LICENSE
	cp README.md vonsh_0.0-1/usr/share/doc/vonsh/README.md
	dpkg-deb --build vonsh_0.0-1/
$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	$(RM) $(OBJ) $(EXE) $(DEB)
	$(RM) -r vonsh_0.0-1/usr/share/doc/vonsh/LICENSE
	$(RM) -r vonsh_0.0-1/usr/share/doc/vonsh/README.md
