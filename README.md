# PS4-MediaVault

Homebrew media player for jailbroken PS4 consoles (GoldHEN), built on the open-source
[OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain).

## Roadmap
- [x] **Phase 0** - repo, CI, portable list/sort/filter code with host unit tests
- [x] **Phase 1** - controller-driven file browser (internal `/data`; USB shown but gated by the sandbox)
- [ ] **Phase 1.5** - USB access via a sandbox escape (e.g. libjbc)
- [ ] **Phase 2** - video playback via the system `libSceAvPlayer` (hardware decode)
- [ ] **Phase 3** - seek/pause/volume, resume position, subtitles
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

## Controls (Phase 1)
D-pad Up/Down move, L1/R1 page, X open, O back. Close the app with the PS button.
Put test videos somewhere under `/data` (for example `/data/videos`).

The toolchain's `samples/_common/graphics.cpp` is compiled at build time and is not
copied into this repo (the toolchain is GPLv3 - check licensing before redistributing).
