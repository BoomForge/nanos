
; Prints a string given at address bx

puts:
	pusha
	mov ah, 0x0e
	jmp putc

putc:
	mov al, [bx]
	cmp al, 0
	je endputs
	int 0x10
	add bx, 1
	jmp putc
	
endputs:
	popa
	ret
	
	
	
	
	
	
	

	