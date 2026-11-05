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
2. `ui` - framebuffer + controller (Phase 1)
3. `player` - AvPlayer wrapper: open/play/pause/seek/close, frame -> VideoOut (Phase 2)

## Known risks / unknowns
- AvPlayer init params and frame-buffer handoff must be verified against the
  toolchain headers on real hardware.
- Library link names and Makefile flags differ between toolchain releases.
