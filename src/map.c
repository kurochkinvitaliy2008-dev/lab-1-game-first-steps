#include "map.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define MIN_MAP_SIZE 5
#define MAX_MAP_SIZE 30
#define MIN_OBSTACLE_PERCENT 15
#define MAX_OBSTACLE_PERCENT 30
#define MIN_TRAP_PERCENT 5
#define MAX_TRAP_PERCENT 10
#define SAFE_PASS_PERCENT 50
#define PERCENT_BASE 100
#define HOURS_PER_DAY 24
#define NIGHT_HOUR 2
#define TILE_WALL '#'
#define TILE_FLOOR '.'
#define TILE_TREE 'T'
#define TILE_ROCK 'R'
#define TILE_DOWN 'D'
#define TILE_UP 'U'
#define TILE_CRACKED ','
#define TRAP_NONE 0
#define TRAP_HIDDEN 1
#define TRAP_EXPOSED 2

static void free_grid(char ***grid, int height) {
    if (!*grid) return;
    for (int y = 0; y < height; y++) free((*grid)[y]);
    free(*grid); *grid = NULL;
}

static char **alloc_chars(int h, int w) {
    char **a = calloc((size_t)h, sizeof(*a)); if (!a) return NULL;
    for (int y = 0; y < h; y++) if (!(a[y] = malloc((size_t)w))) { free_grid(&a, h); return NULL; }
    return a;
}

static unsigned char **alloc_traps(int h, int w) {
    unsigned char **a = calloc((size_t)h, sizeof(*a)); if (!a) return NULL;
    for (int y = 0; y < h; y++) if (!(a[y] = calloc((size_t)w, 1))) { for (int i = 0; i < h; i++) free(a[i]); free(a); return NULL; }
    return a;
}

Map *create_map(void) {
    Map *m = calloc(1, sizeof(*m));
    if (!m) puts("Не удалось выделить память под структуру карты.");
    return m;
}

static void clear_map_data(Map *m) {
    if (!m) return;
    if (m->traps) { for (int y = 0; y < m->height; y++) free(m->traps[y]); free(m->traps); m->traps = NULL; }
    free_grid(&m->cells, m->height); m->width = m->height = 0;
}

void free_map(Map **map) {
    if (!map || !*map) return;
    clear_map_data(*map);
    printf("Память структуры по адресу %p освобождена\n", (void *)*map);
    free(*map); *map = NULL;
}

static int random_floor(const Map *m, int ex, int ey, int *rx, int *ry) {
    int count = 0;
    for (int y = 1; y < m->height - 1; y++) for (int x = 1; x < m->width - 1; x++)
        if (m->cells[y][x] == TILE_FLOOR && m->traps[y][x] == TRAP_NONE && (x != ex || y != ey)) count++;
    if (!count) return 0;
    int selected = rand() % count;
    for (int y = 1; y < m->height - 1; y++) for (int x = 1; x < m->width - 1; x++)
        if (m->cells[y][x] == TILE_FLOOR && m->traps[y][x] == TRAP_NONE && (x != ex || y != ey)) {
            if (!selected--) { *rx = x; *ry = y; return 1; }
        }
    return 0;
}

int generate_map(Map *m, int upstairs, Player *p) {
    if (!m || !p) return 0;
    clear_map_data(m);
    m->height = rand() % (MAX_MAP_SIZE - MIN_MAP_SIZE + 1) + MIN_MAP_SIZE;
    m->width = rand() % (MAX_MAP_SIZE - MIN_MAP_SIZE + 1) + MIN_MAP_SIZE;
    m->cells = alloc_chars(m->height, m->width); m->traps = alloc_traps(m->height, m->width);
    if (!m->cells || !m->traps) { clear_map_data(m); return 0; }
    int obstacles = rand() % (MAX_OBSTACLE_PERCENT - MIN_OBSTACLE_PERCENT + 1) + MIN_OBSTACLE_PERCENT;
    for (int y = 0; y < m->height; y++) for (int x = 0; x < m->width; x++)
        m->cells[y][x] = (!x || !y || x == m->width - 1 || y == m->height - 1) ? TILE_WALL :
            rand() % PERCENT_BASE < obstacles ? (rand() % 2 ? TILE_TREE : TILE_ROCK) : TILE_FLOOR;
    int floors = 0;
    for (int y = 1; y < m->height - 1; y++) for (int x = 1; x < m->width - 1; x++) floors += m->cells[y][x] == TILE_FLOOR;
    while (floors < (upstairs ? 3 : 2)) {
        int done = 0;
        for (int y = 1; y < m->height - 1 && !done; y++) for (int x = 1; x < m->width - 1; x++)
            if (m->cells[y][x] == TILE_TREE || m->cells[y][x] == TILE_ROCK) { m->cells[y][x] = TILE_FLOOR; floors++; done = 1; break; }
        if (!done) { clear_map_data(m); return 0; }
    }
    int dx, dy;
    if (!random_floor(m, -1, -1, &p->x, &p->y) || !random_floor(m, p->x, p->y, &dx, &dy)) { clear_map_data(m); return 0; }
    m->cells[dy][dx] = TILE_DOWN;
    if (upstairs) {
        const int vx[4] = {0, 1, 0, -1}, vy[4] = {-1, 0, 1, 0}; int cx[4], cy[4], n = 0;
        for (int i = 0; i < 4; i++) { int x = p->x + vx[i], y = p->y + vy[i]; if (x > 0 && x < m->width - 1 && y > 0 && y < m->height - 1 && m->cells[y][x] == TILE_FLOOR) { cx[n] = x; cy[n++] = y; } }
        if (n) m->cells[cy[rand() % n]][cx[rand() % n]] = TILE_UP;
    }
    int traps = rand() % (MAX_TRAP_PERCENT - MIN_TRAP_PERCENT + 1) + MIN_TRAP_PERCENT;
    for (int y = 1; y < m->height - 1; y++) for (int x = 1; x < m->width - 1; x++)
        if (m->cells[y][x] == TILE_FLOOR && (x != p->x || y != p->y) && rand() % PERCENT_BASE < traps) m->traps[y][x] = TRAP_HIDDEN;
    return 1;
}

