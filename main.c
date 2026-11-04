#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <wchar.h>
#include <errno.h>
#include <time.h>

#define INVENTORY_SIZE 10
#define ITEM_COUNT 10
#define MAX_NAME_LEN 64
#define MAX_INPUT_LEN 256
#define MAX_ARTICLE_LEN 512
#define MAX_ITEM_LINE_LEN 256

#define MIN_MAP_SIZE 5
#define MAX_MAP_SIZE 30
#define MIN_OBSTACLE_PERCENT 15
#define MAX_OBSTACLE_PERCENT 30
#define MIN_TRAP_PERCENT 5
#define MAX_TRAP_PERCENT 10
#define SAFE_PASS_PERCENT 50
#define PERCENT_BASE 100

#define TILE_WALL '#'
#define TILE_FLOOR '.'
#define TILE_TREE 'T'
#define TILE_ROCK 'R'
#define TILE_DOWN_STAIR 'D'
#define TILE_UP_STAIR 'U'
#define TILE_CRACKED ','

#define TRAP_NONE 0
#define TRAP_HIDDEN 1
#define TRAP_EXPOSED 2

typedef struct
{
    char **cells;
    unsigned char **traps;
    int width;
    int height;
    int player_x;
    int player_y;
    int depth;
} Dungeon;

/* Чтение текста с поддержкой русского ввода в Windows */
int read_text(char *text, int size)
{
    wchar_t wide_text[MAX_INPUT_LEN];
    char utf8_text[MAX_INPUT_LEN * 4];
    DWORD count;

    if (!ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE),
                      wide_text, MAX_INPUT_LEN - 1, &count, NULL))
    {
        text[0] = '\0';
        return 0;
    }

    while (count > 0 &&
           (wide_text[count - 1] == L'\n' ||
            wide_text[count - 1] == L'\r'))
        count--;

    wide_text[count] = L'\0';

    int bytes = WideCharToMultiByte(
        CP_UTF8, 0, wide_text, -1,
        utf8_text, sizeof(utf8_text), NULL, NULL);

    if (bytes == 0 || bytes > size)
    {
        text[0] = '\0';
        return 0;
    }

    memcpy(text, utf8_text, bytes);
    return 1;
}

/* Чтение и проверка числа */
int read_number(const char *prompt, int *number)
{
    char text[MAX_INPUT_LEN];
    char *end;
    long value;

    printf("%s", prompt);

    if (!read_text(text, sizeof(text)))
        return 0;

    errno = 0;
    value = strtol(text, &end, 10);

    if (text == end)
        return 0;

    while (*end == ' ' || *end == '\t')
        end++;

    if (*end != '\0' || errno == ERANGE ||
        value < -2147483647L - 1 || value > 2147483647L)
        return 0;

    *number = (int)value;
    return 1;
}

/* Загрузка каталога из items.txt */
void load_items(char names[ITEM_COUNT][MAX_NAME_LEN])
{
    FILE *file = fopen("items.txt", "r");

    if (file == NULL)
    {
        printf("Файл items.txt не найден. Используются названия по умолчанию.\n");
        return;
    }

    char line[MAX_ITEM_LINE_LEN];
    int loaded = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        int id;
        char name[MAX_NAME_LEN];

        if (sscanf(line, "%d %63s", &id, name) == 2 &&
            id >= 0 && id < ITEM_COUNT)
        {
            strcpy(names[id], name);
            loaded++;
        }
    }

    fclose(file);

    if (loaded > 0)
        printf("Каталог предметов загружен.\n");
    else
        printf("Каталог пуст или имеет неверный формат.\n");
}

/* Вывод инвентаря */
void show_inventory(const int inventory[INVENTORY_SIZE],
                    char names[ITEM_COUNT][MAX_NAME_LEN])
{
    for (int i = 0; i < INVENTORY_SIZE; i++)
        printf("Слот %d: ID %d (%s)\n",
               i, inventory[i], names[inventory[i]]);
}

/* Запись состояния фермы в дневник */
void write_diary(const char *farmer, int day, int hour,
                 const int inventory[INVENTORY_SIZE],
                 char names[ITEM_COUNT][MAX_NAME_LEN])
{
    FILE *file = fopen("diary.txt", "a");

    if (file == NULL)
    {
        printf("Не удалось открыть diary.txt.\n");
        return;
    }

    fprintf(file, "===== Дневник фермера =====\n");
    fprintf(file, "Фермер: %s\n", farmer);
    fprintf(file, "День %d, %02d:00\n", day, hour);
    fprintf(file, "Инвентарь:\n");

    for (int i = 0; i < INVENTORY_SIZE; i++)
        fprintf(file, "Слот %d: ID %d (%s)\n",
                i, inventory[i], names[inventory[i]]);

    fprintf(file, "\n");
    fclose(file);

    printf("Состояние сохранено в diary.txt.\n");
}

