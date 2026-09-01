# M.I.L.O

M.I.L.O is an extremely small offline graphical x86 operating system with its
own BIOS boot sector, two-stage loader, protected-mode assembly kernel, FAT12
storage, stacking desktop, text tools, and deterministic M.I.L.O interaction.
It does not use Linux, GRUB, an LLM, or an external desktop runtime.

The `M.I.L.O` branch is the primary project. The inherited modular C89 NanOS
work remains in the repository for upstream development and general reusable
improvements; it is not the M.I.L.O runtime.

## Current release

V0.28.2 provides:

- silent normal loading followed by a native ASCII M.I.L.O splash held for at
  least three seconds;
- a Fluxbox-inspired right-click root menu and four movable, resizable
  application windows;
- a minimized-only taskbar with click-to-restore buttons;
- CMOS-backed local date/time and right-aligned version status;
- a native Conky-inspired overlay with a real 0--120+ events/second graph,
  application states, RAM, FAT12 capacity, file/event counts, video, network,
  and honest unsupported-sensor reporting;
- native FileHound-inspired FAT12 browsing and real open/edit/copy/rename/delete;
- a fifth native Writer window with resize-aware text, mouse caret placement,
  a visible 8.3 name field, Save/Save As, guarded close, and compact persistent
  Bold, Italic, and Underline formatting;
- a corrected clipped-glyph path that keeps application text readable through
  window resizing, focused Writer keystroke repainting, and full-frame buffered
  desktop composition to prevent lower layers flashing through, plus a
  30-entry command history; and
- Nyx-derived deterministic pattern and trait learning without phrase storage.

The complete scope, architectural boundaries, roadmap, and commit rationale
are maintained in [`direction.md`](direction.md).

## Requirements

- `gcc` with 32-bit assembly support
- GNU `objcopy` and `nm`
- Python 3
- QEMU i386 for runtime testing

## Build and verify M.I.L.O

```sh
make milo-floppy
make verify-milo-boot
```

The build produces:

```text
build/M.I.L.O-floppy-V0.28.2.img
```

## Run on Windows

```powershell
$img = "$env:USERPROFILE\Downloads\M.I.L.O-floppy-V0.28.2.img"
& "C:\Program Files\qemu\qemu-system-i386.exe" `
    -m 128M `
    -rtc base=localtime `
    -boot a `
    -drive "if=floppy,format=raw,file=$img"
```

`-rtc base=localtime` supplies the offline CMOS clock with the host's local
date and time. M.I.L.O contains no network time client or timezone database.

## Interaction

- Right-click the root desktop to open applications.
- Minimize a window to place it on the taskbar; click its button to restore it.
- Drag a non-maximized window's bottom-right grip to resize it.
- Open Writer directly or choose Edit in FileHound for native document work.
- Use Ctrl+B, Ctrl+I, and Ctrl+U (or the B/I/U toolbar buttons) to toggle
  formatting at the Writer caret.
- Typing automatically opens or restores Terminal.
- File-changing operations remain explicit and guarded.

See [`docs/MILO_SHELL.md`](docs/MILO_SHELL.md) for the full desktop and testing
contract.
