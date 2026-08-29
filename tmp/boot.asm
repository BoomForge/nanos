
; Simple boot sector

; BIOS loads the boot sector to address 0x7c00
org 0x7c00

mov bx, boot_msg
call puts
mov bx, goodbye_msg
call puts

jmp $							; jump forever


include "puts.inc"
; data
boot_msg:
	db "Booting NanOS...", 0xA, 0xD, 0
	
goodbye_msg:
	db "Good bye...", 0
	


; Fill rest with 0 bytes (Bood sector has to be exactly 512 bytes)
times 510-($-$$) db 0

; BIOS magic number (tell BIOS this is a boot sector)
dw 0xaa55