/* Проверка наличия слова без учёта регистра английских букв */
int contains_word_ci(const char *line, const char *word)
{
    size_t length = strlen(word);

    if (length == 0)
        return 0;

    for (int i = 0; line[i] != '\0'; i++)
    {
        size_t j = 0;

        while (j < length && line[i + j] != '\0' &&
               tolower((unsigned char)line[i + j]) ==
               tolower((unsigned char)word[j]))
            j++;

        if (j == length)
            return 1;
    }

    return 0;
}

/* Выделение найденных слов квадратными скобками */
void wrap_word(const char *src, const char *word,
               char *dest, int size)
{
    int out = 0;
    size_t length = strlen(word);

    if (length == 0 || size <= 0)
    {
        if (size > 0)
            dest[0] = '\0';
        return;
    }

    for (int i = 0; src[i] != '\0' && out < size - 1; )
    {
        size_t j = 0;

        while (j < length && src[i + j] != '\0' &&
               tolower((unsigned char)src[i + j]) ==
               tolower((unsigned char)word[j]))
            j++;

        if (j == length)
        {
            dest[out++] = '[';

            for (j = 0; j < length && out < size - 1; j++)
                dest[out++] = src[i + j];

            if (out < size - 1)
                dest[out++] = ']';

            i += (int)length;
        }
        else
            dest[out++] = src[i++];
    }

    dest[out] = '\0';
}

/* Поиск слова в энциклопедии */
void process_encyclopedia(const char *word)
{
    FILE *in = fopen("input.txt", "r");

    if (in == NULL)
    {
        printf("Не удалось открыть input.txt.\n");
        return;
    }

    FILE *out = fopen("output.txt", "w");

    if (out == NULL)
    {
        printf("Не удалось создать output.txt.\n");
        fclose(in);
        return;
    }

    char line[MAX_ARTICLE_LEN];
    int line_number = 0;
    int found = 0;

    while (fgets(line, sizeof(line), in) != NULL)
    {
        line_number++;

        line[strcspn(line, "\r\n")] = '\0';

        if (!contains_word_ci(line, word))
            continue;

        char result[MAX_ARTICLE_LEN];
        wrap_word(line, word, result, sizeof(result));

        printf("Статья %d: %s\n", line_number, result);
        fprintf(out, "Статья %d: %s\n", line_number, result);
        found++;
    }

    if (found == 0)
    {
        printf("Статей с таким словом не найдено.\n");
        fprintf(out, "Статей с таким словом не найдено.\n");
    }
    else
        printf("Всего найдено статей: %d\n", found);

    fclose(in);
    fclose(out);

    printf("Результат сохранён в output.txt.\n");
}

/* Выделение динамической двумерной карты */
char **allocate_char_matrix(int height, int width)
{
    char **matrix = calloc((size_t)height, sizeof(char *));

    if (matrix == NULL)
        return NULL;

    for (int y = 0; y < height; y++)
    {
        matrix[y] = malloc((size_t)width * sizeof(char));

        if (matrix[y] == NULL)
        {
            for (int i = height - 1; i >= 0; i--)
                free(matrix[i]);

            free(matrix);
            return NULL;
        }
    }

    return matrix;
}

/* Выделение динамической матрицы состояний ловушек */
unsigned char **allocate_trap_matrix(int height, int width)
{
    unsigned char **matrix = calloc((size_t)height, sizeof(unsigned char *));

    if (matrix == NULL)
        return NULL;

    for (int y = 0; y < height; y++)
    {
        matrix[y] = calloc((size_t)width, sizeof(unsigned char));

        if (matrix[y] == NULL)
        {
            for (int i = height - 1; i >= 0; i--)
                free(matrix[i]);

            free(matrix);
            return NULL;
        }
    }

    return matrix;
}

