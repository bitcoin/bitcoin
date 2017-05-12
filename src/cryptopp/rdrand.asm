;; rdrand.asm - written and placed in public domain by Jeffrey Walton and Uri Blumenthal.
;;              Copyright assigned to the Crypto++ project.

;; This ASM file provides RDRAND and RDSEED to downlevel Microsoft tool chains.
;; Everything "just works" under Visual Studio. Other platforms will have to
;; run MASM/MASM-64 and then link to the object files.

;; set ASFLAGS=/nologo /D_M_X86 /W3 /Cx /Zi /safeseh
;; set ASFLAGS64=/nologo /D_M_X64 /W3 /Cx /Zi
;; "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\ml.exe" %ASFLAGS% /Fo rdrand-x86.obj /c rdrand.asm
;; "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\amd64\ml64.exe" %ASFLAGS64% /Fo rdrand-x64.obj /c rdrand.asm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TITLE    MASM_RRA_GenerateBlock and MASM_RSA_GenerateBlock
SUBTITLE Microsoft specific ASM code to utilize RDRAND and RDSEED for down level Microsoft toolchains

PUBLIC MASM_RRA_GenerateBlock
PUBLIC MASM_RSA_GenerateBlock

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Naming convention used in rdrand.{h|cpp|asm}
;;   MSC = Microsoft Compiler (and compatibles)
;;   GCC = GNU Compiler (and compatibles)
;;   ALL = MSC and GCC (and compatibles)
;;   RRA = RDRAND, Assembly
;;   RSA = RDSEED, Assembly
;;   RRI = RDRAND, Intrinsic
;;   RSA = RDSEED, Intrinsic

;;  Caller/Callee Saved Registers
;;    https://msdn.microsoft.com/en-us/library/6t169e9c.aspx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; C/C++ Function prototypes
;;   X86:
;;      extern "C" int MASM_RRA_GenerateBlock(byte* ptr, size_t size, unsigned int safety);
;;   X64:
;;      extern "C" int __fastcall MASM_RRA_GenerateBlock(byte* ptr, size_t size, unsigned int safety);

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Return values
RDRAND_SUCCESS EQU 1
RDRAND_FAILURE EQU 0

RDSEED_SUCCESS EQU 1
RDSEED_FAILURE EQU 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X86    ;; Set via the command line

.486
.MODEL FLAT

ENDIF

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X86    ;; Set via the command line

.CODE
ALIGN   8
OPTION LANGUAGE:C
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

;; Base relative (in): arg1, byte* buffer
;; Base relative (in): arg2, size_t bsize
;; Base relative (in): arg3, unsigned int safety
;; EAX (out): success (1), failure (0)

MASM_RRA_GenerateBlock PROC arg1:DWORD,arg2:DWORD,arg3:DWORD

	MWSIZE EQU 04h    ;; machine word size
	buffer EQU edi
	bsize  EQU edx
	safety EQU ecx

Load_Arguments:

	mov		buffer, arg1
	mov		bsize,  arg2
	mov		safety, arg3

Validate_Pointer:

	cmp 	buffer, 0
	je 		GenerateBlock_PreRet

			;; Top of While loop
GenerateBlock_Top:

			;; Check remaining size
	cmp 	bsize, 0
	je		GenerateBlock_Success

Call_RDRAND_EAX:
			;; RDRAND is not available prior to VS2012. Just emit
			;;   the byte codes using DB. This is `rdrand eax`.
	DB		0Fh, 0C7h, 0F0h

			;; If CF=1, the number returned by RDRAND is valid.
			;; If CF=0, a random number was not available.
	jc 		RDRAND_succeeded

RDRAND_failed:

			;; Exit if we've reached the limit
	cmp		safety, 0
	je		GenerateBlock_Failure

	dec 	safety
	jmp		GenerateBlock_Top

RDRAND_succeeded:

	cmp		bsize, MWSIZE
	jb		Partial_Machine_Word

Full_Machine_Word:

	mov		DWORD PTR [buffer], eax
	add		buffer, MWSIZE		;; No need for Intel Core 2 slow workarounds, like
	sub		bsize, MWSIZE		;;   `lea buffer,[buffer+MWSIZE]` for faster adds

			;; Continue
	jmp		GenerateBlock_Top

			;; 1,2,3 bytes remain
Partial_Machine_Word:

			;; Test bit 1 to see if size is at least 2
	test	bsize, 2
	jz		Bit_1_Not_Set

	mov		WORD PTR [buffer], ax
	shr		eax, 16
	add 	buffer, 2

Bit_1_Not_Set:

			;; Test bit 0 to see if size is at least 1
	test	bsize, 1
	jz  	GenerateBlock_Success

	mov		BYTE PTR [buffer], al

