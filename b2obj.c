#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <elf.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include "mir-alloc-default.c"
#include "mir-gen.h"  // mir.h gets included as well

#define MIR_TYPE_INTERP 1
#define MIR_TYPE_INTERP_NAME "interp"
#define MIR_TYPE_GEN 2
#define MIR_TYPE_GEN_NAME "gen"
#define MIR_TYPE_LAZY 3
#define MIR_TYPE_LAZY_NAME "lazy"

#define MIR_TYPE_DEFAULT MIR_TYPE_LAZY

#define MIR_ENV_VAR_LIB_DIRS "MIR_LIB_DIRS"
#define MIR_ENV_VAR_EXTRA_LIBS "MIR_LIBS"
#define MIR_ENV_VAR_TYPE "MIR_TYPE"

struct lib {
  char *name;
  void *handler;
};
typedef struct lib lib_t;

/* stdlibs according to c2mir */
#if defined(__unix__)
#if UINTPTR_MAX == 0xffffffff
static lib_t std_libs[]
  = {{"/lib/libc.so.6", NULL},   {"/lib32/libc.so.6", NULL},     {"/lib/libm.so.6", NULL},
     {"/lib32/libm.so.6", NULL}, {"/lib/libpthread.so.0", NULL}, {"/lib32/libpthread.so.0", NULL}};
static const char *std_lib_dirs[] = {"/lib", "/lib32"};
#elif UINTPTR_MAX == 0xffffffffffffffff
#if defined(__x86_64__)
static lib_t std_libs[] = {{"/lib64/libc.so.6", NULL},
                           {"/lib/x86_64-linux-gnu/libc.so.6", NULL},
                           {"/lib64/libm.so.6", NULL},
                           {"/lib/x86_64-linux-gnu/libm.so.6", NULL},
                           {"/usr/lib64/libpthread.so.0", NULL},
                           {"/lib/x86_64-linux-gnu/libpthread.so.0", NULL},
                           {"/usr/lib/libc.so", NULL}};
static const char *std_lib_dirs[] = {"/lib64", "/lib/x86_64-linux-gnu"};
#elif (__aarch64__)
static lib_t std_libs[]
  = {{"/lib64/libc.so.6", NULL},       {"/lib/aarch64-linux-gnu/libc.so.6", NULL},
     {"/lib64/libm.so.6", NULL},       {"/lib/aarch64-linux-gnu/libm.so.6", NULL},
     {"/lib64/libpthread.so.0", NULL}, {"/lib/aarch64-linux-gnu/libpthread.so.0", NULL}};
static const char *std_lib_dirs[] = {"/lib64", "/lib/aarch64-linux-gnu"};
#elif (__PPC64__)
static lib_t std_libs[] = {
  {"/lib64/libc.so.6", NULL},
  {"/lib64/libm.so.6", NULL},
  {"/lib64/libpthread.so.0", NULL},
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  {"/lib/powerpc64le-linux-gnu/libc.so.6", NULL},
  {"/lib/powerpc64le-linux-gnu/libm.so.6", NULL},
  {"/lib/powerpc64le-linux-gnu/libpthread.so.0", NULL},
#else
  {"/lib/powerpc64-linux-gnu/libc.so.6", NULL},
  {"/lib/powerpc64-linux-gnu/libm.so.6", NULL},
  {"/lib/powerpc64-linux-gnu/libpthread.so.0", NULL},
#endif
};
static const char *std_lib_dirs[] = {
  "/lib64",
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  "/lib/powerpc64le-linux-gnu",
#else
  "/lib/powerpc64-linux-gnu",
#endif
};
#elif (__s390x__)
static lib_t std_libs[]
  = {{"/lib64/libc.so.6", NULL},       {"/lib/s390x-linux-gnu/libc.so.6", NULL},
     {"/lib64/libm.so.6", NULL},       {"/lib/s390x-linux-gnu/libm.so.6", NULL},
     {"/lib64/libpthread.so.0", NULL}, {"/lib/s390x-linux-gnu/libpthread.so.0", NULL}};
