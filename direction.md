# M.I.L.O project direction

This file is the authoritative statement of what the M.I.L.O operating-system
fork is building, what it deliberately is not building, and why each committed
change exists. It must be updated in every commit made on the `M.I.L.O` branch.

## Project goal

M.I.L.O is an extremely small, fast, offline operating system for a dedicated
low-cost x86 computer. It is intended to become a useful personal terminal,
document workstation, floppy-disk tool, and low-resolution image workstation
with a deterministic M.I.L.O personality at the centre of the interface.

The finished machine should feel like an object from the M.I.L.O universe while
remaining practical: it should boot quickly, manage real files, edit text,
inspect and edit simple images, and respond with a personality that develops
from structural interaction patterns. It must not depend on Linux, GRUB, an
internet service, an LLM, or a large external runtime.

The long-term hardware target is a very small, inexpensive, low-power x86 board
with modest memory, VGA output, at least two USB connections, and access to a
real floppy drive or equivalent removable FAT storage. The operating system is
being proven in QEMU before a specific board or custom carrier is locked in.

## Non-negotiable principles

1. **Own the boot path.** M.I.L.O uses its own BIOS boot sector, loader, and
   kernel rather than relying on GRUB or a host operating system.
2. **Remain tiny and fast.** Every feature must justify its code and memory
   cost. Reserved capacity is not permission for careless growth.
3. **Remain completely offline.** Networking, telemetry, cloud inference, and
   remote dependencies are outside the intended appliance.
4. **Use deterministic intelligence.** M.I.L.O may learn bounded abstract
   patterns and traits, but it does not use an LLM and does not store the
   operator's sentences as personality data.
5. **Prefer useful behaviour over demonstrations.** Files must persist, editors
   must save correctly, and graphical controls must invoke real operations.
6. **Protect destructive actions.** Typo correction may assist safe reading and
   navigation. It must never silently manufacture delete, rename, overwrite, or
   copy authority.
7. **Keep the early architecture direct.** The M.I.L.O branch intentionally
   uses a compact kernel-driven model instead of copying the abstraction layers
   of a conventional Linux kernel.
8. **Preserve a terminal path.** Graphical features add to the terminal and do
   not remove the transparent command interface used for testing and recovery.
9. **Separate project direction from upstream NanOS.** M.I.L.O-specific work
   belongs on `M.I.L.O`; genuinely general improvements may also be applied to
   the upstream branch as a separate deliberate change.

## Intended system scope

### Boot and platform

- BIOS boot on 32-bit x86 hardware.
- Two-stage custom loader and protected-mode kernel entry.
- Reliable memory discovery, framebuffer selection, keyboard, mouse, timers,
  simple sound, and power-state handling appropriate to the target hardware.
- A bootable 1.44 MB floppy image for development and physical-media testing.

### Storage and documents

- Full-volume FAT12 reading and writing with verified persistence.
- Root file browser, file creation, reading, searching, copying, renaming, and
  guarded deletion.
- A compact text editor suitable for notes and small documents.
- Physical floppy read/write and disk-change support before 1.0.
- Later removable-media support where it can be kept small and auditable.

### M.I.L.O interface

- Canonical pink-purple-on-dark visual language.
- A sparse native stacking desktop whose visual discipline and root-menu
  workflow are inspired by Fluxbox: a quiet root surface with a low-density
  system-status overlay, a compact pointer-positioned right-click application
  menu, overlapping movable windows, visible focus, title controls, and a
  bottom taskbar reserved for genuinely minimized applications plus local
  version/date/time state.
  Fluxbox and Conky are behavioral and visual references only; their code, X11,
  and Linux runtime are not dependencies.
- A recognisable M.I.L.O system shell with the ring as the central system-state
  indicator rather than decoration alone.
- The terminal is an application window, not a permanently embedded dashboard
  region. It must remain instantly reachable for commands and recovery.
- Mouse and keyboard operation, clear errors, and fast navigation on a
  low-resolution display.
- Small local sound cues tied to boot, success, warning, activity, and M.I.L.O
  state without requiring a full multimedia stack.

### Deterministic personality

- Nyx-derived intent-first routing adapted to the tiny kernel.
- Structural learning from punctuation, interaction shape, courtesy, action
  patterns, and other abstract signals rather than memorised dialogue.
- Persistent warmth, cheek, curiosity, and routing traits.
- Canon M.I.L.O response material selected deterministically from learned state.
- Bounded typo handling with explicit ambiguity and non-execution feedback.
- No generated claims of actions that did not occur.

### Applications

- Terminal and command environment.
- Native FileHound-inspired FAT12 file browser with a clean single-pane
  location, metadata, selection, paging, and real-operation workflow.
- Text editor and document viewer.
- Low-resolution native image viewer and pixel editor.
- Palette tools, bounded undo, and verified floppy saving.
- Begin image support with a compact uncompressed indexed format. Mavica-style
  JPEG support is a later optional target because a correct JPEG decoder has a
  much larger implementation and memory cost.

## Explicitly outside scope

