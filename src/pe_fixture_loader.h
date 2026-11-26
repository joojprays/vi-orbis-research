/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef VI_PE_FIXTURE_LOADER_H
#define VI_PE_FIXTURE_LOADER_H
#include <stddef.h>
#include <stdint.h>

enum { VI_READ=1, VI_WRITE=2, VI_EXEC=4 };
typedef struct {
    void *context;
    size_t page_size;
    void *(*allocate_rw)(size_t bytes, void *context);
    int (*protect)(void *address, size_t bytes, unsigned access, void *context);
    void (*release)(void *address, size_t bytes, void *context);
    void *(*resolve_import)(const char *dll, const char *symbol, void *context);
} ViPeHost;

typedef struct {
    void *base;
    size_t size;
    uint32_t entry_rva;
    uint32_t imports_bound;
    uint32_t relocations_applied;
    ViPeHost host;
} ViPeImage;

/* Strict lab profile, not a general Windows loader. Does not execute anything.
 * Unsupported TLS, exception tables, delay imports, CLR, ordinal imports and
 * shared writable/executable host pages are rejected, never stubbed as success.
 * Only self-authored fixtures belong in this experiment. Initialize out to zero;
 * unload an existing image before reusing it. The caller owns successful output. */
int vi_pe_load_fixture(const void *file, size_t file_size, const ViPeHost *host,
                      ViPeImage *out, char *error, size_t error_size);
void vi_pe_unload(ViPeImage *image);
#endif
