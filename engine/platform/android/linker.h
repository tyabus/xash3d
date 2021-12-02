/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _LINKER_H_
#define _LINKER_H_
#ifdef __ANDROID__

#include <unistd.h>
#include <sys/types.h>
#include <elf.h>

#include <link.h>

// Returns the address of the page containing address 'x'.
#define PAGE_START(x)  ((x) & PAGE_MASK)

// Returns the offset of address 'x' in its page.
#define PAGE_OFFSET(x) ((x) & ~PAGE_MASK)

// Returns the address of the next page after address 'x', unless 'x' is
// itself at the start of a page.
#define PAGE_END(x)    PAGE_START((x) + (PAGE_SIZE-1))

// Magic shared structures that GDB knows about.

struct link_map_t {
  uintptr_t l_addr;
  char*  l_name;
  uintptr_t l_ld;
  link_map_t* l_next;
  link_map_t* l_prev;
};

// Values for r_debug->state
enum {
  RT_CONSISTENT,
  RT_ADD,
  RT_DELETE
};
#if 0
struct r_debug {
  int32_t r_version;
  link_map_t* r_map;
  void (*r_brk)(void);
  int32_t r_state;
  uintptr_t r_ldbase;
};
#endif
#define FLAG_LINKED     0x00000001
#define FLAG_EXE        0x00000004 // The main executable
#define FLAG_LINKER     0x00000010 // The linker itself

#define SOINFO_NAME_LEN 128

typedef void (*linker_function_t)();

// Android uses REL for 32-bit but only uses RELA for 64-bit.
#if defined(__LP64__)
#define USE_RELA 1
#endif

#if !defined(__LP64__)
#define Elf_Ehdr	Elf32_Ehdr
#define Elf_Phdr	Elf32_Phdr
#define Elf_Shdr	Elf32_Shdr
#define Elf_Sym		Elf32_Sym
#define Elf_Rel		Elf32_Rel
#define Elf_Rela	Elf32_Rela
#define Elf_Dyn		Elf32_Dyn
#define Elf_Word	Elf32_Word
#define Elf_Sword	Elf32_Sword
#define Elf_Half	Elf32_Half
#define Elf_Addr	Elf32_Addr
#define Elf_Off		Elf32_Off
#define Elf_SOff	Elf32_SOff
#define Elf_Nhdr	Elf32_Nhdr
#define Elf_Verdef	Elf32_Verdef
#define Elf_Verdaux	Elf32_Verdaux
#define Elf_Verneed	Elf32_Verneed
#define Elf_Vernaux	Elf32_Vernaux
#define Elf_Versym	Elf32_Versym
#define ELF_R_SYM	ELF32_R_SYM
#define ELF_R_TYPE	ELF32_R_TYPE
#define ELFCLASS	ELFCLASS32
#define AuxInfo		Aux32Info
#else
#define Elf_Ehdr	Elf64_Ehdr
#define Elf_Phdr	Elf64_Phdr
#define Elf_Shdr	Elf64_Shdr
#define Elf_Sym		Elf64_Sym
#define Elf_Rel		Elf64_Rel
#define Elf_Rela	Elf64_Rela
#define Elf_Dyn		Elf64_Dyn
#define Elf_Word	Elf64_Word
#define Elf_Sword	Elf64_Sword
#define Elf_Half	Elf64_Half
#define Elf_Addr	Elf64_Addr
#define Elf_Off		Elf64_Off
#define Elf_SOff	Elf64_SOff
#define Elf_Nhdr	Elf64_Nhdr
#define Elf_Verdef	Elf64_Verdef
#define Elf_Verdaux	Elf64_Verdaux
#define Elf_Verneed	Elf64_Verneed
#define Elf_Vernaux	Elf64_Vernaux
#define Elf_Versym	Elf64_Versym
#define ELF_R_SYM	ELF64_R_SYM
#define ELF_R_TYPE	ELF64_R_TYPE
#define ELFCLASS	ELFCLASS64
#define AuxInfo		Aux64Info
#endif
#ifndef Elf_Symindx
#define Elf_Symindx	uint32_t
#endif

struct soinfo {
 public:
  char name[SOINFO_NAME_LEN];
  const Elf_Phdr* phdr;
  size_t phnum;
  Elf_Addr entry;
  Elf_Addr base;
  unsigned size;

#ifndef __LP64__
  uint32_t unused1;  // DO NOT USE, maintained for compatibility.
#endif

  Elf_Dyn* dynamic;

#ifndef __LP64__
  uint32_t unused2; // DO NOT USE, maintained for compatibility
  uint32_t unused3; // DO NOT USE, maintained for compatibility
#endif

  soinfo* next;
  unsigned flags;

  const char* strtab;
  Elf_Sym* symtab;

  size_t nbucket;
  size_t nchain;
  unsigned* bucket;
  unsigned* chain;

#if !defined(__LP64__)
  // This is only used by 32-bit MIPS, but needs to be here for
  // all 32-bit architectures to preserve binary compatibility.
  unsigned* plt_got;
#endif

#if defined(USE_RELA)
  Elf_Rela* plt_rela;
  size_t plt_rela_count;

  Elf_Rela* rela;
  size_t rela_count;
#else
  Elf_Rel* plt_rel;
  size_t plt_rel_count;

  Elf_Rel* rel;
  size_t rel_count;
#endif

  linker_function_t* preinit_array;
  size_t preinit_array_count;

  linker_function_t* init_array;
  size_t init_array_count;
  linker_function_t* fini_array;
  size_t fini_array_count;

  linker_function_t init_func;
  linker_function_t fini_func;

#if defined(__arm__)
  // ARM EABI section used for stack unwinding.
  unsigned* ARM_exidx;
  size_t ARM_exidx_count;
#elif defined(__mips__)
  unsigned mips_symtabno;
  unsigned mips_local_gotno;
  unsigned mips_gotsym;
#endif

  size_t ref_count;
  link_map_t link_map;

  bool constructors_called;

  // When you read a virtual address from the ELF file, add this
  // value to get the corresponding address in the process' address space.
  Elf_Addr load_bias;

#if !defined(__LP64__)
  bool has_text_relocations;
#endif
  bool has_DT_SYMBOLIC;

  void CallConstructors();
  void CallDestructors();
  void CallPreInitConstructors();

 private:
  void CallArray(const char* array_name, linker_function_t* functions, size_t count, bool reverse);
  void CallFunction(const char* function_name, linker_function_t function);
};

extern soinfo libdl_info;

void do_android_update_LD_LIBRARY_PATH(const char* ld_library_path);
soinfo* do_dlopen(const char* name, int flags);
int do_dlclose(soinfo* si);

Elf_Sym* dlsym_linear_lookup(const char* name, soinfo** found, soinfo* start);
soinfo* find_containing_library(const void* addr);

Elf_Sym* dladdr_find_symbol(soinfo* si, const void* addr);
Elf_Sym* dlsym_handle_lookup(soinfo* si, const char* name);

void debuggerd_init();
extern "C" void notify_gdb_of_libraries();

char* linker_get_error_buffer();
size_t linker_get_error_buffer_size();

#endif
#endif