- General-purpose Linux or POSIX compatibility.
- Internet browsing, network accounts, cloud services, or online updates.
- LLM inference or probabilistic text generation.
- Modern desktop multitasking for its own sake.
- Large application frameworks, package managers, or dependency ecosystems.
- Hardware breadth that compromises the selected dedicated machine.

## Current architecture and status

The combined **V0.23** release booted successfully in QEMU and exposed a mouse
edge defect during runtime testing. **V0.24** corrected the packet handling and
was runtime-verified, but its fixed dashboard did not meet the intended desktop
interaction model. **V0.25** added a working stacking window manager and was
runtime-verified, but its permanent launchers, header, taskbar, and initially
open System window still presented as a dashboard. **V0.26** kept the proven
native windows while replacing the launch chrome with an empty root desktop
and compact right-click menu; that interaction model is now runtime-verified.
The **V0.26.1** candidate fixed cursor flicker, added focus and menu-hover
polish, and placed a native Conky-like status readout behind application
windows. Runtime screenshots then exposed two genuine composition defects:
desktop text inherited Terminal's wrap boundary and the file-name renderer
bypassed moved-window translation. **V0.26.2** replaces that shared text path,
audits the native layouts, and adapts Files into a recognisable native
FileHound edition. **V0.27** keeps the sparse right-click workflow while adding
a minimized-only taskbar, CMOS date/time, live measured activity history, and
real application-state reporting. It also makes normal Stage 1 and Stage 2
loading visually silent until the M.I.L.O splash is ready, then retains that
splash until kernel and mouse initialization finish. The remaining apparent
pointer-edge limit was identified as QEMU's relative mouse capture behaviour
rather than a kernel coordinate defect. Runtime testing then showed that fast
hardware could make the splash effectively invisible and that the first graph
scale saturated during ordinary input. **V0.27.1** holds the splash for at
least three seconds, scales the graph across 0--120+ events per second, and
adds bounded native window resizing with clipped, adaptive application
layouts and a size-aware Terminal viewport. Runtime resizing then exposed a
shared clipped-text bug: coordinate checks overwrote the glyph byte before it
was drawn. **V0.28** reloads the source byte at the draw boundary and adds a
fifth native Writer window with a resize-derived viewport, mouse caret
placement, verified Save and Save As, and guarded close. Runtime typing then
showed that full desktop composition per character briefly exposed every lower
layer. **V0.28.1** limits routine Writer edits to a front-window repaint while
retaining full composition for actual desktop state changes. Continued runtime
testing found that timed whole-desktop refreshes could still expose intermediate
layers and that Writer's prefilled 12-character Save As value rejected all new
input. **V0.28.2** composes complete desktop frames in RAM before presenting
them, opens a visible empty 8.3 name field, and adds persistent one-byte
Bold/Italic/Underline toggles to the native Writer.
Runtime use then showed that extensionless Save As names failed, existing text
could not be selected and styled, paragraph alignment was absent, and focused
Writer repaint still touched video memory incrementally. **V0.28.3** normalizes
extensionless names to `.TXT`, adds visible keyboard and mouse selection,
selection-aware styles, four paragraph alignments, and RAM-backed region
presentation for routine Writer updates.
Runtime verification accepted the editor features but found that the pointer
still disappeared for too much of each movement packet and that Enter reset a
new paragraph to left alignment. **V0.28.4** stages new pointer coordinates
before erasing the old cursor, redraws before leaving the packet handler,
suppresses same-cell selection repaint, and writes the current alignment marker
after a newly inserted paragraph break. Further runtime use showed that rapid
non-overlapping pointer movement could still expose the short erase/redraw gap,
and that mouse selection anchored at the previous keyboard caret before the
clicked cell was resolved. The **V0.29** candidate adds a second saved-under
cursor buffer for a no-blank fast-motion handoff, anchors Writer selection at
the pressed cell, and makes the shell launch workflow keyboard accessible with
F1, arrows, Enter, and Escape. It also adds the sixth native application: a
read-only Pixel Viewer for the compact M16 V1 indexed format. FileHound can
identify and open M16 files, the bundled 96x64 test image exposes all sixteen
palette entries, and the viewer reports invalid, unsupported, oversized, and
failed FAT12 reads without disturbing an open Writer document.
**V0.29.1** completes the top-level keyboard workflow with `Alt+F9` minimizing
the focused application and `Alt+F4` closing it through the same guarded path
as the title-bar control. Dirty Writer documents therefore remain protected.
**V0.30** turns the accepted read-only viewer into native Pixel Studio. It edits
the packed M16 buffer directly with Pencil, Eraser, Picker, bounded Fill, Line,
Rectangle, palette and zoom controls, a keyboard pixel caret, and one fixed
undo snapshot. New creates a strict 64x48 canvas; Save and Save As reuse the
verified FAT12 writer, append `.M16` when needed, and protect dirty work with a
second-close guard. The application remains independent from Writer and adds no
heap, codec framework, external runtime, or Linux code.
Runtime verification then exposed a state-width defect: four editing paths read
the one-byte image status as a 32-bit value, so the adjacent nonzero palette and
tool bytes made valid canvases appear unavailable. **V0.30.1** uses byte-accurate
status gates for pointer drawing, keyboard movement, tool dispatch, and Save.
It also replaces the fixed New operation with a modal, mouse-and-keyboard canvas
picker offering 32x32, 64x48, 96x64, and the full 128x96 M16 ceiling.
Runtime drawing then proved correct but unacceptably slow because every pointer
sample recomposed and copied the complete desktop. **V0.30.2** keeps full-frame
composition for desktop transitions, but gives live Pencil/Eraser strokes a
bounded dirty-pixel path that updates the visible and backing frames together.
Integer line interpolation fills pixels between PS/2 samples, so fast strokes
remain continuous and aligned with the pointer rather than becoming sparse.
Runtime testing then found that Rectangle's visible button extended forty
pixels beyond its routed tool region even though the rectangle rasterizer was
sound. **V0.31** restores the complete button hitbox and deliberately moves the
active feature budget away from image interchange. It establishes the
text-first milestone instead: categorized and aliased Terminal help, local
status/clock/application discovery, direct Writer launch, deterministic social
token patterns, and a minimal native PC-speaker layer with boot, M.I.L.O,
Writer-save, and explicit test cues. Pixel Studio remains a maintained
supporting application while Writer, command interaction, and M.I.L.O become
the primary development path.

