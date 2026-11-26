# Initial upstream review

This is a **selected-source architecture review**, not a complete security audit, successful build, or hardware test. Commits and inspected-file hashes are in [upstream-audit.json](upstream-audit.json). Reported performance belongs to upstream authors and does not establish PS4 feasibility.

## VIctory — reference architecture

Pinned commit: `63bcca43c6a00a758510398fa43096ce7e4f7df0`.

The workspace separates PE analysis, loading, host API implementations, graphics, shader translation, formats, caching and title-specific handling. This is useful for defining interfaces rather than building one undifferentiated emulator. The loader, graphics and API manifests explicitly depend on Windows libraries. The graphics backend targets host D3D12, not Orbis. The workspace requests Rust 1.88 / edition 2024.

**Potential value:** reference PE/ABI behaviour, interface boundaries and synthetic-fixture methodology.

**Not portable unchanged:** host virtual memory, exception/TLS handling, Windows pass-through, and the D3D12 implementation. Source availability is not an Orbis runtime.

Sources: [architecture](https://github.com/Marvisak/VIctory/blob/63bcca43c6a00a758510398fa43096ce7e4f7df0/docs/architecture.md), [workspace](https://github.com/Marvisak/VIctory/blob/63bcca43c6a00a758510398fa43096ce7e4f7df0/Cargo.toml), [loader dependencies](https://github.com/Marvisak/VIctory/blob/63bcca43c6a00a758510398fa43096ce7e4f7df0/crates/victory-loader/Cargo.toml).

## Belobug AMD fork — optimisation research

Pinned commit: `cd8dfc06ace7ccb91a19431e74bc2d0de2a10145`.

The fork retains the same major crate boundaries and Windows/D3D12 dependencies. Its README describes reducing descriptor scans, changing constant-buffer access and reducing memory-guard contention. These identify work that can be measured later, not ready-made PS4 fixes. Its stated PC memory use is substantial and cannot be equated to a PS4 budget. The workspace also changes the compression dependency and enables release link-time optimisation.

**Potential value:** understand where redundant host work was removed, then reproduce the relevant behaviour using original fixtures.

**Do not infer:** shared AMD branding does not establish matching GPU features, shader support, memory behaviour, or frame rate. These changes are not an Orbis backend.

Sources: [README](https://github.com/Belobug/VIctory-Belobug-AMD/blob/cd8dfc06ace7ccb91a19431e74bc2d0de2a10145/README.md), [workspace](https://github.com/Belobug/VIctory-Belobug-AMD/blob/cd8dfc06ace7ccb91a19431e74bc2d0de2a10145/Cargo.toml).

## TuxBox — alternative host separation

Pinned commit: `b3184d35e00afc0af30349301a404786d0ea78c0`.

This project describes a Linux-first route, with Proton providing the Windows-facing environment on Linux. That is **not** direct PS4-native execution and does not remove the need for an Orbis host layer. Its self-authored Lanes game is a useful example of proving infrastructure without using a retail title. The workspace manifest requests Rust 1.95 and still contains Windows API dependencies.

**Potential value:** compare host/API boundaries, shader infrastructure and original test-game design.

**Not established:** successful execution of its layers on Orbis, graphics compatibility with the PS4, or a complete title running on this console.

Sources: [README](https://github.com/brcly/TuxBox/blob/b3184d35e00afc0af30349301a404786d0ea78c0/README.md), [workspace](https://github.com/brcly/TuxBox/blob/b3184d35e00afc0af30349301a404786d0ea78c0/Cargo.toml).

## Initial decision

Use VIctory as an architectural reference, the AMD fork as later optimisation evidence, and TuxBox as an alternative-design comparison. **No fork is selected as a drop-in PS4 port.** No upstream result is credited to this project.

Next implement the small original PE fixture and a strict loader contract. Measure mapping, relocation, import resolution, calling convention, thread/TLS needs, and executable-memory permissions separately. Only then propose a bounded, attended native-console test. Graphics and game assets are deliberately outside that first experiment.

Preserve upstream licences if code is reused. No upstream source has been copied into this repository's implementation; local inspection snapshots are excluded from Git.
