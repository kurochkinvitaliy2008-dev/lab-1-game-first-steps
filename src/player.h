#ifndef PLAYER_H
#define PLAYER_H

#include "utils.h"

#define BASE_HP 100
#define BASE_STAMINA 50
#define HELMET_HP_BONUS 20
#define ARMOR_HP_BONUS 10
#define ARMOR_STAMINA_BONUS 10
#define BOOTS_STAMINA_BONUS 20

typedef enum { SLOT_HEAD, SLOT_BODY, SLOT_FEET } EquipmentSlot;

typedef struct {
    char name[MAX_NAME_LEN];
    EquipmentSlot slot;
    int hp_bonus;
    int stamina_bonus;
    int equipped;
} Equipment;

typedef struct Player {
    char name[MAX_NAME_LEN];
    int hp, max_hp;
    int stamina, max_stamina;
    int gold;
    int x, y;
    int inventory[INVENTORY_SIZE];
    Equipment equipment[EQUIPMENT_SLOTS];
} Player;

Player *create_player(char names[ITEM_COUNT][MAX_NAME_LEN]);
void free_player(Player **player);
void show_player_card(const Player *player, const GameTime *time, char names[ITEM_COUNT][MAX_NAME_LEN]);
void show_inventory(const Player *player, char names[ITEM_COUNT][MAX_NAME_LEN]);
void manage_inventory(Player *player, int action, char names[ITEM_COUNT][MAX_NAME_LEN]);
void manage_equipment(Player *player);
void write_diary(const Player *player, const GameTime *time, char names[ITEM_COUNT][MAX_NAME_LEN]);
void rest_player(Player *player);

#endif
