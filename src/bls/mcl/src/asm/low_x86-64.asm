
; Linux     rdi     rsi    rdx    rcx
; Win       rcx     rdx    r8     r9

%ifdef _WIN64
	%define p1org rcx
	%define p2org rdx
	%define p3org r8
	%define p4org r9
%else
	%define p1org rdi
	%define p2org rsi
	%define p3org rdx
	%define p4org rcx
%endif

%imacro proc 1
global %1
%1:
%endmacro

segment .text

%imacro addPre 1
	mov rax, [p2org]
	add rax, [p3org]
	mov [p1org], rax
%assign i 1
%rep %1
	mov rax, [p2org + i * 8]
	adc rax, [p3org + i * 8]
	mov [p1org + i * 8], rax
%assign i (i+1)
%endrep
	setc al
	movzx eax, al
	ret
%endmacro

%imacro subNC 1
	mov rax, [p2org]
	sub rax, [p3org]
	mov [p1org], rax
%assign i 1
%rep %1
	mov rax, [p2org + i * 8]
	sbb rax, [p3org + i * 8]
	mov [p1org + i * 8], rax
%assign i (i+1)
%endrep
	setc al
	movzx eax, al
	ret
%endmacro

proc mcl_fp_addPre64
        addPre 0
proc mcl_fp_addPre128
        addPre 1
proc mcl_fp_addPre192
        addPre 2
proc mcl_fp_addPre256
        addPre 3
proc mcl_fp_addPre320
        addPre 4
proc mcl_fp_addPre384
        addPre 5
proc mcl_fp_addPre448
        addPre 6
proc mcl_fp_addPre512
        addPre 7
proc mcl_fp_addPre576
        addPre 8
proc mcl_fp_addPre640
        addPre 9
proc mcl_fp_addPre704
        addPre 10
proc mcl_fp_addPre768
        addPre 11
proc mcl_fp_addPre832
        addPre 12
proc mcl_fp_addPre896
        addPre 13
proc mcl_fp_addPre960
        addPre 14
proc mcl_fp_addPre1024
        addPre 15
proc mcl_fp_addPre1088
        addPre 16
proc mcl_fp_addPre1152
        addPre 17
proc mcl_fp_addPre1216
        addPre 18
proc mcl_fp_addPre1280
        addPre 19
proc mcl_fp_addPre1344
        addPre 20
proc mcl_fp_addPre1408
        addPre 21
proc mcl_fp_addPre1472
        addPre 22
proc mcl_fp_addPre1536
        addPre 23

proc mcl_fp_subNC64
        subNC 0
proc mcl_fp_subNC128
        subNC 1
proc mcl_fp_subNC192
        subNC 2
proc mcl_fp_subNC256
        subNC 3
proc mcl_fp_subNC320
        subNC 4
proc mcl_fp_subNC384
        subNC 5
proc mcl_fp_subNC448
        subNC 6
proc mcl_fp_subNC512
        subNC 7
proc mcl_fp_subNC576
        subNC 8
proc mcl_fp_subNC640
        subNC 9
proc mcl_fp_subNC704
        subNC 10
proc mcl_fp_subNC768
        subNC 11
proc mcl_fp_subNC832
        subNC 12
proc mcl_fp_subNC896
        subNC 13
proc mcl_fp_subNC960
        subNC 14
proc mcl_fp_subNC1024
        subNC 15
proc mcl_fp_subNC1088
        subNC 16
proc mcl_fp_subNC1152
        subNC 17
proc mcl_fp_subNC1216
        subNC 18
proc mcl_fp_subNC1280
        subNC 19
proc mcl_fp_subNC1344
        subNC 20
proc mcl_fp_subNC1408
        subNC 21
proc mcl_fp_subNC1472
        subNC 22
proc mcl_fp_subNC1536
        subNC 23

