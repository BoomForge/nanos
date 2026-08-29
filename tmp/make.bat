@echo off

cd %~dp0
fasm boot.asm boot.bin
bochs