/* Освобождение обеих динамических матриц */
void free_dungeon(Dungeon *dungeon)
{
    if (dungeon == NULL)
        return;

    if (dungeon->traps != NULL)
    {
        for (int y = dungeon->height - 1; y >= 0; y--)
            free(dungeon->traps[y]);

        free(dungeon->traps);
        dungeon->traps = NULL;
    }

    if (dungeon->cells != NULL)
    {
        for (int y = dungeon->height - 1; y >= 0; y--)
            free(dungeon->cells[y]);

        free(dungeon->cells);
        dungeon->cells = NULL;
    }

    dungeon->width = 0;
    dungeon->height = 0;
    dungeon->player_x = 0;
    dungeon->player_y = 0;
}

/* Подсчёт обычных свободных клеток */
int count_free_cells(const Dungeon *dungeon)
{
    int count = 0;

    for (int y = 1; y < dungeon->height - 1; y++)
        for (int x = 1; x < dungeon->width - 1; x++)
            if (dungeon->cells[y][x] == TILE_FLOOR)
                count++;

    return count;
}

/* Выбор случайной свободной клетки, кроме указанной позиции */
int find_random_floor(const Dungeon *dungeon, int excluded_x,
                      int excluded_y, int *result_x, int *result_y)
{
    int count = 0;

    for (int y = 1; y < dungeon->height - 1; y++)
    {
        for (int x = 1; x < dungeon->width - 1; x++)
        {
            if (dungeon->cells[y][x] == TILE_FLOOR &&
                dungeon->traps[y][x] == TRAP_NONE &&
                !(x == excluded_x && y == excluded_y))
                count++;
        }
    }

    if (count == 0)
        return 0;

    int selected = rand() % count;

    for (int y = 1; y < dungeon->height - 1; y++)
    {
        for (int x = 1; x < dungeon->width - 1; x++)
        {
            if (dungeon->cells[y][x] == TILE_FLOOR &&
                dungeon->traps[y][x] == TRAP_NONE &&
                !(x == excluded_x && y == excluded_y))
            {
                if (selected == 0)
                {
                    *result_x = x;
                    *result_y = y;
                    return 1;
                }

                selected--;
            }
        }
    }

    return 0;
}

/* Размещение лестницы наверх рядом с героем */
void place_upstairs_near_player(Dungeon *dungeon)
{
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};
    int candidate_x[4];
    int candidate_y[4];
    int candidate_count = 0;

    for (int i = 0; i < 4; i++)
    {
        int x = dungeon->player_x + dx[i];
        int y = dungeon->player_y + dy[i];

        if (x <= 0 || x >= dungeon->width - 1 ||
            y <= 0 || y >= dungeon->height - 1)
            continue;

        if (dungeon->cells[y][x] == TILE_DOWN_STAIR)
            continue;

        if (dungeon->cells[y][x] == TILE_FLOOR)
        {
            candidate_x[candidate_count] = x;
            candidate_y[candidate_count] = y;
            candidate_count++;
        }
    }

    if (candidate_count == 0)
    {
        for (int i = 0; i < 4; i++)
        {
            int x = dungeon->player_x + dx[i];
            int y = dungeon->player_y + dy[i];

            if (x > 0 && x < dungeon->width - 1 &&
                y > 0 && y < dungeon->height - 1 &&
                dungeon->cells[y][x] != TILE_DOWN_STAIR)
            {
                candidate_x[candidate_count] = x;
                candidate_y[candidate_count] = y;
                candidate_count++;
            }
        }
    }

    if (candidate_count > 0)
    {
        int selected = rand() % candidate_count;
        int x = candidate_x[selected];
        int y = candidate_y[selected];

        dungeon->cells[y][x] = TILE_UP_STAIR;
        dungeon->traps[y][x] = TRAP_NONE;
    }
}

