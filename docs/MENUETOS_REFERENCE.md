# MenuetOS reference notes

MenuetOS is checked out locally at:

`/home/micha/DEV/projects/menuetos-reference`

Use it as an architectural reference, not as code to copy. NanOS should keep its
implementation in clean, modular C89 with small assembly boundaries.

## What to borrow

MenuetOS is simple because applications talk to a small direct OS ABI. A normal
GUI application has:

- A tiny binary header.
- A fixed entry point, requested memory size, stack pointer, optional parameters,
  and optional icon pointer.
- An event loop that waits for one OS event, handles it, and waits again.

The basic application shape is visible in:

- `menuetos-app-sources-32b/EXAMPLE.ASM`
- `menuetos-app-sources-32b/MACROS.INC`

The core event pattern is:

- draw the window once
- wait for an event
- handle redraw, key, button, IPC, mouse, or exit
- redraw only when the OS asks for redraw

This is the philosophy NanOS should follow for first-class graphical programs.

NanOS `.nx` headers now carry the same kind of direct process metadata:
entry offset, file image size, requested mapped memory size, stack size, initial
stack pointer, and an optional icon offset.

## System call style

MenuetOS uses direct system calls through `int 0x40`. The syscall table in
`menuetos-kernel-sources-32b/SYS32.INC` shows the important shape:

- `0`: draw or define a window
- `1`: put pixel
- `2`: get key
- `4`: write text
- `8`: define/draw button
- `9`: get process information
- `10`: wait for event
- `11`: check for event
- `12`: begin/end window draw
- `14`: get screen size
- `17`: get button event
- `18`: system/app control, including terminate
- `19`: start application
- `23`: wait for event with timeout
- `40`: set wanted event mask
- `47`: write number
- `51`: create thread
- `58`: common filesystem interface
- `60`: IPC
- `64`: resize application memory

NanOS does not need these exact numbers, but the ABI should have the same plain
shape: small structs, explicit calls, and no hidden framework requirement.
The current NanOS application contract is documented in `docs/APP_ABI.md`.

NanOS now keeps the first event-facing numbers aligned with this shape:

- `1`: `NANOS_SYSCALL_PUT_PIXEL`
- `7`: `NANOS_SYSCALL_PUT_IMAGE`
- `8`: `NANOS_SYSCALL_DEFINE_BUTTON`
- `9`: `NANOS_SYSCALL_PROCESS_INFO`
- `10`: `NANOS_SYSCALL_WAIT_EVENT`
- `11`: `NANOS_SYSCALL_POLL_EVENT`
- `12`: `NANOS_SYSCALL_WINDOW_DRAW`
- `14`: `NANOS_SYSCALL_GET_SCREEN_SIZE`
- `18`: `NANOS_SYSCALL_APP_CONTROL`
- `19`: `NANOS_SYSCALL_START_APP`
- `23`: `NANOS_SYSCALL_WAIT_EVENT_TIMEOUT`
- `38`: `NANOS_SYSCALL_DRAW_LINE`
- `40`: `NANOS_SYSCALL_SET_EVENT_MASK`
- `47`: `NANOS_SYSCALL_WRITE_NUMBER`
- `51`: `NANOS_SYSCALL_THREAD`
- `58`: `NANOS_SYSCALL_FILE`
- `60`: `NANOS_SYSCALL_IPC`

The earlier fd, cwd, and process-control syscalls have been removed from the
app ABI. Kernel-only monitor commands may still use VFS/process internals for
bring-up debugging, but applications should use the Menuet-style calls above.

## Files and launching

MenuetOS does not center its application model on POSIX `fork`, pipes, terminals,
or inherited file descriptors. It uses path-oriented OS services and explicit
start-application calls.

Good NanOS direction:

- Keep the kernel monitor as a bring-up/debug tool.
- Do not deepen POSIX compatibility unless it serves a concrete NanOS app.
- Prefer `NANOS_SYSCALL_FILE` for app-facing file operations.
- Prefer `NANOS_SYSCALL_START_APP` for launching another app by path with one
  optional parameter string.
- Keep path handling explicit; do not make a Unix shell the primary user
  interface.

## Windowing model

MenuetOS applications draw their own windows and controls through kernel-provided
GUI syscalls. The OS owns event routing, redraw requests, focus, buttons, and
window movement.

MenuetOS keeps drawing behind the syscall boundary: applications wrap redraws
with syscall `12`, define/draw the window with syscall `0`, and then use kernel
drawing calls for text, buttons, images, and rectangles. The kernel tracks the
window stack/window map and screen redraw state, so drawing is constrained by
the app's window visibility instead of letting background apps paint over
foreground windows.

Good NanOS direction:

- Keep drawing primitives and framebuffer ownership in the kernel/core.
- Keep window ownership in userspace apps through simple window/event syscalls.
- Make apps responsible for their client-area UI.
- Let the OS provide window frame, redraw events, button ids, keyboard/mouse
  events, and application launch.
- Clip app drawing syscalls to the currently visible parts of the owning window.
- Keep the previous modular split in source code so this can later move behind a
  server boundary if needed.

## Near-term NanOS plan

1. Keep removed POSIX-like app syscalls out of the ABI.
2. Keep `docs/APP_ABI.md`, `abi/include/nanos/app.h`, and
   `userspace/guidemo.S` as the first app ABI contract and reference app.
3. Keep the first graphical experience in userspace `.nx` programs, not
   kernel-resident demos.
4. Route real mouse, button, close, move, resize, and redraw events through the
   process event queue.
5. Extend minimal GUI syscalls with more control drawing primitives as needed.
6. Keep one tiny graphical userspace app as the reference ABI example.

This keeps NanOS close to MenuetOS philosophy: direct, small, understandable, and
usable without building a full Unix compatibility layer first.