static void draw_map(const Map *m, const Player *p, const GameTime *t) {
    printf("\n===== ПОДЗЕМЕЛЬЕ: ЭТАЖ %d =====\nHP: %d/%d | Выносливость: %d/%d | Золото: %d | Время: %02d:00\n", m->depth, p->hp, p->max_hp, p->stamina, p->max_stamina, p->gold, t->hour);
    for (int y = 0; y < m->height; y++) { for (int x = 0; x < m->width; x++) putchar(x == p->x && y == p->y ? '@' : m->cells[y][x]); putchar('\n'); }
    puts("W/A/S/D - движение, Q - домой на U");
}

static int evacuate(Player *p, Map **m) {
    puts("Герой обессилел. Спасательная служба доставляет его домой.");
    p->gold /= 2; free_map(m); return 0;
}

static int next_floor(Map *m, Player *p, GameTime *t, int upstairs) {
    p->stamina -= 5; t->hour++;
    if (t->hour >= HOURS_PER_DAY) { t->hour = 0; t->day++; }
    if (p->stamina <= 0 || t->hour == NIGHT_HOUR) return 0;
    m->depth++;
    if (!generate_map(m, upstairs, p)) { puts("Не удалось создать новый этаж."); return 0; }
    p->gold += BASE_FLOOR_REWARD * m->depth;
    printf("Новый этаж %d. Получено золота: %d.\n", m->depth, BASE_FLOOR_REWARD * m->depth);
    return 1;
}

static int move_player(Map *m, Player *p, GameTime *t, int dx, int dy) {
    int x = p->x + dx, y = p->y + dy;
    if (x < 0 || x >= m->width || y < 0 || y >= m->height) { puts("За пределами карты пройти нельзя."); return 1; }
    char target = m->cells[y][x];
    if (target == TILE_WALL || target == TILE_TREE || target == TILE_ROCK) { puts("Путь преграждает препятствие."); return 1; }
    p->stamina--;
    if (m->traps[y][x] == TRAP_HIDDEN) { m->traps[y][x] = TRAP_EXPOSED; m->cells[y][x] = TILE_CRACKED; p->x = x; p->y = y; puts("Осторожно, скрипучий пол!"); }
    else if (m->traps[y][x] == TRAP_EXPOSED) {
        if (rand() % PERCENT_BASE < SAFE_PASS_PERCENT) { p->x = x; p->y = y; puts("Пол скрипит! Вы прошли безопасно."); }
        else { puts("Пол проломился! Вы падаете на следующий этаж."); if (!next_floor(m, p, t, 0)) return 0; }
    } else { p->x = x; p->y = y; if (target == TILE_DOWN) { puts("Вы спустились по лестнице."); if (!next_floor(m, p, t, 1)) return 0; } }
    return p->stamina > 0 && t->hour < 2;
}

void run_adventure(Player *p, GameTime *t) {
    if (!p) { puts("Сначала создайте персонажа."); return; }
    if (p->stamina <= 0 || t->hour == NIGHT_HOUR) { puts("Нельзя отправиться в поход: нет сил или уже ночь."); return; }
    Map *m = create_map(); if (!m) return; m->depth = 1;
    if (!generate_map(m, 1, p)) { puts("Не удалось создать подземелье."); free_map(&m); return; }
    puts("Вы отправились в подземелье. Найдите лестницу D.");
    int active = 1;
    while (active) {
        char command[MAX_INPUT_LEN]; draw_map(m, p, t); printf("Ваш ход: ");
        if (!read_text(command, sizeof(command)) || !command[0] || command[1]) { puts("Введите одну клавишу: W, A, S, D или Q."); continue; }
        switch (toupper((unsigned char)command[0])) {
            case 'Q': if (m->cells[p->y][p->x] == TILE_UP) { puts("Вы вернулись домой."); active = 0; } else puts("Домой можно вернуться только с лестницы U."); break;
            case 'W': active = move_player(m, p, t, 0, -1); break;
            case 'A': active = move_player(m, p, t, -1, 0); break;
            case 'S': active = move_player(m, p, t, 0, 1); break;
            case 'D': active = move_player(m, p, t, 1, 0); break;
            default: puts("Неизвестная команда. Используйте W, A, S, D или Q.");
        }
        if (p->stamina <= 0 || t->hour == NIGHT_HOUR) { evacuate(p, &m); active = 0; }
    }
    if (m) free_map(&m);
    puts("Приключение завершено. Вы вернулись в меню фермы.");
}
