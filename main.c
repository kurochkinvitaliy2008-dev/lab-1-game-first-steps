
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <wchar.h>
#include <errno.h>

#define INVENTORY_SIZE 10
#define ITEM_COUNT 10
#define MAX_NAME_LEN 64
#define MAX_INPUT_LEN 256
#define MAX_ARTICLE_LEN 512

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

    char line[256];
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

int main(void)
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Ферма");

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
        printf("9 - Поиск в энциклопедии\n10 - Удалить предметы по ID\n");

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