- Stage 1 occupies the BIOS boot sector and loads Stage 2.
- Stage 2 detects memory, selects a 1024x768 32-bit VBE framebuffer, caches the
  FAT12 metadata and near data window, enters protected mode, and supplies BIOS
  sector services to the kernel.
- The assembly kernel owns the terminal, editor, deterministic router, pattern
  memory, FAT12 operations, keyboard input, and framebuffer drawing.
- The V0.31 candidate kernel is 45,075 bytes; 48 KiB is reserved for controlled
  growth, leaving 4,077 bytes in the current kernel region.
- Whole-desktop composition uses a fixed 3 MiB backbuffer from physical 1 MiB
  through 4 MiB; the intended machine and QEMU profile provide at least 5 MiB.
- The FAT12 data area contains 2,734 accessible clusters. Far clusters are read
  on demand and all written sectors are read back and verified.
- Text editing is deliberately limited to 8,191 bytes per document.
- Image work uses an independent fixed 8 KiB buffer at physical `0x34000`, one
  8 KiB undo snapshot at `0x36000`, and a 24 KiB fill queue at `0x38000`, so it
  cannot overwrite Writer's 8,191-byte document buffer and all memory remains
  bounded at the 128x96 M16 ceiling.
- `MILO.MEM` stores a checksummed 24-byte structural personality profile.

## Release roadmap

### V0.23 — combined V0.22 and V0.23 milestone

- First persistent graphical M.I.L.O shell after the splash.
- Central M.I.L.O ring and clear local/offline/system state.
- PS/2 mouse initialization, packet decoding, software cursor, and click
  routing without breaking keyboard polling.
- Graphical FAT12 root browser with real file selection.
- Graphical open and edit actions.
- Copy and rename flows that safely request a destination 8.3 name.
- Guarded graphical deletion requiring explicit confirmation.
- Terminal and editor remain available as the recovery and power-user path.

There is deliberately no separate V0.22 image: the two milestones share one
input and redraw path, so shipping them together avoids testing an artificial
half-state that will not be maintained.

### V0.24 — compact traditional desktop

- Correct fast PS/2 edge movement instead of discarding overflow packets.
- Adapt the `master` branch desktop layout principles to the direct M.I.L.O
  kernel: desktop shortcuts, framed application surface, title controls, a
  dedicated terminal window, and bottom taskbar.
- Keep one active kernel-owned application surface for now rather than adding
  the generic branch's heap, process, compositor, and overlapping-window cost.
- Preserve all real FAT12 actions, deterministic traits, and terminal recovery.

Runtime result: the functions worked, but the single fixed application surface
still felt like a dashboard rather than a traditional operating-system desktop.
That visual and interaction mismatch is the reason for the V0.25 replacement.

### V0.25 — classic stacking desktop

- Four kernel-owned application windows: Home, Files, Traits, and Terminal.
- Overlapping rendering with deterministic focus and z-order.
- Movable titlebars using a lightweight outline drag path.
- Working minimize, maximize/restore, and close controls.
- Beveled classic frames, active/inactive titlebars, desktop launchers, a
  stateful bottom taskbar, and an upward-opening M.I.L.O application menu.
- A 100-by-25 logical terminal surface that survives covering, movement,
  minimizing, and full desktop redraws.
- Keep the proven FAT12 browser/actions and deterministic traits inside the new
  window model without adding a heap, process server, or external runtime.

Runtime result: window movement, focus, controls, file paths, and terminal
retention worked, but the permanent desktop chrome still looked like the older
dashboard. V0.26 keeps the mechanics and replaces the presentation contract.

### V0.26 — native Fluxbox-style root desktop

