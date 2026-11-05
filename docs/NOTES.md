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

## Phase 2 risk
The toolchain's `orbis/AvPlayer.h` only declares bare names (`void sceAvPlayerInit();`)
with no parameter types or structs. Real prototypes/structs must be sourced elsewhere
and verified on hardware before video playback can work.
