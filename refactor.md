# Kernel Refactor Tracker

Goal: keep the kernel structure aligned with a small MenuetOS-like system:
direct app syscalls, path-based app loading, event-driven ring 3 programs, and
debug tooling that does not shape normal kernel APIs.

## Completed Passes

- [x] Remove stale kernel UI/app scaffolding.
- [x] Remove current-directory launch handling from the process layer.
- [x] Move process inspection/reap helpers out of the normal process API.
- [x] Keep debug monitor behavior working through debug-only APIs.
- [x] Build, verify, and smoke boot.

## Keep

- `block`, `ramdisk`, `tarfs`, `vfs`: needed for initrd/file loading.
- `nx_loader`: needed for `.nx` image loading.
- `process`: needed for ring 3 apps, event queues, and scheduling.
- `syscall/*`: the Menuet-style app syscall surface.
- `desktop`, `gui`, `shell/panel`, `cursor`, `framebuffer`: GUI server/window
  manager path.
- `debug/monitor`: fallback/debug tool only.

## Later Passes

- [x] Rename `exec` to `nx_loader`.
- [x] Move boot self-tests from `kmain.c` into debug self-test code.
- [x] Move process exit desktop cleanup behind a lifecycle hook.
- [x] Tighten the arch/process boundary for future ARM/RISC-V ports.

## Current Pass

- [x] Split the debug monitor into smaller modules.
- [x] Move debug process helpers out of `process.c`.
- [x] Split process responsibilities into core/events/scheduler.
- [x] Gate boot self-tests behind a build flag.
- [x] Simplify VFS directory listing to one API.
- [x] Move first-app boot launch behind init policy.
- [x] Remove legacy `&` to `/` tarfs path mapping.
- [x] Refresh stale docs.