/* Генерация нового этажа подземелья */
int generate_dungeon(Dungeon *dungeon, int create_upstairs)
{
    if (dungeon == NULL)
        return 0;

    free_dungeon(dungeon);

    dungeon->height = rand() % (MAX_MAP_SIZE - MIN_MAP_SIZE + 1) + MIN_MAP_SIZE;
    dungeon->width = rand() % (MAX_MAP_SIZE - MIN_MAP_SIZE + 1) + MIN_MAP_SIZE;

    dungeon->cells = allocate_char_matrix(dungeon->height, dungeon->width);
    dungeon->traps = allocate_trap_matrix(dungeon->height, dungeon->width);

    if (dungeon->cells == NULL || dungeon->traps == NULL)
    {
        free_dungeon(dungeon);
        return 0;
    }

    int obstacle_percent = rand() %
        (MAX_OBSTACLE_PERCENT - MIN_OBSTACLE_PERCENT + 1) +
        MIN_OBSTACLE_PERCENT;

    for (int y = 0; y < dungeon->height; y++)
    {
        for (int x = 0; x < dungeon->width; x++)
        {
            if (y == 0 || y == dungeon->height - 1 ||
                x == 0 || x == dungeon->width - 1)
                dungeon->cells[y][x] = TILE_WALL;
            else if (rand() % PERCENT_BASE < obstacle_percent)
                dungeon->cells[y][x] = (rand() % 2 == 0) ? TILE_TREE : TILE_ROCK;
            else
                dungeon->cells[y][x] = TILE_FLOOR;
        }
    }

    int required_floor = create_upstairs ? 3 : 2;

    while (count_free_cells(dungeon) < required_floor)
    {
        int cleared = 0;

        for (int y = 1; y < dungeon->height - 1 && !cleared; y++)
        {
            for (int x = 1; x < dungeon->width - 1; x++)
            {
                if (dungeon->cells[y][x] == TILE_TREE ||
                    dungeon->cells[y][x] == TILE_ROCK)
                {
                    dungeon->cells[y][x] = TILE_FLOOR;
                    cleared = 1;
                    break;
                }
            }
        }

        if (!cleared)
        {
            free_dungeon(dungeon);
            return 0;
        }
    }

    if (!find_random_floor(dungeon, -1, -1,
                           &dungeon->player_x, &dungeon->player_y))
    {
        free_dungeon(dungeon);
        return 0;
    }

    int down_x;
    int down_y;

    if (!find_random_floor(dungeon, dungeon->player_x, dungeon->player_y,
                           &down_x, &down_y))
    {
        free_dungeon(dungeon);
        return 0;
    }

    dungeon->cells[down_y][down_x] = TILE_DOWN_STAIR;

    if (create_upstairs)
        place_upstairs_near_player(dungeon);

    int trap_percent = rand() %
        (MAX_TRAP_PERCENT - MIN_TRAP_PERCENT + 1) + MIN_TRAP_PERCENT;

    for (int y = 1; y < dungeon->height - 1; y++)
    {
        for (int x = 1; x < dungeon->width - 1; x++)
        {
            if (dungeon->cells[y][x] == TILE_FLOOR &&
                !(x == dungeon->player_x && y == dungeon->player_y) &&
                rand() % PERCENT_BASE < trap_percent)
                dungeon->traps[y][x] = TRAP_HIDDEN;
        }
    }

    return 1;
}

/* Отображение карты и положения героя */
void print_dungeon(const Dungeon *dungeon)
{
    printf("\n===== ПОДЗЕМЕЛЬЕ: ЭТАЖ %d =====\n", dungeon->depth);
    printf("Размер карты: %d x %d\n", dungeon->width, dungeon->height);

    for (int y = 0; y < dungeon->height; y++)
    {
        for (int x = 0; x < dungeon->width; x++)
        {
            if (x == dungeon->player_x && y == dungeon->player_y)
                putchar('@');
            else
                putchar(dungeon->cells[y][x]);
        }

        putchar('\n');
    }

    printf("W/A/S/D - движение, Q - домой (только на U)\n");
}

/* Создание следующего этажа с освобождением предыдущего */
int go_to_next_floor(Dungeon *dungeon, int create_upstairs)
{
    dungeon->depth++;

    if (!generate_dungeon(dungeon, create_upstairs))
    {
        printf("Не удалось выделить память для новой карты.\n");
        return 0;
    }

    printf("Сгенерирован этаж %d.\n", dungeon->depth);
    return 1;
}

