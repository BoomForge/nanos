# NanOS MenuetOS-Parity Tracker

Goal: keep NanOS close to the MenuetOS model: small direct app syscalls,
event-driven graphical programs, kernel-owned windowing/redraw rules, and a
plain implementation in modular C89.

## Current Foundation

- [x] Ring 3 `.nx` apps loaded from initrd.
- [x] Direct syscall ABI for apps.
- [x] Event loop model: wait, redraw, key, mouse, button, exit, IPC.
- [x] Window creation/redraw through app syscalls.
- [x] App launch syscall.
- [x] Fixed-size IPC message syscall.
- [x] Window manager owns focus, stacking, moving, minimize/maximize, taskbar.
- [x] App drawing syscalls are clipped to the visible parts of the owning
  window.

## Biggest Gaps

1. Drawing API breadth
   - [x] Add `PUT_IMAGE`-style bitmap blit with visible-region clipping.
   - [x] Add pixel/number/screen-size helpers where they simplify apps.
   - [x] Keep every primitive inside the same visibility rules.

2. App image and memory model
   - [x] Extend `.nx` header with requested memory size.
   - [x] Add explicit stack size / stack pointer metadata.
   - [x] Add optional icon pointer metadata.
   - [ ] Add app memory resize syscall later.

3. Window redraw architecture
   - [x] Define the rule: app drawing is clipped by window visibility.
   - [x] Move visible-region logic out of syscall handlers into `gui`.
   - [x] Make expose/redraw behavior use one shared helper.
   - [x] Keep redraw events coalesced and predictable.

4. IPC completeness
   - [x] Add basic fixed-size app-to-app IPC.
   - [x] Add sender discovery / app identity helpers.
   - [x] Consider simple service naming or broadcast later.
   - [x] Improve receive semantics without turning it into POSIX pipes.

5. Process/thread API
   - [x] Start app by path.
   - [x] Add app-created threads.
   - [x] Add process info syscall.
   - [x] Add clean app termination/kill syscall.

6. Filesystem
   - [x] Read-only initrd VFS.
   - [x] Path-oriented app file syscall.
   - [ ] Add write/create/delete when the storage layer supports it.
   - [ ] Add real disk driver path after the RAM disk path is stable.

7. Input model
   - [x] Basic keyboard character events.
   - [x] Basic mouse events.
   - [x] Add scancodes/special keys.
   - [x] Add modifier key state.
   - [x] Add richer mouse button/wheel state.

8. GUI usability
   - [x] Basic windows, buttons, text, rectangles, lines.
   - [x] Add standard app-side helpers for text fields, lists, scrollbars, and
     menus.
   - [x] Add simple dialogs.
   - [x] Keep widgets app-side unless kernel support is clearly needed.

## Next Work

1. Add app memory resize syscall later.
