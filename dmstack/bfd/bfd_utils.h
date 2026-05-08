#ifndef BFD_UTILS_H
#define BFD_UTILS_H


namespace bfd_utils
{
    bool startswith(const char* str, const char* prefix);

    bool check_exist(const char* file_name);

    bool check_exists_and_is_regular(const char* file_name);

    bool check_stripped(const char* file);

    bool check_shlib(const char* file, int64_t& vaddr);

    const char* get_filename(const char* filepath);

    bool is_same_file(const char* path1, const char* path2);

    bool startwith(const char* str, const char* start);

    char* get_demangled_symbol(const char* symbol_name);
}
#endif // !BFD_UTILS_H