static const char *std_lib_dirs[] = {"/lib64", "/lib/s390x-linux-gnu"};
#elif (__riscv)
static lib_t std_libs[]
  = {{"/lib64/libc.so.6", NULL},       {"/lib/riscv64-linux-gnu/libc.so.6", NULL},
     {"/lib64/libm.so.6", NULL},       {"/lib/riscv64-linux-gnu/libm.so.6", NULL},
     {"/lib64/libpthread.so.0", NULL}, {"/lib/riscv64-linux-gnu/libpthread.so.0", NULL}};
static const char *std_lib_dirs[] = {"/lib64", "/lib/riscv64-linux-gnu"};
#else
#error cannot recognize 32- or 64-bit target
#endif
#endif
static const char *lib_suffix = ".so";
#endif

#ifdef _WIN32
static const int slash = '\\';
#else
static const int slash = '/';
#endif

#if defined(__APPLE__)
static lib_t std_libs[] = {{"/usr/lib/libc.dylib", NULL}, {"/usr/lib/libm.dylib", NULL}};
static const char *std_lib_dirs[] = {"/usr/lib"};
static const char *lib_suffix = ".dylib";
#endif

#ifdef _WIN32
static lib_t std_libs[] = {{"C:\\Windows\\System32\\msvcrt.dll", NULL},
                           {"C:\\Windows\\System32\\kernel32.dll", NULL},
                           {"C:\\Windows\\System32\\ucrtbase.dll", NULL}};
static const char *std_lib_dirs[] = {"C:\\Windows\\System32"};
static const char *lib_suffix = ".dll";
#define dlopen(n, f) LoadLibrary (n)
#define dlclose(h) FreeLibrary (h)
#define dlsym(h, s) GetProcAddress (h, s)
#endif

static void close_std_libs (void) {
  for (int i = 0; i < sizeof (std_libs) / sizeof (lib_t); i++)
    if (std_libs[i].handler != NULL) dlclose (std_libs[i].handler);
}

static void open_std_libs (void) {
  for (int i = 0; i < sizeof (std_libs) / sizeof (struct lib); i++)
    std_libs[i].handler = dlopen (std_libs[i].name, RTLD_LAZY);
}

DEF_VARR (lib_t);
static VARR (lib_t) * extra_libs;

typedef const char *char_ptr_t;
DEF_VARR (char_ptr_t);
static VARR (char_ptr_t) * lib_dirs;

DEF_VARR (char);
static VARR (char) * temp_string;

static void *open_lib (const char *dir, const char *name) {
  const char *last_slash = strrchr (dir, slash);
  void *res;
  FILE *f;

  VARR_TRUNC (char, temp_string, 0);
  VARR_PUSH_ARR (char, temp_string, dir, strlen (dir));
  if (last_slash == NULL || last_slash[1] != '\0') VARR_PUSH (char, temp_string, slash);
#ifndef _WIN32
  VARR_PUSH_ARR (char, temp_string, "lib", 3);
#endif
  VARR_PUSH_ARR (char, temp_string, name, strlen (name));
  VARR_PUSH_ARR (char, temp_string, lib_suffix, strlen (lib_suffix));
  VARR_PUSH (char, temp_string, 0);
  if ((res = dlopen (VARR_ADDR (char, temp_string), RTLD_LAZY)) == NULL) {
#ifndef _WIN32
    if ((f = fopen (VARR_ADDR (char, temp_string), "rb")) != NULL) {
      fclose (f);
      fprintf (stderr, "loading %s:%s\n", VARR_ADDR (char, temp_string), dlerror ());
    }
#endif
  }
  return res;
}

