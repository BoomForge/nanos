# M.I.L.O native root-menu desktop

V0.30.2 extends the accepted V0.26 native root desktop. Its interaction model
deliberately resembles Fluxbox: begin on a quiet root surface, right-click at
the pointer to open a compact application menu, and work in independent
stacking windows. A sparse minimized-application taskbar now anchors the bottom
edge. A live right-side status readout borrows Conky's useful root-level
information density without importing its implementation. The FileHound
application remains a native edition adapted to M.I.L.O's FAT12 root.

Fluxbox, Conky, and FileHound are visual and usability references only. M.I.L.O
does not contain or link their code and does not add Win32, Go, X11, POSIX,
Linux, libc, C++, a window server, or an external runtime. The complete path
remains owned by the custom BIOS loader and direct assembly kernel.

## Root desktop

- Stage 1 clears the firmware text and remains visually silent during normal
  loading. Stage 2 displays the M.I.L.O splash and initialization status,
  holds the completed splash for a true minimum of three seconds, and the
  kernel keeps it visible until RTC, terminal, window, filesystem, and mouse
  initialization are complete.
- Startup then renders the dark M.I.L.O root surface, pointer, right-side
  status overlay, and bottom taskbar. All ordinary applications remain hidden.
- No dashboard, permanent launcher panel, desktop launch strip, or header
  occupies the workspace.
- A new right-button press opens `M.I.L.O APPLICATIONS` at the pointer. `F1`
  opens the same menu at a fixed keyboard-friendly location.
- Menu placement is clamped at the right edge and above the taskbar so the
  complete menu remains visible.
- The compact menu opens System, FileHound, Traits, Terminal, Writer, or the
  Pixel Studio. Up/Down selects, Enter launches/restores, and Esc closes it.
- `Alt+F9` minimizes the focused application and `Alt+F4` closes it. Writer
  continues to guard unsaved work through its existing second-close flow.
- Clicking an application entry opens or restores its independent window and
  closes the menu. Clicking elsewhere dismisses it.
- Moving across menu entries changes the highlighted row only when a boundary
  is crossed, providing feedback without redrawing continuously.

## Minimized-application taskbar

- The 64-pixel bottom bar is permanently visible but intentionally sparse.
- Only applications that are both open and minimized receive a task button.
- Task buttons are built from the real window flags on every composition; no
  duplicate application state is maintained.
- Clicking a task button restores, focuses, and raises that application.
- When no application is minimized, the bar explicitly says so rather than
  displaying non-functional placeholders.
- The right edge shows `M.I.L.O V0.30.2` above the CMOS date and time. Both lines
  share the same right boundary.
- The date format is `DD/MM/YYYY` and the clock is 24-hour `HH:MM`.

## Native status overlay

The status overlay is rendered on the root before application windows, so
normal windows naturally cover it. It reports:

- kernel version;
- `I386 // POLL` CPU mode;
- live input-event rate and a rolling 20-second graph scaled across 0--120+
  events per second;
- current state for System, FileHound, Traits, Terminal, Writer, and Pixel
  Viewer (`ACTIVE`, `OPEN`, `MINIMIZED`, or `CLOSED`);
- open and minimized application counts;
- total processed keyboard and complete mouse-packet event count;
- BIOS-detected total usable RAM;
- live FAT12 free and total capacity calculated from the real allocation table;
- live root file count;
- framebuffer mode;
- offline network state; and
- `N/A` for temperature until target hardware provides a supported sensor.

The current kernel has no scheduler or idle-time sampling and intentionally
does not claim a CPU-use percentage. Its input loop busy-polls, so a percentage
would be misleading until timer accounting and an idle path exist. The graph is
therefore labelled `EVENT RATE` and plots measured events per RTC second.

The clock reads the PC-compatible CMOS RTC directly. It handles BCD or binary
fields and 12-hour or 24-hour firmware modes. QEMU should use
`-rtc base=localtime`; M.I.L.O deliberately contains no timezone database or
network time dependency.

## Native window model

- System, FileHound, Traits, Terminal, and Writer are five kernel-owned application
  windows, not regions of one dashboard.
