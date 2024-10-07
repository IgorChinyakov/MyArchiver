#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

// Структура для хранения информации о файле
typedef struct {
    char file_name[256];
    long file_size;
} FileHeader;

// Функция для рекурсивного архивации файлов и директорий
void archive_directory(const char *dir_path, const char *rel_path, FILE *archive) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char file_path[512];
    char relative_file_path[512];
    FILE *input_file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    if ((dir = opendir(dir_path)) == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);
        snprintf(relative_file_path, sizeof(relative_file_path), "%s/%s", rel_path, entry->d_name);

        if (stat(file_path, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        // Если это директория, рекурсивно заходим в неё
        if (S_ISDIR(file_stat.st_mode)) {
            archive_directory(file_path, relative_file_path, archive);
        } else {
            // Если это файл, сохраняем его метаданные и содержимое
            FileHeader header;
            strncpy(header.file_name, relative_file_path, sizeof(header.file_name));
            header.file_size = file_stat.st_size;

            // Записываем заголовок файла (имя и размер)
            fwrite(&header, sizeof(header), 1, archive);

            printf("Путь к файлу: %s", header.file_name);
            printf(" Размер: %ld", header.file_size);
            printf(" байт;\n");

            // Открываем файл для чтения и записываем его содержимое в архив
            input_file = fopen(file_path, "rb");
            if (input_file == NULL) {
                perror("fopen");
                continue;
            }

            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, input_file)) > 0) {
                fwrite(buffer, 1, bytes_read, archive);
            }

            fclose(input_file);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_directory = argv[1];
    const char *output_file = argv[2];

    FILE *archive = fopen(output_file, "wb");
    if (archive == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    // Архивируем содержимое директории
    archive_directory(input_directory, "", archive);

    fclose(archive);

    printf("Архивация завершена. Файл сохранен как %s\n", output_file);
    return EXIT_SUCCESS;
}