/* Попытка переместить героя на соседнюю клетку */
int move_in_dungeon(Dungeon *dungeon, int dx, int dy)
{
    int new_x = dungeon->player_x + dx;
    int new_y = dungeon->player_y + dy;

    if (new_x < 0 || new_x >= dungeon->width ||
        new_y < 0 || new_y >= dungeon->height)
    {
        printf("За пределами карты пройти нельзя.\n");
        return 1;
    }

    char target = dungeon->cells[new_y][new_x];

    if (target == TILE_WALL || target == TILE_TREE || target == TILE_ROCK)
    {
        printf("Путь преграждает препятствие.\n");
        return 1;
    }

    if (dungeon->traps[new_y][new_x] == TRAP_HIDDEN)
    {
        dungeon->traps[new_y][new_x] = TRAP_EXPOSED;
        dungeon->cells[new_y][new_x] = TILE_CRACKED;
        dungeon->player_x = new_x;
        dungeon->player_y = new_y;
        printf("Осторожно, скрипучий пол!\n");
        return 1;
    }

    if (dungeon->traps[new_y][new_x] == TRAP_EXPOSED)
    {
        if (rand() % PERCENT_BASE < SAFE_PASS_PERCENT)
        {
            dungeon->player_x = new_x;
            dungeon->player_y = new_y;
            printf("Пол скрипит! Вы прошли безопасно.\n");
            return 1;
        }

        printf("Пол с треском проломился! Вы падаете на следующий этаж.\n");
        return go_to_next_floor(dungeon, 0);
    }

    dungeon->player_x = new_x;
    dungeon->player_y = new_y;

    if (target == TILE_DOWN_STAIR)
    {
        printf("Вы спустились по лестнице вниз.\n");
        return go_to_next_floor(dungeon, 1);
    }

    return 1;
}

/* Игровой цикл приключения */
void run_adventure(void)
{
    Dungeon dungeon = {0};
    dungeon.depth = 1;

    if (!generate_dungeon(&dungeon, 1))
    {
        printf("Не удалось создать подземелье.\n");
        free_dungeon(&dungeon);
        return;
    }

    printf("Вы отправились в подземелье. Найдите лестницу D.\n");

    int adventure_active = 1;

    while (adventure_active)
    {
        char command[MAX_INPUT_LEN];
        print_dungeon(&dungeon);
        printf("Ваш ход: ");

        if (!read_text(command, sizeof(command)) || command[0] == '\0')
        {
            printf("Введите одну клавишу: W, A, S, D или Q.\n");
            continue;
        }

        if (command[1] != '\0')
        {
            printf("Введите только одну клавишу.\n");
            continue;
        }

        char key = (char)toupper((unsigned char)command[0]);

        if (key == 'Q')
        {
            if (dungeon.cells[dungeon.player_y][dungeon.player_x] == TILE_UP_STAIR)
            {
                printf("Вы нашли путь домой!\n");
                adventure_active = 0;
            }
            else
                printf("Домой можно вернуться только с лестницы U.\n");

            continue;
        }

        switch (key)
        {
            case 'W':
                adventure_active = move_in_dungeon(&dungeon, 0, -1);
                break;

            case 'A':
                adventure_active = move_in_dungeon(&dungeon, -1, 0);
                break;

            case 'S':
                adventure_active = move_in_dungeon(&dungeon, 0, 1);
                break;

            case 'D':
                adventure_active = move_in_dungeon(&dungeon, 1, 0);
                break;

            default:
                printf("Неизвестная команда. Используйте W, A, S, D или Q.\n");
        }
    }

    free_dungeon(&dungeon);
    printf("Приключение завершено. Вы вернулись в меню фермы.\n");
}