- Start with all four applications hidden on an unoccupied dark root surface.
- Remove the permanent header, desktop launcher column, and bottom taskbar.
- Open a compact M.I.L.O application menu at the pointer on a new right click,
  with edge clamping that keeps the full menu on screen.
- Launch or restore System, Files, Traits, and Terminal as independent floating
  windows; closed and minimized applications remain reachable from the menu.
- Preserve focus, z-order, outline dragging, minimize, maximize/restore, close,
  translated file controls, real FAT12 operations, and terminal retention.
- Implement the complete workflow in native M.I.L.O assembly. Fluxbox defines
  the intended visual economy and usability, not the driving code or runtime.

### V0.26.1 — interface polish and native status overlay

- Erase and redraw the software pointer once per complete PS/2 packet instead
  of once for each of its three bytes, removing movement flicker.
- Add pointer-row hover feedback to the root menu and clearer active/inactive
  title decoration without adding a widget framework.
- Add a slim right-side, Conky-inspired root overlay behind application
  windows. It reports the detected RAM total, real FAT12 free/total capacity,
  root file count, input-event count, video mode, network state, and version.
- Report CPU mode as `I386 // POLL` rather than inventing a utilization number:
  the current pre-scheduler kernel busy-polls input and cannot yet calculate a
  meaningful idle percentage.
- Report temperature as `N/A` until selected hardware exposes a supported
  sensor path; never display fabricated telemetry.
- Keep this implementation entirely native. No Conky, Fluxbox, X11, Linux, or
  external monitoring code is included.

Runtime result: the cursor and overlay functions were present, but terminal
line wrapping displaced right-side GUI strings to the far left and one legacy
file-name draw path ignored window translation. V0.26.2 corrects the rendering
contract rather than compensating with per-screen coordinate offsets.

### V0.26.2 — corrected composition and native FileHound

- Give desktop text and numbers a dedicated screen-clipped emitter that never
  inherits Terminal capture, newline, or right-edge wrapping behavior.
- Make every FileHound row, name, type, size, marker, button, and click target
  use the same translated reference coordinates when its window moves.
- Replace the rough Files list with a native FileHound-inspired single pane:
  `A:\` location strip, Name/Type/Size columns, alternating compact rows,
  selection highlight, type-category accents, paging, item status, and classic
  controls for the already working Open/Edit/Copy/Rename/Delete operations.
- Keep the adaptation honest to the current FAT12 root-only filesystem. Do not
  display fake folder navigation, search, dual-pane, or toolbar controls before
  those capabilities exist.
- Keep FileHound as a visual and usability reference only. Its Go/Win32 code is
  not imported; this edition is implemented directly in the M.I.L.O kernel.
- Maintain a separate render-name buffer so repainting the browser cannot
  corrupt the source filename of a pending copy, rename, or delete operation.

### V0.27 — live desktop state and minimized workflow

- Silence normal text-mode loader chatter so the first OS-controlled visual is
  the M.I.L.O splash; keep it present until kernel, RTC, terminal, window, FAT12,
  and mouse initialization are complete.
- Add a permanent 64-pixel taskbar that creates buttons only for genuinely open
  and minimized applications. Clicking a task button restores, focuses, and
  raises that application.
- Reserve the taskbar's right edge for two right-aligned lines: `M.I.L.O V0.27`
  above Australian `DD/MM/YYYY` and 24-hour `HH:MM` time.
- Read the PC-compatible CMOS RTC directly, including BCD/binary and
  12-hour/24-hour normalization. Remain offline and timezone-database-free;
  QEMU uses `-rtc base=localtime`.
- Extend the native Conky-style root overlay with measured input activity per
  RTC second, a rolling 20-sample graph, open/minimized counts, and real state
  for System, FileHound, Traits, and Terminal.
- Continue to label CPU as `I386 // POLL`. Event activity is useful and real;
  CPU percentage would be fabricated until timer interrupts, scheduler/idle
  accounting, and a non-busy wait path exist.
- Constrain maximize, dragging, and root-menu placement to the workspace above
  the taskbar while preserving every verified storage and terminal path.

### V0.27.1 — visible boot and resizable applications

- Hold the completed M.I.L.O splash for a true minimum of three seconds using
  the hardware RTC, with a watchdog that preserves boot if the RTC stalls.
- Relabel the graph `EVENT RATE` and map its rolling history across 0--120+
  events per second so ordinary mouse and keyboard activity remains readable
  instead of immediately saturating.
- Add a visible bottom-right grip to every non-maximized window. Resize within
  per-application minimums and the workspace boundary above the taskbar.
- Clip all application drawing to the current client rectangle and adapt Home,
  FileHound, and Traits geometry when their windows change size.
- Derive Terminal's visible rows and columns from its current client area while
  retaining the small fixed 100-by-25 backing surface.

### V0.28 — graphical document workspace

- Add Writer as a fifth native stacking application, launched from the root
  menu or FileHound's Edit action rather than borrowing Terminal.
- Derive visible rows and columns from the current client rectangle, keep the
  caret in view, pan long lines, and support mouse caret placement.
