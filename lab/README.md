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

Outputs remain under ignored `build/loader-lab/`. The script compiles source, executes **only our generated host experiment**, cross-compiles native objects, creates an archive and links an original-fixture ELF. It does not connect to, install on, or reboot a PS4. Generated binaries are not committed.

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

The host uses Windows memory APIs. Explicit ABI attributes exercise the bridge on that host, but do not establish that runtime permissions, cache coherence, entry execution or cleanup work on Orbis. The runner links as ELF64/FreeBSD x86-64, **not an installable PKG or a console-validated app**.

The next hardware experiment must be separately reviewed and attended. It needs visible diagnostics, clean failure/exit behaviour and confirmation that the console is available. No GTA VI code, assets, shader pack or executable was used.

## Native runner prepared, not executed

`lab/orbis_fixture_main.c` embeds only our generated fixture; it has no downloader,
external executable input or console connection. Without the exact
`--execute-original-fixture` argument, it returns 2 without mapping or executing code.
With that argument it maps the fixture, checks the expected single import and relocation,
explicitly invokes the entry, releases the image and expects 42. Load or permission errors
stop rather than attempting an exploit or changing console permissions.

The linked `build/loader-lab/original-fixture-orbis.elf` is a **build artifact only**.
Its stdout/stderr diagnostics are not a TV interface. It still needs an attended native
app wrapper with visible diagnostics and explicit user confirmation, lifecycle review,
and hardware validation. Do not inject this ELF as a payload. No PKG was built.
The embedded PE has no TLS; the native SDK C runtime itself uses native TLS,
which does not establish support for Windows/PE TLS.

PE link timestamps are fixed to zero for repeatable fixture bytes with the same toolchain.
