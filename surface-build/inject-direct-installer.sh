#!/usr/bin/env bash
set -Eeuo pipefail

LIVE=${1:?live upper-layer path required}
ISO=${2:?ISO tree path required}
STD_MERGED=${3:?installable merged-root path required}
FULL_MERGED=${4:?live merged-root path required}

sudo install -d "$LIVE/usr/local/sbin" "$LIVE/etc/systemd/system/multi-user.target.wants"

# Refuse to build an installer that depends on tools which are not actually
# present in the live/installable image.
for cmd in parted mkfs.vfat mkfs.ext4 mount cp lsblk blkid findmnt chroot; do
  sudo chroot "$FULL_MERGED" /bin/sh -c "command -v '$cmd' >/dev/null" \
    || { echo "Missing live installer tool: $cmd" >&2; exit 1; }
done
for cmd in grub-install update-grub update-initramfs useradd chpasswd; do
  sudo chroot "$STD_MERGED" /bin/sh -c "command -v '$cmd' >/dev/null" \
    || { echo "Missing target-system tool: $cmd" >&2; exit 1; }
done

sudo tee "$LIVE/usr/local/sbin/surface-direct-install" >/dev/null <<'INSTALLER'
#!/usr/bin/env bash
set -Eeuo pipefail

LOG=/var/log/surface-direct-install.log
exec > >(tee -a "$LOG" /dev/tty1) 2>&1

fail() {
  local rc=${1:-1}
  echo
  echo '============================================================'
  echo 'SURFACE INSTALL FAILED'
  echo "Exit code: $rc"
  echo "Log: $LOG"
  echo 'The machine will remain on this screen. Photograph it before'
  echo 'holding Power to shut down.'
  echo '============================================================'
  sync
  while :; do sleep 3600; done
}
trap 'rc=$?; trap - ERR; fail "$rc"' ERR

if ! grep -qw surface_direct_install /proc/cmdline; then
  exit 0
fi

clear || true
echo '============================================================'
echo '   SURFACE PRO 4 - DIRECT XUBUNTU INSTALLER'
echo '============================================================'
echo
echo 'No graphical installer is being used.'
echo 'The internal Surface drive will be erased and Xubuntu copied'
echo 'directly from the validated Surface live image.'
echo

[[ -d /sys/firmware/efi ]] || { echo 'ERROR: System was not booted in UEFI mode.'; exit 20; }
[[ -f /cdrom/casper/minimal.squashfs ]] || { echo 'ERROR: Base Xubuntu filesystem is missing.'; exit 21; }
[[ -f /cdrom/casper/minimal.standard.squashfs ]] || { echo 'ERROR: Installable Xubuntu layer is missing.'; exit 22; }

