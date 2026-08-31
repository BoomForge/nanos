# M.I.L.O native root-menu desktop

V0.26.2 refines the accepted V0.26 native root desktop. Its interaction model
deliberately resembles Fluxbox: begin on a quiet root surface, right-click at
the pointer to open a compact application menu, and work in independent
stacking windows. A slim right-side status readout borrows Conky's useful
root-level information density without importing its implementation. The
FileHound application is a native edition adapted to M.I.L.O's FAT12 root.

Fluxbox, Conky, and FileHound are visual and usability references only. M.I.L.O
does not contain or link their code and does not add Win32, Go, X11, POSIX,
Linux, libc, C++, a window server, or an external runtime. The complete path
remains owned by the custom BIOS loader and direct assembly kernel.

## Root desktop

- Startup renders the dark M.I.L.O root surface, pointer, and a narrow status
  overlay on the right. All ordinary applications remain hidden.
- No dashboard, permanent application panel, desktop launch strip, header, or
  taskbar occupies the workspace.
- A new right-button press opens `M.I.L.O APPLICATIONS` at the pointer.
- Menu placement is clamped at the right and bottom framebuffer edges so the
  complete menu remains visible.
- The compact menu opens System, FileHound, Traits, or Terminal.
- Clicking an application entry opens or restores its independent window and
  closes the menu. Clicking elsewhere dismisses it.
- Moving across menu entries changes the highlighted row only when a boundary
  is crossed, providing feedback without redrawing continuously.

## Native status overlay

The status overlay is rendered on the root before application windows, so
normal windows naturally cover it. It reports:

- kernel version;
- `I386 // POLL` CPU mode;
- processed keyboard and complete mouse-packet event count;
- BIOS-detected total usable RAM;
- live FAT12 free and total capacity calculated from the real allocation table;
- live root file count;
- framebuffer mode;
- offline network state; and
- `N/A` for temperature until target hardware provides a supported sensor.

The current kernel has no scheduler or idle-time sampling and intentionally
does not claim a CPU-use percentage. Its input loop busy-polls, so a percentage
would be misleading until timer accounting and an idle path exist.

## Native window model

- System, FileHound, Traits, and Terminal are four kernel-owned application
  windows, not regions of one dashboard.
- Windows overlap and a four-entry z-order determines back-to-front rendering.
- Clicking a visible window focuses and raises it.
- Dragging a non-maximized titlebar shows a lightweight XOR outline; releasing
  commits a position bounded to the full 1024-by-768 root desktop.
- Each titlebar has minimize, maximize/restore, and close controls.
- Maximize uses the full framebuffer because V0.26 reserves no taskbar or
  global header area.
- Closed or minimized applications are reopened from the right-click menu.

Typing while Terminal is hidden or minimized restores and focuses it. This
keeps an immediate keyboard recovery path without forcing a terminal-shaped
region onto the desktop.

## Terminal surface

Terminal output is stored in a fixed 100-column by 25-row character surface at
physical address `0x32800`. This 2,500-byte surface is redrawn inside the
Terminal client area after a move, focus change, cover/uncover, minimize,
maximize, or whole-desktop recomposition.

The terminal cursor is tracked relative to the Terminal window when that
window moves or changes size. GUI captions temporarily disable terminal
capture, so application text cannot leak into command history.

## Application rendering

System, FileHound, and Traits draw from a common reference origin translated
into the owning window, keeping the native manager small. Text and numbers use
a GUI-only emitter clipped to the physical framebuffer. They cannot inherit
Terminal capture or wrapping, so a Terminal window boundary cannot displace
desktop content. FileHound uses the same translation for pointer hit-testing,
so visible rows, controls, and active regions remain aligned after movement.

The shell redraws from current kernel state. It is not a second copy of FAT12
or personality data.

## Native FileHound edition

The current filesystem exposes one FAT12 root rather than a directory tree, so
this is deliberately a small single-pane FileHound adaptation rather than a
pretend desktop explorer. It provides:

- a visible `A:\` location strip;
- aligned Name, Type, and Size columns;
- compact alternating rows with a clear selected row;
- text, data, and generic file-category accents;
- honest item count, local/FAT12 status, and previous/next paging; and
- the existing real Open, Edit, Copy, Rename, and guarded Delete operations.

Back, Up, New Folder, search, and dual-pane controls will appear only after the
underlying filesystem supports them. Repainting uses a separate filename
buffer, so a covered or moved FileHound window cannot alter a pending file
operation.

## File actions

- **Open** restores Terminal and displays the selected file through the
  multi-cluster FAT12 reader.
- **Edit** restores Terminal and starts the existing 8,191-byte editor.
- **Copy** and **Rename** request an explicit destination 8.3 filename, then
  use the verified FAT12 mutation paths.
- **Delete** requires the operator to type exactly `YES`.
- `Esc` cancels a pending mutation and explicitly reports that no file changed.

Mouse controls never weaken the command router's safety rules. Typo correction
remains read-only assistance and cannot authorize a file mutation.

## Mouse path

The kernel enables the standard PS/2 auxiliary device, requests default
settings, enables streaming, and polls three-byte packets beside keyboard
input. V0.26.2 leaves the cursor visible while bytes one and two arrive, then
erases, moves, and redraws it once when byte three completes the packet. This
removes the former three-redraw movement flicker. A rising right-button bit
invokes the root-menu binding; the left button continues to dispatch selection,
dragging, and release.

The cursor is clamped so its complete 12-by-16 shape remains inside the
1024-by-768 framebuffer. The apparent inability to reach an edge was traced to
QEMU relative mouse capture when the host pointer begins off-centre. The kernel
already decodes quick-movement overflow packets and reaches its full coordinate
range.

The cursor uses a 768-byte saved-under buffer at `0x32400`. It is removed
before input handling or recomposition and restored afterward.

## Verification

Run:

```sh
make verify-milo-boot
```

This audits the boot layout, routing fixtures, full-volume FAT12 contents,
binary version markers, application-hidden startup, native status collection,
GUI/Terminal text isolation, absence of launcher/taskbar chrome, root-menu
geometry, hover and routing, four-window behavior, translated FileHound rows
and controls, pending-action buffer isolation, drag dispatch, title actions,
terminal surface isolation, complete-packet cursor redraw, PS/2 button
handling, and cursor storage.

Final interaction must be checked in QEMU on Windows:

```powershell
$img = "$env:USERPROFILE\Downloads\M.I.L.O-floppy-V0.26.2.img"
& "C:\Program Files\qemu\qemu-system-i386.exe" -m 128M -boot a -drive "if=floppy,format=raw,file=$img"
```