static void process_extra_lib (char *lib_name) {
  lib_t lib;

  lib.name = lib_name;
  for (size_t i = 0; i < VARR_LENGTH (char_ptr_t, lib_dirs); i++)
    if ((lib.handler = open_lib (VARR_GET (char_ptr_t, lib_dirs, i), lib_name)) != NULL) break;
  if (lib.handler == NULL) {
    fprintf (stderr, "cannot find library lib%s -- good bye\n", lib_name);
    exit (1);
  }
  VARR_PUSH (lib_t, extra_libs, lib);
}

static void close_extra_libs (void) {
  void *handler;

  for (size_t i = 0; i < VARR_LENGTH (lib_t, extra_libs); i++)
    if ((handler = VARR_GET (lib_t, extra_libs, i).handler) != NULL) dlclose (handler);
}

#if defined(__APPLE__) && defined(__aarch64__)
float __nan (void) {
  union {
    uint32_t i;
    float f;
  } u = {0x7fc00000};
  return u.f;
}
#endif

static void *import_resolver (const char *name) {
  void *handler, *sym = NULL;

  for (int i = 0; i < sizeof (std_libs) / sizeof (struct lib); i++)
    if ((handler = std_libs[i].handler) != NULL && (sym = dlsym (handler, name)) != NULL) break;
  if (sym == NULL)
    for (int i = 0; i < VARR_LENGTH (lib_t, extra_libs); i++)
      if ((handler = VARR_GET (lib_t, extra_libs, i).handler) != NULL
          && (sym = dlsym (handler, name)) != NULL)
        break;
  if (sym == NULL) {
#ifdef _WIN32
    if (strcmp (name, "LoadLibrary") == 0) return LoadLibrary;
    if (strcmp (name, "FreeLibrary") == 0) return FreeLibrary;
    if (strcmp (name, "GetProcAddress") == 0) return GetProcAddress;
#else
    if (strcmp (name, "dlopen") == 0) return dlopen;
    if (strcmp (name, "dlerror") == 0) return dlerror;
    if (strcmp (name, "dlclose") == 0) return dlclose;
    if (strcmp (name, "dlsym") == 0) return dlsym;
    if (strcmp (name, "stat") == 0) return stat;
    if (strcmp (name, "lstat") == 0) return lstat;
    if (strcmp (name, "fstat") == 0) return fstat;
#if defined(__APPLE__) && defined(__aarch64__)
    if (strcmp (name, "__nan") == 0) return __nan;
    if (strcmp (name, "_MIR_set_code") == 0) return _MIR_set_code;
#endif
#endif
    fprintf (stderr, "can not load symbol %s\n", name);
    close_std_libs ();
    exit (1);
  }
  return sym;
}

void lib_dirs_from_env_var (const char *env_var) {
  const char *var_value = getenv (env_var);
  if (var_value == NULL || var_value[0] == '\0') return;

  // copy to an allocated buffer
  int value_len = strlen (var_value);
  char *value = (char *) malloc (value_len + 1);
  strcpy (value, var_value);

  // colon separated list
  char *value_ptr = value;
  char *colon = NULL;
  while ((colon = strchr (value_ptr, ':')) != NULL) {
    colon[0] = '\0';
    VARR_PUSH (char_ptr_t, lib_dirs, value_ptr);
    // goto next
    value_ptr = colon + 1;
  }
  // final part of string
  // colon == NULL
  VARR_PUSH (char_ptr_t, lib_dirs, value_ptr);
}

int get_mir_type (void) {
  const char *type_value = getenv (MIR_ENV_VAR_TYPE);
  if (type_value == NULL || type_value[0] == '\0') return MIR_TYPE_DEFAULT;

  if (strcmp (type_value, MIR_TYPE_INTERP_NAME) == 0) return MIR_TYPE_INTERP;

  if (strcmp (type_value, MIR_TYPE_GEN_NAME) == 0) return MIR_TYPE_GEN;

  if (strcmp (type_value, MIR_TYPE_LAZY_NAME) == 0) return MIR_TYPE_LAZY;

  fprintf (stderr, "warning: unknown MIR_TYPE '%s', using default one\n", type_value);
  return MIR_TYPE_DEFAULT;
}

