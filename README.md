# M.I.L.O

M.I.L.O is an extremely small offline graphical x86 operating system with its
own BIOS boot sector, two-stage loader, protected-mode assembly kernel, FAT12
storage, stacking desktop, text tools, and deterministic M.I.L.O interaction.
It does not use Linux, GRUB, an LLM, or an external desktop runtime.

The `M.I.L.O` branch is the primary project. The inherited modular C89 NanOS
work remains in the repository for upstream development and general reusable
improvements; it is not the M.I.L.O runtime.

## Current release

V0.30.2 provides:

- silent normal loading followed by a native ASCII M.I.L.O splash held for at
  least three seconds;
- a Fluxbox-inspired root menu available by right-click or `F1`, with
  Arrow/Enter/Esc navigation and six movable, resizable application windows;
- a minimized-only taskbar with click-to-restore buttons;
- CMOS-backed local date/time and right-aligned version status;
- a native Conky-inspired overlay with a real 0--120+ events/second graph,
  application states, RAM, FAT12 capacity, file/event counts, video, network,
  and honest unsupported-sensor reporting;
- native FileHound-inspired FAT12 browsing and real open/edit/copy/rename/delete;
- a fifth native Writer window with resize-aware text, mouse caret placement,
  Save/Save As with automatic `.TXT` extension handling, guarded close,
  keyboard and mouse text selection, compact persistent Bold/Italic/Underline,
  and inherited Left/Center/Right/Justified paragraph alignment;
- a corrected clipped-glyph path that keeps application text readable through
  window resizing, buffered Writer-region repainting, and full-frame buffered
  desktop composition to prevent lower layers flashing through;
- a native M16 Pixel Studio for validated 128x96-or-smaller, 16-colour indexed
  images, with pencil/eraser drag, colour picker, bounded flood fill, line and
  rectangle tools, palette selection, zoom, a pixel caret, one full-image undo,
  selectable 32x32/64x48/96x64/128x96 New canvases, verified FAT12 Save/Save As,
  automatic `.M16`, guarded dirty close, corrected byte-accurate edit gates,
  direct dirty-pixel presentation, and interpolated fast mouse strokes;
- a no-blank fast-pointer handoff, corrected mouse-selection anchoring,
  keyboard FileHound browsing, `Alt+F9` minimize, `Alt+F4` guarded close, and a
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
build/M.I.L.O-floppy-V0.30.2.img
```

## Run on Windows

```powershell
$img = "$env:USERPROFILE\Downloads\M.I.L.O-floppy-V0.30.2.img"
& "C:\Program Files\qemu\qemu-system-i386.exe" `
    -m 128M `
    -rtc base=localtime `
    -boot a `
    -drive "if=floppy,format=raw,file=$img"
```

`-rtc base=localtime` supplies the offline CMOS clock with the host's local
date and time. M.I.L.O contains no network time client or timezone database.

## Interaction

- Press `F1` or right-click the root desktop to open applications. Use
  Up/Down and Enter to launch, or Esc to close the menu.
- Press `Alt+F9` to minimize the focused window or `Alt+F4` to close it. An
  unsaved Writer document retains its existing second-close confirmation.
- Minimize a window to place it on the taskbar; click its button to restore it.
- Drag a non-maximized window's bottom-right grip to resize it.
- Open Writer directly or choose Edit in FileHound for native document work.
- In FileHound, use Up/Down to select, Left/Right to change pages, and Enter to
  open. Opening `MILO.M16` launches native Pixel Studio.
- In Pixel Studio, use P/E/I/F/L/R for Pencil, Eraser, Picker, Fill, Line, and
  Rectangle; arrows move the pixel caret, Enter draws, `[`/`]` changes colour,
  `+`/`-` zooms, Ctrl+Z undoes, Ctrl+S saves, and Ctrl+N opens the canvas-size
  picker. Select a size with arrows and Enter or click it; Esc cancels.
- Select Writer text with Shift+Arrow or a mouse drag, then use Ctrl+B/I/U or
  the toolbar to style existing text. Ctrl+L/C/R/J applies paragraph alignment.
- Typing automatically opens or restores Terminal.
- File-changing operations remain explicit and guarded.

See [`docs/MILO_SHELL.md`](docs/MILO_SHELL.md) for the full desktop and testing
contract.
