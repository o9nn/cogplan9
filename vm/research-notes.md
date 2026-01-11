# Browser-Based VM Research Notes

## v86 Emulator (copy.sh/v86)

**URL:** https://copy.sh/v86/
**GitHub:** https://github.com/copy/v86

### Key Features:
- x86 PC emulator running in the browser
- Machine code translated to WebAssembly for performance
- Already supports Plan 9:
  - **9front** (5.2+ MB) - Modern, actively maintained fork
  - **9legacy** (13 MB) - Historic, patch-based

### Configuration Options:
- CD image, Floppy disk image, Hard disk image support
- Memory size configurable
- Video memory configurable
- Networking proxy support (inbrowser, public relay, wisp, fetch)
- ACPI support (experimental)
- Boot order configurable

### Technical Details:
- Uses WebAssembly for x86 emulation
- Supports 9pfs for file sharing
- Can boot from CD, floppy, or hard disk
- Version: ccb5e1ce (Sep 20, 2025)

## WebVM (CheerpX)

**URL:** https://webvm.io/
**GitHub:** https://github.com/leaningtech/webvm

### Key Features:
- Linux virtualization in WebAssembly
- Powered by CheerpX virtualization engine
- Full Linux desktop environment support (WebVM 2.0)
- Persistent data storage
- Networking support

### Limitations:
- Primarily focused on Linux
- CheerpX is proprietary (free for non-commercial use)

## Wanix

**URL:** https://wanix.sh/
**GitHub:** https://github.com/tractordev/wanix

### Key Features:
- "The Spirit of Plan 9 in Wasm"
- Runs WASI and x86 programs on web pages
- Plan 9 ideas applied in the browser
- Built-in emulator for x86 support

## Recommendation for Plan9Cog

**Best Choice: v86**

Reasons:
1. Already has Plan 9 support (9front and 9legacy)
2. Open source (BSD-2-Clause license)
3. Active development
4. WebAssembly-based for good performance
5. Can be self-hosted on GitHub Pages
6. Supports custom disk images

### Implementation Plan:
1. Fork/use v86 library
2. Create custom 9front disk image with cogplan9 extensions
3. Build GitHub Pages site with v86 embedded
4. Configure networking for 9P access


## Disk Image Hosting Research (Updated)

### v86 Library CDN Sources
- **libv86.js**: `https://cdn.jsdelivr.net/npm/v86@latest/build/libv86.js`
- **v86.wasm**: `https://cdn.jsdelivr.net/npm/v86@latest/build/v86.wasm`
- **SeaBIOS**: `https://cdn.jsdelivr.net/gh/copy/v86@master/bios/seabios.bin`
- **VGA BIOS**: `https://cdn.jsdelivr.net/gh/copy/v86@master/bios/vgabios.bin`

### Disk Image Challenge
The official v86 demo uses a private CDN (`v86-user-images.b-cdn.net`) that is not publicly accessible.

### Options for 9front Disk Image

1. **Official 9front ISO** (http://9front.org/iso/)
   - Latest: `9front-11321.386.iso.gz` (~232 MB compressed)
   - Too large for browser download without preprocessing

2. **Self-hosted compressed image**
   - Create a minimal 9front installation
   - Compress with zstd for async loading
   - Host on GitHub releases or CDN

3. **Use saved state**
   - v86 supports loading saved states
   - Pre-boot 9front and save state
   - Much faster startup time

### Current Solution
Using a minimal Linux image (Buildroot) as a fallback while we prepare a proper 9front image.
The simonw/tools example uses: `https://cdn.jsdelivr.net/gh/nicholasadamou/v86@latest/images/linux.iso`
