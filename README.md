# VI Orbis Research

**PT-BR:** pesquisa aberta de compatibilidade para PS4/Orbis, motivada pela possibilidade de executar GTA VI localmente. **Ainda não é um port jogável.** Não há jogo iniciando, primeiro quadro, menu ou desempenho medido no PS4.

**EN:** open compatibility research for PS4/Orbis, motivated by the question of local GTA VI execution. **This is not a playable port.** No GTA VI executable, frame, menu, gameplay or FPS result has been demonstrated on PS4.

## Current scope

- Study publicly available compatibility-layer code, beginning with VIctory, its AMD-focused fork, and TuxBox.
- Identify Windows, CPU/ABI, graphics and memory assumptions before adapting anything.
- Use OpenOrbis and self-authored synthetic programs for initial loader experiments.
- Keep PS4-native execution distinct from Linux, Wine and remote streaming. They are not interchangeable milestones.

The current repository contains a reproducible **read-only upstream source audit** and a strict **original-fixture loader experiment**. The Windows host experiment mapped, relocated, bound and executed our own fixture successfully; the loader and Orbis adapter also cross-compiled. **There is no PS4 execution evidence yet.** Work on Morrowind/OpenMW is a separate project and does not count as GTA VI compatibility evidence.

## Roadmap

Inspired by [PortWatch VI's public methodology](https://github.com/4lwayz/portwatch-vi/blob/main/assets/js/scoring.js), adapted to **PS4**, not its PC leaderboard:

1. **Groundwork:** loader/system APIs, shader translation, graphics backend, and validation with another program/game.
2. **Executable starts:** actual title instructions execute after mapping, relocation and import resolution.
3. **First frame:** frames rendered by the title, not our test app or a splash image.
4. **Menu:** real menus respond to input.
5. **World:** the game loads and renders a world.
6. **Playable:** player movement and interaction work.
7. **Quality:** story progress, rendering correctness, performance and long-session stability.

No percentage is assigned to untested work. An upstream's PC results do not transfer to this project.

### First technical milestone

Build an original tiny PE32+ fixture and prove mapping, base relocation, an explicitly allowed imported function, and the Windows-x64/Orbis calling-convention boundary. Reject unsupported features visibly. Verify on a host before proposing an attended PS4 test. A fixture passing is **groundwork**, never “GTA VI boots.”

The host portion now passes. See [loader lab, limitations and reproduction](lab/README.md) and [published result](research/loader-host-result.json). The Orbis implementation has only been compiled, not executed or installed on a console.

## Reproduce the source audit

Python 3.10+; standard library only:

```sh
python tools/audit_upstreams.py
# Optional existing authenticated GitHub CLI for API rate limits:
python tools/audit_upstreams.py --gh /path/to/gh
```

The script reads public metadata and selected text files at recorded commits. It does not clone/run project code, download game data, fetch game executables, or modify the console. Source snapshots stay local and are ignored by Git; the report records URLs and hashes. Lexical marker counts are navigation aids, **not proof of compatibility**.

## Useful contributions

- Loader/PE/ABI analysis using independently written fixtures.
- Documented CPU-instruction and address-space constraints.
- OpenOrbis graphics, audio and threading expertise.
- Review of reusable upstream components and their licences.
- Reproducible hardware results with exact commits, settings, logs and measurements.

Please distinguish **proposal**, **compiled**, **host-tested**, and **PS4-tested**. Do not substitute success-returning stubs for missing functionality or claim support from a build alone.

## Repository boundaries

Do not submit game assets/executables, leaked builds/source/SDKs, keys, credentials, DRM-bypass material, links to obtain those materials, or another person's saves. Keep captures and logs free of account details and secrets. There is no game downloader or installer in this repository.

We do not claim that the PS4 can ultimately run this title, meet a target frame rate, or finish the game. This project investigates that question with evidence.

Not affiliated with Rockstar Games, Take-Two, Sony, Microsoft, OpenOrbis, PortWatch or the upstream maintainers. All marks belong to their respective owners.

## Licence

Original repository code: GPL-3.0-or-later. External projects retain their own licences. Linking to a project or inspecting its public source does not transfer authorship or licence its code under ours.
