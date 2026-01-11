# Plan9Cog Web VM

This directory contains the browser-based Plan 9 virtual machine for Plan9Cog.

## Overview

Plan9Cog Web VM allows you to run Plan 9 (9front) directly in your web browser using the [v86](https://github.com/copy/v86) x86 emulator. The emulator translates x86 machine code to WebAssembly for near-native performance.

## Live Demo

Visit the live demo at: **https://cogpy.github.io/cogplan9/**

## Features

- **Browser-based**: No installation required, runs entirely in your browser
- **WebAssembly powered**: Uses v86's x86-to-WASM JIT for good performance
- **Full Plan 9 experience**: Runs 9front with rio window manager
- **Persistent state**: Save and restore VM state (coming soon)
- **Networking**: WebSocket-based networking support (coming soon)

## Technical Details

### Emulator Configuration

| Setting | Value |
|---------|-------|
| Memory | 512 MB |
| VGA Memory | 8 MB |
| Disk Image | 9front ISO (~700 MB) |
| Emulator | v86 (WebAssembly) |

### Browser Requirements

- Modern browser with WebAssembly support
- Chrome 57+, Firefox 52+, Safari 11+, Edge 16+
- Recommended: 4GB+ RAM available

### CORS Headers

The deployment includes proper CORS headers for SharedArrayBuffer support:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

## Local Development

To run locally:

```bash
# From the repository root
cd web

# Start a local server with proper headers
python3 -c "
from http.server import HTTPServer, SimpleHTTPRequestHandler

class CORSRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

HTTPServer(('localhost', 8000), CORSRequestHandler).serve_forever()
"
```

Then open http://localhost:8000 in your browser.

## Directory Structure

```
web/
├── index.html          # Main page with v86 emulator
├── assets/
│   └── favicon.svg     # Site favicon
├── images/             # Custom disk images (optional)
├── bios/               # Custom BIOS files (optional)
└── README.md           # This file
```

## Customization

### Using a Custom Disk Image

To use a custom Plan 9 disk image:

1. Create or obtain a disk image (ISO, IMG, or QCOW2 converted to raw)
2. Compress with zstd: `zstd -19 your-image.iso -o your-image.iso.zst`
3. Place in the `images/` directory
4. Update `VM_CONFIG.hda.url` in `index.html`

### Building a Custom 9front Image with Plan9Cog

```bash
# 1. Download 9front ISO
wget http://9front.org/iso/9front-10522.amd64.iso.gz
gunzip 9front-10522.amd64.iso.gz

# 2. Mount and modify (requires root on Linux)
# ... add cogplan9 files ...

# 3. Compress for web deployment
zstd -19 9front-cogplan9.iso -o images/9front-cogplan9.iso.zst
```

## Deployment

The site is automatically deployed to GitHub Pages when changes are pushed to the `web/` directory on the `main` branch.

Manual deployment:
1. Go to repository Settings > Pages
2. Set Source to "GitHub Actions"
3. Push changes to trigger deployment

## Credits

- [v86](https://github.com/copy/v86) - x86 emulator by copy
- [9front](https://9front.org/) - Plan 9 fork
- [Plan 9 from Bell Labs](https://9p.io/plan9/) - Original OS

## License

The web interface is part of the Plan9Cog project. See the main repository LICENSE for details.

v86 is licensed under BSD-2-Clause.
9front is licensed under the Lucent Public License.