- Provide native Save, validated 8.3 Save As, status feedback, and a guarded
  close for modified documents using the existing verified FAT12 write path.
- Fix the shared clipped-glyph renderer so coordinates cannot replace source
  characters after a window resize.

### V0.28.1 — stable Writer repaint

- Repaint only the active frontmost Writer window for character insertion,
  deletion, navigation, Save As input, and status changes.
- Retain full desktop composition for opening, closing, moving, resizing,
  minimizing, restoring, focus changes, and timed live-state refreshes.
- Preserve and restore Terminal drawing state around the focused repaint.

### V0.28.2 — buffered desktop and formatted Writer

- Assemble full 1024x768x32 desktop refreshes in a fixed RAM backbuffer and
  copy only complete frames to video memory, eliminating visible z-order
  intermediates during the RTC refresh and state changes.
- Replace the full `UNTITLED.TXT` Save As value with a visible, genuinely empty
  8.3 input field so new and renamed document targets can be entered.
- Add native Bold, Italic, and Underline toggles through Ctrl+B/I/U and toolbar
  controls. Persist each style boundary as one inline control byte, keep it out
  of logical columns, and ignore it in ordinary Terminal file output.

### V0.28.3 — selection, alignment, and stable Writer presentation

- Append `.TXT` when Save As receives an extensionless name and normalize the
  base to FAT12's eight-character limit.
- Add visible Shift+Arrow, Ctrl+A, and mouse-drag selection. Apply Bold,
  Italic, or Underline after typing by wrapping the selected byte range.
- Add persistent Left, Center, Right, and Justified paragraph alignment through
  toolbar controls and Ctrl+L/C/R/J, including multi-paragraph selections.
- Compose routine Writer updates in RAM and copy only the completed Writer
  rectangle to video memory to remove the remaining incremental repaint flash.

### V0.28.4 — stable pointer and inherited paragraph alignment

- Keep the old software cursor visible while a complete PS/2 packet and its new
  coordinates are decoded, then erase/commit/redraw at the last safe point.
- Redraw the cursor inside the completed packet handler and avoid expensive
  Writer repaints when a selection drag remains inside the same character cell.
- When Enter creates a paragraph, inherit the active Left, Center, Right, or
  Justified alignment and persist its marker directly in the saved document.

### V0.29 — low-resolution image viewing

- Add the M16 V1 format: one 64-byte header, 1--16 RGB palette entries, and two
  four-bit pixel indexes per byte, with deliberate 128x96 and 8 KiB limits.
- Load images through a separate fixed workspace and display them in a sixth
  movable, resizable, minimizable native window using crisp integer scaling.
- Show the filename, dimensions, active palette, and explicit invalid,
  unsupported, oversized, or FAT12-read errors. Ship a generated 96x64 fixture.
- Add F1/arrow/Enter/Escape root-menu control and keyboard FileHound navigation,
  while correcting fast pointer handoff and mouse-selection anchoring.

### V0.29.1 — complete keyboard window control

- Minimize the focused native application with `Alt+F9`.
- Close the focused application with `Alt+F4` while retaining Writer's guarded
  second-close requirement for unsaved documents.
- Ignore both shortcuts safely when the root desktop has no focused window.

### V0.30 — pixel image editing

- Turn Pixel Viewer into a compact Paint-like workspace with pencil, eraser,
  palette selection, colour picker, fill, line, rectangle, zoom, and one bounded
  undo state.
- Add New, Save, and Save As through the verified FAT12 path while keeping M16
  as the small lossless editable working format.
- Keep dimensions and memory limits strict enough for the dedicated appliance.

### V0.30.1 — verified drawing and selectable canvases

- Correct every Pixel Studio edit/save availability check to read only the
  declared one-byte status instead of neighbouring palette and tool state.
- Replace fixed-size New with a visible modal choice of 32x32, 64x48, 96x64,
  and 128x96 canvases.
- Keep the picker fully operable by arrows/Enter/Escape and by direct clicks,
  with no heap or unbounded allocation.

### V0.30.2 — responsive continuous drawing

- Remove whole-desktop composition and 3 MiB frame copies from continuous
  Pencil/Eraser movement.
- Present only the Pixel Studio window at stroke boundaries and only changed
  scaled pixel blocks during movement, updating both framebuffer copies.
- Interpolate source pixels between pointer samples so quick motion produces a
  continuous, accurate stroke rather than disconnected points.

### V0.31 — text-first interaction and native sound

- Correct the complete Rectangle-button hit region while retaining the already
  working bounded M16 rasterizer and treating Pixel Studio as a stable support
  application.
- Make Terminal help task-oriented and add `status`, `clock`, `date`, `apps`,
  `writer`, `sound on`, `sound off`, `beep`, and compact `cls`/`ls`/`ver`
  aliases without weakening guarded file changes.
- Recognize greeting, gratitude, farewell, identity, wellbeing, and capability
  token patterns after operational routing declines the input. Reuse bounded
  typo matching while persisting only structural counters and traits.
- Add a native PC-speaker/PIT layer with short boot, M.I.L.O interaction, and
  Writer-save cues, an explicit test command, and an in-session mute control.
