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
INITRD_REPORT="$OUTDIR/$OUT_ISO.initrd-report.txt"

log(){ printf '\n\033[1;35m==> %s\033[0m\n' "$*"; }
die(){ echo "ERROR: $*" >&2; exit 1; }

umount_chroot(){
  local root="$1"
  set +e
  for m in run sys proc dev/pts dev etc/resolv.conf; do
    mountpoint -q "$root/$m" && sudo umount -lf "$root/$m"
  done
  set -e
}

mount_chroot(){
  local root="$1"
  sudo mount --bind /dev "$root/dev"
  sudo mount --bind /dev/pts "$root/dev/pts"
  sudo mount -t proc proc "$root/proc"
  sudo mount -t sysfs sys "$root/sys"
  sudo mount --bind /run "$root/run"
  sudo mount --bind /etc/resolv.conf "$root/etc/resolv.conf"
}

cleanup(){
  set +e
  umount_chroot "$FULL_MERGED"
  umount_chroot "$STD_MERGED"
  mountpoint -q "$FULL_MERGED" && sudo umount -lf "$FULL_MERGED"
  mountpoint -q "$STD_MERGED" && sudo umount -lf "$STD_MERGED"
}
trap cleanup EXIT INT TERM

log "Installing remaster tools"
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  xorriso squashfs-tools curl wget gnupg ca-certificates initramfs-tools-core

mkdir -p "$DL" "$WORKDIR"

log "Downloading and verifying official Xubuntu ${BASE_VERSION}"
curl -fL --retry 5 --retry-delay 5 "$BASE_URL/$BASE_ISO" -o "$DL/$BASE_ISO"
curl -fsSL "$BASE_URL/SHA256SUMS" -o "$DL/SHA256SUMS"
(
  cd "$DL"
  grep "${BASE_ISO}$" SHA256SUMS | sed 's/ \*/  /' | sha256sum -c -
) || die "Official Xubuntu checksum failed"

log "Extracting ISO and its three Noble squashfs layers"
rm -rf "$ISO" "$BASE" "$STD" "$LIVE" "$STD_MERGED" "$FULL_MERGED" "$STD_WORK" "$LIVE_WORK"
mkdir -p "$ISO" "$BASE" "$STD" "$LIVE" "$STD_MERGED" "$FULL_MERGED" "$STD_WORK" "$LIVE_WORK"
xorriso -osirrox on -indev "$DL/$BASE_ISO" -extract / "$ISO" >/dev/null 2>&1
chmod -R u+w "$ISO"

for f in minimal.squashfs minimal.standard.squashfs minimal.standard.live.squashfs; do
  [[ -f "$ISO/casper/$f" ]] || die "Expected Xubuntu layer missing: casper/$f"
done

sudo unsquashfs -d "$BASE" "$ISO/casper/minimal.squashfs" >/dev/null
sudo unsquashfs -d "$STD" "$ISO/casper/minimal.standard.squashfs" >/dev/null
sudo unsquashfs -d "$LIVE" "$ISO/casper/minimal.standard.live.squashfs" >/dev/null

log "Installing Surface packages into the installable Xubuntu layer"
sudo mount -t overlay overlay -o "lowerdir=$BASE,upperdir=$STD,workdir=$STD_WORK" "$STD_MERGED"
mount_chroot "$STD_MERGED"

sudo tee "$STD_MERGED/usr/sbin/policy-rc.d" >/dev/null <<'POLICY'
#!/bin/sh
exit 101
POLICY
sudo chmod +x "$STD_MERGED/usr/sbin/policy-rc.d"

sudo chroot "$STD_MERGED" /bin/bash -eux <<'CHROOT'
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y wget gnupg ca-certificates
wget -qO- https://raw.githubusercontent.com/linux-surface/linux-surface/master/pkg/keys/surface.asc \
  | gpg --dearmor > /etc/apt/trusted.gpg.d/linux-surface.gpg
echo 'deb [arch=amd64] https://pkg.surfacelinux.com/debian release main' \
  > /etc/apt/sources.list.d/linux-surface.list
