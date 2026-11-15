#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void recalculate_stats(Player *p) {
    int hp = BASE_HP, stamina = BASE_STAMINA;
    for (int i = 0; i < EQUIPMENT_SLOTS; i++) if (p->equipment[i].equipped) { hp += p->equipment[i].hp_bonus; stamina += p->equipment[i].stamina_bonus; }
    p->max_hp = hp; p->max_stamina = stamina;
    if (p->hp > hp) p->hp = hp;
    if (p->stamina > stamina) p->stamina = stamina;
}

Player *create_player(char names[ITEM_COUNT][MAX_NAME_LEN]) {
    Player *p = calloc(1, sizeof(*p));
    if (!p) { puts("Не удалось выделить память для персонажа."); return NULL; }
    printf("Введите имя персонажа: ");
    if (!read_text(p->name, sizeof(p->name)) || !p->name[0]) strcpy(p->name, "Фермер");
    p->max_hp = p->hp = BASE_HP; p->max_stamina = p->stamina = BASE_STAMINA; p->gold = 0;
    { const int start[INVENTORY_SIZE] = {ITEM_SHOVEL, ITEM_HOE, ITEM_EMPTY, ITEM_STONE, ITEM_EMPTY, ITEM_SEEDS, ITEM_WATERING_CAN, ITEM_EMPTY, ITEM_WOOD, ITEM_EMPTY}; memcpy(p->inventory, start, sizeof(start)); }
    (void)names;
    puts("Персонаж создан."); return p;
}

void free_player(Player **player) {
    if (!player || !*player) return;
    printf("Память структуры по адресу %p освобождена\n", (void *)*player);
    free(*player); *player = NULL;
}

void show_inventory(const Player *p, char names[ITEM_COUNT][MAX_NAME_LEN]) {
    for (int i = 0; i < INVENTORY_SIZE; i++) printf("Слот %d: ID %d (%s)\n", i, p->inventory[i], names[p->inventory[i]]);
}

void show_player_card(const Player *p, const GameTime *game_time, char names[ITEM_COUNT][MAX_NAME_LEN]) {
    time_t now = time(NULL); struct tm *t = localtime(&now);
    puts("\n===== КАРТОЧКА ПЕРСОНАЖА =====");
    printf("Имя: %s\nЗдоровье: %d/%d\nВыносливость: %d/%d\nЗолото: %d\n", p->name, p->hp, p->max_hp, p->stamina, p->max_stamina, p->gold);
    printf("Игровое время: день %d, %02d:00\n", game_time->day, game_time->hour);
    if (t) printf("Дата: %02d.%02d.%04d %02d:%02d\n", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min);
    puts("Инвентарь:"); show_inventory(p, names);
    puts("Экипировка:"); for (int i = 0; i < EQUIPMENT_SLOTS; i++) printf("%s: %s\n", i == SLOT_HEAD ? "Голова" : i == SLOT_BODY ? "Тело" : "Обувь", p->equipment[i].equipped ? p->equipment[i].name : "пусто");
}

void manage_inventory(Player *p, int action, char names[ITEM_COUNT][MAX_NAME_LEN]) {
    int slot, id;
    if (action == 3) { puts("\n===== ИНВЕНТАРЬ ====="); show_inventory(p, names); return; }
    if (action == 4) {
        if (!read_number("Номер слота (0-9): ", &slot) || slot < 0 || slot >= INVENTORY_SIZE || !read_number("ID предмета (0-9): ", &id) || id < 0 || id >= ITEM_COUNT) { puts("Неверный слот или ID предмета."); return; }
        p->inventory[slot] = id; printf("В слот %d добавлен предмет: %s\n", slot, names[id]); return;
    }
    if (!read_number("Номер слота для очистки (0-9): ", &slot) || slot < 0 || slot >= INVENTORY_SIZE) { puts("Неверный номер слота."); return; }
    printf("Удалён предмет: %s\n", names[p->inventory[slot]]); p->inventory[slot] = 0;
}

void manage_equipment(Player *p) {
    const Equipment gear_catalog[EQUIPMENT_SLOTS] = {
        {"Железный шлем", SLOT_HEAD, HELMET_HP_BONUS, 0, 0},
        {"Кожаная броня", SLOT_BODY, ARMOR_HP_BONUS, ARMOR_STAMINA_BONUS, 0},
        {"Походные сапоги", SLOT_FEET, 0, BOOTS_STAMINA_BONUS, 0}
    };
    int action, slot;
    puts("\n===== СНАРЯЖЕНИЕ =====\n1 - Надеть предмет\n2 - Снять предмет\n0 - Назад");
    if (!read_number("Выбор: ", &action) || action < 0 || action > 2) { puts("Неверный выбор."); return; }
    if (!action) return;
    puts("Слоты: 0 - Голова, 1 - Тело, 2 - Обувь");
    if (!read_number("Слот: ", &slot) || slot < 0 || slot >= EQUIPMENT_SLOTS) { puts("Неверный слот."); return; }
    if (action == 2) {
        if (!p->equipment[slot].equipped) { puts("Этот слот уже пуст."); return; }
        printf("Снято: %s\n", p->equipment[slot].name); p->equipment[slot].equipped = 0;
    } else {
        p->equipment[slot] = gear_catalog[slot]; p->equipment[slot].equipped = 1;
        printf("Надето: %s\n", p->equipment[slot].name);
    }
    recalculate_stats(p);
    printf("Новые характеристики: HP %d/%d, выносливость %d/%d\n", p->hp, p->max_hp, p->stamina, p->max_stamina);
}

void write_diary(const Player *p, const GameTime *game_time, char names[ITEM_COUNT][MAX_NAME_LEN]) {
    FILE *f = fopen("data/diary.txt", "a"); if (!f) f = fopen("diary.txt", "a");
    if (!f) { puts("Не удалось открыть diary.txt."); return; }
    fprintf(f, "===== Дневник фермера =====\nФермер: %s\nДень %d, %02d:00\nИнвентарь:\n", p->name, game_time->day, game_time->hour);
    for (int i = 0; i < INVENTORY_SIZE; i++) fprintf(f, "Слот %d: ID %d (%s)\n", i, p->inventory[i], names[p->inventory[i]]);
    for (int i = 0; i < EQUIPMENT_SLOTS; i++) if (p->equipment[i].equipped) fprintf(f, "Экипировка: %s\n", p->equipment[i].name);
    fputc('\n', f); fclose(f); puts("Состояние сохранено в diary.txt.");
}

void rest_player(Player *p) { p->hp = p->max_hp; p->stamina = p->max_stamina; puts("Персонаж полностью восстановил здоровье и выносливость."); }
