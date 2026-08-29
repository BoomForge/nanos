# NanOS
NanOS is a project for learning os development (32 bit protected mode) from scratch for the intel x86 architecture

## Design Principles
- Everything is written from scratch in assembly (fasm) and C.
- Writing the boot sector, boot loader and kernel by hand, so that there is no magic going on behind the scenes
- Switching from 16 bit real mode to 32 bit protected mode
- defining the global descriptor table and a filesystem (probably FAT16 and later FAT32)

## Goal
- Goal is to learn how an OS really works under the hood right from the start when BIOS kicks in and loads the boot sector.

## Disclaimer
- NanOS is not ment to be a real OS for everyday use, instead it is for learning and educational purposes.
- Do not run it on real hardware! I don't take any responsibilties when you damage your hardware or on data loss
- I'd suggest to run it in the bochs emulator (a config file *.bochsrc*) is already set up.