USB_SOURCE=$(findmnt -nro SOURCE /cdrom 2>/dev/null || true)
USB_PARENT=''
if [[ "$USB_SOURCE" == /dev/* ]]; then
  if [[ "$(lsblk -dnro TYPE "$USB_SOURCE" 2>/dev/null || true)" == disk ]]; then
    USB_PARENT="$USB_SOURCE"
  else
    parent=$(lsblk -no PKNAME "$USB_SOURCE" 2>/dev/null | head -n1 || true)
    [[ -n "$parent" ]] && USB_PARENT="/dev/$parent"
  fi
fi

echo "Installation media: ${USB_PARENT:-$USB_SOURCE}"

declare -a candidates=()
while IFS= read -r disk; do
  [[ -b "$disk" ]] || continue
  type=$(lsblk -dnro TYPE "$disk" | xargs)
  rmflag=$(lsblk -dnro RM "$disk" | xargs)
  tran=$(lsblk -dnro TRAN "$disk" 2>/dev/null | xargs || true)
  size=$(lsblk -bdnro SIZE "$disk" | xargs)

  [[ "$type" == disk ]] || continue
  [[ "$rmflag" == 0 ]] || continue
  [[ "$tran" != usb ]] || continue
  [[ "$disk" != "$USB_PARENT" ]] || continue
  (( size >= 20 * 1024 * 1024 * 1024 )) || continue
  candidates+=("$disk")
done < <(lsblk -dnpo NAME)

if (( ${#candidates[@]} != 1 )); then
  echo "ERROR: Expected exactly one internal non-USB disk; found ${#candidates[@]}."
  printf 'Candidate: %s\n' "${candidates[@]:-none}"
  exit 23
fi

DISK=${candidates[0]}
echo
echo "TARGET DISK: $DISK"
lsblk -d -o NAME,MODEL,SIZE,TRAN "$DISK" || true
echo
echo 'Erasing and partitioning internal disk...'

# Unmount anything already mounted from the target disk.
while read -r part mnt; do
  [[ -n "${mnt:-}" ]] && umount -lf "$part" || true
done < <(lsblk -lnpo NAME,MOUNTPOINT "$DISK" | tail -n +2)

wipefs -af "$DISK"
parted -s "$DISK" mklabel gpt
parted -s "$DISK" mkpart ESP fat32 1MiB 513MiB
parted -s "$DISK" set 1 esp on
parted -s "$DISK" mkpart primary ext4 513MiB 100%
partprobe "$DISK" || true
udevadm settle
sleep 2

EFI_PART=$(lsblk -lnpo NAME,PARTN "$DISK" | awk '$2==1 {print $1; exit}')
ROOT_PART=$(lsblk -lnpo NAME,PARTN "$DISK" | awk '$2==2 {print $1; exit}')
[[ -b "$EFI_PART" ]] || { echo 'ERROR: EFI partition was not created.'; exit 24; }
[[ -b "$ROOT_PART" ]] || { echo 'ERROR: Root partition was not created.'; exit 25; }

mkfs.vfat -F 32 -n EFI "$EFI_PART"
mkfs.ext4 -F -L XUBUNTU "$ROOT_PART"

WORK=/run/surface-direct-install
BASE="$WORK/base"
STD="$WORK/std"
MERGED="$WORK/merged"
TARGET=/target
mkdir -p "$BASE" "$STD" "$MERGED" "$TARGET"

mount -t squashfs -o loop,ro /cdrom/casper/minimal.squashfs "$BASE"
mount -t squashfs -o loop,ro /cdrom/casper/minimal.standard.squashfs "$STD"
mount -t overlay overlay -o "lowerdir=$STD:$BASE" "$MERGED"
mount "$ROOT_PART" "$TARGET"
mkdir -p "$TARGET/boot/efi"
mount "$EFI_PART" "$TARGET/boot/efi"

echo
echo 'Copying Xubuntu + Linux-Surface system to internal SSD...'
cp -a "$MERGED/." "$TARGET/"
sync

ROOT_UUID=$(blkid -s UUID -o value "$ROOT_PART")
EFI_UUID=$(blkid -s UUID -o value "$EFI_PART")
cat > "$TARGET/etc/fstab" <<EOF
UUID=$ROOT_UUID / ext4 defaults,noatime 0 1
UUID=$EFI_UUID /boot/efi vfat umask=0077 0 1
EOF

echo surface-pro4 > "$TARGET/etc/hostname"
cat > "$TARGET/etc/hosts" <<'EOF'
127.0.0.1 localhost
127.0.1.1 surface-pro4
::1 localhost ip6-localhost ip6-loopback
ff02::1 ip6-allnodes
ff02::2 ip6-allrouters
EOF
ln -sf /usr/share/zoneinfo/Australia/Hobart "$TARGET/etc/localtime"

# Provide the target chroot with the real hardware/system interfaces needed
# by grub-install, initramfs generation and user setup.
for p in dev dev/pts proc sys run; do
  mount --rbind "/$p" "$TARGET/$p"
  mount --make-rslave "$TARGET/$p"
done

chroot "$TARGET" /bin/bash -eux <<'CHROOT'
if ! id surface >/dev/null 2>&1; then
  useradd -m -s /bin/bash surface
fi
echo 'surface:surface' | chpasswd
for g in sudo adm audio video plugdev netdev lpadmin scanner bluetooth; do
  if getent group "$g" >/dev/null 2>&1; then
    usermod -aG "$g" surface
  fi
done
passwd -l root || true
systemctl set-default graphical.target
systemctl enable lightdm NetworkManager bluetooth || true
update-initramfs -u -k all
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=xubuntu --recheck
grub-install --target=x86_64-efi --efi-directory=/boot/efi --removable --recheck
update-grub
CHROOT

mkdir -p "$TARGET/etc/lightdm/lightdm.conf.d" \
         "$TARGET/usr/local/bin" \
         "$TARGET/home/surface/.config/autostart" \
         "$TARGET/home/surface/Desktop"

cat > "$TARGET/etc/lightdm/lightdm.conf.d/50-surface-autologin.conf" <<'EOF'
[Seat:*]
autologin-user=surface
autologin-user-timeout=0
user-session=xubuntu
EOF

cat > "$TARGET/usr/local/bin/surface-onboard" <<'EOF'
#!/bin/sh
sleep 2
gsettings set org.onboard.keyboard input-event-source 'GTK' || true
gsettings set org.onboard.keyboard touch-input 'none' || true
exec onboard
EOF
chmod +x "$TARGET/usr/local/bin/surface-onboard"

cat > "$TARGET/home/surface/.config/autostart/onboard.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Onboard On-Screen Keyboard
Exec=/usr/local/bin/surface-onboard
Terminal=false
X-GNOME-Autostart-enabled=true
X-XFCE-Autostart-Override=true
EOF

cat > "$TARGET/home/surface/Desktop/Restart-Onboard.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Restart On-Screen Keyboard
Exec=sh -c 'pkill onboard || true; sleep 1; /usr/local/bin/surface-onboard'
Icon=input-keyboard
Terminal=false
EOF
chmod +x "$TARGET/home/surface/Desktop/Restart-Onboard.desktop"
chown -R surface:surface "$TARGET/home/surface/.config" "$TARGET/home/surface/Desktop"

cat > "$TARGET/etc/surface-direct-installed" <<'EOF'
Surface Pro 4 direct Xubuntu installation
Linux-Surface + IPTS image
EOF

sync

echo
echo 'Verifying installed Surface kernel and EFI boot files...'
ls "$TARGET"/boot/vmlinuz-*-surface* >/dev/null
ls "$TARGET"/boot/initrd.img-*-surface* >/dev/null
[[ -f "$TARGET/boot/efi/EFI/BOOT/BOOTX64.EFI" ]]

echo
echo '============================================================'
echo 'INSTALLATION COMPLETE'
echo 'The Surface will power off now.'
echo 'REMOVE THE USB, then press Power to boot the installed system.'
echo 'Desktop login is automatic. sudo password: surface'
echo '============================================================'
sync
sleep 5
systemctl poweroff --force --force
INSTALLER
sudo chmod +x "$LIVE/usr/local/sbin/surface-direct-install"

sudo tee "$LIVE/etc/systemd/system/surface-direct-install.service" >/dev/null <<'UNIT'
[Unit]
Description=Surface Pro 4 zero-input direct installer
ConditionKernelCommandLine=surface_direct_install
After=local-fs.target systemd-udev-settle.service
Before=getty.target

[Service]
Type=oneshot
ExecStart=/usr/local/sbin/surface-direct-install
StandardInput=null
StandardOutput=tty
StandardError=tty
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
UNIT
sudo ln -sfn ../surface-direct-install.service \
  "$LIVE/etc/systemd/system/multi-user.target.wants/surface-direct-install.service"

sudo bash -n "$LIVE/usr/local/sbin/surface-direct-install"

GRUB="$ISO/boot/grub/grub.cfg"
[[ -f "$GRUB" ]] || { echo "Missing GRUB config: $GRUB" >&2; exit 1; }
python3 - "$GRUB" <<'PY'
from pathlib import Path
import re, sys
p = Path(sys.argv[1])
s = p.read_text()
s = re.sub(r'(?m)^\s*set\s+(?:default|timeout|timeout_style)=.*\n?', '', s)
entry = r'''set default=0
set timeout_style=menu
set timeout=5

menuentry 'INSTALL SURFACE PRO 4 - ERASE INTERNAL DRIVE (AUTO IN 5 SECONDS)' {
    set gfxpayload=keep
    linux /casper/vmlinuz boot=casper surface_direct_install=1 systemd.unit=multi-user.target systemd.show_status=1 plymouth.enable=0 console=tty1 ---
    initrd /casper/initrd
}

'''
p.write_text(entry + s)
PY

python3 - "$GRUB" <<'PY'
from pathlib import Path
import sys
s = Path(sys.argv[1]).read_text()
first = s.find('menuentry ')
assert first >= 0
assert s[first:].startswith("menuentry 'INSTALL SURFACE PRO 4")
assert 'set default=0' in s[:first]
assert 'set timeout=5' in s[:first]
assert 'surface_direct_install=1' in s[first:first+600]
assert 'systemd.unit=multi-user.target' in s[first:first+600]
PY

echo 'Direct installer injected into live layer and GRUB.'