apt-get update
apt-get install -y \
  linux-image-surface linux-headers-surface iptsd libwacom-surface \
  bluez libinput-tools xinput blueman onboard

# The direct installer copies this installable layer to the SSD.  The stock
# Xubuntu live environment has firmware in its live-only layer, so without
# explicitly installing linux-firmware here the installed Surface loses its
# Marvell 8897 Wi-Fi/Bluetooth firmware even though networking works on the USB.
apt-get install -y linux-firmware

dpkg-query -W linux-firmware >/dev/null
# Noble stores firmware compressed with zstd; the kernel firmware loader reads
# the .zst transparently.  Validate the actual packaged Marvell 8897 payload.
test -f /lib/firmware/mrvl/pcie8897_uapsta.bin.zst

update-initramfs -u -k all
apt-get clean
rm -rf /var/lib/apt/lists/*
CHROOT

sudo rm -f "$STD_MERGED/usr/sbin/policy-rc.d"

SURFACE_VMLINUZ="$(find "$STD_MERGED/boot" -maxdepth 1 -type f -name 'vmlinuz-*-surface*' | sort -V | tail -n1)"
[[ -n "$SURFACE_VMLINUZ" ]] || die "Surface kernel was not installed"
SURFACE_VER="${SURFACE_VMLINUZ##*/vmlinuz-}"
[[ -d "$STD_MERGED/lib/modules/$SURFACE_VER" ]] || die "Surface module tree missing: $SURFACE_VER"
echo "Surface kernel selected: $SURFACE_VER"

umount_chroot "$STD_MERGED"

log "Creating the full live Xubuntu layer"
sudo mount -t overlay overlay -o "lowerdir=$STD_MERGED,upperdir=$LIVE,workdir=$LIVE_WORK" "$FULL_MERGED"
mount_chroot "$FULL_MERGED"

sudo tee "$FULL_MERGED/usr/sbin/policy-rc.d" >/dev/null <<'POLICY'
#!/bin/sh
exit 101
POLICY
sudo chmod +x "$FULL_MERGED/usr/sbin/policy-rc.d"

log "Installing Ubuntu casper live-boot support into the live-only layer"
sudo chroot "$FULL_MERGED" /bin/bash -eux <<'CHROOT'
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y casper initramfs-tools
apt-get clean
CHROOT
sudo rm -f "$FULL_MERGED/usr/sbin/policy-rc.d"

[[ -f "$FULL_MERGED/usr/share/initramfs-tools/scripts/casper" ]] \
  || die "casper package installed but its initramfs script is missing"
[[ -f "$FULL_MERGED/usr/share/initramfs-tools/hooks/casper" ]] \
  || die "casper initramfs hook is missing"

log "Building a Surface-kernel initrd WITH Ubuntu casper"
sudo chroot "$FULL_MERGED" /bin/bash -eux -c "
  rm -f /tmp/surface-live-initrd
  mkinitramfs -o /tmp/surface-live-initrd '$SURFACE_VER'
"
LIVE_INITRD="$FULL_MERGED/tmp/surface-live-initrd"
[[ -s "$LIVE_INITRD" ]] || die "Surface live initrd was not created"

log "Validating casper and Surface modules inside the new initrd"
{
  echo "Surface kernel: $SURFACE_VER"
  echo
  echo "Casper entries:"
  lsinitramfs "$LIVE_INITRD" | grep -E '^scripts/casper($|-|/)' || true
  echo
  echo "Surface module-tree entries:"
  lsinitramfs "$LIVE_INITRD" | grep -F "lib/modules/$SURFACE_VER/" | head -n 60 || true
} | tee "$INITRD_REPORT"

lsinitramfs "$LIVE_INITRD" | grep -qE '^scripts/casper($|-|/)' \
  || die "Generated initrd does not contain casper live-boot scripts"
lsinitramfs "$LIVE_INITRD" | grep -qF "lib/modules/$SURFACE_VER/" \
  || die "Generated initrd does not contain Surface kernel modules"

