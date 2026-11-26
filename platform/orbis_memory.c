/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native adapter: documented toolchain calls only. Permission failures stop
 * the fixture. No kernel patch, permission workaround, or implicit RWX memory.
 */
#include "pe_fixture_loader.h"
#include <orbis/libkernel.h>
#include <stdint.h>
#include <string.h>

static void *allocate_rw(size_t n,void *context)
{
    (void)context;void *address=0;
    if(sceKernelMapFlexibleMemory(&address,n,VI_READ|VI_WRITE,0)<0)return 0;
    return address;
}
static int protect(void *address,size_t bytes,unsigned bits,void *context)
{
    (void)context;
    if((bits&(VI_WRITE|VI_EXEC))==(VI_WRITE|VI_EXEC))return 0;
    return sceKernelMprotect(address,bytes,(int)bits)>=0;
}
static void release(void *address,size_t bytes,void *context)
{(void)context;sceKernelReleaseFlexibleMemory(address,bytes);}
static int __attribute__((sysv_abi,noinline)) native_add_logic(int a,int b){return a+b;}
static int __attribute__((ms_abi,noinline)) fixture_add_bridge(int a,int b){return native_add_logic(a,b);}
static void *resolve(const char *dll,const char *symbol,void *context)
{
    (void)context;
    if(!strcmp(dll,"vi-fixture.dll")&&!strcmp(symbol,"fixture_add"))
        return (void*)(uintptr_t)&fixture_add_bridge;
    return 0;
}
ViPeHost vi_orbis_fixture_host(void)
{
    ViPeHost result={0,16384,allocate_rw,protect,release,resolve};
    return result;
}
