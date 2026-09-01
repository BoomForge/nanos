#!/usr/bin/env bash
set -Eeuo pipefail

FULL_MERGED=${1:?full live merged-root path required}

CASPER="$FULL_MERGED/usr/share/initramfs-tools/scripts/casper"
PREMOUNT_DIR="$FULL_MERGED/usr/share/initramfs-tools/scripts/casper-premount"
MODULES_FILE="$FULL_MERGED/etc/initramfs-tools/modules"

[[ -f "$CASPER" ]] || { echo "Missing casper initramfs script: $CASPER" >&2; exit 1; }
sudo install -d "$PREMOUNT_DIR" "$(dirname "$MODULES_FILE")"

# Force the USB/storage/filesystem modules the Surface needs to see a DD-written
# hybrid ISO before casper starts looking for its /casper directory.
for mod in xhci_hcd xhci_pci usb_storage uas sd_mod isofs squashfs overlay; do
  if ! sudo grep -qxF "$mod" "$MODULES_FILE" 2>/dev/null; then
    echo "$mod" | sudo tee -a "$MODULES_FILE" >/dev/null
  fi
done

# Runs inside the initramfs BEFORE casper performs media discovery. It never
# reads stdin. For the dedicated Surface direct-install boot it waits for the
# USB, verifies that the media really contains the expected Xubuntu squashfs
# payload, and creates the exact by-label path passed on the kernel command line.
sudo tee "$PREMOUNT_DIR/00surface-direct-media" >/dev/null <<'HOOK'
#!/bin/sh
PREREQ=""
prereqs() { echo "$PREREQ"; }
case "${1:-}" in
  prereqs) prereqs; exit 0 ;;
esac

case " $(cat /proc/cmdline 2>/dev/null) " in
  *" surface_direct_install=1 "*) ;;
  *) exit 0 ;;
esac

exec </dev/null
PATH=/usr/bin:/usr/sbin:/bin:/sbin
export PATH

say() { echo "SURFACE MEDIA: $*" >/dev/tty1 2>/dev/null || echo "SURFACE MEDIA: $*"; }

poweroff_no_input() {
  say "Installer USB could not be found. Powering off automatically."
  sleep 10
  poweroff -f 2>/dev/null || reboot -f 2>/dev/null || true
  while :; do sleep 3600; done
}

say "Waiting for the installer USB. No input is required."

# Make sure the Surface USB stack is loaded before scanning.
for mod in xhci_hcd xhci_pci usb_storage uas sd_mod isofs squashfs overlay; do
  modprobe "$mod" 2>/dev/null || true
done
udevadm trigger --action=add 2>/dev/null || true
udevadm settle --timeout=20 2>/dev/null || true

LABEL_PATH=/dev/disk/by-label/XUBUNTU_SURFACE
CHECK=/run/surface-media-check
mkdir -p /dev/disk/by-label "$CHECK"

find_surface_media() {
  # Prefer the normal udev label symlink if it already exists.
  if [ -b "$LABEL_PATH" ]; then
    echo "$LABEL_PATH"
    return 0
  fi

  # Rufus DD mode can expose the ISO9660 filesystem on the whole USB disk or
  # on a partition depending on firmware/kernel timing, so test both forms.
  for dev in /dev/sd? /dev/sd?? /dev/sr? /dev/nvme*n?p* /dev/mmcblk*p*; do
    [ -b "$dev" ] || continue
    umount "$CHECK" 2>/dev/null || true
    if mount -r -t iso9660 "$dev" "$CHECK" 2>/dev/null; then
      if [ -f "$CHECK/casper/minimal.squashfs" ] && \
         [ -f "$CHECK/casper/minimal.standard.squashfs" ] && \
         [ -f "$CHECK/casper/vmlinuz" ] && \
         [ -f "$CHECK/casper/initrd" ]; then
        umount "$CHECK" 2>/dev/null || true
        ln -sf "$dev" "$LABEL_PATH"
        echo "$LABEL_PATH"
        return 0
      fi
      umount "$CHECK" 2>/dev/null || true
    fi
  done
  return 1
}

MEDIA=""
i=0
while [ "$i" -lt 120 ]; do
  MEDIA="$(find_surface_media 2>/dev/null || true)"
  if [ -n "$MEDIA" ] && [ -b "$MEDIA" ]; then
    break
  fi
  if [ $((i % 10)) -eq 0 ]; then
    udevadm trigger --action=add 2>/dev/null || true
    udevadm settle --timeout=5 2>/dev/null || true
  fi
  sleep 1
  i=$((i + 1))
done

[ -n "$MEDIA" ] && [ -b "$MEDIA" ] || poweroff_no_input

# One final content check before casper gets control. If this succeeds, casper's
# explicit live-media path is guaranteed to point at the correct USB.
umount "$CHECK" 2>/dev/null || true
if ! mount -r -t iso9660 "$MEDIA" "$CHECK" 2>/dev/null; then
  poweroff_no_input
fi
if [ ! -f "$CHECK/casper/minimal.squashfs" ] || \
   [ ! -f "$CHECK/casper/minimal.standard.squashfs" ]; then
  umount "$CHECK" 2>/dev/null || true
  poweroff_no_input
fi
umount "$CHECK" 2>/dev/null || true

say "Installer USB located. Continuing automatically."
exit 0
HOOK
sudo chmod +x "$PREMOUNT_DIR/00surface-direct-media"

# Defense in depth: casper's normal failure path offers an interactive netboot
# prompt. For the Surface direct-install kernel flag that is forbidden. Replace
# only that failure site so a media-discovery failure powers off instead.
sudo python3 - "$CASPER" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
s = p.read_text()
needle = 'panic "Unable to find a medium containing a live file system"'
if s.count(needle) != 1:
    raise SystemExit(f'Expected exactly one casper media panic, found {s.count(needle)}')
replacement = r'''if grep -Eq '(^| )surface_direct_install=1( |$)' /proc/cmdline 2>/dev/null; then
            echo 'SURFACE MEDIA: live filesystem not found; interactive netboot is disabled.' >/dev/tty1 2>/dev/null || true
            echo 'SURFACE MEDIA: powering off automatically in 10 seconds.' >/dev/tty1 2>/dev/null || true
            sleep 10
            poweroff -f 2>/dev/null || reboot -f 2>/dev/null || true
            while :; do sleep 3600; done
        fi
        panic "Unable to find a medium containing a live file system"'''
s = s.replace(needle, replacement, 1)
p.write_text(s)
PY

# Fail the build now if any of the no-input guarantees were not installed.
sudo grep -q 'surface_direct_install=1' "$PREMOUNT_DIR/00surface-direct-media"
sudo grep -q 'XUBUNTU_SURFACE' "$PREMOUNT_DIR/00surface-direct-media"
sudo grep -q 'exec </dev/null' "$PREMOUNT_DIR/00surface-direct-media"
sudo grep -q 'interactive netboot is disabled' "$CASPER"

echo 'Surface initramfs media pinning and no-netboot guard installed.'
