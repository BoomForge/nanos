#!/usr/bin/env bash
set -Eeuo pipefail

LIVE=${1:?live upper-layer path required}
ISO=${2:?ISO tree path required}
STD_MERGED=${3:?installable merged-root path required}
FULL_MERGED=${4:?live merged-root path required}

sudo install -d "$LIVE/usr/local/sbin" "$LIVE/etc/systemd/system"

# Refuse to build an installer that is missing anything it will need at run time.
# In particular, every destructive/storage/boot command must already exist in the
# live image or copied target. The physical Surface has NO usable input at this
# stage, so there is deliberately no fallback shell or interactive recovery path.
for cmd in parted mkfs.vfat mkfs.ext4 mount cp lsblk blkid findmnt chroot \
           wipefs partprobe udevadm awk xargs timeout sync systemctl tee; do
  sudo chroot "$FULL_MERGED" /bin/sh -c "command -v '$cmd' >/dev/null" \
    || { echo "Missing live installer tool: $cmd" >&2; exit 1; }
done
for cmd in grub-install update-grub update-initramfs useradd chpasswd timeout; do
  sudo chroot "$STD_MERGED" /bin/sh -c "command -v '$cmd' >/dev/null" \
    || { echo "Missing target-system tool: $cmd" >&2; exit 1; }
done

sudo tee "$LIVE/usr/local/sbin/surface-direct-install" >/dev/null <<'INSTALLER'
#!/usr/bin/env bash
set -Eeuo pipefail

# HARD REQUIREMENT: this installer must never wait for a keyboard, touchscreen,
# password, confirmation, shell response, or any other human input.
exec </dev/null
export DEBIAN_FRONTEND=noninteractive
export NEEDRESTART_MODE=a
export UCF_FORCE_CONFFOLD=1
export LANG=C.UTF-8
export LC_ALL=C.UTF-8

LOG=/var/log/surface-direct-install.log
exec > >(tee -a "$LOG" /dev/tty1) 2>&1

power_off_now() {
  sync || true
  systemctl poweroff --force --force || poweroff -f || true
  while :; do sleep 3600; done
}

fail() {
  local rc=${1:-1}
  trap - ERR
  echo
  echo '============================================================'
  echo 'SURFACE INSTALL FAILED'
  echo "Exit code: $rc"
  echo "Log: $LOG"
  echo 'NO INPUT IS REQUIRED OR ACCEPTED.'
  echo 'The Surface will power off automatically in 20 seconds.'
  echo 'Photograph this screen if you want me to diagnose the failure.'
  echo '============================================================'
  sync || true
  sleep 20
  power_off_now
}
trap 'rc=$?; fail "$rc"' ERR

if ! grep -Eq '(^| )surface_direct_install=1( |$)' /proc/cmdline; then
  echo 'ERROR: Direct-install kernel flag is missing.'
  fail 90
fi

clear || true
echo '============================================================'
echo '   SURFACE PRO 4 - HANDS-OFF XUBUNTU INSTALL'
echo '============================================================'
echo
echo 'NO KEYBOARD OR TOUCH INPUT IS REQUIRED.'
echo 'The internal Surface drive will be erased automatically.'
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

while read -r part mnt; do
  [[ -n "${mnt:-}" ]] && umount -lf "$part" || true
done < <(lsblk -lnpo NAME,MOUNTPOINT "$DISK" | tail -n +2)

timeout 60 wipefs -af "$DISK"
timeout 60 parted -s "$DISK" mklabel gpt
timeout 60 parted -s "$DISK" mkpart ESP fat32 1MiB 513MiB
timeout 60 parted -s "$DISK" set 1 esp on
timeout 60 parted -s "$DISK" mkpart primary ext4 513MiB 100%
partprobe "$DISK" || true
udevadm settle || true
sleep 2

EFI_PART=$(lsblk -lnpo NAME,PARTN "$DISK" | awk '$2==1 {print $1; exit}')
ROOT_PART=$(lsblk -lnpo NAME,PARTN "$DISK" | awk '$2==2 {print $1; exit}')
[[ -b "$EFI_PART" ]] || { echo 'ERROR: EFI partition was not created.'; exit 24; }
[[ -b "$ROOT_PART" ]] || { echo 'ERROR: Root partition was not created.'; exit 25; }

timeout 120 mkfs.vfat -F 32 -n EFI "$EFI_PART"
timeout 300 mkfs.ext4 -F -L XUBUNTU "$ROOT_PART"

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
timeout 1800 cp -a "$MERGED/." "$TARGET/"
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

