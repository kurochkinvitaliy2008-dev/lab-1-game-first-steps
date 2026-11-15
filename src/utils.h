#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define MAX_NAME_LEN 64
#define MAX_INPUT_LEN 256
#define MAX_ARTICLE_LEN 512
#define ITEM_COUNT 10
#define INVENTORY_SIZE 10
#define BASE_FLOOR_REWARD 10
#define MAX_ITEM_LINE_LEN 256
#define HOURS_PER_DAY 24
#define NIGHT_HOUR 2
#define START_DAY 1
#define START_HOUR 8

typedef enum { ITEM_EMPTY, ITEM_WOOD, ITEM_STONE, ITEM_SEEDS, ITEM_SHOVEL, ITEM_RAKE, ITEM_HOE, ITEM_CART, ITEM_WATERING_CAN, ITEM_BASKET } ItemId;
#define EQUIPMENT_SLOTS 3

typedef enum { MENU_EXIT, MENU_CREATE_PLAYER, MENU_CARD, MENU_TIME, MENU_INVENTORY, MENU_PUT_ITEM, MENU_THROW_ITEM, MENU_FAVORITE, MENU_SEARCH_ITEM, MENU_DIARY, MENU_ENCYCLOPEDIA, MENU_ADVENTURE, MENU_DELETE_ID, MENU_EQUIPMENT } MenuOption;

typedef struct {
    int day;
    int hour;
} GameTime;

void init_console(void);
int read_text(char *text, size_t size);
int read_number(const char *prompt, int *number);
void load_items(char names[ITEM_COUNT][MAX_NAME_LEN]);
void process_encyclopedia(const char *word);
int contains_word_ci(const char *line, const char *word);
void wrap_word(const char *src, const char *word, char *dest, size_t size);
void print_time(const GameTime *time);

#endif
