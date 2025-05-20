VERSION_STR ?= DEV
INC_DIR = inc
SRC_DIR = src
OBJ_DIR = obj
BUILD_DIR = usr
EXE_DIR = $(BUILD_DIR)/games
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXE = $(EXE_DIR)/vonsh
STRIP ?= strip
CFLAGS ?= -Wall -Wextra -Werror=format-security
CFLAGS += -std=c99 -D_DEFAULT_SOURCE -pedantic -I$(INC_DIR) -DVERSION_STR=\"$(VERSION_STR)\"
LDFLAGS ?= -Wl,-z,relro,-z,now
LDLIBS = -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer -lcjson -lm
.PHONY: all clean deb_build deb_clean
all: release
release: CFLAGS += -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-strong
release: $(EXE)
	$(STRIP) --strip-all $^
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
deb_build: deb_clean
	git archive --format=tar.gz --output=../vonsh_$(UPSTREAM_VERSION_STR).orig.tar.gz v$(UPSTREAM_VERSION_STR) -- . ':!debian'
	mkdir -p $(DEB_BUILD_DIR)
	tar xf ../vonsh_$(UPSTREAM_VERSION_STR).orig.tar.gz -C $(DEB_BUILD_DIR)
	cp -a debian $(DEB_BUILD_DIR)
	cd $(DEB_BUILD_DIR)/ && dpkg-buildpackage
	rm -rf $(DEB_BUILD_DIR)
deb_clean:
	$(eval UPSTREAM_VERSION_STR := $(shell dpkg-parsechangelog -S Version | sed 's/-[^-]*$$//'))
	$(eval DEB_BUILD_DIR := ../vonsh_$(UPSTREAM_VERSION_STR)_deb_build)
	rm -rf $(DEB_BUILD_DIR)
	rm -rf ../vonsh_$(UPSTREAM_VERSION_STR)* ../vonsh-dbgsym_$(UPSTREAM_VERSION_STR)*