Bit_0_Not_Set:

			;; We've hit all the bits
	jmp 	GenerateBlock_Success

GenerateBlock_PreRet:

			;; Test for success (was the request completely fulfilled?)
	cmp 	bsize, 0
	je 		GenerateBlock_Success

GenerateBlock_Failure:

	xor		eax, eax
	mov		al, RDRAND_FAILURE
	ret

GenerateBlock_Success:

	xor		eax, eax
	mov		al, RDRAND_SUCCESS
	ret

MASM_RRA_GenerateBlock ENDP

ENDIF    ;; _M_X86

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X64    ;; Set via the command line

.CODE
ALIGN   16
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

;; RCX (in): arg1, byte* buffer
;; RDX (in): arg2, size_t bsize
;; R8d (in): arg3, unsigned int safety
;; RAX (out): success (1), failure (0)

MASM_RRA_GenerateBlock PROC

	MWSIZE EQU 08h    ;; machine word size
	buffer EQU rcx
	bsize  EQU rdx
	safety EQU r8d

	;; No need for Load_Arguments due to fastcall

Validate_Pointer:

			;; Validate pointer
	cmp 	buffer, 0
	je 		GenerateBlock_PreRet

			;; Top of While loop
GenerateBlock_Top:

			;; Check remaining size
	cmp 	bsize, 0
	je		GenerateBlock_Success

Call_RDRAND_RAX:
			;; RDRAND is not available prior to VS2012. Just emit
			;;   the byte codes using DB. This is `rdrand rax`.
	DB		048h, 0Fh, 0C7h, 0F0h

			;; If CF=1, the number returned by RDRAND is valid.
			;; If CF=0, a random number was not available.
	jc 		RDRAND_succeeded

RDRAND_failed:

			;; Exit if we've reached the limit
	cmp		safety, 0
	je		GenerateBlock_Failure

	dec 	safety
	jmp		GenerateBlock_Top

RDRAND_succeeded:

	cmp		bsize, MWSIZE
	jb		Partial_Machine_Word

Full_Machine_Word:

	mov		QWORD PTR [buffer], rax
	add		buffer, MWSIZE
	sub		bsize, MWSIZE

			;; Continue
	jmp		GenerateBlock_Top

			;; 1,2,3,4,5,6,7 bytes remain
Partial_Machine_Word:

			;; Test bit 2 to see if size is at least 4
	test	bsize, 4
	jz		Bit_2_Not_Set

	mov		DWORD PTR [buffer], eax
	shr		rax, 32
	add		buffer, 4

Bit_2_Not_Set:

			;; Test bit 1 to see if size is at least 2
	test	bsize, 2
	jz		Bit_1_Not_Set

	mov		WORD PTR [buffer], ax
	shr		eax, 16
	add 	buffer, 2

Bit_1_Not_Set:

			;; Test bit 0 to see if size is at least 1
	test	bsize, 1
	jz  	GenerateBlock_Success

	mov		BYTE PTR [buffer], al

Bit_0_Not_Set:

			;; We've hit all the bits
	jmp		GenerateBlock_Success

GenerateBlock_PreRet:

			;; Test for success (was the request completely fulfilled?)
	cmp 	bsize, 0
	je 		GenerateBlock_Success

GenerateBlock_Failure:

	xor		rax, rax
	mov		al, RDRAND_FAILURE
	ret

GenerateBlock_Success:

	xor		rax, rax
	mov		al, RDRAND_SUCCESS
	ret

MASM_RRA_GenerateBlock ENDP

ENDIF    ;; _M_X64

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X86    ;; Set via the command line

.CODE
ALIGN   8
OPTION LANGUAGE:C
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

;; Base relative (in): arg1, byte* buffer
;; Base relative (in): arg2, size_t bsize
;; Base relative (in): arg3, unsigned int safety
;; EAX (out): success (1), failure (0)

MASM_RSA_GenerateBlock PROC arg1:DWORD,arg2:DWORD,arg3:DWORD

	MWSIZE EQU 04h    ;; machine word size
	buffer EQU edi
	bsize  EQU edx
	safety EQU ecx

Load_Arguments:

	mov		buffer, arg1
	mov		bsize,  arg2
	mov		safety, arg3

Validate_Pointer:

	cmp		buffer, 0
	je		GenerateBlock_PreRet

			;; Top of While loop
GenerateBlock_Top:

			;; Check remaining size
	cmp		bsize, 0
	je		GenerateBlock_Success

Call_RDSEED_EAX:
			;; RDSEED is not available prior to VS2012. Just emit
			;;   the byte codes using DB. This is `rdseed eax`.
	DB		0Fh, 0C7h, 0F8h

			;; If CF=1, the number returned by RDSEED is valid.
			;; If CF=0, a random number was not available.
	jc 		RDSEED_succeeded

