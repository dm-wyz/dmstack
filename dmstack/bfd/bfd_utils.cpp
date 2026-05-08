#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>   // open()
#include <unistd.h>  // close()
#include <sys/stat.h>

// elf need these headers
#include <gelf.h>
#include <libelf.h>

#define HAVE_DECL_BASENAME 1
#include <libiberty/demangle.h>
#undef HAVE_DECL_BASENAME

#include "utils/defer.h"

namespace bfd_utils
{

    bool check_exist(const char* file_name)
    {
        struct stat buffer;
        return (stat(file_name, &buffer) == 0);
    }

    bool startswith(const char* str, const char* prefix)
    {
        return strncmp(str, prefix, strlen(prefix)) == 0;
    }

    bool check_exists_and_is_regular(const char* file_name)
    {
        struct stat buffer;
        return (stat(file_name, &buffer) == 0 && S_ISREG(buffer.st_mode));
    }

    bool check_shlib(const char* file, int64_t& vaddr)
    {
        FILE* fp = fopen(file, "r");
        if (fp == 0)
        {
            return false;
        }
        DEFER(fclose(fp));

        elf_version(EV_CURRENT);
        Elf* elf = elf_begin(fileno(fp), ELF_C_READ, nullptr);
        if (elf == 0)
        {
            return false;
        }
        DEFER(elf_end(elf));

        vaddr = 0;
        #if defined(__i386__)
        Elf32_Ehdr* const ehdr = elf32_getehdr(elf);
        Elf32_Phdr* const phdr = elf32_getphdr(elf);
        #else
        Elf64_Ehdr* const ehdr = elf64_getehdr(elf);
        Elf64_Phdr* const phdr = elf64_getphdr(elf);
        #endif

        if (!ehdr)
        {
            return false;
        }
        const int num_phdr = ehdr->e_phnum;
        for (int i = 0; i < num_phdr; ++i)
        {
            #if defined(__i386__)
            Elf32_Phdr* const p = phdr + i;
            #else
            Elf64_Phdr* const p = phdr + i;
            #endif
            if (p->p_type == PT_LOAD && (p->p_flags & 1) != 0)
            {
                vaddr = p->p_vaddr;
                break;
            }
        }
        return vaddr == 0;
    }

    bool check_stripped(const char* file)
    {
        bool stripped = true;
        int fd;

        Elf* elf;
        Elf_Scn* scn;
        GElf_Shdr shdr;

        elf_version(EV_CURRENT);
        if ((fd = open(file, O_RDONLY)) < 0)
        {
            return false;
        }
        DEFER(close(fd));

        elf = elf_begin(fd, ELF_C_READ, NULL);
        DEFER(elf_end(elf));

        scn = NULL;

        while ((scn = elf_nextscn(elf, scn)) != NULL)
        {
            gelf_getshdr(scn, &shdr);

            if (shdr.sh_type == SHT_SYMTAB)
            {
                stripped = false;
                break;
            }
        }
        return stripped;
    }

    const char* get_filename(const char* filepath)
    {
        return (strrchr(filepath, '/') ? (strrchr(filepath, '/') + 1) : filepath);
    }

    bool is_same_file(const char* path1, const char* path2)
    {
        struct stat sb1, sb2;
        return 0 == stat(path1, &sb1)
            && 0 == stat(path2, &sb2)
            && sb1.st_dev == sb2.st_dev
            && sb1.st_ino == sb2.st_ino;
    }

    bool startwith(const char* str, const char* start)
    {
        size_t string_len = strlen(str);
        size_t start_len = strlen(start);

        if (string_len < start_len)
        {
            return false;
        }

        if (!strncmp(str, start, start_len))
        {
            return true;
        }
        return false;
    }

    char* get_demangled_symbol(const char* symbol_name)
    {
        unsigned int arg = DMGL_ANSI;
        arg |= DMGL_PARAMS;
        arg |= DMGL_TYPES;
        char* demangled = cplus_demangle(symbol_name, arg);
        if (!demangled)
        {
            demangled = strdup(symbol_name);
        }
        return demangled;
    }
}
