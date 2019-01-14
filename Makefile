ifndef VERSION_STR
	VERSION_STR = DEV
endif
INC_DIR = inc
SRC_DIR = src
OBJ_DIR = obj
BUILD_DIR = usr
EXE_DIR = $(BUILD_DIR)/games
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXE = $(EXE_DIR)/vonsh
CFLAGS = -I$(INC_DIR) -DVERSION_STR=\"$(VERSION_STR)\" -Wall -Wformat -Werror=format-security
LDFLAGS = -Wl,-z,relro,-z,now
LDLIBS = -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer
.PHONY: all clean install
all: release
release: CFLAGS += -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong
release: $(EXE)
	strip --strip-all $^
debug: CFLAGS += -g
debug: $(EXE)
$(EXE): $(OBJ)
	mkdir -p $(EXE_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	$(RM) -rf $(EXE_DIR) $(OBJ_DIR)
install:
	cp -R usr $(DESTDIR)/
	mkdir -p $(DESTDIR)/usr/share/doc/vonsh
	cp README.md $(DESTDIR)/usr/share/doc/vonsh/
