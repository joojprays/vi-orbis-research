/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pe_fixture_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IMAGE (16u*1024u*1024u)
#define MAX_SECTIONS 32u
typedef struct { uint32_t rva, span, raw, raw_size, access; } Section;
static uint16_t u16(const unsigned char *p) { return (uint16_t)(p[0]|p[1]<<8); }
static uint32_t u32(const unsigned char *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint64_t u64(const unsigned char *p) { return u32(p)|((uint64_t)u32(p+4)<<32); }
static void put64(unsigned char *p,uint64_t v) { for(unsigned i=0;i<8;++i)p[i]=(unsigned char)(v>>(i*8)); }
static int fits(size_t offset,size_t bytes,size_t limit) { return offset<=limit&&bytes<=limit-offset; }
static int fail(char *e,size_t n,const char *s) { if(e&&n)snprintf(e,n,"%s",s);return 0; }
static int power2(size_t n) { return n&&!(n&(n-1)); }
static int overlaps(uint32_t a,uint32_t n,uint32_t b,uint32_t m) { return n&&m&&(uint64_t)a+n>b&&(uint64_t)b+m>a; }
static int mapped(uint32_t rva,uint32_t size,uint32_t headers,const Section *s,unsigned count)
{
    if(fits(rva,size,headers))return 1;
    for(unsigned i=0;i<count;++i)
        if(rva>=s[i].rva&&fits(rva-s[i].rva,size,s[i].span))return 1;
    return 0;
}
static int text_at(const unsigned char *base,uint32_t rva,uint32_t headers,
                   const Section *s,unsigned count,char *out,size_t capacity)
{
    for(size_t i=0;i<capacity;++i){
        if((uint64_t)rva+i>UINT32_MAX||!mapped(rva+(uint32_t)i,1,headers,s,count))return 0;
        unsigned char c=base[rva+i];out[i]=(char)c;
        if(!c)return i>0;
        if(c<32||c>126)return 0;
    }
    return 0;
}
void vi_pe_unload(ViPeImage *image)
{
    if(image&&image->base){image->host.release(image->base,image->size,image->host.context);memset(image,0,sizeof(*image));}
}

int vi_pe_load_fixture(const void *input,size_t file_size,const ViPeHost *h,
                      ViPeImage *out,char *error,size_t error_size)
{
    const unsigned char *file=(const unsigned char*)input;
    Section sections[MAX_SECTIONS];uint32_t directories[16][2]={{0}};
    ViPeImage image={0};unsigned char *base=NULL;unsigned char *pages=NULL;
    struct { uint32_t rva; uint64_t address; } bindings[1024];
    unsigned binding_count=0;
    uint32_t headers,image_size,entry;uint64_t preferred;unsigned section_count;
    if(error&&error_size)error[0]=0;
    if(!out)return fail(error,error_size,"missing output");
    if(out->base)return fail(error,error_size,"output still owns an image; unload it first");
    memset(out,0,sizeof(*out));
    if(!file||!h||!h->allocate_rw||!h->protect||!h->release||!h->resolve_import)
        return fail(error,error_size,"incomplete host contract");
    if(!power2(h->page_size)||h->page_size<4096||h->page_size>65536)
        return fail(error,error_size,"unsupported host page size");
    if(!fits(0,64,file_size)||file[0]!='M'||file[1]!='Z')return fail(error,error_size,"invalid DOS header");
    uint32_t pe=u32(file+60);
    if(!fits(pe,24,file_size)||memcmp(file+pe,"PE\0\0",4))return fail(error,error_size,"invalid PE header");
    const unsigned char *coff=file+pe+4;
    section_count=u16(coff+2);unsigned optional_size=u16(coff+16);
    if(u16(coff)!=0x8664||!section_count||section_count>MAX_SECTIONS||(u16(coff+18)&0x2000))
        return fail(error,error_size,"expected x64 fixture executable, not DLL");
    size_t opt_offset=(size_t)pe+24;
    if(optional_size<112||!fits(opt_offset,optional_size,file_size))return fail(error,error_size,"truncated optional header");
    const unsigned char *opt=file+opt_offset;
    if(u16(opt)!=0x20b)return fail(error,error_size,"expected PE32+");
    entry=u32(opt+16);preferred=u64(opt+24);image_size=u32(opt+56);headers=u32(opt+60);
    uint32_t section_alignment=u32(opt+32);uint32_t count=u32(opt+108);
    if(!image_size||image_size>MAX_IMAGE||!headers||headers>image_size||headers>file_size||
       !power2(section_alignment)||section_alignment<h->page_size||image_size%h->page_size||
       count>16||112u+count*8u>optional_size)
        return fail(error,error_size,"invalid fixture image geometry or host-page alignment");
    for(unsigned i=0;i<count;++i){directories[i][0]=u32(opt+112+i*8);directories[i][1]=u32(opt+116+i*8);}
    const unsigned forbidden[]={3,4,9,10,11,13,14};
    for(unsigned i=0;i<sizeof(forbidden)/sizeof(forbidden[0]);++i)
        if(directories[forbidden[i]][0]||directories[forbidden[i]][1])
            return fail(error,error_size,"unsupported exception/security/TLS/load-config/bound/delay/CLR directory");
    size_t table=opt_offset+optional_size;
    if(!fits(table,section_count*40u,file_size)||table+section_count*40u>headers)
        return fail(error,error_size,"invalid section table");
    int executable_entry=0;
    for(unsigned i=0;i<section_count;++i){
        const unsigned char *s=file+table+i*40u;uint32_t vsize=u32(s+8),raw_size=u32(s+16),flags=u32(s+36);
        sections[i]=(Section){u32(s+12),vsize>raw_size?vsize:raw_size,u32(s+20),raw_size,
            ((flags&0x40000000)?VI_READ:0)|((flags&0x80000000)?VI_WRITE:0)|((flags&0x20000000)?VI_EXEC:0)};
        Section *r=&sections[i];
        if(r->rva%section_alignment||r->rva<headers||!fits(r->rva,r->span,image_size)||
           !fits(r->raw,r->raw_size,file_size)||(r->raw_size&&r->raw<headers))
            return fail(error,error_size,"invalid section range");
        for(unsigned k=0;k<i;++k)
            if(overlaps(r->rva,r->span,sections[k].rva,sections[k].span)||
               overlaps(r->raw,r->raw_size,sections[k].raw,sections[k].raw_size))
                return fail(error,error_size,"overlapping sections");
        if((r->access&(VI_WRITE|VI_EXEC))==(VI_WRITE|VI_EXEC))return fail(error,error_size,"writable executable section refused");
        if((r->access&VI_EXEC)&&entry>=r->rva&&fits(entry-r->rva,1,r->span))executable_entry=1;
    }
    if(!executable_entry)return fail(error,error_size,"entrypoint outside executable sections");
    for(unsigned i=0;i<count;++i)
        if((directories[i][0]||directories[i][1])&&
           (!directories[i][0]||!directories[i][1]||!mapped(directories[i][0],directories[i][1],headers,sections,section_count)))
            return fail(error,error_size,"directory outside mapped data");
    size_t page_count=image_size/h->page_size;
    pages=(unsigned char*)calloc(page_count,1);
    if(!pages)return fail(error,error_size,"page plan allocation failed");
    for(size_t p=0;p<(headers+h->page_size-1)/h->page_size;++p)pages[p]|=VI_READ;
    for(unsigned i=0;i<section_count;++i)
        for(size_t p=sections[i].rva/h->page_size;p<(sections[i].rva+sections[i].span+h->page_size-1)/h->page_size;++p)
            pages[p]|=(unsigned char)sections[i].access;
    for(size_t p=0;p<page_count;++p)
        if((pages[p]&(VI_WRITE|VI_EXEC))==(VI_WRITE|VI_EXEC)){free(pages);return fail(error,error_size,"host page would combine writable and executable data");}
    image.host=*h;image.size=image_size;image.entry_rva=entry;
    image.base=h->allocate_rw(image_size,h->context);base=(unsigned char*)image.base;
    if(!base){free(pages);return fail(error,error_size,"host allocation failed");}
#define LOAD_FAIL(message) do { fail(error,error_size,message);goto cleanup; } while(0)
    if((uintptr_t)base%h->page_size)LOAD_FAIL("host returned misaligned allocation");
    memset(base,0,image_size);memcpy(base,file,headers);
    for(unsigned i=0;i<section_count;++i)memcpy(base+sections[i].rva,file+sections[i].raw,sections[i].raw_size);
    uint64_t delta=(uint64_t)(uintptr_t)base-preferred;
    if(delta){
        uint32_t rva=directories[5][0],size=directories[5][1],offset=0;
        if(!rva||!size)LOAD_FAIL("relocation required but absent");
        while(offset<size){
            if(size-offset<8)LOAD_FAIL("truncated relocation block");
            uint32_t page=u32(base+rva+offset),block=u32(base+rva+offset+4);
            if(block<8||block>size-offset||(block&1))LOAD_FAIL("invalid relocation block size");
            for(uint32_t k=8;k<block;k+=2){
                uint16_t item=u16(base+rva+offset+k);unsigned type=item>>12;
                if(!type)continue;
                uint64_t target=(uint64_t)page+(item&4095);
                if(type!=10||target>UINT32_MAX||!mapped((uint32_t)target,8,headers,sections,section_count))
                    LOAD_FAIL("unsupported or out-of-range relocation");
                /* Keep loader metadata immutable during relocation parsing. */
                for(unsigned d=0;d<count;++d)
                    if(overlaps((uint32_t)target,8,directories[d][0],directories[d][1]))LOAD_FAIL("relocation overlaps loader metadata");
                if(target<headers)LOAD_FAIL("relocation targets headers");
                put64(base+target,u64(base+target)+delta);++image.relocations_applied;
            }
            offset+=block;
        }
    }
    if(directories[1][0]){
        uint32_t rva=directories[1][0],size=directories[1][1];int ended=0;
        for(uint32_t offset=0;offset+20<=size;offset+=20){
            unsigned char *desc=base+rva+offset;uint32_t names=u32(desc),dll_rva=u32(desc+12),iat=u32(desc+16);
            if(!u32(desc)&&!u32(desc+4)&&!u32(desc+8)&&!dll_rva&&!iat){ended=1;break;}
            char dll[128];
            if(!text_at(base,dll_rva,headers,sections,section_count,dll,sizeof(dll))||!iat)LOAD_FAIL("invalid import DLL name/IAT");
            if(!names)names=iat;
            int thunk_end=0;
            for(unsigned k=0;k<1024;++k){
                uint64_t nr=(uint64_t)names+k*8,ir=(uint64_t)iat+k*8;
                if(nr>UINT32_MAX||ir>UINT32_MAX||!mapped((uint32_t)nr,8,headers,sections,section_count)||!mapped((uint32_t)ir,8,headers,sections,section_count))LOAD_FAIL("import thunk outside image");
                uint64_t name=u64(base+nr);
                if(!name){thunk_end=1;break;}
                if(name&(1ull<<63))LOAD_FAIL("ordinal imports unsupported in lab profile");
                char symbol[128];
                if(name>UINT32_MAX-2||!mapped((uint32_t)name,2,headers,sections,section_count)||
                   !text_at(base,(uint32_t)name+2,headers,sections,section_count,symbol,sizeof(symbol)))LOAD_FAIL("invalid import function name");
                void *address=h->resolve_import(dll,symbol,h->context);
                if(!address)LOAD_FAIL("import not explicitly allowed by host");
                if(ir<headers||binding_count>=1024)LOAD_FAIL("IAT header write or import budget exceeded");
                for(unsigned d=0;d<count;++d)
                    if(d!=12&&overlaps((uint32_t)ir,8,directories[d][0],directories[d][1]))LOAD_FAIL("IAT overlaps loader metadata");
                for(unsigned b=0;b<binding_count;++b)
                    if(overlaps((uint32_t)ir,8,bindings[b].rva,8))LOAD_FAIL("duplicate or overlapping IAT slots");
                bindings[binding_count].rva=(uint32_t)ir;
                bindings[binding_count].address=(uint64_t)(uintptr_t)address;++binding_count;
            }
            if(!thunk_end)LOAD_FAIL("unterminated import thunk table");
        }
        if(!ended)LOAD_FAIL("unterminated import directory");
    }
    /* Bind only after parsing all import descriptors, so writes cannot alter
       metadata still being parsed, including the OriginalFirstThunk==0 case. */
    for(unsigned b=0;b<binding_count;++b)put64(base+bindings[b].rva,bindings[b].address);
    image.imports_bound=binding_count;
    for(size_t p=0;p<page_count;){
        size_t end=p+1;while(end<page_count&&pages[end]==pages[p])++end;
        if(!h->protect(base+p*h->page_size,(end-p)*h->page_size,pages[p],h->context))LOAD_FAIL("host rejected final memory permissions");
        p=end;
    }
    free(pages);*out=image;return 1;
cleanup:
    free(pages);vi_pe_unload(&image);return 0;
#undef LOAD_FAIL
}
