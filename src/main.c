#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "map.h"
#include "utils.h"

static void show_menu(const Player *p) {
    puts("\n===== ФЕРМА =====");
    printf("Персонаж: %s\n", p ? p->name : "не создан");
    puts("0 - Выход\n1 - Создать / пересоздать персонажа\n2 - Карточка персонажа\n3 - Управление временем\n4 - Инвентарь\n5 - Положить предмет\n6 - Выбросить предмет\n7 - Любимый ресурс\n8 - Поиск предмета по названию\n9 - Записать состояние в дневник\n10 - Поиск в энциклопедии\n11 - Отправиться в приключение\n12 - Удалить предметы по ID\n13 - Управление снаряжением");
}

static int require_player(const Player *p) {
    if (p) return 1;
    puts("Сначала создайте персонажа в пункте 1."); return 0;
}

static void time_menu(Player *p, GameTime *time) {
    int choice;
    puts("\n===== ВРЕМЯ =====\n1 - Посмотреть на часы\n2 - Пропустить время\n3 - Отдых\n0 - Назад");
    if (!read_number("Выбор: ", &choice)) { puts("Нужно ввести число."); return; }
    switch (choice) {
        case 1: print_time(time); break;
        case 2: {
            int hours, available = time->hour == NIGHT_HOUR ? 0 : time->hour > 2 ? 26 - time->hour : 2 - time->hour;
            if (available <= 0) { puts("Уже 02:00. Нужно лечь спать."); break; }
            if (!read_number("Сколько часов ждать? ", &hours) || hours < 1 || hours > available) { printf("Можно пропустить от 1 до %d ч., не переходя за 02:00.\n", available); break; }
            time->hour += hours;
            if (time->hour >= HOURS_PER_DAY) { time->hour -= HOURS_PER_DAY; time->day++; }
            printf("Прошло %d ч. ", hours); print_time(time); break;
        }
        case 3: time->day++; time->hour = 8; if (p) rest_player(p); else puts("Персонаж не создан. Время переведено на утро."); print_time(time); break;
        case 0: break;
        default: puts("Такого пункта нет.");
    }
}

static void favorite_resource(const Player *p, char names[ITEM_COUNT][MAX_NAME_LEN]) {
    int best = 0, max = 0;
    for (int i = 1; i < ITEM_COUNT; i++) { int count = 0; for (int j = 0; j < INVENTORY_SIZE; j++) count += p->inventory[j] == i; if (count > max) { max = count; best = i; } }
    if (!max) puts("В инвентаре нет предметов.");
    else if (max == 1) puts("Все предметы встречаются по одному разу.");
    else printf("Любимый ресурс: %s (ID %d), количество: %d\n", names[best], best, max);
}

static void search_inventory(const Player *p, char names[ITEM_COUNT][MAX_NAME_LEN]) {
    char search[MAX_NAME_LEN]; int id = -1, found = 0;
    printf("Введите название предмета: ");
    if (!read_text(search, sizeof(search))) { puts("Ошибка чтения названия."); return; }
    for (int i = 0; i < ITEM_COUNT; i++) if (!strcmp(names[i], search)) { id = i; break; }
    if (id < 0) { puts("Такого предмета нет в каталоге."); return; }
    printf("Предмет \"%s\": ", names[id]);
    for (int i = 0; i < INVENTORY_SIZE; i++) if (p->inventory[i] == id) { printf("слот %d ", i); found = 1; }
    if (!found) printf("в инвентаре отсутствует");
    putchar('\n');
}

static void delete_by_id(Player *p) {
    int id, count = 0;
    if (!read_number("ID предмета для удаления (1-9): ", &id) || id <= ITEM_EMPTY || id >= ITEM_COUNT) { puts("Неверный ID предмета."); return; }
    for (int i = 0; i < INVENTORY_SIZE; i++) if (p->inventory[i] == id) { p->inventory[i] = 0; count++; }
    printf("Очищено слотов: %d\n", count);
}

int main(void) {
    char names[ITEM_COUNT][MAX_NAME_LEN] = {"Пусто", "Дерево", "Камень", "Семена", "Лопата", "Грабли", "Тяпка", "Телега", "Лейка", "Корзина"};
    Player *player = NULL; GameTime time = {START_DAY, START_HOUR}; int choice;
    init_console(); load_items(names);
    while (1) {
        show_menu(player);
        if (!read_number("Выберите действие: ", &choice)) { puts("Нужно ввести число."); continue; }
        switch ((MenuOption)choice) {
            case MENU_EXIT: free_player(&player); puts("Программа завершена."); return 0;
            case MENU_CREATE_PLAYER:
                if (player) { char answer[8]; printf("Удалить текущего персонажа и создать нового? (да/нет): "); if (!read_text(answer, sizeof(answer)) || (strcmp(answer, "да") && strcmp(answer, "Да") && strcmp(answer, "ДА"))) { puts("Пересоздание отменено."); break; } free_player(&player); }
                player = create_player(names); break;
            case MENU_CARD: if (require_player(player)) show_player_card(player, &time, names); break;
            case MENU_TIME: time_menu(player, &time); break;
            case MENU_INVENTORY: case MENU_PUT_ITEM: case MENU_THROW_ITEM:
                if (require_player(player)) manage_inventory(player, choice == MENU_INVENTORY ? 3 : choice == MENU_PUT_ITEM ? 4 : 5, names);
                break;
            case MENU_FAVORITE: if (require_player(player)) favorite_resource(player, names); break;
            case MENU_SEARCH_ITEM: if (require_player(player)) search_inventory(player, names); break;
            case MENU_DIARY: if (require_player(player)) write_diary(player, &time, names); break;
            case MENU_ENCYCLOPEDIA: { char word[MAX_NAME_LEN]; printf("Введите поисковое слово: "); if (!read_text(word, sizeof(word)) || !word[0]) puts("Поисковое слово не должно быть пустым."); else process_encyclopedia(word); break; }
            case MENU_ADVENTURE: if (require_player(player)) run_adventure(player, &time); break;
            case MENU_DELETE_ID: if (require_player(player)) delete_by_id(player); break;
            case MENU_EQUIPMENT: if (require_player(player)) manage_equipment(player); break;
            default: puts("Такого пункта меню нет.");
        }
    }
}
