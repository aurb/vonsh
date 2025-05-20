#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <cjson/cJSON.h>
#include <wordexp.h>
#include <time.h>
#include "types.h"
#include "hiscores.h"

static Hiscore hiscores[MAX_HISCORES];
static char hiscore_path[256];

static void get_hiscore_path(void) {
    if (hiscore_path[0] != '\0') {
        return;
    }

    wordexp_t p;
    if (wordexp(USER_SHARE_DIR TOP_SCORES_FILE, &p, 0) == 0) {
        strncpy(hiscore_path, p.we_wordv[0], sizeof(hiscore_path) - 1);
        hiscore_path[sizeof(hiscore_path) - 1] = '\0';
        wordfree(&p);
    }
}


static void ensure_dir_exists(const char *dir_path) {
    char *p = strdup(dir_path);
    if (!p) return;

    char *slash = p;
    if (p[0] == '/') slash++;

    while((slash = strchr(slash, '/'))) {
        *slash = '\0';
        if (mkdir(p, 0755) != 0 && errno != EEXIST) {
            perror("mkdir");
            free(p);
            return;
        }
        *slash = '/';
        slash++;
    }

    if (mkdir(p, 0755) != 0 && errno != EEXIST) {
        perror("mkdir");
    }

    free(p);
}

static void set_default_scores(void) {
    struct tm tm_date = {0};
    tm_date.tm_year = 2000 - 1900;
    tm_date.tm_mon = 0;
    tm_date.tm_mday = 1;
    time_t default_date = mktime(&tm_date);

    for (int i = 0; i < MAX_HISCORES; i++) {
        strcpy(hiscores[i].player_name, "None");
        hiscores[i].score = 0;
        hiscores[i].date = default_date;
        hiscores[i].board_width = 0;
        hiscores[i].board_height = 0;
    }
}

void hiscores_save(void) {
    get_hiscore_path();
    
    char dir_path[256];
    strncpy(dir_path, hiscore_path, sizeof(dir_path)-1);
    dir_path[sizeof(dir_path)-1] = '\0';
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        ensure_dir_exists(dir_path);
    }
    
    FILE *f = fopen(hiscore_path, "w");
    if (!f) {
        perror("Could not open hiscores file for writing");
        return;
    }

    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < MAX_HISCORES; i++) {
        cJSON *score_json = cJSON_CreateObject();
        cJSON_AddStringToObject(score_json, "player_name", hiscores[i].player_name);
        cJSON_AddNumberToObject(score_json, "score", hiscores[i].score);
        
        struct tm *tm_date = localtime(&hiscores[i].date);
        char date_str[11];
        if (tm_date) {
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_date);
        } else {
            strcpy(date_str, "2000-01-01");
        }
        cJSON_AddStringToObject(score_json, "date", date_str);

        char dim_str[10];
        snprintf(dim_str, sizeof(dim_str), "%dx%d", hiscores[i].board_width, hiscores[i].board_height);
        cJSON_AddStringToObject(score_json, "board_dimensions", dim_str);

        cJSON_AddItemToArray(root, score_json);
    }

    char *json_str = cJSON_Print(root);
    fprintf(f, "%s\n", json_str);
    free(json_str);

    cJSON_Delete(root);
    fclose(f);
}

void hiscores_load(void) {
    set_default_scores();

    get_hiscore_path();
    FILE *f = fopen(hiscore_path, "r");
    if (!f) {
        hiscores_save();
        return;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    if ((long)fread(buffer, 1, fsize, f) != fsize) {
        //somehow read has obtained less bytes from hiscores file than previously determined
        //save current hiscores to file and return
        free(buffer);
        hiscores_save();
        return;
    }
    fclose(f);
    buffer[fsize] = 0;

    cJSON *root = cJSON_Parse(buffer);
    if (!root) {
        free(buffer);
        hiscores_save();
        return;
    }

    int i = 0;
    cJSON *score_json;
    cJSON_ArrayForEach(score_json, root) {
        if (i >= MAX_HISCORES) break;

        cJSON *player_name = cJSON_GetObjectItem(score_json, "player_name");
        if (cJSON_IsString(player_name) && (player_name->valuestring != NULL)) {
            strncpy(hiscores[i].player_name, player_name->valuestring, MAX_NAME_LEN);
        }

        cJSON *score = cJSON_GetObjectItem(score_json, "score");
        if (cJSON_IsNumber(score)) {
            hiscores[i].score = score->valueint;
        }

        cJSON *date = cJSON_GetObjectItem(score_json, "date");
        if (cJSON_IsString(date) && (date->valuestring != NULL)) {
            struct tm tm_date = {0};
            sscanf(date->valuestring, "%d-%d-%d", &tm_date.tm_year, &tm_date.tm_mon, &tm_date.tm_mday);
            tm_date.tm_year -= 1900;
            tm_date.tm_mon -= 1;
            hiscores[i].date = mktime(&tm_date);
        }

        cJSON *board_dimensions = cJSON_GetObjectItem(score_json, "board_dimensions");
        if (cJSON_IsString(board_dimensions) && (board_dimensions->valuestring != NULL)) {
            sscanf(board_dimensions->valuestring, "%dx%d", &hiscores[i].board_width, &hiscores[i].board_height);
        }
        
        i++;
    }

    cJSON_Delete(root);
    free(buffer);
}

void hiscores_init(void) {
    hiscores_load();
}

int hiscores_is_highscore(int score) {
    if (score <= 0) return 0;
    return score > hiscores[MAX_HISCORES - 1].score;
}

void hiscores_add(const char *player_name, int score, int board_width, int board_height) {
    int i;
    for (i = MAX_HISCORES - 1; i >= 0; i--) {
        if (score <= hiscores[i].score) {
            break;
        }
    }
    int insert_pos = i + 1;

    if (insert_pos >= MAX_HISCORES) {
        return;
    }

    for (i = MAX_HISCORES - 2; i >= insert_pos; i--) {
        hiscores[i+1] = hiscores[i];
    }

    strncpy(hiscores[insert_pos].player_name, player_name, MAX_NAME_LEN);
    hiscores[insert_pos].player_name[MAX_NAME_LEN] = '\0';
    hiscores[insert_pos].score = score;
    hiscores[insert_pos].date = time(NULL);
    hiscores[insert_pos].board_width = board_width;
    hiscores[insert_pos].board_height = board_height;

    hiscores_save();
}

const Hiscore* hiscores_get_scores(void) {
    return hiscores;
}

void hiscores_clear(void) {
    get_hiscore_path();
    if (hiscore_path[0] != '\0') {
        remove(hiscore_path);
    }
    set_default_scores();
    hiscores_save();
}
