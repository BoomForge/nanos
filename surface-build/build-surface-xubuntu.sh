#!/usr/bin/env bash
set -Eeuo pipefail

BASE_VERSION="24.04.4"
BASE_ISO="xubuntu-${BASE_VERSION}-desktop-amd64.iso"
OUT_ISO="xubuntu-${BASE_VERSION}-surface-pro-4-amd64.iso"
BASE_URL="https://cdimage.ubuntu.com/xubuntu/releases/noble/release"
WORKDIR="${SURFACE_BUILD_WORKDIR:-${RUNNER_TEMP:-/tmp}/surface-xubuntu-work}"
OUTDIR="${GITHUB_WORKSPACE:-$PWD}"
DL="$WORKDIR/download"
ISO="$WORKDIR/iso"
BASE="$WORKDIR/base"
STD="$WORKDIR/std-upper"
LIVE="$WORKDIR/live-upper"
STD_MERGED="$WORKDIR/std-merged"
FULL_MERGED="$WORKDIR/full-merged"
STD_WORK="$WORKDIR/std-work"
LIVE_WORK="$WORKDIR/live-work"

log(){ printf '\n\033[1;35m==> %s\033[0m\n' "$*"; }
die(){ echo "ERROR: $*" >&2; exit 1; }

cleanup(){
  set +e
  for m in "$FULL_MERGED/run" "$FULL_MERGED/sys" "$FULL_MERGED/proc" "$FULL_MERGED/dev/pts" "$FULL_MERGED/dev" "$FULL_MERGED/etc/resolv.conf" "$STD_MERGED/run" "$STD_MERGED/sys" "$STD_MERGED/proc" "$STD_MERGED/dev/pts" "$STD_MERGED/dev" "$STD_MERGED/etc/resolv.conf"; do
    mountpoint -q "$m" && sudo umount -lf "$m"
  done
  mountpoint -q "$FULL_MERGED" && sudo umount -lf "$FULL_MERGED"
  mountpoint -q "$STD_MERGED" && sudo umount -lf "$STD_MERGED"
}
trap cleanup EXIT INT TERM

log "Installing remaster tools"
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y xorriso squashfs-tools rsync curl wget gnupg ca-certificates

mkdir -p "$DL" "$WORKDIR"
log "Downloading official Xubuntu ${BASE_VERSION} ISO"
curl -fL --retry 5 --retry-delay 5 "$BASE_URL/$BASE_ISO" -o "$DL/$BASE_ISO"
curl -fsSL "$BASE_URL/SHA256SUMS" -o "$DL/SHA256SUMS"
(
  cd "$DL"
  grep " ${BASE_ISO}$" SHA256SUMS | sha256sum -c -
) || die "Official Xubuntu checksum failed"

log "Extracting ISO"
rm -rf "$ISO" "$BASE" "$STD" "$LIVE" "$STD_MERGED" "$FULL_MERGED" "$STD_WORK" "$LIVE_WORK"
mkdir -p "$ISO" "$BASE" "$STD" "$LIVE" "$STD_MERGED" "$FULL_MERGED" "$STD_WORK" "$LIVE_WORK"
xorriso -osirrox on -indev "$DL/$BASE_ISO" -extract / "$ISO" >/dev/null 2>&1
chmod -R u+w "$ISO"

for f in minimal.squashfs minimal.standard.squashfs minimal.standard.live.squashfs; do
  [[ -f "$ISO/casper/$f" ]] || die "Expected Xubuntu layer missing: casper/$f"
done

log "Unpacking Xubuntu layered filesystem"
sudo unsquashfs -d "$BASE" "$ISO/casper/minimal.squashfs" >/dev/null
sudo unsquashfs -d "$STD" "$ISO/casper/minimal.standard.squashfs" >/dev/null
sudo unsquashfs -d "$LIVE" "$ISO/casper/minimal.standard.live.squashfs" >/dev/null

# Xubuntu 24.04 uses layered squashfs images. Mount the standard install root
# as base + standard upper, so apt sees a complete filesystem while all package
# changes are captured back into the standard layer used by the installer.
log "Mounting standard install filesystem"
sudo mount -t overlay overlay -o "lowerdir=$BASE,upperdir=$STD,workdir=$STD_WORK" "$STD_MERGED"

