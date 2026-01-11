#!/bin/bash
# Plan9Cog QEMU Virtual Machine Setup Script
# This script sets up a QEMU virtual machine for testing cogplan9

set -e

# Configuration
VM_DIR="${VM_DIR:-$HOME/plan9cog-vm}"
DISK_SIZE="${DISK_SIZE:-20G}"
MEMORY="${MEMORY:-2048}"
CPUS="${CPUS:-2}"
ISO_URL="${ISO_URL:-http://9front.org/iso/9front-10522.amd64.iso.gz}"
ISO_NAME="9front.iso"
DISK_NAME="plan9cog.qcow2"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_dependencies() {
    log_info "Checking dependencies..."
    
    local missing=()
    
    if ! command -v qemu-system-x86_64 &> /dev/null; then
        missing+=("qemu-system-x86")
    fi
    
    if ! command -v qemu-img &> /dev/null; then
        missing+=("qemu-utils")
    fi
    
    if ! command -v wget &> /dev/null && ! command -v curl &> /dev/null; then
        missing+=("wget or curl")
    fi
    
    if [ ${#missing[@]} -ne 0 ]; then
        log_error "Missing dependencies: ${missing[*]}"
        echo ""
        echo "Install on Debian/Ubuntu:"
        echo "  sudo apt-get install qemu-system-x86 qemu-utils wget"
        echo ""
        echo "Install on macOS:"
        echo "  brew install qemu wget"
        echo ""
        echo "Install on Fedora/RHEL:"
        echo "  sudo dnf install qemu-system-x86 qemu-img wget"
        exit 1
    fi
    
    log_info "All dependencies satisfied."
}

create_vm_directory() {
    log_info "Creating VM directory: $VM_DIR"
    mkdir -p "$VM_DIR"
    mkdir -p "$VM_DIR/shared"
}

download_iso() {
    local iso_path="$VM_DIR/$ISO_NAME"
    local iso_gz_path="$VM_DIR/${ISO_NAME}.gz"
    
    if [ -f "$iso_path" ]; then
        log_info "ISO already exists: $iso_path"
        return 0
    fi
    
    log_info "Downloading 9front ISO..."
    log_info "URL: $ISO_URL"
    
    if command -v wget &> /dev/null; then
        wget -O "$iso_gz_path" "$ISO_URL"
    else
        curl -L -o "$iso_gz_path" "$ISO_URL"
    fi
    
    log_info "Extracting ISO..."
    gunzip "$iso_gz_path"
    
    # Rename to standard name
    local extracted_name=$(ls "$VM_DIR"/*.iso 2>/dev/null | head -1)
    if [ -n "$extracted_name" ] && [ "$extracted_name" != "$iso_path" ]; then
        mv "$extracted_name" "$iso_path"
    fi
    
    log_info "ISO ready: $iso_path"
}

create_disk_image() {
    local disk_path="$VM_DIR/$DISK_NAME"
    
    if [ -f "$disk_path" ]; then
        log_warn "Disk image already exists: $disk_path"
        read -p "Overwrite? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            log_info "Keeping existing disk image."
            return 0
        fi
    fi
    
    log_info "Creating disk image: $disk_path ($DISK_SIZE)"
    qemu-img create -f qcow2 "$disk_path" "$DISK_SIZE"
    log_info "Disk image created."
}

create_launch_scripts() {
    log_info "Creating launch scripts..."
    
    # Install script (boot from ISO)
    cat > "$VM_DIR/install.sh" << 'INSTALL_EOF'
#!/bin/bash
# Boot Plan 9 from ISO for installation
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

qemu-system-x86_64 \
    -m ${MEMORY:-2048} \
    -smp ${CPUS:-2} \
    -cpu host \
    -enable-kvm \
    -hda "$SCRIPT_DIR/plan9cog.qcow2" \
    -cdrom "$SCRIPT_DIR/9front.iso" \
    -boot d \
    -net nic,model=virtio \
    -net user,hostfwd=tcp::5640-:564,hostfwd=tcp::17010-:17010 \
    -vga std \
    -usb \
    -device usb-tablet \
    -name "Plan9Cog Install" \
    "$@"
INSTALL_EOF
    chmod +x "$VM_DIR/install.sh"
    
    # Run script (boot from disk)
    cat > "$VM_DIR/run.sh" << 'RUN_EOF'
#!/bin/bash
# Boot Plan 9 from installed disk
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

qemu-system-x86_64 \
    -m ${MEMORY:-2048} \
    -smp ${CPUS:-2} \
    -cpu host \
    -enable-kvm \
    -hda "$SCRIPT_DIR/plan9cog.qcow2" \
    -boot c \
    -net nic,model=virtio \
    -net user,hostfwd=tcp::5640-:564,hostfwd=tcp::17010-:17010 \
    -vga std \
    -usb \
    -device usb-tablet \
    -virtfs local,path="$SCRIPT_DIR/shared",mount_tag=host0,security_model=mapped-xattr \
    -name "Plan9Cog" \
    "$@"
RUN_EOF
    chmod +x "$VM_DIR/run.sh"
    
    # Headless run script (no GUI)
    cat > "$VM_DIR/run-headless.sh" << 'HEADLESS_EOF'
#!/bin/bash
# Boot Plan 9 headless (for servers/CI)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

qemu-system-x86_64 \
    -m ${MEMORY:-2048} \
    -smp ${CPUS:-2} \
    -cpu host \
    -enable-kvm \
    -hda "$SCRIPT_DIR/plan9cog.qcow2" \
    -boot c \
    -net nic,model=virtio \
    -net user,hostfwd=tcp::5640-:564,hostfwd=tcp::17010-:17010 \
    -nographic \
    -serial mon:stdio \
    -virtfs local,path="$SCRIPT_DIR/shared",mount_tag=host0,security_model=mapped-xattr \
    -name "Plan9Cog-Headless" \
    "$@"
HEADLESS_EOF
    chmod +x "$VM_DIR/run-headless.sh"
    
    # macOS version (no KVM)
    cat > "$VM_DIR/run-macos.sh" << 'MACOS_EOF'
#!/bin/bash
# Boot Plan 9 on macOS (no KVM)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

qemu-system-x86_64 \
    -m ${MEMORY:-2048} \
    -smp ${CPUS:-2} \
    -cpu qemu64 \
    -hda "$SCRIPT_DIR/plan9cog.qcow2" \
    -boot c \
    -net nic,model=virtio \
    -net user,hostfwd=tcp::5640-:564,hostfwd=tcp::17010-:17010 \
    -vga std \
    -usb \
    -device usb-tablet \
    -virtfs local,path="$SCRIPT_DIR/shared",mount_tag=host0,security_model=mapped-xattr \
    -name "Plan9Cog" \
    "$@"
MACOS_EOF
    chmod +x "$VM_DIR/run-macos.sh"
    
    log_info "Launch scripts created."
}

create_cogplan9_sync_script() {
    log_info "Creating cogplan9 sync script..."
    
    cat > "$VM_DIR/sync-cogplan9.sh" << 'SYNC_EOF'
#!/bin/bash
# Sync cogplan9 source to the shared folder for VM access
# Run this from the cogplan9 repository root

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHARED_DIR="$SCRIPT_DIR/shared"
COGPLAN9_DIR="${1:-.}"

if [ ! -d "$COGPLAN9_DIR/sys" ]; then
    echo "Error: Not in cogplan9 repository root"
    echo "Usage: $0 [path-to-cogplan9]"
    exit 1
fi

echo "Syncing cogplan9 to shared folder..."

# Create cogplan9 directory in shared folder
mkdir -p "$SHARED_DIR/cogplan9"

# Sync the Plan9Cog specific files
rsync -av --delete \
    --include='sys/***' \
    --include='PLAN9COG*.md' \
    --include='ARCHITECTURE.md' \
    --include='test-plan9cog.*' \
    --exclude='*' \
    "$COGPLAN9_DIR/" "$SHARED_DIR/cogplan9/"

echo "Sync complete!"
echo ""
echo "In Plan 9, mount the shared folder with:"
echo "  mount -a /srv/host0 /n/host"
echo ""
echo "Then copy cogplan9 files:"
echo "  cp -r /n/host/cogplan9/sys/* /sys/"
SYNC_EOF
    chmod +x "$VM_DIR/sync-cogplan9.sh"
}

print_instructions() {
    echo ""
    echo "=============================================="
    echo "  Plan9Cog VM Setup Complete!"
    echo "=============================================="
    echo ""
    echo "VM Directory: $VM_DIR"
    echo ""
    echo "Quick Start:"
    echo ""
    echo "1. Install Plan 9 (first time only):"
    echo "   cd $VM_DIR && ./install.sh"
    echo ""
    echo "   During installation:"
    echo "   - Boot from CD"
    echo "   - Run 'inst/start' to begin installation"
    echo "   - Follow the prompts (use defaults for most)"
    echo "   - Reboot when complete"
    echo ""
    echo "2. Run Plan 9:"
    echo "   cd $VM_DIR && ./run.sh"
    echo ""
    echo "3. Sync cogplan9 source (from cogplan9 repo):"
    echo "   $VM_DIR/sync-cogplan9.sh /path/to/cogplan9"
    echo ""
    echo "Network Ports:"
    echo "  - 9P/Styx: localhost:5640 -> VM:564"
    echo "  - Custom:  localhost:17010 -> VM:17010"
    echo ""
    echo "Shared Folder:"
    echo "  Host: $VM_DIR/shared/"
    echo "  VM:   mount -a /srv/host0 /n/host"
    echo ""
    echo "For macOS (no KVM):"
    echo "   ./run-macos.sh"
    echo ""
    echo "For headless/CI:"
    echo "   ./run-headless.sh"
    echo ""
}

main() {
    echo "========================================"
    echo "  Plan9Cog QEMU VM Setup"
    echo "========================================"
    echo ""
    
    check_dependencies
    create_vm_directory
    download_iso
    create_disk_image
    create_launch_scripts
    create_cogplan9_sync_script
    print_instructions
}

# Run main function
main "$@"
