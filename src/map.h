#ifndef MAP_H
#define MAP_H

#include "player.h"

typedef struct Map {
    char **cells;
    unsigned char **traps;
    int width, height, depth;
} Map;

typedef enum { DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_QUIT } Direction;

Map *create_map(void);
void free_map(Map **map);
int generate_map(Map *map, int create_upstairs, Player *player);
void run_adventure(Player *player, GameTime *time);

#endif