log "Adding Surface diagnostics to the live desktop"
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
echo "IPTS processes/services:"; pgrep -a iptsd || true; systemctl --no-pager --full status 'iptsd*' 2>/dev/null || true
echo
echo "Input devices:"; libinput list-devices 2>/dev/null | grep -E 'Device:|Kernel:|Capabilities:' || true
echo
echo "Bluetooth devices:"; bluetoothctl devices 2>/dev/null || true
echo
echo "The kernel line above should contain -surface."
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
Surface input: iptsd + libwacom-surface
Live boot: Ubuntu casper
EOF

log "Updating live manifests"
sudo chroot "$STD_MERGED" dpkg-query -W --showformat='${Package} ${Version}\n' > "$ISO/casper/minimal.standard.manifest"
sudo chroot "$FULL_MERGED" dpkg-query -W --showformat='${Package} ${Version}\n' > "$ISO/casper/minimal.standard.live.manifest"
sudo chroot "$FULL_MERGED" dpkg-query -W --showformat='${Package} ${Version}\n' > "$ISO/casper/filesystem.manifest"
printf '%s' "$(sudo du -sx --block-size=1 "$STD_MERGED" | cut -f1)" > "$ISO/casper/minimal.standard.size"
printf '%s' "$(sudo du -sx --block-size=1 "$FULL_MERGED" | cut -f1)" > "$ISO/casper/minimal.standard.live.size"
printf '%s' "$(sudo du -sx --block-size=1 "$FULL_MERGED" | cut -f1)" > "$ISO/casper/filesystem.size"

log "Installing Surface kernel + casper initrd into ISO boot path"
sudo cp -f "$SURFACE_VMLINUZ" "$ISO/casper/vmlinuz"
sudo cp -f "$LIVE_INITRD" "$ISO/casper/initrd"
sudo rm -f "$LIVE_INITRD"

umount_chroot "$FULL_MERGED"
sudo umount -lf "$FULL_MERGED"
sudo umount -lf "$STD_MERGED"

log "Repacking modified Xubuntu layers"
rm -f "$ISO/casper/minimal.standard.squashfs" "$ISO/casper/minimal.standard.live.squashfs"
sudo mksquashfs "$STD" "$ISO/casper/minimal.standard.squashfs" -noappend -comp xz -b 1M >/dev/null
sudo mksquashfs "$LIVE" "$ISO/casper/minimal.standard.live.squashfs" -noappend -comp xz -b 1M >/dev/null
sudo chown "$(id -u):$(id -g)" "$ISO/casper/minimal.standard.squashfs" "$ISO/casper/minimal.standard.live.squashfs"

printf 'Xubuntu %s LTS Surface Pro 4 custom amd64\n' "$BASE_VERSION" > "$ISO/.disk/info"

log "Refreshing casper checksums"
(
  cd "$ISO/casper"
  find . -maxdepth 1 -type f ! -name SHA256SUMS ! -name SHA256SUMS.gpg -printf '%P\0' \
    | sort -z | xargs -0 sha256sum > SHA256SUMS
)
rm -f "$ISO/casper/SHA256SUMS.gpg"

log "Rebuilding bootable BIOS/UEFI ISO"
rm -f "$OUTDIR/$OUT_ISO"
xorriso -indev "$DL/$BASE_ISO" -outdev "$OUTDIR/$OUT_ISO" \
  -update_r "$ISO" / -boot_image any replay -volid XUBUNTU_SURFACE -commit >/dev/null

log "Validating finished ISO"
xorriso -indev "$OUTDIR/$OUT_ISO" -report_el_torito plain | tee "$OUTDIR/$OUT_ISO.boot-report.txt"
sha256sum "$OUTDIR/$OUT_ISO" | tee "$OUTDIR/$OUT_ISO.sha256"
ls -lh "$OUTDIR/$OUT_ISO"
echo "BUILD_COMPLETE=$OUTDIR/$OUT_ISO"
