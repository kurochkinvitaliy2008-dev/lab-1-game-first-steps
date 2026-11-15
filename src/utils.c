#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

void init_console(void) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8); SetConsoleOutputCP(CP_UTF8); SetConsoleTitleW(L"Ферма");
#endif
    srand((unsigned)time(NULL));
}

int read_text(char *text, size_t size) {
#ifdef _WIN32
    wchar_t wide[MAX_INPUT_LEN]; char utf8[MAX_INPUT_LEN * 4]; DWORD count;
    if (!ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), wide, MAX_INPUT_LEN - 1, &count, NULL)) { text[0] = '\0'; return 0; }
    while (count && (wide[count - 1] == L'\n' || wide[count - 1] == L'\r')) count--;
    wide[count] = L'\0';
    int bytes = WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof(utf8), NULL, NULL);
    if (!bytes || (size_t)bytes > size) { text[0] = '\0'; return 0; }
    memcpy(text, utf8, (size_t)bytes); return 1;
#else
    if (!fgets(text, (int)size, stdin)) { text[0] = '\0'; return 0; }
    text[strcspn(text, "\r\n")] = '\0'; return 1;
#endif
}

int read_number(const char *prompt, int *number) {
    char text[MAX_INPUT_LEN], *end; long value;
    printf("%s", prompt);
    if (!read_text(text, sizeof(text))) return 0;
    errno = 0; value = strtol(text, &end, 10);
    if (text == end) return 0;
    while (*end == ' ' || *end == '\t') end++;
    if (*end || errno == ERANGE || value < -2147483647L - 1 || value > 2147483647L) return 0;
    *number = (int)value; return 1;
}

void load_items(char names[ITEM_COUNT][MAX_NAME_LEN]) {
    FILE *f = fopen("data/items.txt", "r"); char line[MAX_ITEM_LINE_LEN]; int id, loaded = 0; char name[MAX_NAME_LEN];
    if (!f) f = fopen("items.txt", "r");
    if (!f) { puts("Файл items.txt не найден. Используются названия по умолчанию."); return; }
    while (fgets(line, sizeof(line), f)) if (sscanf(line, "%d %63s", &id, name) == 2 && id >= 0 && id < ITEM_COUNT) { strcpy(names[id], name); loaded++; }
    fclose(f); puts(loaded ? "Каталог предметов загружен." : "Каталог пуст или имеет неверный формат.");
}

int contains_word_ci(const char *line, const char *word) {
    size_t n = strlen(word); if (!n) return 0;
    for (size_t i = 0; line[i]; i++) { size_t j = 0; while (j < n && line[i + j] && tolower((unsigned char)line[i + j]) == tolower((unsigned char)word[j])) j++; if (j == n) return 1; }
    return 0;
}

void wrap_word(const char *src, const char *word, char *dest, size_t size) {
    size_t i = 0, out = 0, n = strlen(word);
    if (!size) return;
    dest[0] = '\0';
    if (!n) return;
    while (src[i] && out + 1 < size) {
        size_t j = 0; while (j < n && src[i + j] && tolower((unsigned char)src[i + j]) == tolower((unsigned char)word[j])) j++;
        if (j == n && out + n + 2 < size) { dest[out++] = '['; memcpy(dest + out, src + i, n); out += n; dest[out++] = ']'; i += n; }
        else dest[out++] = src[i++];
    }
    dest[out] = '\0';
}

void process_encyclopedia(const char *word) {
    FILE *in = fopen("data/input.txt", "r"), *out; char line[MAX_ARTICLE_LEN]; int row = 0, found = 0;
    if (!in) in = fopen("input.txt", "r");
    if (!in) { puts("Не удалось открыть input.txt."); return; }
    out = fopen("data/output.txt", "w"); if (!out) out = fopen("output.txt", "w");
    if (!out) { puts("Не удалось создать output.txt."); fclose(in); return; }
    while (fgets(line, sizeof(line), in)) { char result[MAX_ARTICLE_LEN]; row++; line[strcspn(line, "\r\n")] = '\0'; if (!contains_word_ci(line, word)) continue; wrap_word(line, word, result, sizeof(result)); printf("Статья %d: %s\n", row, result); fprintf(out, "Статья %d: %s\n", row, result); found++; }
    if (!found) { puts("Статей с таким словом не найдено."); fputs("Статей с таким словом не найдено.\n", out); } else printf("Всего найдено статей: %d\n", found);
    fclose(in); fclose(out); puts("Результат сохранён в output.txt.");
}

void print_time(const GameTime *time) { printf("День %d, %02d:00\n", time->day, time->hour); }
