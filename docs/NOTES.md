# Hardware findings (GoldHEN, installed as a PKG)

Learned from klog output on a real console:

- **Bundle the runtime PRXs.** The package must contain `sce_module/libc.prx` and
  `sce_module/libSceFios2.prx` (`make setup` copies them from a toolchain sample).
  Without them the app dies on launch and the console reports "data is corrupted".
- **Use PS4 kernel file calls, not POSIX.** `sceKernelOpen/Stat/Getdents/Close`
  work in the sandbox; POSIX `access()` reported nothing readable.
- **Directory entries** use the classic layout: `u32 fileno, u16 reclen, u8 type, u8 namlen, name[]`
  (type 4 = directory, 8 = file).
- **Do not return from `main()`.** It raised SIGSYS (a blocked syscall) on exit.
  The app loops forever; close it with the PS button. A clean quit path is still TODO.
- **`/data` is readable. `/mnt` (and so `/mnt/usb0`) is NOT visible** from the sandboxed app.
  USB support needs a sandbox escape (e.g. libjbc) - not implemented yet.

## Phase 2 status
The toolchain's `orbis/AvPlayer.h` only declares bare names (`void sceAvPlayerInit();`)
with no parameter types or structs, as expected. `src/mv_avplayer.h` now has real
prototypes/structs instead, transcribed from the public PS Vita AvPlayer headers
(same library family, same ABI going back to PS3/Vita - see
https://docs.vitasdk.org/group__SceAvPlayerUser.html), plus the PS4-only functions
(`EnableStream`, `GetStreamInfo`, `JumpToTime`, the `Ex` variants, etc.) added by
name only, with no known signature yet.

## First hardware run: fixed
First attempt crashed immediately on calling into AvPlayer:
```
# exception: 0xa0020101 (PRX_NOT_RESOLVED_FUNCTION)
# === Call to unpatched function is detected!!! ===
# Required Module Name    : libSceAvPlayer
# Required Function NID   : 0x692EBA448D201A0A
```
and `libSceAvPlayer.sprx` was conspicuously absent from the backtrace's loaded-module
list (libkernel, libSceVideoOut, libScePad, etc. were all there). Root cause: unlike
VideoOut/Pad/UserService, AvPlayer is **not** one of the modules a fresh process
has loaded by default - it has to be brought in with
`sceSysmoduleLoadModule(ORBIS_SYSMODULE_AV_PLAYER)` (`orbis/Sysmodule.h`,
id `0x00A5`) before any `sceAvPlayer*` call, or every call traps exactly like
this. Fixed in `mv_player_init()` (called once at startup), with
`-lSceSysmodule` added to the Makefile. `mv_player_open()` now also checks the
module actually loaded and fails cleanly instead of calling into an unresolved
import if it didn't.

This was the only hardware feedback so far - the struct-layout and NV12 questions
below are still open, since playback hasn't gotten past this point yet.

**Still unverified on hardware** - first things to check once the above gets you
into `sceAvPlayerInit()`:
- `sceAvPlayerInit()` returns a negative/garbage handle, or crashes outright ->
  the `SceAvPlayerInitData`/`SceAvPlayerMemAllocator`/`SceAvPlayerFileReplacement`
  field order or sizes in `mv_avplayer.h` are wrong for this firmware's
  `libSceAvPlayer.sprx`; check against a disassembly.
- Playback starts but the picture is wrong colors/shifted/garbled -> the NV12
  layout assumption in `mv_player.cpp`'s `nv12_to_rgb()` is wrong (could be a
  different plane order, a non-zero row pitch/padding, or the output isn't
  NV12 at all).
- No crash, nothing drawn -> check `sceAvPlayerGetVideoData()`'s return value
  and `info.pData`/`details.video.width/height` with a debug `printf`; the
  struct offsets inside `SceAvPlayerFrameInfo`/`SceAvPlayerStreamDetails`
  are the next thing to suspect.
- `-lSceAvPlayer` fails to link -> the toolchain's `lib/` stub for it is
  missing; it needs to come from wherever the other stubs (`-lSceVideoOut`
  etc.) in this project's `Makefile` came from.
