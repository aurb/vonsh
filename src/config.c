#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <wordexp.h>
#include "types.h"
#include "error_handling.h"
#include "config.h"
#include "audio.h"

/* ===== Configuration persistence implementation ===== */
static void expand_path(const char *in_path, char *out_path, size_t out_size) {
    wordexp_t p;
    if (wordexp(in_path, &p, 0) == 0) {
        strncpy(out_path, p.we_wordv[0], out_size - 1);
        out_path[out_size - 1] = '\0';
        wordfree(&p);
    }
}

static void save_config_to_path(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "window_board_w", g_game.window_board_w);
    cJSON_AddNumberToObject(root, "window_board_h", g_game.window_board_h);
    cJSON_AddBoolToObject(root, "fullscreen", g_game.fullscreen);
    cJSON_AddBoolToObject(root, "music_on", g_game.music_on);
    cJSON_AddBoolToObject(root, "sfx_on", g_game.sfx_on);
    cJSON_AddStringToObject(root, "key_left", SDL_GetKeyName(g_game.key_left));
    cJSON_AddStringToObject(root, "key_right", SDL_GetKeyName(g_game.key_right));
    cJSON_AddStringToObject(root, "key_up", SDL_GetKeyName(g_game.key_up));
    cJSON_AddStringToObject(root, "key_down", SDL_GetKeyName(g_game.key_down));
    cJSON_AddStringToObject(root, "key_pause", SDL_GetKeyName(g_game.key_pause));

    char *json_str = cJSON_Print(root);
    fprintf(f, "%s\n", json_str);
    free(json_str);

    cJSON_Delete(root);
    fclose(f);
}

static void load_config_from_path(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 64 * 1024) { fclose(f); return; }
    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return; }
    buf[sz] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        free(buf);
        return;
    }

    cJSON *window_board_w = cJSON_GetObjectItem(root, "window_board_w");
    if (cJSON_IsNumber(window_board_w) && window_board_w->valueint >= BOARD_MIN_WIDTH) {
        g_game.window_board_w = window_board_w->valueint;
    }
    else {
        g_game.window_board_w = BOARD_MIN_WIDTH;
    }

    cJSON *window_board_h = cJSON_GetObjectItem(root, "window_board_h");
    if (cJSON_IsNumber(window_board_h) && window_board_h->valueint >= BOARD_MIN_HEIGHT) {
        g_game.window_board_h = window_board_h->valueint;
    }
    else {
        g_game.window_board_h = BOARD_MIN_HEIGHT;
    }

    cJSON *fullscreen = cJSON_GetObjectItem(root, "fullscreen");
    if (cJSON_IsBool(fullscreen)) {
        g_game.fullscreen = cJSON_IsTrue(fullscreen);
    }

    cJSON *music_on = cJSON_GetObjectItem(root, "music_on");
    if (cJSON_IsBool(music_on)) {
        g_game.music_on = cJSON_IsTrue(music_on);
    }

    cJSON *sfx_on = cJSON_GetObjectItem(root, "sfx_on");
    if (cJSON_IsBool(sfx_on)) {
        g_game.sfx_on = cJSON_IsTrue(sfx_on);
    }

    const char* keys[] = {"key_left", "key_right", "key_up", "key_down", "key_pause"};
    SDL_KeyCode* key_vars[] = {&g_game.key_left, &g_game.key_right, &g_game.key_up, &g_game.key_down, &g_game.key_pause};

    for (int i = 0; i < (int)(sizeof(keys)/sizeof(keys[0])); i++) {
        cJSON *key_item = cJSON_GetObjectItem(root, keys[i]);
        if (cJSON_IsString(key_item) && (key_item->valuestring != NULL)) {
            SDL_KeyCode key_code = SDL_GetKeyFromName(key_item->valuestring);
            if (key_code != SDLK_UNKNOWN) {
                *key_vars[i] = key_code;
            }
        }
    }

    cJSON_Delete(root);
    free(buf);
}

void ensure_user_config_exists(void) {
    char user_conf_dir[PATH_MAX];
    char user_conf_path[PATH_MAX];
    expand_path(USER_SHARE_DIR, user_conf_dir, sizeof(user_conf_dir));
    expand_path(USER_SHARE_DIR USER_CONFIG_FILE, user_conf_path, sizeof(user_conf_path));

    struct stat st;

    /* ensure user config directory exists */
    if (stat(user_conf_dir, &st) != 0) {
        mkdir(user_conf_dir, 0755);
    }

    /* ensure user config file exists */
    if (stat(user_conf_path, &st) != 0) {
        //user config file does not exist, so need to copy from initial config
        /* try to copy from initial config in RES_DIR */
        char init_conf_path[PATH_MAX];
        //build path to initial config file
        expand_path(RES_DIR INIT_CONFIG_FILE, init_conf_path, sizeof(init_conf_path));
        //open initial config file
        FILE *init_conf_file = fopen(init_conf_path, "r");
        if (init_conf_file) {
            //open user config file for writing
            FILE *user_conf_file = fopen(user_conf_path, "w");
            if (user_conf_file) {
                //copy initial config to user config file
                char buf[4096];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), init_conf_file)) > 0) {
                    fwrite(buf, 1, n, user_conf_file);
                }
                fclose(user_conf_file);
            }
            else {
                set_error("Error opening user config file %s.", user_conf_path);
            }
            fclose(init_conf_file);
        } else {
            set_error("Initial config file %s not found.", init_conf_path);
        }
    }
    else {
        //user config file exists, so no need to write defaults
        return;
    }
}

void save_user_config(void) {
    char path[PATH_MAX];
    expand_path(USER_SHARE_DIR USER_CONFIG_FILE, path, sizeof(path));
    save_config_to_path(path);
}
     
void load_user_config(void) {
    char path[PATH_MAX];
    expand_path(USER_SHARE_DIR USER_CONFIG_FILE, path, sizeof(path));
    load_config_from_path(path);
}