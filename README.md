# NanOS

NanOS is the start of a tiny graphical x86 operating system in modular C89.
The current tree builds a 32-bit Multiboot2 kernel, boots it with GRUB, writes
diagnostics to serial/VGA text, and draws an initial framebuffer desktop.

The design goal is closer to MenuetOS' spirit than its implementation: compact,
graphical, fast to boot, and useful. It is not a FASM port and should not copy
MenuetOS internals.

## Requirements

- `gcc`
- GNU `ld`
- `grub-mkstandalone`
- `xorriso`
- `qemu-system-i386` for the default run target
- Optional: `bochs` for CPU-level emulator debugging

## Build

```sh
make
make verify
make iso
```

## Run

```sh
make run
```

This boots `build/nanos-i386.iso` in QEMU and connects COM1 to the terminal.
The serial output should end with:

```text
graphical desktop initialized
memory total ...
pmm test page ...
paging enabled
heap start ...
heap free test ok
interrupts enabled
halt loop entered
timer ticking
```

The graphical window now has a tiny framebuffer console. Click/focus QEMU and
type; PS/2 keyboard input should echo into that console.
Moving the mouse in QEMU should move the software cursor.

Bochs support is prepared but depends on Bochs being installed:

```sh
make bochs
```

## Current architecture

- `boot/`: Multiboot2 entry code and linker scripts.
- `kernel/`: C89 kernel code that should remain mostly architecture neutral.
- `arch/x86/`: 32-bit x86 port I/O, serial, and VGA text support.
- `grub/`: bootloader configuration.
- `docs/`: roadmap and design notes.

## Status

`ARCH=i386` is bootable. `ARCH=x86_64` is intentionally reserved for the next
stage after memory management and interrupt ownership are in place.
