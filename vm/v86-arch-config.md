# v86 Arch Linux Configuration Findings

The v86 demo for Arch Linux successfully boots to a graphical text console at 1024x768x32 resolution.

## Key Observations

The Arch Linux image boots to a root shell with the following features:
- VGA Mode: Graphical 1024x768x32
- Networking available via `./networking.sh`
- X.org desktop available via `./startx.sh`
- Serial console support for text I/O

## Configuration Requirements

For a working v86 browser VM, we need:
1. A proper disk image (not just bzImage)
2. The image must include VGA drivers for graphical output
3. The v86 emulator creates a canvas element automatically

## Next Steps for Plan9Cog

To get 9front working in the browser:
1. Download the 9front ISO from http://9front.org/iso/
2. Convert to a format suitable for v86 (raw or split)
3. Host on a CDN with CORS headers
4. Configure v86 with cdrom or hda pointing to the image

Alternatively, use the copy.sh hosted images if they become publicly available.
