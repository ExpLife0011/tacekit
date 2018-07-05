; See override.h
; © 2018 fereh

IFDEF rax
ELSE
.model flat, C
ENDIF

.data

IFDEF rax
	gRetValue QWORD ?
	gRetAddr QWORD ?
ELSE
	gRetValue DWORD ?
	gRetAddr DWORD ?
ENDIF


.code

IFDEF rax
	OverrideReturn PROC
		mov [gRetValue], rcx
		mov rcx, [rdx]
		mov [gRetAddr], rcx
		mov rcx, _OverrideReturn
		mov QWORD PTR [rdx], rcx
		ret
	OverrideReturn ENDP
ELSE
	OverrideReturn PROC
		push ebp
		mov ebp, esp
		mov ecx, [ebp + 8]
		mov [gRetValue], ecx
		mov edx, [ebp + 12]
		mov ecx, [edx]
		mov [gRetAddr], ecx
		mov ecx, _OverrideReturn
		mov DWORD PTR [edx], ecx
		pop ebp
		ret
	OverrideReturn ENDP
ENDIF

	_OverrideReturn PROC
		mov eax, [gRetValue]
		jmp [gRetAddr]
	_OverrideReturn ENDP

PUBLIC OverrideReturn
END
