#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_PATH 1024

// Структура для хранения информации о файле
typedef struct {
    char file_name[MAX_PATH];
    long file_size;
} FileHeader;

// // Рекурсивная функция для обхода директории
// void archive_directory(FILE *archive, const char *dir_path, const char *base_path) {
//     DIR *dir = opendir(dir_path);
//     if (!dir) {
//         perror("Ошибка открытия директории");
//         return;
//     }
//
//     struct dirent *entry;
//     while ((entry = readdir(dir)) != NULL) {
//         // Пропускаем "." и ".."
//         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
//             continue;
//         }
//
//         char full_path[MAX_PATH];
//         snprintf(full_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);
//
//         struct stat path_stat;
//         stat(full_path, &path_stat);
//
//         // Если это файл, архивируем его
//         if (S_ISREG(path_stat.st_mode)) {
//             archive_file(archive, full_path, base_path);
//         }
//
//         // Если это директория, рекурсивно обрабатываем её
//         if (S_ISDIR(path_stat.st_mode)) {
//             archive_directory(archive, full_path, base_path);
//         }
//     }
//
//     closedir(dir);
// }

// Функция для записи файла в архив
void write_file_to_archive(FILE *archive, const char *file_path, const char *base_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Ошибка при открытии файла");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Запись заголовка
    FileHeader header;

    // Подготовка заголовка с относительным путем к файлу
    snprintf(header.file_name, MAX_PATH, "%s", file_path + strlen(base_path) + 1); // Относительный путь
    header.file_size = file_size;
    printf("Имя файлаааа%s\n", header.file_name);
    // strncpy(header.file_name, file_path, MAX_PATH);
    // header.file_size = file_size;
    fwrite(&header, sizeof(FileHeader), 1, archive);
    printf("%s", header.file_name);
    printf("\n");
    printf("File size: %ld\n", header.file_size);

    // Копирование содержимого файла
    char *buffer = (char *)malloc(file_size);
    fread(buffer, 1, file_size, file);
    fwrite(buffer, 1, file_size, archive);

    free(buffer);
    fclose(file);
}

// Рекурсивный обход директории и архивация файлов
void archive_directory(FILE *archive, const char *dir_path, const char* base_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Ошибка при открытии директории");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH];

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        printf("%s\n", dir_path);

        struct stat path_stat;
        stat(full_path, &path_stat);

        if (S_ISREG(path_stat.st_mode)) {
            // Это файл, архивируем его
            write_file_to_archive(archive, full_path, base_path);
        } else if (S_ISDIR(path_stat.st_mode)) {
            // Это директория, рекурсивно обходим ее
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                archive_directory(archive, full_path, base_path);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Использование: %s <папка> <имя архива>\n", argv[0]);
        return 1;
    }

    const char *folder_path = argv[1];
    const char *archive_name = argv[2];
    char *archive_path = argv[1];
    char divider = '/';
    char *token = NULL;
    char *directory_name = NULL;

    // Открываем файл для записи архива
    FILE *archive = fopen(archive_name, "wb");
    if (!archive) {
        perror("Ошибка при создании архива");
        return 1;
    }

    token = strtok(archive_path, &divider);

    while (token != NULL) {
        directory_name = token;
        token = strtok(NULL, &divider);
    }

    printf("Название директории: %s\n", directory_name);

    // Архивируем директорию
    archive_directory(archive, folder_path, directory_name);

    fclose(archive);
    printf("Архивация завершена. Архив: %s\n", archive_name);
    return 0;
}