- Keep Writer and transparent commands as the appliance's primary workflow.

### V0.32 — deeper Writer, M.I.L.O, and Terminal workflow

- Expand Nyx-derived intent routing and structural traits without phrase
  storage, including clearer explanations of what M.I.L.O did or declined.
- Add stronger document navigation, discoverable file operations, useful
  command aliases/macros, and small recoverable script execution.
- Improve keyboard-first Writer and Terminal transitions before adding more
  graphical application breadth.
- Keep sound cues short, optional, and state-driven rather than introducing a
  continuous multimedia engine.

### V0.33 — in-system application development

- Define a tiny documented application ABI for windows, drawing, keyboard,
  files, and lifecycle events instead of allowing applications to depend on
  arbitrary internal kernel labels.
- Add a compact interpreted or bytecode language, assembler/compiler, loader,
  and `run` command so source can be written in Writer and executed in M.I.L.O.
- Ship small example applications that can be rebuilt inside the OS. A full C
  toolchain and unrestricted native kernel code are explicitly not the first
  target because they would be much larger and less recoverable.

### V0.34 — optional image interchange and richer physical state

- Measure uncompressed BMP interchange and read-only baseline JPEG import as
  optional modules; do not consume the fixed kernel region merely to claim a
  file extension.
- Add ring states for idle, activity, success, warning, and M.I.L.O interaction.
- Extend sound only when the selected hardware provides a small, auditable path.

### Hardware and V1.0

- Boot and run on the selected low-power x86 board.
- Validate VGA, keyboard, pointing device, sound, power use, and physical floppy
  reading/writing.
- Remove temporary regression documents and replace them with intentional local
  help and canon material.
- V1.0 requires a reliable graphical shell, terminal, file browser, editor,
  deterministic M.I.L.O core, removable FAT storage, and recoverable errors.

## Commit rule

Every commit on `M.I.L.O` must add or update an entry in the journal below that
states both **what changed** and **why it was necessary**. The tracked pre-commit
guard rejects a M.I.L.O commit when `direction.md` is not part of the staged
change. Install the guard in a fresh clone with `make milo-install-hooks`.

Hash values are not stored for the commit being created because changing this
file changes that hash. Git remains the authoritative source for hashes; this
journal records intent and rationale.

## Commit journal

