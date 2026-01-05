#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "syscall.h" 

typedef struct {
    char name[12];
    uint32_t size;
    uint16_t cluster;
    uint8_t attr;
} fs_entry_t;

typedef struct {
    uint16_t cluster_idx;
    uint32_t file_size;
    uint32_t seek_pos;
    int error;
} FILE;

#define stdin  ((FILE*)0)
#define stderr ((FILE*)1)
#define stdout ((FILE*)2)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

static inline char* fgets(char *s, int size, FILE *stream);
static inline int printf(const char *format, ...);
static inline int vfprintf(FILE *stream, const char *format, va_list ap);
static inline int fputs(const char *s, FILE *stream);
static inline void fflush(FILE* stream);
static inline FILE* fopen(const char* filename, const char* mode);
static inline size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
static inline size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
static inline int fclose(FILE* stream);
//static inline int fseek(FILE* stream, long offset, int whence);
static inline int puts(const char* s);


// --- РЕАЛИЗАЦИИ ---

static inline char* fgets(char *s, int size, FILE *stream) {
    // В нашей ОС пока один источник ввода - клавиатура, поэтому `stream` игнорируется.
    (void)stream;

    if (size <= 0) return NULL;

    int i = 0;
    char c = 0;

    while (i < size - 1) {
        c = (char)syscall_getchar();

        if (c == 0) {
            // Если буфер клавиатуры пуст, можно либо ждать (как здесь),
            // либо возвращать управление. Для fgets лучше ждать.
            continue;
        }

        if (c == '\b' || c == 127) { // Обработка Backspace
            if (i > 0) {
                i--;
                // Можно добавить вывод '\b' на экран, чтобы стереть символ,
                // но это требует более сложного управления курсором.
                // Пока просто удаляем из буфера.
            }
        } else if (c == '\n' || c == '\r') {
            s[i] = '\n'; // Записываем новую строку
            i++;
            break; // Завершаем ввод
        } else {
            s[i] = c;
            i++;
        }
    }

    s[i] = '\0'; // Завершаем строку
    return s;
}

static inline int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    // В нашей ОС нет разницы между stdout и stderr, выводим все на экран
    int result = vfprintf(NULL, format, ap);
    va_end(ap);
    return result;
}

// Перемещаем реализацию vfprintf из log.c сюда
static inline int vfprintf(FILE *stream, const char *format, va_list ap) {
    (void)stream; // Пока что игнорируем, куда выводить. Все идет на экран.
    
    // ВАЖНО: Это очень упрощенная реализация для поддержки %s и %lu
    // Полная реализация vfprintf - это большая и сложная задача.
    char temp_buffer[1024]; // Буфер для вывода
    char num_buffer[22];    // Буфер для конвертации чисел
    int written = 0;

    char* p_out = temp_buffer;

    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's': {
                    const char *s = va_arg(ap, const char*);
                    while (*s) *p_out++ = *s++;
                    break;
                }
                case 'l': {
                    format++;
                    if (*format == 'u') { // Обрабатываем %lu
                        uint64_t val = va_arg(ap, uint64_t);
                        char* p_num = &num_buffer[20];
                        *p_num = '\0';
                        if (val == 0) {
                           *--p_num = '0';
                        } else {
                            while(val > 0) {
                                *--p_num = (val % 10) + '0';
                                val /= 10;
                            }
                        }
                        while (*p_num) *p_out++ = *p_num++;
                    }
                    break;
                }
                 case 'h': {
                    format++;
                     if (*format == 'h' && *(format+1) == 'u') { // Обрабатываем %hhu
                        format++;
                        // В va_arg младшие типы (char, short) продвигаются до int
                        unsigned int val_int = va_arg(ap, unsigned int);
                        uint8_t val = (uint8_t)val_int;

                        char* p_num = &num_buffer[20];
                        *p_num = '\0';
                         if (val == 0) {
                           *--p_num = '0';
                        } else {
                            while(val > 0) {
                                *--p_num = (val % 10) + '0';
                                val /= 10;
                            }
                        }
                        while (*p_num) *p_out++ = *p_num++;
                    }
                    break;
                }
                case '%':
                    *p_out++ = '%';
                    break;
                default:
                    // Просто выводим символ как есть, если не знаем формат
                    *p_out++ = '%';
                    *p_out++ = *format;
                    break;
            }
        } else {
            *p_out++ = *format;
        }
        format++;
    }
    *p_out = '\0';
    written = p_out - temp_buffer;

    // Используем puts для вывода на экран
    puts(temp_buffer);

    return written;
}

static inline int fputs(const char *s, FILE *stream) {
    (void)stream; // Игнорируем поток
    // Просто выводим строку на экран. puts добавит перенос строки, fputs не должен.
    // Поэтому используем сырой syscall.
    // TODO: нужна функция вывода без переноса строки. Пока используем puts.
    puts(s); 
    return 0; // В случае успеха
}

static inline void fflush(FILE* stream) {
    (void)stream; // Игнорируем, какой поток
    syscall_gfx_update_screen();
}

static inline int puts(const char* s) {
    // Выводим строку и перенос строки по разным координатам
    // Это временное решение. В идеале нужно управлять курсором.
    syscall_gfx_draw_string(10, 10, 16, 0xFFFFFFFF, s);
    // syscall_gfx_draw_string(10, 26, 16, 0xFFFFFFFF, "\n");
    syscall_gfx_update_screen();
    return 0;
}


// ... (остальные функции fopen, fread и т.д. остаются без изменений) ...

static inline FILE* fopen(const char* filename, const char* mode) {
    // (Упрощенно) Игнорируем `mode` на данный момент.
    // В будущем можно будет проверять права на чтение/запись.
    (void)mode;
    
    fs_entry_t entry;
    int file_idx = syscall_fs_find(filename, &entry);

    if (file_idx < 0) {
        return NULL;
    }

    FILE* stream = (FILE*)syscall_malloc(sizeof(FILE));
    if (!stream) {
        return NULL;
    }

    stream->cluster_idx = entry.cluster;
    stream->file_size = entry.size;
    stream->seek_pos = 0;
    stream->error = 0;

    return stream;
}

static inline int fclose(FILE* stream) {
    if (!stream) return -1;
    syscall_free(stream);
    return 0;
}

static inline size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!ptr || !stream || size == 0 || nmemb == 0) {
        return 0;
    }
    
    size_t total_bytes_to_read = size * nmemb;

    if (stream->seek_pos + total_bytes_to_read > stream->file_size) {
        total_bytes_to_read = stream->file_size - stream->seek_pos;
    }

    if (total_bytes_to_read == 0) {
        return 0;
    }

    // TODO: Добавить в ядро поддержку seek в syscall_fs_read!
    size_t bytes_read = syscall_fs_read(stream->cluster_idx, ptr, total_bytes_to_read);

    if (bytes_read > 0) {
        stream->seek_pos += bytes_read;
    } else {
        stream->error = 1;
    }
    
    return bytes_read / size;
}

static inline size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
     if (!ptr || !stream || size == 0 || nmemb == 0) {
        return 0;
    }
    size_t total_bytes_to_write = size * nmemb;
    
    // TODO: Добавить в ядро поддержку seek и расширения файла.
    size_t bytes_written = syscall_fs_write(stream->cluster_idx, ptr, total_bytes_to_write);

    if (bytes_written > 0) {
        stream->seek_pos += bytes_written;
        if(stream->seek_pos > stream->file_size) {
            stream->file_size = stream->seek_pos;
        }
    } else {
        stream->error = 1;
    }

    return bytes_written / size;
}


#endif // LIBC_STDIO_H