void open_extra_libs (void) {
  const char *var_value = getenv (MIR_ENV_VAR_EXTRA_LIBS);
  if (var_value == NULL || var_value[0] == '\0') return;

  int value_len = strlen (var_value);
  char *value = (char *) malloc (value_len + 1);
  strcpy (value, var_value);

  char *value_ptr = value;
  char *colon = NULL;
  while ((colon = strchr (value_ptr, ':')) != NULL) {
    colon[0] = '\0';
    process_extra_lib (value_ptr);

    value_ptr = colon + 1;
  }
  process_extra_lib (value_ptr);
}

// Structure to hold relocation information (assumed to be provided by MIR)
typedef struct {
    size_t offset;      // Offset in the machine code
    const char *symbol; // External symbol name
    int type;           // Relocation type (e.g., R_X86_64_PC32)
} reloc_t;

// Structure to return code and relocations from generation
typedef struct {
    void *code;
    size_t code_size;
    reloc_t *relocs;    // Array of relocations
    size_t n_relocs;    // Number of relocations
} code_data_t;

// Structure to store relocation info (adjust as needed)
typedef struct {
    const char *symbol;  // Symbol name
    void *addr;          // Resolved address
} symbol_entry_t;

// Global or context-passed list to record symbols
typedef struct {
    symbol_entry_t *entries;
    size_t n_entries;
    size_t capacity;
} symbol_list_t;

static symbol_list_t symbols = {0};

void *hybrid_import_resolver(const char *name) {
    // Call the original resolver to get the real address
    void *addr = import_resolver(name);

	printf("hybrid_import_resolver: importing %s at %p\n", name, addr);
    
    // Record the symbol and its address
    if (symbols.n_entries >= symbols.capacity) {
        symbols.capacity = symbols.capacity ? symbols.capacity * 2 : 16;
        symbols.entries = realloc(symbols.entries, symbols.capacity * sizeof(symbol_entry_t));
    }
    symbols.entries[symbols.n_entries].symbol = strdup(name);  // Duplicate to manage memory
    symbols.entries[symbols.n_entries].addr = addr;
    symbols.n_entries++;
    
    return addr;  // Return real address for MIR_link
}

code_data_t generate_pic_machine_code(MIR_context_t ctx, MIR_item_t func) {
    code_data_t result = {0};
    symbols.entries = NULL;
    symbols.n_entries = 0;
    symbols.capacity = 0;

    // Set up MIR generator
    MIR_gen_init(ctx);
	MIR_gen_set_save_relocs(ctx, 1);
    MIR_link(ctx, MIR_set_gen_interface, hybrid_import_resolver);
    uint64_t (*fun_addr)(int, char **, char **) = MIR_gen(ctx, func);

    unsigned char *code = (unsigned char *)func->addr;
    for (size_t i = 0; i < 4096; i++) {
        if (code[i] == 0xC3 && code[i + 1] == 0x0) {
            result.code_size = i + 1; // Length up to and including ret
            break;
        }
    }
    printf("code size=%zu bytes\n", result.code_size);

    result.code = malloc(result.code_size);
    memcpy(result.code, func->addr, result.code_size);

    result.relocs = NULL;
    result.n_relocs = 0;

    // Precompute target addresses for faster comparison
    uint64_t *targets = malloc(symbols.n_entries * sizeof(uint64_t));
    for (size_t j = 0; j < symbols.n_entries; j++) {
        targets[j] = (uint64_t)symbols.entries[j].addr;
    }

    // Scan code byte by byte for 8-byte address matches
    for (size_t i = 0; i <= result.code_size - 8; i++) {
        uint64_t val;
        memcpy(&val, code + i, sizeof(uint64_t)); // Safe unaligned read
        for (size_t j = 0; j < symbols.n_entries; j++) {
            if (val == targets[j]) {
                result.relocs = realloc(result.relocs, (result.n_relocs + 1) * sizeof(reloc_t));
                result.relocs[result.n_relocs].offset = i;
                result.relocs[result.n_relocs].symbol = symbols.entries[j].symbol;
                result.relocs[result.n_relocs].type = R_X86_64_64;
                result.n_relocs++;
                printf("FOUND SYMBOL %s @ %zu\n", symbols.entries[j].symbol, i);
                // Zero out the 8 bytes
                for (size_t k = 0; k < 8; k++) {
                    code[i + k] = 0;
                }
                break;
            }
        }
    }

    free(targets);
    MIR_gen_finish(ctx);
    return result;
}

