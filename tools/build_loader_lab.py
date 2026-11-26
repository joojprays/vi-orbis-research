#!/usr/bin/env python3
"""Build self-authored PE fixture, execute host lab, and cross-compile native adapter.

No downloads, game files, console connection, payload injection or installation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--llvm-bin',type=Path,required=True)
parser.add_argument('--host-cc',type=Path,required=True,help='Existing MSYS/Cygwin gcc on Windows')
parser.add_argument('--orbis-sdk',type=Path,required=True)
args=parser.parse_args()
llvm=args.llvm_bin.resolve();sdk=args.orbis_sdk.resolve();hostcc=args.host_cc.resolve()
out=ROOT/'build/loader-lab';out.mkdir(parents=True,exist_ok=True)
env=os.environ|{'PATH':str(hostcc.parent)+os.pathsep+os.environ.get('PATH','')}
commands=[]
def run(argv,host_env=False):
    argv=[str(a) for a in argv];commands.append(argv)
    p=subprocess.run(argv,cwd=ROOT,env=env if host_env else None,capture_output=True,
                     text=True,encoding='utf-8',errors='replace')
    if p.returncode:raise RuntimeError('Command failed:\n'+' '.join(argv)+'\n'+p.stdout+'\n'+p.stderr)
    if p.stdout:print(p.stdout,flush=True)
    return p.stdout

clang=llvm/'clang.exe';lld=llvm/'lld.exe'
windows=['--target=x86_64-pc-windows-msvc','-O1','-ffreestanding','-fno-stack-protector',
         '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-msse2','-mno-avx','-mno-avx2']
for name in ['fixture','import_stub']:
    run([clang,*windows,'-c',ROOT/'fixtures'/(name+'.c'),'-o',out/(name+'.obj')])
run([lld,'-flavor','link','/dll','/noentry','/nodefaultlib',out/'import_stub.obj',
     '/out:'+str(out/'vi-fixture.dll'),'/implib:'+str(out/'vi-fixture.lib')])
run([lld,'-flavor','link','/nodefaultlib','/entry:lab_entry','/subsystem:console',
     '/dynamicbase','/fixed:no','/align:16384','/filealign:512',
     '/out:'+str(out/'fixture.exe'),out/'fixture.obj',out/'vi-fixture.lib'])
run([hostcc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src'),
     ROOT/'src/pe_fixture_loader.c',ROOT/'lab/host_windows.c','-o',out/'host-lab.exe','-lkernel32'],True)
host_output=run([out/'host-lab.exe',out/'fixture.exe'],True)

native_flags=['--target=x86_64-pc-freebsd12-elf','-isysroot',sdk,'-isystem',sdk/'include',
              '-O2','-std=c11','-Wall','-Wextra','-Werror','-fPIC','-D__ORBIS__','-DPS4',
              '-msse2','-mno-avx','-mno-avx2','-I'+str(ROOT/'src')]
native=[]
for source in [ROOT/'src/pe_fixture_loader.c',ROOT/'platform/orbis_memory.c']:
    obj=out/(source.stem+'-orbis.o')
    run([clang,*native_flags,'-c',source,'-o',obj]);native.append(obj)
run([llvm/'llvm-ar.exe','rcs',out/'libvi_fixture_orbis.a',*native])
headers=run([llvm/'llvm-readelf.exe','-h',*native])
if headers.count('ELF64')!=2:raise RuntimeError('Unexpected native object format')
record={'stage':'groundwork_original_fixture','fixture_sha256':hashlib.sha256((out/'fixture.exe').read_bytes()).hexdigest(),
        'host_executed':True,'host_output':host_output,'native_objects_cross_compiled':True,
        'native_code_executed':False,'ps4_tested':False,'gta_vi_booted':False,
        'installable_pkg_built':False,'writable_executable_pages_requested':False}
(out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
# Commands remain local: they contain machine-specific paths, not published evidence.
(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
print('Original host fixture passed; native objects compiled. No console action.',flush=True)