# Prevent package postinst scripts from trying to start services in the chroot.
sudo tee "$STD_MERGED/usr/sbin/policy-rc.d" >/dev/null <<'EOF'
#!/bin/sh
exit 101
EOF
sudo chmod +x "$STD_MERGED/usr/sbin/policy-rc.d"

# Give the chroot network/device access only while packages are being installed.
sudo mount --bind /dev "$STD_MERGED/dev"
sudo mount --bind /dev/pts "$STD_MERGED/dev/pts"
sudo mount -t proc proc "$STD_MERGED/proc"
sudo mount -t sysfs sys "$STD_MERGED/sys"
sudo mount --bind /run "$STD_MERGED/run"
sudo mount --bind /etc/resolv.conf "$STD_MERGED/etc/resolv.conf"

log "Installing official linux-surface kernel and Surface Pro 4 touch stack"
sudo chroot "$STD_MERGED" /bin/bash -eux <<'CHROOT'
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y wget gnupg ca-certificates
wget -qO- https://raw.githubusercontent.com/linux-surface/linux-surface/master/pkg/keys/surface.asc \
  | gpg --dearmor > /etc/apt/trusted.gpg.d/linux-surface.gpg
echo 'deb [arch=amd64] https://pkg.surfacelinux.com/debian release main' \
  > /etc/apt/sources.list.d/linux-surface.list
apt-get update
apt-get install -y linux-image-surface linux-headers-surface iptsd libwacom-surface

# Normal Linux input stack for the user's Bluetooth keyboard/trackpad.
apt-get install -y bluez libinput-tools xinput blueman onboard || true

# The first test boot deliberately uses Secure Boot disabled, so do not add the
# MOK enrollment package here.
systemctl enable iptsd.service 2>/dev/null || true
update-initramfs -u -k all
apt-get clean
rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
CHROOT

sudo rm -f "$STD_MERGED/usr/sbin/policy-rc.d"

log "Finding Surface kernel"
SURFACE_VMLINUZ="$(find "$STD_MERGED/boot" -maxdepth 1 -type f -name 'vmlinuz-*-surface*' | sort -V | tail -n1)"
[[ -n "$SURFACE_VMLINUZ" ]] || die "No Surface kernel was installed"
SURFACE_VER="${SURFACE_VMLINUZ##*/vmlinuz-}"
SURFACE_INITRD="$STD_MERGED/boot/initrd.img-$SURFACE_VER"
[[ -f "$SURFACE_INITRD" ]] || die "Surface initrd missing: $SURFACE_INITRD"
echo "Surface live kernel: $SURFACE_VER"

# Finish chroot mounts before creating a full live merged view.
for m in run sys proc dev/pts dev etc/resolv.conf; do
  mountpoint -q "$STD_MERGED/$m" && sudo umount -lf "$STD_MERGED/$m"
done

log "Mounting full live filesystem"
sudo mount -t overlay overlay -o "lowerdir=$STD_MERGED,upperdir=$LIVE,workdir=$LIVE_WORK" "$FULL_MERGED"

# Add simple diagnostics into the existing live-only top layer.
sudo install -d "$LIVE/usr/local/bin" "$LIVE/etc/skel/Desktop"
sudo tee "$LIVE/usr/local/bin/surface-live-check" >/dev/null <<'CHECK'
#!/usr/bin/env bash
set +e
echo "=== Surface Pro 4 Xubuntu hardware check ==="
echo
echo "Kernel:"; uname -a
echo
echo "Surface/IPTS modules:"; lsmod | grep -Ei 'ipts|surface|mei' || true
echo
echo "IPTS units:"; systemctl --no-pager --full status 'iptsd*' 2>/dev/null || true
echo
echo "Input devices:"; libinput list-devices 2>/dev/null | grep -E 'Device:|Kernel:|Capabilities:' || true
echo
echo "Bluetooth devices:"; bluetoothctl devices 2>/dev/null || true
echo
echo "Look for '-surface' in the kernel name and an IPTS/Touch device above."
CHECK
sudo chmod +x "$LIVE/usr/local/bin/surface-live-check"
sudo tee "$LIVE/etc/skel/Desktop/Surface-Hardware-Check.desktop" >/dev/null <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Surface Hardware Check
Comment=Check Surface touchscreen, Bluetooth and trackpad support
Exec=xfce4-terminal --hold -e "sudo surface-live-check"
Icon=utilities-terminal
Terminal=false
Categories=System;
DESKTOP
sudo chmod +x "$LIVE/etc/skel/Desktop/Surface-Hardware-Check.desktop"
sudo tee "$LIVE/etc/surface-xubuntu-build" >/dev/null <<EOF
Surface Pro 4 custom Xubuntu live image
Base: Xubuntu $BASE_VERSION LTS
Surface kernel: $SURFACE_VER
Packages: linux-image-surface linux-headers-surface iptsd libwacom-surface
EOF

