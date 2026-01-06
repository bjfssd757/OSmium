#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stdbool.h>
#include "../mm/vmm.h"

// Базовые типы ELF64
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t  Elf64_Sxword;

#define ELF_NIDENT 16

// --- Основной заголовок (Ehdr) ---
typedef struct {
    unsigned char e_ident[ELF_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;     // Точка входа (64 бит)
    Elf64_Off     e_phoff;     // Смещение таблицы сегментов
    Elf64_Off     e_shoff;     // Смещение таблицы секций
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

// --- Программный заголовок (Phdr) ---
// ВАЖНО: порядок полей отличается от Elf32!
typedef struct {
    Elf64_Word    p_type;    // Тип сегмента
    Elf64_Word    p_flags;   // Флаги (RWX) — в 64 бит они перемещены сюда
    Elf64_Off     p_offset;  // Смещение в файле
    Elf64_Addr    p_vaddr;   // Виртуальный адрес
    Elf64_Addr    p_paddr;   // Физический адрес
    Elf64_Xword   p_filesz;  // Размер в файле
    Elf64_Xword   p_memsz;   // Размер в памяти
    Elf64_Xword   p_align;   // Выравнивание
} Elf64_Phdr;

// --- Заголовок секции (Shdr) ---
typedef struct {
    Elf64_Word    sh_name;
    Elf64_Word    sh_type;
    Elf64_Xword   sh_flags;
    Elf64_Addr    sh_addr;
    Elf64_Off     sh_offset;
    Elf64_Xword   sh_size;
    Elf64_Word    sh_link;
    Elf64_Word    sh_info;
    Elf64_Xword   sh_addralign;
    Elf64_Xword   sh_entsize;
} Elf64_Shdr;

// --- Основные константы ---
#define ELFMAG0     0x7f
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

#define ELFCLASS64  2
#define ELFDATA2LSB 1 // Little Endian

#define EM_X86_64   62
#define ET_EXEC     2
#define PT_LOAD     1

bool load_elf(void *elf_buf, size_t size, page_table_t **out_pml4, Elf64_Addr *entry);

#endif