/**
 * Creates an ELF object file from the provided code and relocation data.
 * @param output_file Path to the output ELF file
 * @param code_data Structure containing machine code and relocations
 */
void create_object_file(const char *output_file, const code_data_t *code_data) {
    // Open the output file
    int fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    // Define section names
    const char *section_names[] = {".text", ".symtab", ".strtab", ".rel.text", ".shstrtab"};
    int n_sections = sizeof(section_names) / sizeof(section_names[0]);

    // Build .shstrtab (section header string table)
    size_t shstrtab_size = 1;  // Start with null byte
    for (int i = 0; i < n_sections; i++) {
        shstrtab_size += strlen(section_names[i]) + 1;
    }
    char *shstrtab_data = malloc(shstrtab_size);
    char *p = shstrtab_data;
    *p++ = '\0';  // Initial null byte
    for (int i = 0; i < n_sections; i++) {
        strcpy(p, section_names[i]);
        p += strlen(section_names[i]) + 1;
    }

    // Collect unique external symbols from relocations
    char **extern_symbols = NULL;
    size_t n_extern_symbols = 0;
    for (size_t i = 0; i < code_data->n_relocs; i++) {
        const char *sym = code_data->relocs[i].symbol;
        int found = 0;
        for (size_t j = 0; j < n_extern_symbols; j++) {
            if (strcmp(extern_symbols[j], sym) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            extern_symbols = realloc(extern_symbols, (n_extern_symbols + 1) * sizeof(char *));
            extern_symbols[n_extern_symbols] = strdup(sym);
            n_extern_symbols++;
			printf("external %s\n", sym);
        }
    }

    // Build .strtab (symbol string table)
    size_t strtab_size = 1;  // Start with null byte
    size_t main_name_offset = 1;
    strtab_size += strlen("main") + 1;
    size_t *extern_name_offsets = malloc(n_extern_symbols * sizeof(size_t));
    for (size_t i = 0; i < n_extern_symbols; i++) {
        extern_name_offsets[i] = strtab_size;
        strtab_size += strlen(extern_symbols[i]) + 1;
    }
    char *strtab_data = malloc(strtab_size);
    p = strtab_data;
    *p++ = '\0';  // Initial null byte
    strcpy(p, "main");
    p += strlen("main") + 1;
    for (size_t i = 0; i < n_extern_symbols; i++) {
        strcpy(p, extern_symbols[i]);
        p += strlen(extern_symbols[i]) + 1;
    }

    // Build .symtab (symbol table)
    size_t n_symbols = 1 + 1 + n_extern_symbols;  // null + main + externals
    Elf64_Sym *symtab = calloc(n_symbols, sizeof(Elf64_Sym));
    // Index 0: Null symbol
    symtab[0] = (Elf64_Sym){0};
    // Index 1: "main" symbol (defined in .text)
    symtab[1].st_name = main_name_offset;
    symtab[1].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_FUNC);
    symtab[1].st_shndx = 1;  // .text section index
    symtab[1].st_value = 0;
    symtab[1].st_size = code_data->code_size;
    // Indices 2+: External symbols (undefined)
    for (size_t i = 0; i < n_extern_symbols; i++) {
        symtab[2 + i].st_name = extern_name_offsets[i];
        symtab[2 + i].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
        symtab[2 + i].st_shndx = SHN_UNDEF;
    }

    // Build .rel.text (relocation entries)
    Elf64_Rela *rel_text = calloc(code_data->n_relocs, sizeof(Elf64_Rela));
    for (size_t i = 0; i < code_data->n_relocs; i++) {
        const char *sym = code_data->relocs[i].symbol;
        size_t sym_index = 0;
        for (size_t j = 0; j < n_extern_symbols; j++) {
            if (strcmp(extern_symbols[j], sym) == 0) {
                sym_index = 2 + j;  // null + main + j
                break;
            }
        }
        rel_text[i].r_offset = code_data->relocs[i].offset;
        rel_text[i].r_info = ELF64_R_INFO(sym_index, code_data->relocs[i].type);
        rel_text[i].r_addend = 0;
    }

    // Prepare .text (machine code with relocation targets zeroed)
    void *text_data = malloc(code_data->code_size);
    memcpy(text_data, code_data->code, code_data->code_size);
    for (size_t i = 0; i < code_data->n_relocs; i++) {
        size_t offset = code_data->relocs[i].offset;
        // Zero out bytes based on relocation type
        if (code_data->relocs[i].type == R_X86_64_PC32) {
            memset((char *)text_data + offset, 0, 4);
        } else if (code_data->relocs[i].type == R_X86_64_64) {
            memset((char *)text_data + offset, 0, 8);
        }
        // Add handling for other relocation types as needed
    }

    // Calculate section offsets
    off_t offset = sizeof(Elf64_Ehdr);
    off_t text_offset = offset;
    offset += code_data->code_size;
    off_t symtab_offset = offset;
    offset += n_symbols * sizeof(Elf64_Sym);
    off_t strtab_offset = offset;
    offset += strtab_size;
    off_t rel_text_offset = offset;
    offset += code_data->n_relocs * sizeof(Elf64_Rela);
    off_t shstrtab_offset = offset;
    offset += shstrtab_size;
    off_t sh_offset = offset;

    // Create ELF header
    Elf64_Ehdr ehdr = {0};
    memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
    ehdr.e_ident[EI_CLASS] = ELFCLASS64;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_type = ET_REL;        // Relocatable file
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_shentsize = sizeof(Elf64_Shdr);
    ehdr.e_shnum = n_sections + 1;  // Including null section
    ehdr.e_shoff = sh_offset;
    ehdr.e_shstrndx = 5;  // .shstrtab section index

    // Create section headers
    Elf64_Shdr shdrs[6] = {0};  // 0: null, 1: .text, 2: .symtab, 3: .strtab, 4: .rel.text, 5: .shstrtab
    // Index 0: Null section
    shdrs[0] = (Elf64_Shdr){0};
    // Index 1: .text
    shdrs[1].sh_name = 1;  // Offset of ".text" in .shstrtab
    shdrs[1].sh_type = SHT_PROGBITS;
    shdrs[1].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    shdrs[1].sh_offset = text_offset;
    shdrs[1].sh_size = code_data->code_size;
    shdrs[1].sh_addralign = 16;
    // Index 2: .symtab
    shdrs[2].sh_name = 7;  // ".symtab"
    shdrs[2].sh_type = SHT_SYMTAB;
    shdrs[2].sh_offset = symtab_offset;
    shdrs[2].sh_size = n_symbols * sizeof(Elf64_Sym);
    shdrs[2].sh_link = 3;  // Links to .strtab
    shdrs[2].sh_info = 1;  // Index after last local symbol (all global here)
    shdrs[2].sh_addralign = 8;
    shdrs[2].sh_entsize = sizeof(Elf64_Sym);
    // Index 3: .strtab
    shdrs[3].sh_name = 15;  // ".strtab"
    shdrs[3].sh_type = SHT_STRTAB;
    shdrs[3].sh_offset = strtab_offset;
    shdrs[3].sh_size = strtab_size;
    shdrs[3].sh_addralign = 1;
    // Index 4: .rel.text
    shdrs[4].sh_name = 23;  // ".rel.text"
    shdrs[4].sh_type = SHT_RELA;
    shdrs[4].sh_offset = rel_text_offset;
    shdrs[4].sh_size = code_data->n_relocs * sizeof(Elf64_Rela);
    shdrs[4].sh_link = 2;  // Links to .symtab
    shdrs[4].sh_info = 1;  // Applies to .text
    shdrs[4].sh_addralign = 8;
    shdrs[4].sh_entsize = sizeof(Elf64_Rela);
    // Index 5: .shstrtab
    shdrs[5].sh_name = 32;  // ".shstrtab"
    shdrs[5].sh_type = SHT_STRTAB;
    shdrs[5].sh_offset = shstrtab_offset;
    shdrs[5].sh_size = shstrtab_size;
    shdrs[5].sh_addralign = 1;

    // Write all data to the file
    write(fd, &ehdr, sizeof(Elf64_Ehdr));                    // ELF header
    write(fd, text_data, code_data->code_size);             // .text
    write(fd, symtab, n_symbols * sizeof(Elf64_Sym));       // .symtab
    write(fd, strtab_data, strtab_size);                    // .strtab
    write(fd, rel_text, code_data->n_relocs * sizeof(Elf64_Rela)); // .rel.text
    write(fd, shstrtab_data, shstrtab_size);                // .shstrtab
    write(fd, shdrs, (n_sections + 1) * sizeof(Elf64_Shdr)); // Section headers

    // Close the file
    close(fd);

    // Free allocated memory
    free(text_data);
    free(symtab);
    free(rel_text);
    free(extern_name_offsets);
    for (size_t i = 0; i < n_extern_symbols; i++) {
        free(extern_symbols[i]);
    }
    free(extern_symbols);
    free(shstrtab_data);
    free(strtab_data);
}