int main(void)
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Ферма");
    srand((unsigned int)time(NULL));

    int day = 1;
    int hour = 8;

    int inventory[INVENTORY_SIZE] = {4, 6, 0, 2, 0, 3, 8, 0, 1, 0};

    char names[ITEM_COUNT][MAX_NAME_LEN] = {
        "Пусто", "Дерево", "Камень", "Семена", "Лопата",
        "Грабли", "Тяпка", "Телега", "Лейка", "Корзина"
    };

    char farmer[MAX_NAME_LEN];

    load_items(names);

    printf("Введите имя фермера: ");

    if (!read_text(farmer, sizeof(farmer)) || farmer[0] == '\0')
        strcpy(farmer, "Фермер");

    printf("Добро пожаловать на ферму, %s!\n", farmer);

    while (1)
    {
        int choice;

        printf("\n===== ФЕРМА =====\n");
        printf("Фермер: %s\n", farmer);
        printf("0 - Выход\n1 - Текущее время\n2 - Промотать время\n");
        printf("3 - Инвентарь\n4 - Положить предмет\n5 - Выбросить предмет\n");
        printf("6 - Любимый ресурс\n7 - Поиск предмета по названию\n");
        printf("8 - Записать состояние в дневник\n");
        printf("9 - Поиск в энциклопедии\n10 - Отправиться в приключение\n");
        printf("11 - Удалить предметы по ID\n");

        if (!read_number("Выберите действие: ", &choice))
        {
            printf("Нужно ввести число.\n");
            continue;
        }

        switch (choice)
        {
            case 0:
                printf("Программа завершена.\n");
                return 0;

            case 1:
                printf("Сейчас день %d, %02d:00\n", day, hour);
                break;

            case 2:
            {
                int hours;

                if (!read_number("На сколько часов перемотать время? ", &hours))
                {
                    printf("Нужно ввести число.\n");
                    break;
                }

                if (hours < 0)
                {
                    printf("Количество часов не может быть отрицательным.\n");
                    break;
                }

                hour += hours;

                while (hour >= 24)
                {
                    hour -= 24;
                    day++;
                }

                printf("Время перемотано на %d часов.\n", hours);
                break;
            }

            case 3:
                printf("\n===== ИНВЕНТАРЬ =====\n");
                show_inventory(inventory, names);
                break;

            case 4:
            {
                int slot, id;

                if (!read_number("Номер слота (0-9): ", &slot) ||
                    slot < 0 || slot >= INVENTORY_SIZE)
                {
                    printf("Неверный номер слота.\n");
                    break;
                }

                if (!read_number("ID предмета (0-9): ", &id) ||
                    id < 0 || id >= ITEM_COUNT)
                {
                    printf("Неверный ID предмета.\n");
                    break;
                }

                inventory[slot] = id;
                printf("В слот %d добавлен предмет: %s\n", slot, names[id]);
                break;
            }

            case 5:
            {
                int slot;

                if (!read_number("Номер слота для очистки (0-9): ", &slot) ||
                    slot < 0 || slot >= INVENTORY_SIZE)
                {
                    printf("Неверный номер слота.\n");
                    break;
                }

                printf("Удалён предмет: %s\n", names[inventory[slot]]);
                inventory[slot] = 0;
                break;
            }

            case 6:
            {
                int favorite = 0;
                int max_count = 0;

                for (int i = 0; i < INVENTORY_SIZE; i++)
                {
                    if (inventory[i] == 0)
                        continue;

                    int count = 0;

                    for (int j = 0; j < INVENTORY_SIZE; j++)
                        if (inventory[j] == inventory[i])
                            count++;

                    if (count > max_count)
                    {
                        max_count = count;
                        favorite = inventory[i];
                    }
                }

                if (favorite == 0)
                    printf("В инвентаре нет предметов.\n");
                else if (max_count == 1)
                    printf("Все предметы встречаются по одному разу.\n");
                else
                {
                    printf("Любимый ресурс: %s (ID %d)\n",
                           names[favorite], favorite);
                    printf("Количество: %d\n", max_count);
                }

                break;
            }

            case 7:
            {
                char search[MAX_NAME_LEN];
                int id = -1;
                int found = 0;

                printf("Введите название предмета: ");

                if (!read_text(search, sizeof(search)))
                {
                    printf("Ошибка чтения названия.\n");
                    break;
                }

                for (int i = 0; i < ITEM_COUNT; i++)
                {
                    if (strcmp(names[i], search) == 0)
                    {
                        id = i;
                        break;
                    }
                }

                if (id == -1)
                {
                    printf("Такого предмета нет в каталоге.\n");
                    break;
                }

                printf("Предмет \"%s\": ", names[id]);

                for (int i = 0; i < INVENTORY_SIZE; i++)
                {
                    if (inventory[i] == id)
                    {
                        printf("слот %d ", i);
                        found = 1;
                    }
                }

                if (!found)
                    printf("в инвентаре отсутствует");

                printf("\n");
                break;
            }

            case 8:
                write_diary(farmer, day, hour, inventory, names);
                break;

            case 9:
            {
                char word[MAX_NAME_LEN];

                printf("Введите поисковое слово: ");

                if (!read_text(word, sizeof(word)) || word[0] == '\0')
                {
                    printf("Поисковое слово не должно быть пустым.\n");
                    break;
                }

                process_encyclopedia(word);
                break;
            }

            case 10:
                run_adventure();
                break;

            case 11:
            {
                int id;
                int cleared = 0;

                if (!read_number("ID предмета для удаления: ", &id) ||
                    id < 1 || id >= ITEM_COUNT)
                {
                    printf("Неверный ID предмета.\n");
                    break;
                }

                for (int i = 0; i < INVENTORY_SIZE; i++)
                {
                    if (inventory[i] == id)
                    {
                        inventory[i] = 0;
                        cleared++;
                    }
                }

                printf("Очищено слотов: %d\n", cleared);
                break;
            }

            default:
                printf("Такого пункта меню нет.\n");
        }
    }
}
