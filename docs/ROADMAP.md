# NanOS roadmap

NanOS is intended to be a small graphical x86 operating system written in clean,
modular C89, with assembly limited to CPU entry, context switching, interrupt
stubs, and tiny hardware boundaries where C cannot express the operation.

MenuetOS is a useful inspiration for constraints and user experience: tiny boot
media, a fast graphical desktop, local applications, and direct hardware support.
NanOS should not copy MenuetOS internals or its FASM codebase.

See `docs/MENUETOS_REFERENCE.md` for the concrete MenuetOS patterns NanOS should
borrow: direct app syscalls, explicit event loops, path-oriented app launching,
and kernel-owned window/event routing.

See `docs/APP_ABI.md` for the current `.nx` application format, syscall calling
convention, and graphical app redraw contract.

## Milestone 0: bootable visual kernel

- Multiboot2 boot through GRUB.
- 32-bit protected-mode C kernel entry.
- Serial diagnostics.
- VGA text fallback.
- Linear framebuffer setup and a first graphical desktop surface.
- QEMU smoke test.
- Bochs configuration for CPU-level debugging.

## Milestone 1: kernel foundations

- GDT and IDT owned by NanOS instead of bootloader defaults.
- PIC interrupt setup.
- PIT timer and PS/2 keyboard input.
- PS/2 mouse input and software cursor.
- Framebuffer text renderer.
- Physical page allocator from the boot memory map.
- Early identity paging.
- Page-backed kernel heap with free/coalescing.
- Higher-half kernel.
- Panic screen and structured serial logging.

## Milestone 2: interaction

- Window input routing.
- Event queue.
- Basic 2D compositor with dirty rectangles.
- Bitmap font renderer.
- Window manager primitives: move, focus, close, resize.

## Milestone 3: processes

- Preemptive scheduler.
- Ring 3 user mode.
- System call ABI.
- ELF loader.
- Per-process address spaces.
- IPC primitives for GUI services.

## Milestone 4: storage and applications

- RAM disk for early userspace.
- FAT12/FAT32 read/write.
- Shell, file manager, text editor, terminal, and settings app.
- Package/application format.

## Milestone 5: hardware and daily-driver path

- VESA/GOP graphics path, then native GPU targets where practical.
- AHCI/NVMe storage.
- USB HID and mass storage.
- Intel e1000/rtl8139 networking first, then broader NIC support.
- TCP/IP stack.
- ACPI power, timers, SMP, and laptop basics.

## 64-bit plan

The first bootable target is `ARCH=i386` because it gives the shortest path to a
visible kernel and useful debugging. The source layout keeps architecture code
separate so an `x86_64` path can be added after paging, memory discovery, and
interrupt handling are solid.

The 64-bit path should enter through a separate loader stub, establish long mode,
reuse architecture-neutral kernel services, and keep ABI-specific code under
`arch/x86_64/`.
