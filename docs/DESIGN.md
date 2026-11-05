# Design notes

## Why libSceAvPlayer
The PS4 has hardware video decode. Software-decoding 1080p H.264 with ffmpeg on
the Jaguar CPU is marginal, so Phase 2 targets the system AvPlayer module, which
decodes H.264/AAC (MP4-style containers) in hardware. Expect MKV/HEVC/odd codecs
to be limited; a software ffmpeg fallback is a later, optional phase.

## Storage paths
- Internal HDD: `/data` (homebrew-writable area)
- USB: `/mnt/usb0` ... `/mnt/usb7` (permissions depend on your jailbreak/loader;
  use exFAT or FAT32 drives)

## Layers
1. `fs_browser` - portable, unit-tested (done)
2. `ui` - framebuffer + controller (Phase 1, done)
3. `player` - AvPlayer wrapper: open/play/pause/stop (Phase 2, written; seek is Phase 3)

## Phase 2 implementation notes
- `src/mv_avplayer.h` declares the real `SceAvPlayer*` ABI (the toolchain's
  `orbis/AvPlayer.h` only has empty stubs). See its header comment and
  `docs/NOTES.md` for where this came from and what's unverified.
- `src/mv_player.cpp` feeds AvPlayer kernel file calls (`sceKernelOpen/Pread/Fstat`)
  for its file callbacks, and a simple over-allocating allocator for its memory
  callbacks, matching the style already used by `mv_fs_ps4.cpp`.
- Decoded video frames are assumed NV12 (one Y plane, then an interleaved,
  half-resolution U/V plane) and are converted to RGB on the CPU, one frame at
  a time, into a buffer capped at 1920x1080 (`MV_MAX_W/MV_MAX_H` in
  `mv_player.cpp`). `Scene2D` has no texture/blit primitive (see
  `samples/_common/graphics.h` in the toolchain) - only `DrawPixel` and
  `DrawRectangle` - so the frame is drawn with one `DrawPixel` call per output
  pixel, nearest-neighbour scaled and letterboxed to fit the screen. This is
  a straightforward first pass, not a fast one; if full-screen playback is too
  slow on hardware, the likely next step is drawing at a reduced output size
  (e.g. half-res) rather than 1920x1080 pixel-by-pixel.

## Known risks / unknowns
- **Nothing in Phase 2 has run on a console yet.** Treat `mv_avplayer.h`'s
  struct layout, the NV12 assumption, and the `DrawPixel`-per-pixel blit as
  the first three things to check if playback crashes, hangs, or shows wrong
  colors/garbage on real hardware.
- `basePriority` (1000) and `numOutputVideoFrameBuffers` (4) in
  `mv_player_open()` are plausible defaults, not confirmed values.
- Audio is not wired up yet - `sceAvPlayerGetAudioData` is declared but unused,
  so played-back video will currently be silent.
