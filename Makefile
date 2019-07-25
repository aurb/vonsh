VERSION = 0.0
INC_DIR = inc
SRC_DIR = src
OBJ_DIR = obj
BUILD_DIR = vonsh_$(VERSION)-1
EXE_DIR = $(BUILD_DIR)/usr/games
DOC_DIR = $(BUILD_DIR)/usr/share/doc
DEB = $(BUILD_DIR).deb
EXE = $(EXE_DIR)/vonsh
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CFLAGS = -I$(INC_DIR) -DVERSION_STR=\"$(VERSION)\" -Wall
LDFLAGS =
LDLIBS = -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer
.PHONY: all clean
all: clean release $(DEB)
release: CFLAGS += -O3
release: $(EXE)
debug: CFLAGS += -g
debug: $(EXE)
$(DEB): $(EXE)
	mkdir -p $(DOC_DIR)/vonsh
	cp LICENSE README.md $(DOC_DIR)/vonsh
	dpkg-deb --build $(BUILD_DIR)/
$(EXE): $(OBJ)
	mkdir -p $(EXE_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	$(RM) -rf $(DOC_DIR) $(EXE_DIR) $(OBJ_DIR)
	$(RM) $(DEB)