- Windows overlap and a five-entry z-order determines back-to-front rendering.
- Clicking a visible window focuses and raises it.
- Dragging a non-maximized titlebar shows a lightweight XOR outline; releasing
  commits a position bounded to the 1024-by-704 workspace above the taskbar.
- Dragging the three-step bottom-right grip shows the same lightweight outline;
  releasing commits a size bounded by the application's minimum geometry, the
  right screen edge, and the taskbar workspace.
- Each titlebar has minimize, maximize/restore, and close controls.
- Maximize uses the full workspace above the taskbar.
- Closed applications are reopened from the right-click menu. Minimized
applications can be restored from either the taskbar or root menu.
- Application output is clipped to the current client rectangle. System,
  FileHound, and Traits reposition or scale their useful content as the window
  changes size rather than drawing through the frame.

Typing while Terminal is hidden or minimized restores and focuses it. This
keeps an immediate keyboard recovery path without forcing a terminal-shaped
region onto the desktop.

## Native Writer

Writer is a fifth kernel-owned window, not a Terminal mode. FileHound's Edit
action opens the selected document directly in it. Its logical viewport is
recomputed from the current client width and height on every composition;
long lines pan with the caret and logical rows scroll to keep it visible.
Keyboard navigation and mouse clicks place the caret in the underlying text.
Shift+Arrow extends a visible selection, Ctrl+A selects all, and a mouse drag
selects after typing. `Ctrl+S` and Save use the verified FAT12 write path. Save
As opens a visible, initially empty field and commits it with Enter. An omitted
extension automatically becomes `.TXT`; because the volume is FAT12 8.3,
long bases are normalized to their first eight characters. Closing a modified
document requires a second explicit close.

`Ctrl+B`, `Ctrl+I`, and `Ctrl+U`, or the matching B/I/U toolbar buttons, toggle
Bold, Italic, and Underline at the caret or wrap an existing selection. The
L/C/R/J toolbar controls and Ctrl+L/C/R/J apply Left, Center, Right, or Justified
layout to every paragraph touched by the selection. Each boundary is stored as
a single non-printing control byte in the same FAT12 file. Writer interprets
these bytes while drawing, and Terminal's `type` path ignores them, so layout
remains persistent without a sidecar file, document database, font engine, or
doubled text buffer. Active state is highlighted in the toolbar.

Pressing Enter creates a new paragraph with the current paragraph's alignment.
Writer stores the inherited absolute alignment marker after the newline, so the
new paragraph remains aligned after Save, close, and reopen.

The shared clipped text emitter reloads the source byte after coordinate clip
checks. This prevents resized client text from being replaced by glyphs derived
from screen coordinates—the corruption exposed by V0.27.1 runtime testing.
Writer keystrokes, caret movement, and selection are composed in the same RAM
backbuffer and copied to video only as a completed Writer rectangle. Whole-
desktop changes and the once-per-second live update copy a completed 3 MiB
frame. Lower z-order layers are therefore never deliberately presented as
intermediate frames.

## Terminal surface

Terminal output is stored in a fixed 100-column by 25-row character surface at
physical address `0x32800`. The visible row and column count is derived from
the current Terminal client size, capped by that small backing surface. It is
redrawn inside the client area after a move, resize, focus change,
cover/uncover, minimize, maximize, or whole-desktop recomposition.

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

- **Open** launches Pixel Studio for `.M16`, gives clear unsupported errors
  for common external image formats, or restores Terminal for ordinary files.
- **Edit** opens Writer with the selected file and its 8,191-byte limit.
- **Copy** and **Rename** request an explicit destination 8.3 filename, then
  use the verified FAT12 mutation paths.
- **Delete** requires the operator to type exactly `YES`.
- `Esc` cancels a pending mutation and explicitly reports that no file changed.

Mouse controls never weaken the command router's safety rules. Typo correction
remains read-only assistance and cannot authorize a file mutation.

FileHound is keyboard-operable: Up/Down changes the selected row, Left/Right
changes page, and Enter opens the selection. Combined with `F1`, every native
application can be launched or restored without using the pointer.

## Native Pixel Studio

