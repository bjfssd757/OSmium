#ifndef LIBC_SYS_STAT_H
#define LIBC_SYS_STAT_H

#include <stdint.h>

// Типы файлов для st_mode
#define S_IFMT   0170000 // Битовая маска для типа файла
#define S_IFSOCK 0140000 // Сокет
#define S_IFLNK  0120000 // Символическая ссылка
#define S_IFREG  0100000 // Обычный файл
#define S_IFBLK  0060000 // Блочное устройство
#define S_IFDIR  0040000 // Директория
#define S_IFCHR  0020000 // Символьное устройство
#define S_IFIFO  0010000 // FIFO

// Макросы для проверки типа файла
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

// Структура для хранения информации о файле.
// Мы определяем только те поля, которые используются в exfat/utils.c
// и добавляем несколько стандартных для совместимости.
struct stat {
    uint64_t  st_dev;      // ID устройства, содержащего файл
    uint64_t  st_ino;      // Inode number
    uint32_t  st_mode;     // Тип файла и режим (права доступа)
    uint32_t  st_nlink;    // Количество жестких ссылок
    uint32_t  st_uid;      // User ID владельца
    uint32_t  st_gid;      // Group ID владельца
    uint64_t  st_rdev;     // ID устройства (если это специальный файл)
    int64_t   st_size;     // Общий размер в байтах
    int64_t   st_atime;    // Время последнего доступа
    int64_t   st_mtime;    // Время последней модификации
    int64_t   st_ctime;    // Время последнего изменения статуса
    int64_t   st_blocks;   // Количество выделенных блоков
    uint32_t  st_blksize;  // Размер блока для ФС
};

#endif // LIBC_SYS_STAT_H