int main(int argc, char **argv) {
    MIR_alloc_t alloc = &default_alloc;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mir_input> <object_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    VARR_CREATE(char, temp_string, alloc, 0);
    VARR_CREATE(lib_t, extra_libs, alloc, 16);
    VARR_CREATE(char_ptr_t, lib_dirs, alloc, 16);
    for (int i = 0; i < sizeof(std_lib_dirs) / sizeof(char_ptr_t); i++)
        VARR_PUSH(char_ptr_t, lib_dirs, std_lib_dirs[i]);
    lib_dirs_from_env_var("LD_LIBRARY_PATH");
    lib_dirs_from_env_var(MIR_ENV_VAR_LIB_DIRS);

    const char *mir_input_file = argv[1];
    const char *output_file = argv[2];

    MIR_context_t ctx = MIR_init();
    
    FILE *fp = fopen(mir_input_file, "r");
    if (!fp) {
        perror("Failed to open MIR input file");
        return EXIT_FAILURE;
    }

    MIR_item_t main_func = NULL;
    
    MIR_read(ctx, fp);
    fclose(fp);

    for (MIR_module_t module = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); module != NULL;
         module = DLIST_NEXT(MIR_module_t, module)) {
        for (MIR_item_t func = DLIST_HEAD(MIR_item_t, module->items); func != NULL;
             func = DLIST_NEXT(MIR_item_t, func)) {
            if (func->item_type != MIR_func_item) continue;
            if (strcmp(func->u.func->name, "main") == 0) {
                main_func = func;
            }
            printf("MIR_load_module function %s\n", func->u.func->name);
        }
        MIR_load_module(ctx, module);
    }
    
    if (!main_func) {
        fprintf(stderr, "No main found in input.\n");
        return EXIT_FAILURE;
    }

    open_std_libs();
    open_extra_libs();

    code_data_t code_data = generate_pic_machine_code(ctx, main_func);
    create_object_file(output_file, &code_data);

    printf("Object file '%s' created successfully with PIC.\n", output_file);

    MIR_finish(ctx);

    return EXIT_SUCCESS;
}
