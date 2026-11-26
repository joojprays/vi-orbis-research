# Original-fixture PE loader lab

## What exists

- `fixtures/fixture.c`: independently written x64 PE code with a pointer needing base relocation and one imported function. The entry returns 42 when both work.
- `fixtures/import_stub.c`: produces the import library during build. Its DLL is **not loaded** in the experiment.
- `src/pe_fixture_loader.c`: bounded PE32+ mapping, relocation, import binding and per-page final permissions. Loading does not automatically execute the entry.
- `lab/host_windows.c`: Windows memory callbacks and an explicit allowlist mapping `vi-fixture.dll!fixture_add` to our own bridge. The bridge actually calls a non-inlined System V ABI helper from a Microsoft ABI function.
- `platform/orbis_memory.c`: OpenOrbis memory callbacks and the same bridge, compiled only. A rejected executable permission remains a failure; no patch or workaround is attempted.

## Reproduce on the existing Windows build environment

Use an existing LLVM/LLD installation, MSYS/Cygwin GCC and OpenOrbis SDK. No tool or game downloader is included.

```sh
python tools/build_loader_lab.py \
  --llvm-bin /path/to/llvm/bin \
  --host-cc /path/to/msys/usr/bin/gcc.exe \
  --orbis-sdk /path/to/OpenOrbis/PS4Toolchain
```

Outputs remain under ignored `build/loader-lab/`. The script compiles source, executes **only our generated host experiment**, cross-compiles two native objects and creates a native archive. It does not connect to, install on, or reboot a PS4. Generated binaries are not committed.

Expected positive result:

```text
fixture entry returned 42; imports=1 relocations=1
HOST LAB PASS: mapping, relocation, allowlisted import, entry call, W^X permissions and cleanup.
```

The host run also rejects eight malformed variants, 64 truncated headers, an unknown import and a forced protection failure. Allocation/release counts are checked on failure and after execution. Final pages are never requested writable and executable together.

## Deliberately limited profile

This is **not a general-purpose Windows, Xbox or game loader**. It accepts only small AMD64 PE32+ fixture executables, with image and section budgets and section alignment at least the host page size. Our fixture uses 16 KiB section alignment to model the Orbis adapter.

TLS, exception/unwind tables, security/load-config directories, bound/delayed imports, CLR, ordinal imports, overlapping sections/IAT slots, executable+writable pages and unknown imports are rejected. The strict rejection of unwind tables limits the fixture compiler options; it does not mean exceptions have been implemented. The library is a lab artifact, not a hardened boundary for arbitrary untrusted executables.

Import binding is deferred until parsing completes so IAT writes cannot change descriptors still being parsed. Relocations into headers/loader directory data are refused. The caller must initialize output to zero, retain ownership of successful output, and unload it before reuse.

## Remaining evidence

The host uses Windows memory APIs. Explicit ABI attributes exercise the bridge on that host, but do not establish that runtime permissions, cache coherence, entry execution or cleanup work on Orbis. Native objects are ELF64/FreeBSD x86-64, **not a complete native app or installable PKG**.

The next hardware experiment must be separately reviewed and attended. It needs visible diagnostics, clean failure/exit behaviour and confirmation that the console is available. No GTA VI code, assets, shader pack or executable was used.
