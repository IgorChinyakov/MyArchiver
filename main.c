#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_DICT_SIZE 4096

// Структура для хранения информации о файле
typedef struct {
    char file_name[256];
    long file_size;
} FileHeader;

// Структура словаря для LZW
typedef struct {
    char *data;
    int length;
} DictionaryEntry;

// Инициализация словаря
void initialize_dictionary(DictionaryEntry *dict) {
    for (int i = 0; i < 256; i++) {
        dict[i].data = malloc(2);
        dict[i].data[0] = i;
        dict[i].data[1] = '\0';
        dict[i].length = 1;
    }
    for (int i = 256; i < MAX_DICT_SIZE; i++) {
        dict[i].data = NULL;
        dict[i].length = 0;
    }
}

// Освобождение памяти словаря
void free_dictionary(DictionaryEntry *dict) {
    for (int i = 0; i < MAX_DICT_SIZE; i++) {
        if (dict[i].data != NULL) {
            free(dict[i].data);
        }
    }
}

// Функция для сжатия с использованием LZW
void compress_lzw(FILE *input, FILE *output) {
    DictionaryEntry dict[MAX_DICT_SIZE];
    initialize_dictionary(dict);
    int dict_size = 256;

    int prefix = fgetc(input);  // Чтение первого символа
    if (prefix == EOF) return;  // Проверка на пустой файл

    int c;
    while ((c = fgetc(input)) != EOF) {
        // Пытаемся найти строку prefix + c в словаре
        int found = -1;
        for (int i = 0; i < dict_size; i++) {
            if (dict[i].length == dict[prefix].length + 1 &&
                memcmp(dict[i].data, dict[prefix].data, dict[prefix].length) == 0 &&
                dict[i].data[dict[i].length - 1] == c) {
                found = i;
                break;
            }
        }

        if (found != -1) {
            // Строка найдена, обновляем prefix
            prefix = found;
        } else {
            // Строка не найдена, записываем текущий префикс в выходной файл
            fputc(prefix, output);

            // Добавляем новую строку в словарь
            if (dict_size < MAX_DICT_SIZE) {
                dict[dict_size].data = malloc(dict[prefix].length + 2);
                memcpy(dict[dict_size].data, dict[prefix].data, dict[prefix].length);
                dict[dict_size].data[dict[prefix].length] = c;
                dict[dict_size].data[dict[prefix].length + 1] = '\0';
                dict[dict_size].length = dict[prefix].length + 1;
                dict_size++;
            }

            // Сбрасываем префикс на текущий символ
            prefix = c;
        }
    }

    // Записываем последний префикс
    fputc(prefix, output);

    // Освобождаем память
    free_dictionary(dict);
}

// Функция для рекурсивного архивации файлов и директорий
void archive_directory(const char *dir_path, const char *rel_path, FILE *archive) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char file_path[512];
    char relative_file_path[512];
    FILE *input_file, *compressed_file;
    char buffer[BUFFER_SIZE];
    char temp_compressed_file[] = "archived_compressed_file.bin";
    size_t file_size;
    size_t bytes_read;
    FileHeader header;

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
            strncpy(header.file_name, relative_file_path, sizeof(header.file_name));
            header.file_size = file_stat.st_size;

            // Записываем заголовок файла (имя и размер)
            fwrite(&header, sizeof(header), 1, archive);

            // Открываем файл для чтения и записываем его содержимое в архив
            input_file = fopen(file_path, "rb");
            if (input_file == NULL) {
                perror("fopen");
                continue;
            }

            // // Получаем размер сжатого файла
            // fseek(input_file, 0, SEEK_END);
            // file_size = ftell(input_file);
            // fseek(input_file, 0, SEEK_SET);

            // Создаем временный файл для сжатых данных
            compressed_file = fopen(temp_compressed_file, "wb");
            if (compressed_file == NULL) {
                perror("fopen");
                fclose(input_file);
                continue;
            }

            // printf("Путь к файлу: %s", header.file_name);
            // printf(" Размер: %ld", header.file_size);
            // printf(" байт;\n");

            // Сжимаем содержимое файла
            compress_lzw(input_file, compressed_file);

            // Закрываем файлы
            fclose(input_file);
            fclose(compressed_file);

            // Открываем сжатый файл для чтения его размера
            compressed_file = fopen(temp_compressed_file, "rb");
            if (compressed_file == NULL) {
                perror("fopen");
                continue;
            }

            // Получаем размер сжатого файла
            // fseek(compressed_file, 0, SEEK_END);
            // file_size = ftell(input_file);
            // fseek(compressed_file, 0, SEEK_SET);

            // // Записываем заголовок файла
            // strncpy(header.file_name, relative_file_path, sizeof(header.file_name));
            // header.file_size = file_size;
            // fwrite(&header, sizeof(header), 1, archive);

            printf("Путь к файлу: %s", header.file_name);
            printf(" Размер: %ld", header.file_size);
            printf(" байт;\n");

            // Записываем содержимое сжатого файла в архив
            char buffer[BUFFER_SIZE];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, compressed_file)) > 0) {
                fwrite(buffer, 1, bytes_read, archive);
            }

            fclose(compressed_file);

            // Удаляем временный файл
            remove("archived_compressed_file.bin");
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
