/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pe_fixture_loader.h"

static unsigned allocations,releases,executable_pages;
static int deny_import,deny_protect;
static void *allocate_rw(size_t n,void *ctx){(void)ctx;void *p=VirtualAlloc(NULL,n,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(p)++allocations;return p;}
static int protect(void *p,size_t n,unsigned bits,void *ctx)
{
    (void)ctx;if(deny_protect)return 0;
    DWORD flags=PAGE_NOACCESS,old;
    if((bits&(VI_EXEC|VI_WRITE))==(VI_EXEC|VI_WRITE))return 0;
    if(bits&VI_EXEC){flags=(bits&VI_READ)?PAGE_EXECUTE_READ:PAGE_EXECUTE;++executable_pages;}
    else if(bits&VI_WRITE)flags=PAGE_READWRITE;
    else if(bits&VI_READ)flags=PAGE_READONLY;
    return VirtualProtect(p,n,flags,&old)!=0;
}
static void release(void *p,size_t n,void *ctx){(void)n;(void)ctx;if(VirtualFree(p,0,MEM_RELEASE))++releases;}
static int __attribute__((sysv_abi,noinline)) native_add_logic(int a,int b){return a+b;}
static int __attribute__((ms_abi,noinline)) add_bridge(int a,int b){return native_add_logic(a,b);}
static void *resolve(const char *dll,const char *symbol,void *ctx)
{
    (void)ctx;
    if(!deny_import&&!strcmp(dll,"vi-fixture.dll")&&!strcmp(symbol,"fixture_add"))return (void*)(uintptr_t)&add_bridge;
    return NULL;
}
static unsigned rd32(const unsigned char *p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static void wr32(unsigned char *p,unsigned n){for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(n>>(8*i));}
static int reject_bad_fixtures(const unsigned char *original,size_t size,ViPeHost *host)
{
    unsigned char *data=malloc(size);if(!data)return 0;
    unsigned pe=rd32(original+60),opt=pe+24,sections=opt+(original[pe+20]|original[pe+21]<<8);
    for(unsigned test=0;test<8;++test){
        memcpy(data,original,size);
        switch(test){
        case 0:data[0]='X';break;
        case 1:wr32(data+60,0xfffffff0);break;
        case 2:data[pe+4]=0;break;
        case 3:wr32(data+opt+56,0xffffc000);break;
        case 4:wr32(data+opt+16,0);break;
        case 5:wr32(data+sections+36,0xe0000020);break;
        case 6:wr32(data+opt+112+9*8,0x4000);wr32(data+opt+116+9*8,40);break;
        case 7:wr32(data+sections+12,0);break;
        }
        ViPeImage image={0};char error[160];
        if(vi_pe_load_fixture(data,size,host,&image,error,sizeof(error))){vi_pe_unload(&image);free(data);return 0;}
        if(allocations!=releases){free(data);return 0;}
    }
    for(size_t truncated=0;truncated<64;++truncated){
        ViPeImage image={0};char error[160];
        if(vi_pe_load_fixture(original,truncated,host,&image,error,sizeof(error))){vi_pe_unload(&image);free(data);return 0;}
    }
    free(data);puts("8 malformed variants and 64 truncated headers rejected without execution.");return 1;
}
int main(int argc,char **argv)
{
    if(argc!=2){fprintf(stderr,"Use only the generated original fixture: host_windows fixture.exe\n");return 2;}
    FILE *f=fopen(argv[1],"rb");if(!f)return 3;
    if(fseek(f,0,SEEK_END)||ftell(f)<0){fclose(f);return 4;}
    long bytes=ftell(f);if(bytes>1024*1024){fclose(f);return 5;}rewind(f);
    unsigned char *data=malloc((size_t)bytes);if(!data){fclose(f);return 6;}
    if(fread(data,1,(size_t)bytes,f)!=(size_t)bytes){free(data);fclose(f);return 7;}fclose(f);
    ViPeHost host={NULL,16384,allocate_rw,protect,release,resolve};
    ViPeImage image={0};char error[160];
    if(!reject_bad_fixtures(data,(size_t)bytes,&host)){free(data);return 9;}
    /* Check rejected imports and protection errors release allocations. */
    deny_import=1;
    if(vi_pe_load_fixture(data,(size_t)bytes,&host,&image,error,sizeof(error))||allocations!=releases){free(data);return 10;}
    printf("unknown import rejected: %s\n",error);deny_import=0;
    deny_protect=1;
    if(vi_pe_load_fixture(data,(size_t)bytes,&host,&image,error,sizeof(error))||allocations!=releases){free(data);return 11;}
    printf("permission failure handled: %s\n",error);deny_protect=0;
    if(!vi_pe_load_fixture(data,(size_t)bytes,&host,&image,error,sizeof(error))){fprintf(stderr,"load failed: %s\n",error);free(data);return 12;}
    if(!image.relocations_applied||image.imports_bound!=1){vi_pe_unload(&image);free(data);return 13;}
    if(!FlushInstructionCache(GetCurrentProcess(),image.base,image.size)){vi_pe_unload(&image);free(data);return 14;}
    typedef int (__attribute__((ms_abi)) *Entry)(void);
    Entry entry=(Entry)(uintptr_t)((unsigned char*)image.base+image.entry_rva);
    int result=entry();
    printf("fixture entry returned %d; imports=%u relocations=%u\n",result,image.imports_bound,image.relocations_applied);
    vi_pe_unload(&image);free(data);
    if(result!=42||allocations!=releases||!executable_pages)return 15;
    puts("HOST LAB PASS: mapping, relocation, allowlisted import, entry call, W^X permissions and cleanup.");
    puts("This is an original fixture on Windows, NOT a PS4 result and NOT GTA VI.");return 0;
}