# Do not let update-grub probe anything other than the freshly installed system.
mkdir -p "$TARGET/etc/default/grub.d"
cat > "$TARGET/etc/default/grub.d/99-surface-direct-install.cfg" <<'EOF'
GRUB_DISABLE_OS_PROBER=true
GRUB_TIMEOUT=0
GRUB_TIMEOUT_STYLE=hidden
EOF

for p in dev dev/pts proc sys run; do
  mount --rbind "/$p" "$TARGET/$p"
  mount --make-rslave "$TARGET/$p"
done

# The chroot itself receives its program from this here-document. Any program
# inside it that attempts to read interactively sees EOF immediately; none is
# permitted to block waiting for input.
chroot "$TARGET" /usr/bin/env \
  DEBIAN_FRONTEND=noninteractive NEEDRESTART_MODE=a UCF_FORCE_CONFFOLD=1 \
  /bin/bash -eux <<'CHROOT'
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

timeout 600 update-initramfs -u -k all

# Never touch EFI NVRAM here. The removable fallback path EFI/BOOT/BOOTX64.EFI
# is sufficient for the Surface firmware and avoids firmware-variable prompts/
# failures entirely.
timeout 180 grub-install --target=x86_64-efi --efi-directory=/boot/efi \
  --bootloader-id=xubuntu --no-nvram --no-floppy --recheck
timeout 180 grub-install --target=x86_64-efi --efi-directory=/boot/efi \
  --removable --no-nvram --no-floppy --recheck
timeout 300 update-grub
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
exec onboard </dev/null
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
Zero-input installer v2
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
echo 'NO INPUT IS REQUIRED.'
echo 'The Surface will power off automatically in 5 seconds.'
echo 'After it is off: remove USB, then press Power.'
echo 'Desktop login is automatic. sudo password: surface'
echo '============================================================'
sync
sleep 5
power_off_now
INSTALLER
sudo chmod +x "$LIVE/usr/local/sbin/surface-direct-install"

# A dedicated boot target is used instead of multi-user.target. That means the
# direct-install boot does NOT pull in getty/login services and therefore has no
# path that can dump the user at a text login prompt.
sudo tee "$LIVE/etc/systemd/system/surface-direct-install.target" >/dev/null <<'TARGET'
[Unit]
Description=Surface Pro 4 hands-off installation target
Wants=basic.target local-fs.target systemd-udev-settle.service
Requires=surface-direct-install.service
After=basic.target local-fs.target
AllowIsolate=yes
TARGET

sudo tee "$LIVE/etc/systemd/system/surface-direct-install.service" >/dev/null <<'UNIT'
[Unit]
Description=Surface Pro 4 zero-input direct installer
ConditionKernelCommandLine=surface_direct_install=1
After=local-fs.target systemd-udev-settle.service
Wants=systemd-udev-settle.service
FailureAction=poweroff-force

[Service]
Type=oneshot
ExecStart=/usr/local/sbin/surface-direct-install
StandardInput=null
StandardOutput=tty
StandardError=tty
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes
TimeoutStartSec=60min
RemainAfterExit=yes
UNIT

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
set timeout_style=hidden
set timeout=1

menuentry 'INSTALL SURFACE PRO 4 - ZERO INPUT - ERASE INTERNAL DRIVE' {
    set gfxpayload=keep
    linux /casper/vmlinuz boot=casper surface_direct_install=1 systemd.unit=surface-direct-install.target systemd.getty_auto=no systemd.show_status=1 plymouth.enable=0 console=tty1 ---
    initrd /casper/initrd
}

'''
p.write_text(entry + s)
PY

python3 - "$GRUB" "$LIVE/etc/systemd/system/surface-direct-install.service" "$LIVE/etc/systemd/system/surface-direct-install.target" <<'PY'
from pathlib import Path
import sys
s = Path(sys.argv[1]).read_text()
service = Path(sys.argv[2]).read_text()
target = Path(sys.argv[3]).read_text()
first = s.find('menuentry ')
assert first >= 0
assert s[first:].startswith("menuentry 'INSTALL SURFACE PRO 4")
assert 'set default=0' in s[:first]
assert 'set timeout_style=hidden' in s[:first]
assert 'set timeout=1' in s[:first]
boot = s[first:first+800]
assert 'surface_direct_install=1' in boot
assert 'systemd.unit=surface-direct-install.target' in boot
assert 'systemd.getty_auto=no' in boot
assert 'systemd.unit=multi-user.target' not in boot
assert 'ConditionKernelCommandLine=surface_direct_install=1' in service
assert 'StandardInput=null' in service
assert 'FailureAction=poweroff-force' in service
assert 'Requires=surface-direct-install.service' in target
assert 'getty' not in target.lower()
PY

echo 'Strict zero-input direct installer injected into live layer and GRUB.'
