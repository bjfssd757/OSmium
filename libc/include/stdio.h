#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "syscall.h" 

typedef struct {
    char name[12];
    uint32_t size;
    uint16_t cluster;
    uint8_t attr;
} fs_entry_t;

typedef struct {
    uint16_t fb;
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

#define VFS_O_RDONLY    0x0000
#define VFS_O_WRONLY    0x0001
#define VFS_O_RDWR      0x0002
#define VFS_O_CREAT     0x0100
#define VFS_O_TRUNC     0x0200
#define VFS_O_APPEND    0x0400

static inline char* fgets(char *s, int size, FILE *stream);
static inline int printf(const char *format, ...);
static inline int vfprintf(FILE *stream, const char *format, va_list ap);
static inline int fputs(const char *s, FILE *stream);
static inline void fflush(FILE* stream);
static inline FILE* fopen(const char* filename, const char* mode);
static inline size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
static inline size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
static inline int fclose(FILE* stream);
static inline int fseek(FILE* stream, long offset, int whence);
static inline int puts(const char* s);

static inline char* fgets(char *s, int size, FILE *stream) {
    (void)stream;

    if (size <= 0) return NULL;

    int i = 0;
    char c = 0;

    while (i < size - 1) {
        c = (char)syscall_getchar();

        if (c == 0) {
            continue;
        }

        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
            }
        } else if (c == '\n' || c == '\r') {
            s[i] = '\n';
            i++;
            break;
        } else {
            s[i] = c;
            i++;
        }
    }

    s[i] = '\0';
    return s;
}

static inline int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int result = vfprintf(NULL, format, ap);
    va_end(ap);
    return result;
}

static inline int vfprintf(FILE *stream, const char *format, va_list ap) {
    (void)stream;

    char temp_buffer[1024];
    char num_buffer[22];
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
                    if (*format == 'u') { // %lu
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
                     if (*format == 'h' && *(format+1) == 'u') { // %hhu
                        format++;
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

    puts(temp_buffer);

    return written;
}

static inline int fputs(const char *s, FILE *stream) {
    (void)stream;
    puts(s); 
    return 0;
}

static inline void fflush(FILE* stream) {
    (void)stream;
    syscall_gfx_update_screen();
}

static inline int puts(const char* s) {
    syscall_gfx_draw_string(10, 10, 16, 0xFFFFFFFF, s);
    // syscall_gfx_draw_string(10, 26, 16, 0xFFFFFFFF, "\n");
    syscall_gfx_update_screen();
    return 0;
}

static inline FILE* fopen(const char* filename, const char* mode) {
    if (!filename || !mode) return NULL;

    int flags = 0;
    int plus = (strchr(mode, '+') != NULL);

    switch (mode[0]) {
        case 'r':
            flags = plus ? VFS_O_RDWR : VFS_O_RDONLY;
            break;
        case 'w':
            flags = (plus ? VFS_O_RDWR : VFS_O_WRONLY) | VFS_O_CREAT | VFS_O_TRUNC;
            break;
        case 'a':
            flags = (plus ? VFS_O_RDWR : VFS_O_WRONLY) | VFS_O_CREAT | VFS_O_APPEND;
            break;
        default:
            return NULL;
    }

    int fb = syscall_vfs_open(filename, flags);
    
    if (fb < 0) {
        return NULL;
    }

    FILE* stream = (FILE*)syscall_malloc(sizeof(FILE));
    if (!stream) {
        syscall_vfs_close(fb);
        return NULL;
    }

    stream->fb = fb;
    stream->seek_pos = 0;
    stream->error = 0;

    stream->file_size = syscall_vfs_file_size(filename);

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

    size_t bytes_read = syscall_vfs_read(stream->fb, ptr, total_bytes_to_read);

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

    size_t bytes_written = syscall_vfs_write(stream->fb, ptr, total_bytes_to_write);

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

static inline int fseek(FILE* stream, long offset, int whence) {
    if (!stream) return -1;
    int res = syscall_vfs_seek(stream->fb, (uint64_t)offset, (uint32_t)whence);
    if (res == 0) {
        switch (whence) {
            case SEEK_SET: stream->seek_pos = offset; break;
            case SEEK_CUR: stream->seek_pos += offset; break;
            case SEEK_END: stream->seek_pos = stream->file_size + offset; break;
        }
        return 0;
    }
    stream->error = 1;
    return -1;
}


#endif // LIBC_STDIO_H