# PS4-MediaVault

Homebrew media player for jailbroken PS4 consoles (GoldHEN), built on the open-source
[OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain).

## Roadmap
- [x] **Phase 0** - repo, CI, portable list/sort/filter code with host unit tests
- [x] **Phase 1** - controller-driven file browser (internal `/data`; USB shown but gated by the sandbox)
- [ ] **Phase 1.5** - USB access from inside the app, without a separate jailbreak library
- [x] **Phase 2** - video playback via the system `libSceAvPlayer` (hardware decode) - written, **not yet verified on hardware** (see `docs/NOTES.md`)
- [ ] **Phase 3** - seek/volume, resume position, subtitles
- [ ] **Phase 4** - music, photos, thumbnails, network sources

## Build (Linux)
```
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain   # fish: set -Ux OO_PS4_TOOLCHAIN ...
make test     # host unit tests
make setup    # copies icon / runtime PRXs from a toolchain sample (one-time)
make          # builds the .pkg
```
On Arch/CachyOS, PkgTool.Core may also need `openssl-1.1` (AUR) and
`DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1`; see `docs/NOTES.md`.

## Controls
Browser: D-pad Up/Down move, L1/R1 page, X open, O back.
Playback: X play/pause, O stop and return to the browser.
Close the app with the PS button.
Put test videos somewhere under `/data` (for example `/data/videos`).

## Playback (Phase 2) - needs a hardware pass
`src/mv_avplayer.h` declares the real `SceAvPlayer*` structs/functions, because the
toolchain's own `orbis/AvPlayer.h` only has empty, parameterless stubs (just enough
to satisfy the linker). Those declarations are transcribed from the published PS
Vita AvPlayer headers, which share the same struct layout and ABI family, with the
PS4-only functions added by name. **This has not been run on a console yet.** If
`sceAvPlayerInit()` crashes or hands back a bad handle, that header is the first
thing to check against a disassembly of `libSceAvPlayer.sprx` on your firmware.
Add `-lSceAvPlayer` to your toolchain's `lib/` stubs if it isn't already there.

The toolchain's `samples/_common/graphics.cpp` is compiled at build time and is not
copied into this repo (the toolchain is GPLv3 - check licensing before redistributing).