| Change | What was added | Why |
| --- | --- | --- |
| `add M.I.L.O BIOS boot sector` | Independent 512-byte BIOS entry point. | Remove reliance on the inherited GRUB path. |
| `load M.I.L.O stage 2 from floppy` | Multi-stage boot loading. | Give the custom boot path room for hardware setup. |
| `version M.I.L.O floppy images` | Versioned image filenames. | Prevent stale-image confusion during QEMU testing. |
| `detect usable memory in M.I.L.O loader` | BIOS memory-map discovery. | Establish a hardware-aware runtime foundation. |
| `enter 32-bit protected mode in M.I.L.O loader` | GDT and protected-mode kernel transfer. | Move beyond a 16-bit boot demonstration. |
| `render M.I.L.O graphical boot splash` | VBE splash using the supplied ASCII artwork. | Establish M.I.L.O visual identity immediately at boot. |
| `place M.I.L.O boot status below splash` | Bottom-left initialization reporting. | Keep diagnostics visible without covering the splash. |
| `load first dedicated M.I.L.O kernel` | Separate kernel image and loader contract. | Turn the boot path into an extensible operating system. |
| `add experimental custom BIOS boot path` | Preserved the generic boot experiment upstream. | Keep generally useful loader work separate from branding. |
| `add interactive PS2 keyboard input` | Direct keyboard polling and character decoding. | Enable interaction without an external runtime. |
| `add deterministic M.I.L.O command terminal` | Local commands and framebuffer terminal. | Create the first useful operator interface. |
| `add scrolling M.I.L.O terminal history` | Terminal scrolling and recalled commands. | Make repeated testing and ordinary use practical. |
| `add FAT12 directory and file reading` | `dir` and `type` against real floppy metadata. | Move from compiled demonstrations to stored documents. |
| `expand M.I.L.O command history to thirty entries` | Larger bounded history. | Improve usability at a small fixed memory cost. |
| `add persistent FAT12 text file writing` | File creation and sector persistence. | Allow the system to produce useful durable work. |
| `add first M.I.L.O text editor` | Interactive file editing and save/cancel. | Make the machine useful for its primary text role. |
| `fix editor backspace and save length` | Corrected deletion and saved-size handling. | Prevent lost or blank document content. |
| `preserve editor length across cursor redraw` | Stable editor state during rendering. | Stop display work from corrupting file state. |
| `add cursor navigation and in-place editing` | Four-way movement and editing within lines. | Support real document changes rather than append-only input. |
| `fix editor horizontal and column navigation` | Corrected movement across complete lines. | Match visible cursor position to edited content. |
| `keep editor cursor synchronized with viewport` | Stable vertical cursor rendering. | Eliminate intermittent edits on the wrong visible line. |
| `expand M.I.L.O kernel region to 16 KiB` | Larger controlled kernel reservation. | Make room for the reliable file-management milestone. |
| `add persistent FAT12 file management` | Delete, rename, and copy. | Complete the basic document lifecycle. |
| `fix FAT12 copy cluster allocation` | Correct independent destination chains. | Prevent copies from losing or sharing content incorrectly. |
| `add temporary FAT12 test documents` | Disposable workflow fixtures. | Make runtime verification repeatable before canon content. |
| `add multi-cluster M.I.L.O document system` | Chained file reading, writing, editing, and searching. | Remove the one-sector document limitation. |
| `add deterministic M.I.L.O writing and trait system` | Persistent structural traits and response packs. | Begin the offline personality without an LLM. |
| `add Nyx-inspired deterministic M.I.L.O routing` | Intent-first natural requests and bounded typo handling. | Make M.I.L.O flexible without storing phrases. |
| `fix V0.20 runtime display and find regressions` | Register preservation, sizes, traits, and unified find parsing. | Restore correctness after the larger routing change. |
| `clarify rejected file-changing requests` | Explicit non-execution feedback for unsafe near-misses. | Ensure M.I.L.O never implies a destructive action occurred. |
| `add full-volume M.I.L.O FAT12 access` | Complete-volume reads, allocation, verified writes, and far-cluster tests. | Replace the original 48 KiB data-window limitation with genuine floppy access. |
| `establish authoritative M.I.L.O direction` | This scope, roadmap, decision record, journal, and commit guard. | Prevent future work or parallel chat sessions from drifting away from the agreed system. |
| `add combined V0.23 graphical shell` | Kernel-owned Home and Traits views, PS/2 mouse input, saved-under cursor, paged FAT12 browser, real open/edit/copy/rename/delete controls, shell documentation, and build contracts. | Deliver the planned V0.22 and V0.23 interface as one coherent testable state while preserving the terminal, deterministic core, verified storage paths, and tiny offline architecture. |
| `add V0.24 compact desktop shell` | Corrected PS/2 edge movement and reorganized the shell into desktop shortcuts, a framed active window, working hide/restore controls, a dedicated terminal surface, and a bottom taskbar based on the generic branch's desktop conventions. | Make M.I.L.O feel like a familiar operating system while retaining its tiny direct kernel architecture and every verified file and personality path. |
| `add V0.25 classic window manager` | Replaced the fixed dashboard with four overlapping kernel-owned windows, focus/z-order, outline dragging, minimize/maximize/close controls, beveled frames, a stateful taskbar, a M.I.L.O application menu, translated application rendering and hit-testing, and a persistent logical terminal surface. | Meet the required Windows 3.x/classic Mac/Fluxbox-style interaction model while keeping the direct offline kernel small and retaining every verified file, editor, and deterministic personality path. |
| `add V0.26 native root-menu desktop` | Removed the permanent dashboard chrome, hid all applications at startup, added a compact edge-clamped right-click root menu, expanded maximize and drag bounds to the whole framebuffer, and retained the four native stacking application windows and their real operations. | Match Fluxbox's sparse visual language and menu-led usability without importing Fluxbox, X11, Linux, POSIX, or an external window system. |
| `polish V0.26.1 native desktop` | Moved cursor redraw to complete PS/2 packets, added root-menu hover and focus accents, and introduced a native right-side system overlay with measured RAM, FAT12 capacity, file, event, video, network, version, CPU-mode, and honest temperature-availability data. | Remove the runtime cursor flicker and give the accepted Fluxbox-like workflow the useful, coherent finish requested without adding fake telemetry or external desktop code. |
| `fix V0.26.2 UI and add native FileHound` | Separated GUI text and number drawing from Terminal wrapping, brought every browser row into the translated window coordinate contract, and rebuilt Files as a native single-pane FileHound edition with a location strip, aligned Name/Type/Size columns, category accents, paging, item status, classic controls, and isolated repaint storage. | Correct the runtime layout failures at their shared rendering sources and make the primary document workflow polished and recognisably FileHound without importing Go, Win32, Linux, or nonfunctional controls. |
| `add V0.27 live desktop status` | Silenced normal loader chatter until the persistent boot splash, added a CMOS-backed Australian clock, a right-aligned version/date/time block, a minimized-only restore taskbar, taskbar-aware workspace bounds, real application-state reporting, a rolling event-activity graph with one-second live recomposition, and a primary-branch M.I.L.O build/readme contract. | Make the native Fluxbox-like desktop more familiar and useful while keeping telemetry honest, the root workflow sparse, boot unmistakably M.I.L.O, and the implementation independent of Linux, Conky, an RTC service, a scheduler, or fake CPU statistics. |
| `refine V0.27.1 splash and window resizing` | Added an RTC-backed minimum splash hold, a readable 0--120+ events/second graph, bounded bottom-right resize grips, client clipping, adaptive System/FileHound/Traits layouts, and a size-aware Terminal viewport. | Runtime proof showed that fast hardware hid the splash and ordinary input saturated the first graph scale; native resizing completes the expected classic window workflow without fake CPU telemetry or drawing outside application frames. |
| `add V0.28 native Writer workspace` | Corrected clipped glyph emission, added a fifth resizable Writer window, routed FileHound Edit into it, added viewport scrolling and mouse caret placement, and connected Save, validated Save As, status feedback, and guarded close to the verified FAT12 path. | Runtime resizing exposed coordinate bytes being drawn as text, while useful document work required a native application rather than a terminal-shaped editor mode. |
| `stabilize V0.28.1 Writer repaint` | Replaced full desktop recomposition on every Writer edit with a terminal-state-safe repaint of the frontmost Writer window, while retaining full composition for genuine desktop transitions. | Runtime typing showed the root and lower windows flickering through during back-to-front full-frame redraws; focused repaint removes that visible intermediate state without adding a large compositor or backbuffer. |
| `polish V0.28.2 Writer and composition` | Added atomic RAM-backed full-frame presentation, repaired and exposed Writer's empty 8.3 name entry, and added persistent inline Bold/Italic/Underline keyboard and toolbar controls with styled rendering and plain Terminal output. | Runtime verification showed timed redraw flicker and a Save As field whose 12-byte default left no input capacity; useful offline document work also needed minimal formatting without importing a large document model. |
| `add V0.28.3 Writer selection and layout` | Added automatic `.TXT` normalization, visible keyboard/mouse selection, selection-aware styles, persistent Left/Center/Right/Justified paragraph layout, and RAM-backed Writer-region presentation. | Runtime use showed extensionless Save As failure, no post-entry styling or alignment, and minor incremental redraw flicker; these changes make Writer useful without importing a document framework. |
| `stabilize V0.28.4 pointer and paragraph flow` | Staged PS/2 coordinates before cursor erasure, redrew inside the packet handler, skipped same-cell Writer drag repaint, and made Enter persist the active paragraph alignment on the next line. | Runtime verification showed severe residual pointer flicker and alignment reverting to left after a paragraph break; both faults were in state-transition timing rather than the visible controls. |
| `add V0.29 keyboard shell and Pixel Viewer` | Added a no-blank fast-pointer handoff, corrected Writer drag anchoring, exposed the application menu and FileHound through the keyboard, and introduced a sixth resizable M16 V1 image-viewer window with a separate 8 KiB buffer, palette inspection, validation, errors, and a generated 96x64 fixture. | Complete the requested keyboard-first workflow, resolve the two remaining input defects, and begin the planned low-resolution image workstation without importing an image framework or risking open document data. |
| `complete V0.29.1 keyboard window control` | Added standard `Alt+F9` minimize and `Alt+F4` close shortcuts for the focused application, retained Writer's dirty-document guard, and reordered the roadmap around native image editing, Mavica-oriented interchange, deterministic M.I.L.O/terminal expansion, and in-system application development. | Finish genuine keyboard-only desktop control and record the newly agreed priorities before the image editor begins. |
| `combine V0.31 image and deterministic milestones` | Merged image interchange/Mavica import with deeper deterministic M.I.L.O and Terminal expansion as V0.31, then renumbered in-system development to V0.32 and sound/state work to V0.33. | Deliver the post-editor image and intelligence work as one coherent release using the same file, application, command, and error paths. |
| `add V0.30 native Pixel Studio` | Replaced the read-only M16 viewer with a keyboard-complete native pixel editor providing Pencil, Eraser, Picker, bounded Fill, Line, Rectangle, palette and zoom controls, a visible pixel caret, one full-image undo, fixed-size New, verified Save/Save As with automatic `.M16`, and dirty-close protection. | Make low-resolution image work genuinely useful while editing the compact packed format directly, retaining strict fixed-memory limits, and reusing the already verified FAT12 persistence path. |
| `fix V0.30.1 drawing gates and canvas creation` | Corrected four one-byte Pixel Studio state checks that had accidentally included adjacent state, then added a modal keyboard-and-mouse New Canvas picker for 32x32, 64x48, 96x64, and 128x96 M16 images. | Runtime testing showed every drawing tool was blocked despite a valid image, while useful image work needed deliberate canvas sizing rather than a single hard-coded shape. |
| `optimize V0.30.2 continuous pixel drawing` | Replaced per-sample whole-desktop composition with direct dirty-pixel presentation in the visible and backing frames, limited stroke-boundary refreshes to Pixel Studio, and interpolated Pencil/Eraser paths between PS/2 samples. | Runtime testing confirmed correctness but exposed extreme input latency and sparse cursor tracking; continuous drawing must be responsive before image interchange work begins. |
| `add V0.31 text-first interaction and native sound` | Restored Rectangle's full visible tool hitbox, reorganized Terminal help, added local status/clock/application discovery, Writer launch and command aliases, introduced deterministic social token categories with bounded typo matching, and added PIT-timed PC-speaker boot, M.I.L.O, Writer-save, mute, and test cues. | Pixel Studio is now adequate as a supporting tool; the appliance's primary value is offline writing and transparent M.I.L.O interaction, which need the remaining kernel budget more than additional image formats. |
