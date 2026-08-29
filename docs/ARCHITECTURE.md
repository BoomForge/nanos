# NanOS architecture

NanOS has a small kernel image plus a ramdisk with `.nx` userspace programs. The
source tree should keep the boundaries we want for a MenuetOS-like system:
direct app syscalls, path-based app loading, and event-driven graphical programs.

## Layers

### Kernel/core

Core code owns hardware and low-level services:

- CPU setup, interrupts, timers, paging, memory, heap, and panic handling.
- Input drivers and architecture-specific device boundaries.
- Framebuffer access and primitive drawing.
- Shared low-level types such as rectangles.

Core code should not know about widgets, application layout, or specific apps.

### GUI server and window manager

GUI system code owns desktop-wide behavior:

- Windows, compositor, dirty rectangles, focus, cursor policy, and event routing.
- The desktop manager and shell panel.
- Window manager actions such as moving, resizing, minimizing, maximizing, and closing.

This layer may call primitive drawing and route events to apps. It should not
contain application-specific UI or state.

### App-side UI/toolkit code

Reusable controls belong on the application side:

- Buttons, labels, textboxes, separators, layout helpers, and future controls.
- Helpers that apps use to draw and hit-test their own client areas.

This code should live in userspace libraries once NanOS has a C app toolchain.
The kernel should expose primitive drawing, window, file, launch, and event
syscalls, not app-specific widgets.

### Apps

Apps own their own behavior and client-area UI:

- `.nx` programs loaded from the ramdisk or filesystem.
- App event handling and app-local state.
- Calls into the UI toolkit and window APIs.

Apps should not reach into desktop manager internals.

## Current source map

- `arch/`: architecture-specific CPU and device code.
- `kernel/src`: core kernel services and the small in-kernel GUI/window manager.
- `kernel/src/process`: process slots, event queues, and scheduler glue.
- `kernel/src/syscall`: Menuet-style app ABI handlers.
- `kernel/src/gui`: GUI primitives such as windows, compositor, and font drawing.
- `kernel/src/desktop`: desktop/window-manager policy.
- `kernel/src/shell`: shell panel.
- `kernel/src/debug`: fallback monitor, debug process inspection, and optional self-tests.
- `userspace`: current `.nx` userspace programs.
- `abi/include/nanos`: headers shared with userspace programs.

The default boot policy lives in `kernel/src/init.c`. It starts the first
userspace app by path; it should stay policy-level code, not part of the process
or loader internals.