M16 V1 is a deliberately tiny uncompressed indexed format. Its
64-byte header stores `MI16`, version, palette count, 16-bit width/height, and
sixteen RGB triples; the remaining bytes contain two four-bit pixels per byte.
The viewer accepts 1--16 colours and dimensions up to 128x96, loads into a
separate 8 KiB workspace so Writer remains intact, scales with crisp nearest-
neighbour blocks, and displays the active palette and dimensions. `MILO.M16`
is the built-in 96x64 verification image.

V0.30.2 edits that same packed buffer directly. Pencil and eraser support mouse
dragging; Picker changes the active palette index; Fill uses a bounded queue;
Line and Rectangle use a two-Enter/two-click anchor; and `+`/`-` select integer
zoom. A complete 8 KiB image snapshot at `0x36000` provides one real undo, and
the fill queue at `0x38000` is sized exactly for the 128x96 format ceiling.
New opens a modal picker for 32x32, 64x48, 96x64, or 128x96 canvases. The picker
accepts arrows and Enter, a direct click, or Esc to cancel. Save and Save As use
the same verified FAT12 writer as Writer, appending `.M16` when it is omitted.
Dirty images require a second close, matching Writer's explicit loss guard.
V0.30.1 also corrects the image-state checks to read the one-byte status field;
the earlier 32-bit reads included neighbouring palette/tool state and rejected
otherwise valid drawing, keyboard movement, and Save operations.
V0.30.2 removes whole-desktop composition from the continuous brush path.
Mouse-down and release repaint only the completed Pixel Studio window; movement
packets update only changed scaled pixel blocks in both the visible framebuffer
and compositor backing frame. Bresenham interpolation fills the source pixels
between PS/2 samples, preserving fast-stroke continuity and pointer accuracy.

Pixel Studio is keyboard complete: P/E/I/F/L/R select the six tools, arrows
move the caret, Enter or Space applies the active tool, `[`/`]` changes colour,
Ctrl+Z undoes, Ctrl+S saves, and Ctrl+N opens the canvas picker.

## Mouse path

The kernel enables the standard PS/2 auxiliary device, requests default
settings, enables streaming, and polls three-byte packets beside keyboard
input. V0.29 leaves the old cursor visible while all three bytes and the new
coordinates are decoded. Non-overlapping motion-only moves draw the new cursor
before restoring the old position, so fast movement never deliberately
presents an empty pointer frame. Drag packets that remain in one Writer cell
do not repaint the window, and selection begins at the pressed cell rather than
the previous caret. A rising right-button bit
invokes the root-menu binding; the left button continues to dispatch selection,
dragging, and release.

The cursor is clamped so its complete 12-by-16 shape remains inside the
1024-by-768 framebuffer. The apparent inability to reach an edge was traced to
QEMU relative mouse capture when the host pointer begins off-centre. The kernel
already decodes quick-movement overflow packets and reaches its full coordinate
range.

The cursor uses its normal 768-byte saved-under buffer at `0x32400` and a
second handoff buffer at `0x33400` for non-overlapping fast moves.

## Verification

Run:

```sh
make verify-milo-boot
```

This audits the boot layout, routing fixtures, full-volume FAT12 contents,
binary version markers, application-hidden startup, RTC-backed splash hold,
CMOS normalization, scaled live activity sampling, honest native status
collection, minimized-only task slots and restore routing, taskbar-aware
menu/window bounds, bounded resize dispatch, client clipping, adaptive
FileHound layout, dynamic Terminal viewport, GUI/Terminal text isolation,
root-menu geometry, mouse and keyboard routing, six-window behavior, Writer persistence,
selection, inline formatting and alignment, automatic `.TXT` Save As,
atomic full-frame and Writer-region composition,
resize-safe glyph rendering, pending-action
buffer isolation, title actions, no-blank cursor handoff, PS/2 button handling,
M16 validation/rendering, palette data, and cursor storage.

Final interaction must be checked in QEMU on Windows:

```powershell
$img = "$env:USERPROFILE\Downloads\M.I.L.O-floppy-V0.30.2.img"
& "C:\Program Files\qemu\qemu-system-i386.exe" -m 128M -rtc base=localtime -boot a -drive "if=floppy,format=raw,file=$img"
```