RDSEED_failed:

			;; Exit if we've reached the limit
	cmp		safety, 0
	je		GenerateBlock_Failure

	dec 	safety
	jmp		GenerateBlock_Top

RDSEED_succeeded:

	cmp		bsize, MWSIZE
	jb		Partial_Machine_Word

Full_Machine_Word:

	mov		DWORD PTR [buffer], eax
	add		buffer, MWSIZE		;; No need for Intel Core 2 slow workarounds, like
	sub		bsize, MWSIZE		;;   `lea buffer,[buffer+MWSIZE]` for faster adds

			;; Continue
	jmp		GenerateBlock_Top

			;; 1,2,3 bytes remain
Partial_Machine_Word:

			;; Test bit 1 to see if size is at least 2
	test	bsize, 2
	jz		Bit_1_Not_Set

	mov		WORD PTR [buffer], ax
	shr		eax, 16
	add 	buffer, 2

Bit_1_Not_Set:

			;; Test bit 0 to see if size is at least 1
	test	bsize, 1
	jz  	GenerateBlock_Success

	mov		BYTE PTR [buffer], al

Bit_0_Not_Set:

			;; We've hit all the bits
	jmp 	GenerateBlock_Success

GenerateBlock_PreRet:

			;; Test for success (was the request completely fulfilled?)
	cmp 	bsize, 0
	je 		GenerateBlock_Success

GenerateBlock_Failure:

	xor		eax, eax
	mov		al, RDSEED_FAILURE
	ret

GenerateBlock_Success:

	xor		eax, eax
	mov		al, RDSEED_SUCCESS
	ret

MASM_RSA_GenerateBlock ENDP

ENDIF    ;; _M_X86

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X64    ;; Set via the command line

.CODE
ALIGN   16
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

;; RCX (in): arg1, byte* buffer
;; RDX (in): arg2, size_t bsize
;; R8d (in): arg3, unsigned int safety
;; RAX (out): success (1), failure (0)

MASM_RSA_GenerateBlock PROC ;; arg1:QWORD,arg2:QWORD,arg3:DWORD

	MWSIZE EQU 08h    ;; machine word size
	buffer EQU rcx
	bsize  EQU rdx
	safety EQU r8d

	;; No need for Load_Arguments due to fastcall

Validate_Pointer:

			;; Validate pointer
	cmp 	buffer, 0
	je 		GenerateBlock_PreRet

			;; Top of While loop
GenerateBlock_Top:

			;; Check remaining size
	cmp 	bsize, 0
	je		GenerateBlock_Success

Call_RDSEED_RAX:
			;; RDSEED is not available prior to VS2012. Just emit
			;;   the byte codes using DB. This is `rdseed rax`.
	DB 048h, 0Fh, 0C7h, 0F8h

			;; If CF=1, the number returned by RDSEED is valid.
			;; If CF=0, a random number was not available.
	jc 		RDSEED_succeeded

RDSEED_failed:

			;; Exit if we've reached the limit
	cmp		safety, 0
	je		GenerateBlock_Failure

	dec 	safety
	jmp		GenerateBlock_Top

RDSEED_succeeded:

	cmp		bsize, MWSIZE
	jb		Partial_Machine_Word

Full_Machine_Word:

	mov		QWORD PTR [buffer], rax
	add		buffer, MWSIZE
	sub		bsize, MWSIZE

			;; Continue
	jmp		GenerateBlock_Top

			;; 1,2,3,4,5,6,7 bytes remain
Partial_Machine_Word:

			;; Test bit 2 to see if size is at least 4
	test	bsize, 4
	jz		Bit_2_Not_Set

	mov		DWORD PTR [buffer], eax
	shr		rax, 32
	add		buffer, 4

Bit_2_Not_Set:

			;; Test bit 1 to see if size is at least 2
	test	bsize, 2
	jz		Bit_1_Not_Set

	mov		WORD PTR [buffer], ax
	shr		eax, 16
	add 	buffer, 2

Bit_1_Not_Set:

			;; Test bit 0 to see if size is at least 1
	test	bsize, 1
	jz  	GenerateBlock_Success

	mov		BYTE PTR [buffer], al

Bit_0_Not_Set:

			;; We've hit all the bits
	jmp		GenerateBlock_Success

GenerateBlock_PreRet:

			;; Test for success (was the request completely fulfilled?)
	cmp 	bsize, 0
	je 		GenerateBlock_Success

GenerateBlock_Failure:

	xor		rax, rax
	mov		al, RDSEED_FAILURE
	ret

GenerateBlock_Success:

	xor		rax, rax
	mov		al, RDSEED_SUCCESS
	ret

MASM_RSA_GenerateBlock ENDP

ENDIF    ;; _M_X64

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

END
