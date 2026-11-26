/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original, embedded fixture only. Not an arbitrary PE file runner.
 * Link experiment, not an installable or hardware-validated application.
 */
#include "orbis_memory.h"
#include "embedded_fixture.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int (__attribute__((ms_abi)) *FixtureEntry)(void);

int main(int argc, char **argv)
{
    puts("VI original fixture lab. NOT GTA VI.");
    if(argc!=2 || strcmp(argv[1],"--execute-original-fixture")) {
        puts("No code mapped or executed. Explicit attended execution required.");
        return 2;
    }
    ViPeHost host=vi_orbis_fixture_host();
    ViPeImage image={0};
    char error[256]={0};
    puts("Loading embedded original fixture with final W^X permissions.");
    fflush(stdout);
    if(!vi_pe_load_fixture(vi_embedded_fixture,sizeof(vi_embedded_fixture),
                          &host,&image,error,sizeof(error))) {
        fprintf(stderr,"LOAD FAILED: %s\n",error);
        return 3;
    }
    if(image.imports_bound!=1 || image.relocations_applied!=1) {
        puts("Unexpected fixture metadata; refusing entry execution.");
        vi_pe_unload(&image);
        return 4;
    }
    /* x86-64 coherent instruction/data caches; compiler barrier before entry.
       Runtime permission rejection remains an error in the adapter. */
    __builtin___clear_cache((char*)image.base,(char*)image.base+image.size);
    FixtureEntry entry=(FixtureEntry)((uintptr_t)image.base+image.entry_rva);
    puts("Calling original entry through Microsoft x64 ABI.");
    fflush(stdout);
    int result=entry();
    vi_pe_unload(&image);
    printf("Original fixture returned %d; expected 42.\n",result);
    return result==42 ? 0 : 5;
}