log "Updating manifests"
sudo chroot "$STD_MERGED" dpkg-query -W --showformat='${Package} ${Version}\n' > "$ISO/casper/minimal.standard.manifest"
sudo chroot "$FULL_MERGED" dpkg-query -W --showformat='${Package} ${Version}\n' > "$ISO/casper/minimal.standard.live.manifest"
sudo chroot "$FULL_MERGED" dpkg-query -W --showformat='${Package} ${Version}\n' > "$ISO/casper/filesystem.manifest"
printf '%s' "$(sudo du -sx --block-size=1 "$STD_MERGED" | cut -f1)" > "$ISO/casper/minimal.standard.size"
printf '%s' "$(sudo du -sx --block-size=1 "$FULL_MERGED" | cut -f1)" > "$ISO/casper/minimal.standard.live.size"
printf '%s' "$(sudo du -sx --block-size=1 "$FULL_MERGED" | cut -f1)" > "$ISO/casper/filesystem.size"

log "Putting Surface kernel into the live boot path"
sudo cp -f "$SURFACE_VMLINUZ" "$ISO/casper/vmlinuz"
sudo cp -f "$SURFACE_INITRD" "$ISO/casper/initrd"

# Unmount overlays before repacking their upper layers.
sudo umount -lf "$FULL_MERGED"
sudo umount -lf "$STD_MERGED"

log "Repacking modified standard and live layers"
rm -f "$ISO/casper/minimal.standard.squashfs" "$ISO/casper/minimal.standard.live.squashfs"
sudo mksquashfs "$STD" "$ISO/casper/minimal.standard.squashfs" -noappend -comp xz -b 1M >/dev/null
sudo mksquashfs "$LIVE" "$ISO/casper/minimal.standard.live.squashfs" -noappend -comp xz -b 1M >/dev/null
sudo chown "$(id -u):$(id -g)" "$ISO/casper/minimal.standard.squashfs" "$ISO/casper/minimal.standard.live.squashfs"

if [[ -f "$ISO/.disk/info" ]]; then
  printf 'Xubuntu %s LTS Surface Pro 4 custom amd64\n' "$BASE_VERSION" > "$ISO/.disk/info"
fi

# The original casper checksums are signed by Ubuntu and cannot remain valid
# after remastering. Recreate the checksum list and remove the stale signature.
log "Refreshing casper checksums"
(
  cd "$ISO/casper"
  find . -maxdepth 1 -type f ! -name 'SHA256SUMS' ! -name 'SHA256SUMS.gpg' -printf '%P\0' \
    | sort -z | xargs -0 sha256sum > SHA256SUMS
)
rm -f "$ISO/casper/SHA256SUMS.gpg"

log "Rebuilding bootable BIOS/UEFI ISO"
rm -f "$OUTDIR/$OUT_ISO"
xorriso -indev "$DL/$BASE_ISO" -outdev "$OUTDIR/$OUT_ISO" \
  -update_r "$ISO" / -boot_image any replay -volid "XUBUNTU_SURFACE" -commit >/dev/null

log "Validating finished ISO"
xorriso -indev "$OUTDIR/$OUT_ISO" -report_el_torito plain | tee "$OUTDIR/$OUT_ISO.boot-report.txt"
sha256sum "$OUTDIR/$OUT_ISO" | tee "$OUTDIR/$OUT_ISO.sha256"
ls -lh "$OUTDIR/$OUT_ISO"
echo "BUILD_COMPLETE=$OUTDIR/$OUT_ISO"
