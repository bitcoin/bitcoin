define private i128 @mul64x64L(i64 %r2, i64 %r3)
{
%r4 = zext i64 %r2 to i128
%r5 = zext i64 %r3 to i128
%r6 = mul i128 %r4, %r5
ret i128 %r6
}
define private i64 @extractHigh64(i128 %r2)
{
%r3 = lshr i128 %r2, 64
%r4 = trunc i128 %r3 to i64
ret i64 %r4
}
define private i128 @mulPos64x64(i64* noalias  %r2, i64 %r3, i64 %r4)
{
%r5 = getelementptr i64, i64* %r2, i64 %r4
%r6 = load i64, i64* %r5
%r7 = call i128 @mul64x64L(i64 %r6, i64 %r3)
ret i128 %r7
}
define i192 @makeNIST_P192L()
{
%r8 = sub i64 0, 1
%r9 = sub i64 0, 2
%r10 = sub i64 0, 1
%r11 = zext i64 %r8 to i192
%r12 = zext i64 %r9 to i192
%r13 = zext i64 %r10 to i192
%r14 = shl i192 %r12, 64
%r15 = shl i192 %r13, 128
%r16 = add i192 %r11, %r14
%r17 = add i192 %r16, %r15
ret i192 %r17
}
define void @mcl_fpDbl_mod_NIST_P192L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r4 = load i64, i64* %r2
%r5 = zext i64 %r4 to i128
%r7 = getelementptr i64, i64* %r2, i32 1
%r8 = load i64, i64* %r7
%r9 = zext i64 %r8 to i128
%r10 = shl i128 %r9, 64
%r11 = or i128 %r5, %r10
%r12 = zext i128 %r11 to i192
%r14 = getelementptr i64, i64* %r2, i32 2
%r15 = load i64, i64* %r14
%r16 = zext i64 %r15 to i192
%r17 = shl i192 %r16, 128
%r18 = or i192 %r12, %r17
%r19 = zext i192 %r18 to i256
%r21 = getelementptr i64, i64* %r2, i32 3
%r22 = load i64, i64* %r21
%r23 = zext i64 %r22 to i128
%r25 = getelementptr i64, i64* %r21, i32 1
%r26 = load i64, i64* %r25
%r27 = zext i64 %r26 to i128
%r28 = shl i128 %r27, 64
%r29 = or i128 %r23, %r28
%r30 = zext i128 %r29 to i192
%r32 = getelementptr i64, i64* %r21, i32 2
%r33 = load i64, i64* %r32
%r34 = zext i64 %r33 to i192
%r35 = shl i192 %r34, 128
%r36 = or i192 %r30, %r35
%r37 = zext i192 %r36 to i256
%r38 = shl i192 %r36, 64
%r39 = zext i192 %r38 to i256
%r40 = lshr i192 %r36, 128
%r41 = trunc i192 %r40 to i64
%r42 = zext i64 %r41 to i256
%r43 = or i256 %r39, %r42
%r44 = shl i256 %r42, 64
%r45 = add i256 %r19, %r37
%r46 = add i256 %r45, %r43
%r47 = add i256 %r46, %r44
%r48 = lshr i256 %r47, 192
%r49 = trunc i256 %r48 to i64
%r50 = zext i64 %r49 to i256
%r51 = shl i256 %r50, 64
%r52 = or i256 %r50, %r51
%r53 = trunc i256 %r47 to i192
%r54 = zext i192 %r53 to i256
%r55 = add i256 %r54, %r52
%r56 = call i192 @makeNIST_P192L()
%r57 = zext i192 %r56 to i256
%r58 = sub i256 %r55, %r57
%r59 = lshr i256 %r58, 192
%r60 = trunc i256 %r59 to i1
%r61 = select i1 %r60, i256 %r55, i256 %r58
%r62 = trunc i256 %r61 to i192
%r64 = getelementptr i64, i64* %r1, i32 0
%r65 = trunc i192 %r62 to i64
store i64 %r65, i64* %r64
%r66 = lshr i192 %r62, 64
%r68 = getelementptr i64, i64* %r1, i32 1
%r69 = trunc i192 %r66 to i64
store i64 %r69, i64* %r68
%r70 = lshr i192 %r66, 64
%r72 = getelementptr i64, i64* %r1, i32 2
%r73 = trunc i192 %r70 to i64
store i64 %r73, i64* %r72
ret void
}
define void @mcl_fp_sqr_NIST_P192L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = alloca i64, i32 6
call void @mcl_fpDbl_sqrPre3L(i64* %r5, i64* %r2)
call void @mcl_fpDbl_mod_NIST_P192L(i64* %r1, i64* %r5, i64* %r5)
ret void
}
define void @mcl_fp_mulNIST_P192L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r6 = alloca i64, i32 6
call void @mcl_fpDbl_mulPre3L(i64* %r6, i64* %r2, i64* %r3)
call void @mcl_fpDbl_mod_NIST_P192L(i64* %r1, i64* %r6, i64* %r6)
ret void
}
define void @mcl_fpDbl_mod_NIST_P521L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r4 = load i64, i64* %r2
%r5 = zext i64 %r4 to i128
%r7 = getelementptr i64, i64* %r2, i32 1
%r8 = load i64, i64* %r7
%r9 = zext i64 %r8 to i128
%r10 = shl i128 %r9, 64
%r11 = or i128 %r5, %r10
%r12 = zext i128 %r11 to i192
%r14 = getelementptr i64, i64* %r2, i32 2
%r15 = load i64, i64* %r14
%r16 = zext i64 %r15 to i192
%r17 = shl i192 %r16, 128
%r18 = or i192 %r12, %r17
%r19 = zext i192 %r18 to i256
%r21 = getelementptr i64, i64* %r2, i32 3
%r22 = load i64, i64* %r21
%r23 = zext i64 %r22 to i256
%r24 = shl i256 %r23, 192
%r25 = or i256 %r19, %r24
%r26 = zext i256 %r25 to i320
%r28 = getelementptr i64, i64* %r2, i32 4
%r29 = load i64, i64* %r28
%r30 = zext i64 %r29 to i320
%r31 = shl i320 %r30, 256
%r32 = or i320 %r26, %r31
%r33 = zext i320 %r32 to i384
%r35 = getelementptr i64, i64* %r2, i32 5
%r36 = load i64, i64* %r35
%r37 = zext i64 %r36 to i384
%r38 = shl i384 %r37, 320
%r39 = or i384 %r33, %r38
%r40 = zext i384 %r39 to i448
%r42 = getelementptr i64, i64* %r2, i32 6
%r43 = load i64, i64* %r42
%r44 = zext i64 %r43 to i448
%r45 = shl i448 %r44, 384
%r46 = or i448 %r40, %r45
%r47 = zext i448 %r46 to i512
%r49 = getelementptr i64, i64* %r2, i32 7
%r50 = load i64, i64* %r49
%r51 = zext i64 %r50 to i512
%r52 = shl i512 %r51, 448
%r53 = or i512 %r47, %r52
%r54 = zext i512 %r53 to i576
%r56 = getelementptr i64, i64* %r2, i32 8
%r57 = load i64, i64* %r56
%r58 = zext i64 %r57 to i576
%r59 = shl i576 %r58, 512
%r60 = or i576 %r54, %r59
%r61 = zext i576 %r60 to i640
%r63 = getelementptr i64, i64* %r2, i32 9
%r64 = load i64, i64* %r63
%r65 = zext i64 %r64 to i640
%r66 = shl i640 %r65, 576
%r67 = or i640 %r61, %r66
%r68 = zext i640 %r67 to i704
%r70 = getelementptr i64, i64* %r2, i32 10
%r71 = load i64, i64* %r70
%r72 = zext i64 %r71 to i704
%r73 = shl i704 %r72, 640
%r74 = or i704 %r68, %r73
%r75 = zext i704 %r74 to i768
%r77 = getelementptr i64, i64* %r2, i32 11
%r78 = load i64, i64* %r77
%r79 = zext i64 %r78 to i768
%r80 = shl i768 %r79, 704
%r81 = or i768 %r75, %r80
%r82 = zext i768 %r81 to i832
%r84 = getelementptr i64, i64* %r2, i32 12
%r85 = load i64, i64* %r84
%r86 = zext i64 %r85 to i832
%r87 = shl i832 %r86, 768
%r88 = or i832 %r82, %r87
%r89 = zext i832 %r88 to i896
%r91 = getelementptr i64, i64* %r2, i32 13
%r92 = load i64, i64* %r91
%r93 = zext i64 %r92 to i896
%r94 = shl i896 %r93, 832
%r95 = or i896 %r89, %r94
%r96 = zext i896 %r95 to i960
%r98 = getelementptr i64, i64* %r2, i32 14
%r99 = load i64, i64* %r98
%r100 = zext i64 %r99 to i960
%r101 = shl i960 %r100, 896
%r102 = or i960 %r96, %r101
%r103 = zext i960 %r102 to i1024
%r105 = getelementptr i64, i64* %r2, i32 15
%r106 = load i64, i64* %r105
%r107 = zext i64 %r106 to i1024
%r108 = shl i1024 %r107, 960
%r109 = or i1024 %r103, %r108
%r110 = zext i1024 %r109 to i1088
%r112 = getelementptr i64, i64* %r2, i32 16
%r113 = load i64, i64* %r112
%r114 = zext i64 %r113 to i1088
%r115 = shl i1088 %r114, 1024
%r116 = or i1088 %r110, %r115
%r117 = trunc i1088 %r116 to i521
%r118 = zext i521 %r117 to i576
%r119 = lshr i1088 %r116, 521
%r120 = trunc i1088 %r119 to i576
%r121 = add i576 %r118, %r120
%r122 = lshr i576 %r121, 521
%r124 = and i576 %r122, 1
%r125 = add i576 %r121, %r124
%r126 = trunc i576 %r125 to i521
%r127 = zext i521 %r126 to i576
%r128 = lshr i576 %r127, 512
%r129 = trunc i576 %r128 to i64
%r131 = or i64 %r129, -512
%r132 = lshr i576 %r127, 0
%r133 = trunc i576 %r132 to i64
%r134 = and i64 %r131, %r133
%r135 = lshr i576 %r127, 64
%r136 = trunc i576 %r135 to i64
%r137 = and i64 %r134, %r136
%r138 = lshr i576 %r127, 128
%r139 = trunc i576 %r138 to i64
%r140 = and i64 %r137, %r139
%r141 = lshr i576 %r127, 192
%r142 = trunc i576 %r141 to i64
%r143 = and i64 %r140, %r142
%r144 = lshr i576 %r127, 256
%r145 = trunc i576 %r144 to i64
%r146 = and i64 %r143, %r145
%r147 = lshr i576 %r127, 320
%r148 = trunc i576 %r147 to i64
%r149 = and i64 %r146, %r148
%r150 = lshr i576 %r127, 384
%r151 = trunc i576 %r150 to i64
%r152 = and i64 %r149, %r151
%r153 = lshr i576 %r127, 448
%r154 = trunc i576 %r153 to i64
%r155 = and i64 %r152, %r154
%r157 = icmp eq i64 %r155, -1
br i1%r157, label %zero, label %nonzero
zero:
store i64 0, i64* %r1
%r161 = getelementptr i64, i64* %r1, i32 1
store i64 0, i64* %r161
%r164 = getelementptr i64, i64* %r1, i32 2
store i64 0, i64* %r164
%r167 = getelementptr i64, i64* %r1, i32 3
store i64 0, i64* %r167
%r170 = getelementptr i64, i64* %r1, i32 4
store i64 0, i64* %r170
%r173 = getelementptr i64, i64* %r1, i32 5
store i64 0, i64* %r173
%r176 = getelementptr i64, i64* %r1, i32 6
store i64 0, i64* %r176
%r179 = getelementptr i64, i64* %r1, i32 7
store i64 0, i64* %r179
%r182 = getelementptr i64, i64* %r1, i32 8
store i64 0, i64* %r182
ret void
nonzero:
%r184 = getelementptr i64, i64* %r1, i32 0
%r185 = trunc i576 %r127 to i64
store i64 %r185, i64* %r184
%r186 = lshr i576 %r127, 64
%r188 = getelementptr i64, i64* %r1, i32 1
%r189 = trunc i576 %r186 to i64
store i64 %r189, i64* %r188
%r190 = lshr i576 %r186, 64
%r192 = getelementptr i64, i64* %r1, i32 2
%r193 = trunc i576 %r190 to i64
store i64 %r193, i64* %r192
%r194 = lshr i576 %r190, 64
%r196 = getelementptr i64, i64* %r1, i32 3
%r197 = trunc i576 %r194 to i64
store i64 %r197, i64* %r196
%r198 = lshr i576 %r194, 64
%r200 = getelementptr i64, i64* %r1, i32 4
%r201 = trunc i576 %r198 to i64
store i64 %r201, i64* %r200
%r202 = lshr i576 %r198, 64
%r204 = getelementptr i64, i64* %r1, i32 5
%r205 = trunc i576 %r202 to i64
store i64 %r205, i64* %r204
%r206 = lshr i576 %r202, 64
%r208 = getelementptr i64, i64* %r1, i32 6
%r209 = trunc i576 %r206 to i64
store i64 %r209, i64* %r208
%r210 = lshr i576 %r206, 64
%r212 = getelementptr i64, i64* %r1, i32 7
%r213 = trunc i576 %r210 to i64
store i64 %r213, i64* %r212
%r214 = lshr i576 %r210, 64
%r216 = getelementptr i64, i64* %r1, i32 8
%r217 = trunc i576 %r214 to i64
store i64 %r217, i64* %r216
ret void
}
define i256 @mulPv192x64(i64* noalias  %r2, i64 %r3)
{
%r5 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 0)
%r6 = trunc i128 %r5 to i64
%r7 = call i64 @extractHigh64(i128 %r5)
%r9 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 1)
%r10 = trunc i128 %r9 to i64
%r11 = call i64 @extractHigh64(i128 %r9)
%r13 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 2)
%r14 = trunc i128 %r13 to i64
%r15 = call i64 @extractHigh64(i128 %r13)
%r16 = zext i64 %r6 to i128
%r17 = zext i64 %r10 to i128
%r18 = shl i128 %r17, 64
%r19 = or i128 %r16, %r18
%r20 = zext i128 %r19 to i192
%r21 = zext i64 %r14 to i192
%r22 = shl i192 %r21, 128
%r23 = or i192 %r20, %r22
%r24 = zext i64 %r7 to i128
%r25 = zext i64 %r11 to i128
%r26 = shl i128 %r25, 64
%r27 = or i128 %r24, %r26
%r28 = zext i128 %r27 to i192
%r29 = zext i64 %r15 to i192
%r30 = shl i192 %r29, 128
%r31 = or i192 %r28, %r30
%r32 = zext i192 %r23 to i256
%r33 = zext i192 %r31 to i256
%r34 = shl i256 %r33, 64
%r35 = add i256 %r32, %r34
ret i256 %r35
}
define void @mcl_fp_mulUnitPre3L(i64* noalias  %r1, i64* noalias  %r2, i64 %r3)
{
%r4 = call i256 @mulPv192x64(i64* %r2, i64 %r3)
%r6 = getelementptr i64, i64* %r1, i32 0
%r7 = trunc i256 %r4 to i64
store i64 %r7, i64* %r6
%r8 = lshr i256 %r4, 64
%r10 = getelementptr i64, i64* %r1, i32 1
%r11 = trunc i256 %r8 to i64
store i64 %r11, i64* %r10
%r12 = lshr i256 %r8, 64
%r14 = getelementptr i64, i64* %r1, i32 2
%r15 = trunc i256 %r12 to i64
store i64 %r15, i64* %r14
%r16 = lshr i256 %r12, 64
%r18 = getelementptr i64, i64* %r1, i32 3
%r19 = trunc i256 %r16 to i64
store i64 %r19, i64* %r18
ret void
}
define void @mcl_fpDbl_mulPre3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r4 = load i64, i64* %r3
%r5 = call i256 @mulPv192x64(i64* %r2, i64 %r4)
%r6 = trunc i256 %r5 to i64
store i64 %r6, i64* %r1
%r7 = lshr i256 %r5, 64
%r9 = getelementptr i64, i64* %r3, i32 1
%r10 = load i64, i64* %r9
%r11 = call i256 @mulPv192x64(i64* %r2, i64 %r10)
%r12 = add i256 %r7, %r11
%r13 = trunc i256 %r12 to i64
%r15 = getelementptr i64, i64* %r1, i32 1
store i64 %r13, i64* %r15
%r16 = lshr i256 %r12, 64
%r18 = getelementptr i64, i64* %r3, i32 2
%r19 = load i64, i64* %r18
%r20 = call i256 @mulPv192x64(i64* %r2, i64 %r19)
%r21 = add i256 %r16, %r20
%r23 = getelementptr i64, i64* %r1, i32 2
%r25 = getelementptr i64, i64* %r23, i32 0
%r26 = trunc i256 %r21 to i64
store i64 %r26, i64* %r25
%r27 = lshr i256 %r21, 64
%r29 = getelementptr i64, i64* %r23, i32 1
%r30 = trunc i256 %r27 to i64
store i64 %r30, i64* %r29
%r31 = lshr i256 %r27, 64
%r33 = getelementptr i64, i64* %r23, i32 2
%r34 = trunc i256 %r31 to i64
store i64 %r34, i64* %r33
%r35 = lshr i256 %r31, 64
%r37 = getelementptr i64, i64* %r23, i32 3
%r38 = trunc i256 %r35 to i64
store i64 %r38, i64* %r37
ret void
}
define void @mcl_fpDbl_sqrPre3L(i64* noalias  %r1, i64* noalias  %r2)
{
%r3 = load i64, i64* %r2
%r4 = call i256 @mulPv192x64(i64* %r2, i64 %r3)
%r5 = trunc i256 %r4 to i64
store i64 %r5, i64* %r1
%r6 = lshr i256 %r4, 64
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = call i256 @mulPv192x64(i64* %r2, i64 %r9)
%r11 = add i256 %r6, %r10
%r12 = trunc i256 %r11 to i64
%r14 = getelementptr i64, i64* %r1, i32 1
store i64 %r12, i64* %r14
%r15 = lshr i256 %r11, 64
%r17 = getelementptr i64, i64* %r2, i32 2
%r18 = load i64, i64* %r17
%r19 = call i256 @mulPv192x64(i64* %r2, i64 %r18)
%r20 = add i256 %r15, %r19
%r22 = getelementptr i64, i64* %r1, i32 2
%r24 = getelementptr i64, i64* %r22, i32 0
%r25 = trunc i256 %r20 to i64
store i64 %r25, i64* %r24
%r26 = lshr i256 %r20, 64
%r28 = getelementptr i64, i64* %r22, i32 1
%r29 = trunc i256 %r26 to i64
store i64 %r29, i64* %r28
%r30 = lshr i256 %r26, 64
%r32 = getelementptr i64, i64* %r22, i32 2
%r33 = trunc i256 %r30 to i64
store i64 %r33, i64* %r32
%r34 = lshr i256 %r30, 64
%r36 = getelementptr i64, i64* %r22, i32 3
%r37 = trunc i256 %r34 to i64
store i64 %r37, i64* %r36
ret void
}
define void @mcl_fp_mont3L(i64* %r1, i64* %r2, i64* %r3, i64* %r4)
{
%r6 = getelementptr i64, i64* %r4, i32 -1
%r7 = load i64, i64* %r6
%r9 = getelementptr i64, i64* %r3, i32 0
%r10 = load i64, i64* %r9
%r11 = call i256 @mulPv192x64(i64* %r2, i64 %r10)
%r12 = zext i256 %r11 to i320
%r13 = trunc i256 %r11 to i64
%r14 = mul i64 %r13, %r7
%r15 = call i256 @mulPv192x64(i64* %r4, i64 %r14)
%r16 = zext i256 %r15 to i320
%r17 = add i320 %r12, %r16
%r18 = lshr i320 %r17, 64
%r20 = getelementptr i64, i64* %r3, i32 1
%r21 = load i64, i64* %r20
%r22 = call i256 @mulPv192x64(i64* %r2, i64 %r21)
%r23 = zext i256 %r22 to i320
%r24 = add i320 %r18, %r23
%r25 = trunc i320 %r24 to i64
%r26 = mul i64 %r25, %r7
%r27 = call i256 @mulPv192x64(i64* %r4, i64 %r26)
%r28 = zext i256 %r27 to i320
%r29 = add i320 %r24, %r28
%r30 = lshr i320 %r29, 64
%r32 = getelementptr i64, i64* %r3, i32 2
%r33 = load i64, i64* %r32
%r34 = call i256 @mulPv192x64(i64* %r2, i64 %r33)
%r35 = zext i256 %r34 to i320
%r36 = add i320 %r30, %r35
%r37 = trunc i320 %r36 to i64
%r38 = mul i64 %r37, %r7
%r39 = call i256 @mulPv192x64(i64* %r4, i64 %r38)
%r40 = zext i256 %r39 to i320
%r41 = add i320 %r36, %r40
%r42 = lshr i320 %r41, 64
%r43 = trunc i320 %r42 to i256
%r44 = load i64, i64* %r4
%r45 = zext i64 %r44 to i128
%r47 = getelementptr i64, i64* %r4, i32 1
%r48 = load i64, i64* %r47
%r49 = zext i64 %r48 to i128
%r50 = shl i128 %r49, 64
%r51 = or i128 %r45, %r50
%r52 = zext i128 %r51 to i192
%r54 = getelementptr i64, i64* %r4, i32 2
%r55 = load i64, i64* %r54
%r56 = zext i64 %r55 to i192
%r57 = shl i192 %r56, 128
%r58 = or i192 %r52, %r57
%r59 = zext i192 %r58 to i256
%r60 = sub i256 %r43, %r59
%r61 = lshr i256 %r60, 192
%r62 = trunc i256 %r61 to i1
%r63 = select i1 %r62, i256 %r43, i256 %r60
%r64 = trunc i256 %r63 to i192
%r66 = getelementptr i64, i64* %r1, i32 0
%r67 = trunc i192 %r64 to i64
store i64 %r67, i64* %r66
%r68 = lshr i192 %r64, 64
%r70 = getelementptr i64, i64* %r1, i32 1
%r71 = trunc i192 %r68 to i64
store i64 %r71, i64* %r70
%r72 = lshr i192 %r68, 64
%r74 = getelementptr i64, i64* %r1, i32 2
%r75 = trunc i192 %r72 to i64
store i64 %r75, i64* %r74
ret void
}
define void @mcl_fp_montNF3L(i64* %r1, i64* %r2, i64* %r3, i64* %r4)
{
%r6 = getelementptr i64, i64* %r4, i32 -1
%r7 = load i64, i64* %r6
%r8 = load i64, i64* %r3
%r9 = call i256 @mulPv192x64(i64* %r2, i64 %r8)
%r10 = trunc i256 %r9 to i64
%r11 = mul i64 %r10, %r7
%r12 = call i256 @mulPv192x64(i64* %r4, i64 %r11)
%r13 = add i256 %r9, %r12
%r14 = lshr i256 %r13, 64
%r16 = getelementptr i64, i64* %r3, i32 1
%r17 = load i64, i64* %r16
%r18 = call i256 @mulPv192x64(i64* %r2, i64 %r17)
%r19 = add i256 %r14, %r18
%r20 = trunc i256 %r19 to i64
%r21 = mul i64 %r20, %r7
%r22 = call i256 @mulPv192x64(i64* %r4, i64 %r21)
%r23 = add i256 %r19, %r22
%r24 = lshr i256 %r23, 64
%r26 = getelementptr i64, i64* %r3, i32 2
%r27 = load i64, i64* %r26
%r28 = call i256 @mulPv192x64(i64* %r2, i64 %r27)
%r29 = add i256 %r24, %r28
%r30 = trunc i256 %r29 to i64
%r31 = mul i64 %r30, %r7
%r32 = call i256 @mulPv192x64(i64* %r4, i64 %r31)
%r33 = add i256 %r29, %r32
%r34 = lshr i256 %r33, 64
%r35 = trunc i256 %r34 to i192
%r36 = load i64, i64* %r4
%r37 = zext i64 %r36 to i128
%r39 = getelementptr i64, i64* %r4, i32 1
%r40 = load i64, i64* %r39
%r41 = zext i64 %r40 to i128
%r42 = shl i128 %r41, 64
%r43 = or i128 %r37, %r42
%r44 = zext i128 %r43 to i192
%r46 = getelementptr i64, i64* %r4, i32 2
%r47 = load i64, i64* %r46
%r48 = zext i64 %r47 to i192
%r49 = shl i192 %r48, 128
%r50 = or i192 %r44, %r49
%r51 = sub i192 %r35, %r50
%r52 = lshr i192 %r51, 191
%r53 = trunc i192 %r52 to i1
%r54 = select i1 %r53, i192 %r35, i192 %r51
%r56 = getelementptr i64, i64* %r1, i32 0
%r57 = trunc i192 %r54 to i64
store i64 %r57, i64* %r56
%r58 = lshr i192 %r54, 64
%r60 = getelementptr i64, i64* %r1, i32 1
%r61 = trunc i192 %r58 to i64
store i64 %r61, i64* %r60
%r62 = lshr i192 %r58, 64
%r64 = getelementptr i64, i64* %r1, i32 2
%r65 = trunc i192 %r62 to i64
store i64 %r65, i64* %r64
ret void
}
define void @mcl_fp_montRed3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = getelementptr i64, i64* %r3, i32 -1
%r6 = load i64, i64* %r5
%r7 = load i64, i64* %r3
%r8 = zext i64 %r7 to i128
%r10 = getelementptr i64, i64* %r3, i32 1
%r11 = load i64, i64* %r10
%r12 = zext i64 %r11 to i128
%r13 = shl i128 %r12, 64
%r14 = or i128 %r8, %r13
%r15 = zext i128 %r14 to i192
%r17 = getelementptr i64, i64* %r3, i32 2
%r18 = load i64, i64* %r17
%r19 = zext i64 %r18 to i192
%r20 = shl i192 %r19, 128
%r21 = or i192 %r15, %r20
%r22 = load i64, i64* %r2
%r23 = zext i64 %r22 to i128
%r25 = getelementptr i64, i64* %r2, i32 1
%r26 = load i64, i64* %r25
%r27 = zext i64 %r26 to i128
%r28 = shl i128 %r27, 64
%r29 = or i128 %r23, %r28
%r30 = zext i128 %r29 to i192
%r32 = getelementptr i64, i64* %r2, i32 2
%r33 = load i64, i64* %r32
%r34 = zext i64 %r33 to i192
%r35 = shl i192 %r34, 128
%r36 = or i192 %r30, %r35
%r37 = trunc i192 %r36 to i64
%r38 = mul i64 %r37, %r6
%r39 = call i256 @mulPv192x64(i64* %r3, i64 %r38)
%r41 = getelementptr i64, i64* %r2, i32 3
%r42 = load i64, i64* %r41
%r43 = zext i64 %r42 to i256
%r44 = shl i256 %r43, 192
%r45 = zext i192 %r36 to i256
%r46 = or i256 %r44, %r45
%r47 = zext i256 %r46 to i320
%r48 = zext i256 %r39 to i320
%r49 = add i320 %r47, %r48
%r50 = lshr i320 %r49, 64
%r51 = trunc i320 %r50 to i256
%r52 = lshr i256 %r51, 192
%r53 = trunc i256 %r52 to i64
%r54 = trunc i256 %r51 to i192
%r55 = trunc i192 %r54 to i64
%r56 = mul i64 %r55, %r6
%r57 = call i256 @mulPv192x64(i64* %r3, i64 %r56)
%r58 = zext i64 %r53 to i256
%r59 = shl i256 %r58, 192
%r60 = add i256 %r57, %r59
%r62 = getelementptr i64, i64* %r2, i32 4
%r63 = load i64, i64* %r62
%r64 = zext i64 %r63 to i256
%r65 = shl i256 %r64, 192
%r66 = zext i192 %r54 to i256
%r67 = or i256 %r65, %r66
%r68 = zext i256 %r67 to i320
%r69 = zext i256 %r60 to i320
%r70 = add i320 %r68, %r69
%r71 = lshr i320 %r70, 64
%r72 = trunc i320 %r71 to i256
%r73 = lshr i256 %r72, 192
%r74 = trunc i256 %r73 to i64
%r75 = trunc i256 %r72 to i192
%r76 = trunc i192 %r75 to i64
%r77 = mul i64 %r76, %r6
%r78 = call i256 @mulPv192x64(i64* %r3, i64 %r77)
%r79 = zext i64 %r74 to i256
%r80 = shl i256 %r79, 192
%r81 = add i256 %r78, %r80
%r83 = getelementptr i64, i64* %r2, i32 5
%r84 = load i64, i64* %r83
%r85 = zext i64 %r84 to i256
%r86 = shl i256 %r85, 192
%r87 = zext i192 %r75 to i256
%r88 = or i256 %r86, %r87
%r89 = zext i256 %r88 to i320
%r90 = zext i256 %r81 to i320
%r91 = add i320 %r89, %r90
%r92 = lshr i320 %r91, 64
%r93 = trunc i320 %r92 to i256
%r94 = lshr i256 %r93, 192
%r95 = trunc i256 %r94 to i64
%r96 = trunc i256 %r93 to i192
%r97 = zext i192 %r21 to i256
%r98 = zext i192 %r96 to i256
%r99 = sub i256 %r98, %r97
%r100 = lshr i256 %r99, 192
%r101 = trunc i256 %r100 to i1
%r102 = select i1 %r101, i256 %r98, i256 %r99
%r103 = trunc i256 %r102 to i192
%r105 = getelementptr i64, i64* %r1, i32 0
%r106 = trunc i192 %r103 to i64
store i64 %r106, i64* %r105
%r107 = lshr i192 %r103, 64
%r109 = getelementptr i64, i64* %r1, i32 1
%r110 = trunc i192 %r107 to i64
store i64 %r110, i64* %r109
%r111 = lshr i192 %r107, 64
%r113 = getelementptr i64, i64* %r1, i32 2
%r114 = trunc i192 %r111 to i64
store i64 %r114, i64* %r113
ret void
}
define void @mcl_fp_montRedNF3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = getelementptr i64, i64* %r3, i32 -1
%r6 = load i64, i64* %r5
%r7 = load i64, i64* %r3
%r8 = zext i64 %r7 to i128
%r10 = getelementptr i64, i64* %r3, i32 1
%r11 = load i64, i64* %r10
%r12 = zext i64 %r11 to i128
%r13 = shl i128 %r12, 64
%r14 = or i128 %r8, %r13
%r15 = zext i128 %r14 to i192
%r17 = getelementptr i64, i64* %r3, i32 2
%r18 = load i64, i64* %r17
%r19 = zext i64 %r18 to i192
%r20 = shl i192 %r19, 128
%r21 = or i192 %r15, %r20
%r22 = load i64, i64* %r2
%r23 = zext i64 %r22 to i128
%r25 = getelementptr i64, i64* %r2, i32 1
%r26 = load i64, i64* %r25
%r27 = zext i64 %r26 to i128
%r28 = shl i128 %r27, 64
%r29 = or i128 %r23, %r28
%r30 = zext i128 %r29 to i192
%r32 = getelementptr i64, i64* %r2, i32 2
%r33 = load i64, i64* %r32
%r34 = zext i64 %r33 to i192
%r35 = shl i192 %r34, 128
%r36 = or i192 %r30, %r35
%r37 = trunc i192 %r36 to i64
%r38 = mul i64 %r37, %r6
%r39 = call i256 @mulPv192x64(i64* %r3, i64 %r38)
%r41 = getelementptr i64, i64* %r2, i32 3
%r42 = load i64, i64* %r41
%r43 = zext i64 %r42 to i256
%r44 = shl i256 %r43, 192
%r45 = zext i192 %r36 to i256
%r46 = or i256 %r44, %r45
%r47 = zext i256 %r46 to i320
%r48 = zext i256 %r39 to i320
%r49 = add i320 %r47, %r48
%r50 = lshr i320 %r49, 64
%r51 = trunc i320 %r50 to i256
%r52 = lshr i256 %r51, 192
%r53 = trunc i256 %r52 to i64
%r54 = trunc i256 %r51 to i192
%r55 = trunc i192 %r54 to i64
%r56 = mul i64 %r55, %r6
%r57 = call i256 @mulPv192x64(i64* %r3, i64 %r56)
%r58 = zext i64 %r53 to i256
%r59 = shl i256 %r58, 192
%r60 = add i256 %r57, %r59
%r62 = getelementptr i64, i64* %r2, i32 4
%r63 = load i64, i64* %r62
%r64 = zext i64 %r63 to i256
%r65 = shl i256 %r64, 192
%r66 = zext i192 %r54 to i256
%r67 = or i256 %r65, %r66
%r68 = zext i256 %r67 to i320
%r69 = zext i256 %r60 to i320
%r70 = add i320 %r68, %r69
%r71 = lshr i320 %r70, 64
%r72 = trunc i320 %r71 to i256
%r73 = lshr i256 %r72, 192
%r74 = trunc i256 %r73 to i64
%r75 = trunc i256 %r72 to i192
%r76 = trunc i192 %r75 to i64
%r77 = mul i64 %r76, %r6
%r78 = call i256 @mulPv192x64(i64* %r3, i64 %r77)
%r79 = zext i64 %r74 to i256
%r80 = shl i256 %r79, 192
%r81 = add i256 %r78, %r80
%r83 = getelementptr i64, i64* %r2, i32 5
%r84 = load i64, i64* %r83
%r85 = zext i64 %r84 to i256
%r86 = shl i256 %r85, 192
%r87 = zext i192 %r75 to i256
%r88 = or i256 %r86, %r87
%r89 = zext i256 %r88 to i320
%r90 = zext i256 %r81 to i320
%r91 = add i320 %r89, %r90
%r92 = lshr i320 %r91, 64
%r93 = trunc i320 %r92 to i256
%r94 = lshr i256 %r93, 192
%r95 = trunc i256 %r94 to i64
%r96 = trunc i256 %r93 to i192
%r97 = sub i192 %r96, %r21
%r98 = lshr i192 %r97, 191
%r99 = trunc i192 %r98 to i1
%r100 = select i1 %r99, i192 %r96, i192 %r97
%r102 = getelementptr i64, i64* %r1, i32 0
%r103 = trunc i192 %r100 to i64
store i64 %r103, i64* %r102
%r104 = lshr i192 %r100, 64
%r106 = getelementptr i64, i64* %r1, i32 1
%r107 = trunc i192 %r104 to i64
store i64 %r107, i64* %r106
%r108 = lshr i192 %r104, 64
%r110 = getelementptr i64, i64* %r1, i32 2
%r111 = trunc i192 %r108 to i64
store i64 %r111, i64* %r110
ret void
}
define i64 @mcl_fp_addPre3L(i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r3
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r3, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r3, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r21 = load i64, i64* %r4
%r22 = zext i64 %r21 to i128
%r24 = getelementptr i64, i64* %r4, i32 1
%r25 = load i64, i64* %r24
%r26 = zext i64 %r25 to i128
%r27 = shl i128 %r26, 64
%r28 = or i128 %r22, %r27
%r29 = zext i128 %r28 to i192
%r31 = getelementptr i64, i64* %r4, i32 2
%r32 = load i64, i64* %r31
%r33 = zext i64 %r32 to i192
%r34 = shl i192 %r33, 128
%r35 = or i192 %r29, %r34
%r36 = zext i192 %r35 to i256
%r37 = add i256 %r20, %r36
%r38 = trunc i256 %r37 to i192
%r40 = getelementptr i64, i64* %r2, i32 0
%r41 = trunc i192 %r38 to i64
store i64 %r41, i64* %r40
%r42 = lshr i192 %r38, 64
%r44 = getelementptr i64, i64* %r2, i32 1
%r45 = trunc i192 %r42 to i64
store i64 %r45, i64* %r44
%r46 = lshr i192 %r42, 64
%r48 = getelementptr i64, i64* %r2, i32 2
%r49 = trunc i192 %r46 to i64
store i64 %r49, i64* %r48
%r50 = lshr i256 %r37, 192
%r51 = trunc i256 %r50 to i64
ret i64 %r51
}
define i64 @mcl_fp_subPre3L(i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r3
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r3, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r3, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r21 = load i64, i64* %r4
%r22 = zext i64 %r21 to i128
%r24 = getelementptr i64, i64* %r4, i32 1
%r25 = load i64, i64* %r24
%r26 = zext i64 %r25 to i128
%r27 = shl i128 %r26, 64
%r28 = or i128 %r22, %r27
%r29 = zext i128 %r28 to i192
%r31 = getelementptr i64, i64* %r4, i32 2
%r32 = load i64, i64* %r31
%r33 = zext i64 %r32 to i192
%r34 = shl i192 %r33, 128
%r35 = or i192 %r29, %r34
%r36 = zext i192 %r35 to i256
%r37 = sub i256 %r20, %r36
%r38 = trunc i256 %r37 to i192
%r40 = getelementptr i64, i64* %r2, i32 0
%r41 = trunc i192 %r38 to i64
store i64 %r41, i64* %r40
%r42 = lshr i192 %r38, 64
%r44 = getelementptr i64, i64* %r2, i32 1
%r45 = trunc i192 %r42 to i64
store i64 %r45, i64* %r44
%r46 = lshr i192 %r42, 64
%r48 = getelementptr i64, i64* %r2, i32 2
%r49 = trunc i192 %r46 to i64
store i64 %r49, i64* %r48
%r51 = lshr i256 %r37, 192
%r52 = trunc i256 %r51 to i64
%r53 = and i64 %r52, 1
ret i64 %r53
}
define void @mcl_fp_shr1_3L(i64* noalias  %r1, i64* noalias  %r2)
{
%r3 = load i64, i64* %r2
%r4 = zext i64 %r3 to i128
%r6 = getelementptr i64, i64* %r2, i32 1
%r7 = load i64, i64* %r6
%r8 = zext i64 %r7 to i128
%r9 = shl i128 %r8, 64
%r10 = or i128 %r4, %r9
%r11 = zext i128 %r10 to i192
%r13 = getelementptr i64, i64* %r2, i32 2
%r14 = load i64, i64* %r13
%r15 = zext i64 %r14 to i192
%r16 = shl i192 %r15, 128
%r17 = or i192 %r11, %r16
%r18 = lshr i192 %r17, 1
%r20 = getelementptr i64, i64* %r1, i32 0
%r21 = trunc i192 %r18 to i64
store i64 %r21, i64* %r20
%r22 = lshr i192 %r18, 64
%r24 = getelementptr i64, i64* %r1, i32 1
%r25 = trunc i192 %r22 to i64
store i64 %r25, i64* %r24
%r26 = lshr i192 %r22, 64
%r28 = getelementptr i64, i64* %r1, i32 2
%r29 = trunc i192 %r26 to i64
store i64 %r29, i64* %r28
ret void
}
define void @mcl_fp_add3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = load i64, i64* %r3
%r21 = zext i64 %r20 to i128
%r23 = getelementptr i64, i64* %r3, i32 1
%r24 = load i64, i64* %r23
%r25 = zext i64 %r24 to i128
%r26 = shl i128 %r25, 64
%r27 = or i128 %r21, %r26
%r28 = zext i128 %r27 to i192
%r30 = getelementptr i64, i64* %r3, i32 2
%r31 = load i64, i64* %r30
%r32 = zext i64 %r31 to i192
%r33 = shl i192 %r32, 128
%r34 = or i192 %r28, %r33
%r35 = zext i192 %r19 to i256
%r36 = zext i192 %r34 to i256
%r37 = add i256 %r35, %r36
%r38 = trunc i256 %r37 to i192
%r40 = getelementptr i64, i64* %r1, i32 0
%r41 = trunc i192 %r38 to i64
store i64 %r41, i64* %r40
%r42 = lshr i192 %r38, 64
%r44 = getelementptr i64, i64* %r1, i32 1
%r45 = trunc i192 %r42 to i64
store i64 %r45, i64* %r44
%r46 = lshr i192 %r42, 64
%r48 = getelementptr i64, i64* %r1, i32 2
%r49 = trunc i192 %r46 to i64
store i64 %r49, i64* %r48
%r50 = load i64, i64* %r4
%r51 = zext i64 %r50 to i128
%r53 = getelementptr i64, i64* %r4, i32 1
%r54 = load i64, i64* %r53
%r55 = zext i64 %r54 to i128
%r56 = shl i128 %r55, 64
%r57 = or i128 %r51, %r56
%r58 = zext i128 %r57 to i192
%r60 = getelementptr i64, i64* %r4, i32 2
%r61 = load i64, i64* %r60
%r62 = zext i64 %r61 to i192
%r63 = shl i192 %r62, 128
%r64 = or i192 %r58, %r63
%r65 = zext i192 %r64 to i256
%r66 = sub i256 %r37, %r65
%r67 = lshr i256 %r66, 192
%r68 = trunc i256 %r67 to i1
br i1%r68, label %carry, label %nocarry
nocarry:
%r69 = trunc i256 %r66 to i192
%r71 = getelementptr i64, i64* %r1, i32 0
%r72 = trunc i192 %r69 to i64
store i64 %r72, i64* %r71
%r73 = lshr i192 %r69, 64
%r75 = getelementptr i64, i64* %r1, i32 1
%r76 = trunc i192 %r73 to i64
store i64 %r76, i64* %r75
%r77 = lshr i192 %r73, 64
%r79 = getelementptr i64, i64* %r1, i32 2
%r80 = trunc i192 %r77 to i64
store i64 %r80, i64* %r79
ret void
carry:
ret void
}
define void @mcl_fp_addNF3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = load i64, i64* %r3
%r21 = zext i64 %r20 to i128
%r23 = getelementptr i64, i64* %r3, i32 1
%r24 = load i64, i64* %r23
%r25 = zext i64 %r24 to i128
%r26 = shl i128 %r25, 64
%r27 = or i128 %r21, %r26
%r28 = zext i128 %r27 to i192
%r30 = getelementptr i64, i64* %r3, i32 2
%r31 = load i64, i64* %r30
%r32 = zext i64 %r31 to i192
%r33 = shl i192 %r32, 128
%r34 = or i192 %r28, %r33
%r35 = add i192 %r19, %r34
%r36 = load i64, i64* %r4
%r37 = zext i64 %r36 to i128
%r39 = getelementptr i64, i64* %r4, i32 1
%r40 = load i64, i64* %r39
%r41 = zext i64 %r40 to i128
%r42 = shl i128 %r41, 64
%r43 = or i128 %r37, %r42
%r44 = zext i128 %r43 to i192
%r46 = getelementptr i64, i64* %r4, i32 2
%r47 = load i64, i64* %r46
%r48 = zext i64 %r47 to i192
%r49 = shl i192 %r48, 128
%r50 = or i192 %r44, %r49
%r51 = sub i192 %r35, %r50
%r52 = lshr i192 %r51, 191
%r53 = trunc i192 %r52 to i1
%r54 = select i1 %r53, i192 %r35, i192 %r51
%r56 = getelementptr i64, i64* %r1, i32 0
%r57 = trunc i192 %r54 to i64
store i64 %r57, i64* %r56
%r58 = lshr i192 %r54, 64
%r60 = getelementptr i64, i64* %r1, i32 1
%r61 = trunc i192 %r58 to i64
store i64 %r61, i64* %r60
%r62 = lshr i192 %r58, 64
%r64 = getelementptr i64, i64* %r1, i32 2
%r65 = trunc i192 %r62 to i64
store i64 %r65, i64* %r64
ret void
}
define void @mcl_fp_sub3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = load i64, i64* %r3
%r21 = zext i64 %r20 to i128
%r23 = getelementptr i64, i64* %r3, i32 1
%r24 = load i64, i64* %r23
%r25 = zext i64 %r24 to i128
%r26 = shl i128 %r25, 64
%r27 = or i128 %r21, %r26
%r28 = zext i128 %r27 to i192
%r30 = getelementptr i64, i64* %r3, i32 2
%r31 = load i64, i64* %r30
%r32 = zext i64 %r31 to i192
%r33 = shl i192 %r32, 128
%r34 = or i192 %r28, %r33
%r35 = zext i192 %r19 to i256
%r36 = zext i192 %r34 to i256
%r37 = sub i256 %r35, %r36
%r38 = trunc i256 %r37 to i192
%r39 = lshr i256 %r37, 192
%r40 = trunc i256 %r39 to i1
%r42 = getelementptr i64, i64* %r1, i32 0
%r43 = trunc i192 %r38 to i64
store i64 %r43, i64* %r42
%r44 = lshr i192 %r38, 64
%r46 = getelementptr i64, i64* %r1, i32 1
%r47 = trunc i192 %r44 to i64
store i64 %r47, i64* %r46
%r48 = lshr i192 %r44, 64
%r50 = getelementptr i64, i64* %r1, i32 2
%r51 = trunc i192 %r48 to i64
store i64 %r51, i64* %r50
br i1%r40, label %carry, label %nocarry
nocarry:
ret void
carry:
%r52 = load i64, i64* %r4
%r53 = zext i64 %r52 to i128
%r55 = getelementptr i64, i64* %r4, i32 1
%r56 = load i64, i64* %r55
%r57 = zext i64 %r56 to i128
%r58 = shl i128 %r57, 64
%r59 = or i128 %r53, %r58
%r60 = zext i128 %r59 to i192
%r62 = getelementptr i64, i64* %r4, i32 2
%r63 = load i64, i64* %r62
%r64 = zext i64 %r63 to i192
%r65 = shl i192 %r64, 128
%r66 = or i192 %r60, %r65
%r67 = add i192 %r38, %r66
%r69 = getelementptr i64, i64* %r1, i32 0
%r70 = trunc i192 %r67 to i64
store i64 %r70, i64* %r69
%r71 = lshr i192 %r67, 64
%r73 = getelementptr i64, i64* %r1, i32 1
%r74 = trunc i192 %r71 to i64
store i64 %r74, i64* %r73
%r75 = lshr i192 %r71, 64
%r77 = getelementptr i64, i64* %r1, i32 2
%r78 = trunc i192 %r75 to i64
store i64 %r78, i64* %r77
ret void
}
define void @mcl_fp_subNF3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = load i64, i64* %r3
%r21 = zext i64 %r20 to i128
%r23 = getelementptr i64, i64* %r3, i32 1
%r24 = load i64, i64* %r23
%r25 = zext i64 %r24 to i128
%r26 = shl i128 %r25, 64
%r27 = or i128 %r21, %r26
%r28 = zext i128 %r27 to i192
%r30 = getelementptr i64, i64* %r3, i32 2
%r31 = load i64, i64* %r30
%r32 = zext i64 %r31 to i192
%r33 = shl i192 %r32, 128
%r34 = or i192 %r28, %r33
%r35 = sub i192 %r19, %r34
%r36 = lshr i192 %r35, 191
%r37 = trunc i192 %r36 to i1
%r38 = load i64, i64* %r4
%r39 = zext i64 %r38 to i128
%r41 = getelementptr i64, i64* %r4, i32 1
%r42 = load i64, i64* %r41
%r43 = zext i64 %r42 to i128
%r44 = shl i128 %r43, 64
%r45 = or i128 %r39, %r44
%r46 = zext i128 %r45 to i192
%r48 = getelementptr i64, i64* %r4, i32 2
%r49 = load i64, i64* %r48
%r50 = zext i64 %r49 to i192
%r51 = shl i192 %r50, 128
%r52 = or i192 %r46, %r51
%r54 = select i1 %r37, i192 %r52, i192 0
%r55 = add i192 %r35, %r54
%r57 = getelementptr i64, i64* %r1, i32 0
%r58 = trunc i192 %r55 to i64
store i64 %r58, i64* %r57
%r59 = lshr i192 %r55, 64
%r61 = getelementptr i64, i64* %r1, i32 1
%r62 = trunc i192 %r59 to i64
store i64 %r62, i64* %r61
%r63 = lshr i192 %r59, 64
%r65 = getelementptr i64, i64* %r1, i32 2
%r66 = trunc i192 %r63 to i64
store i64 %r66, i64* %r65
ret void
}
define void @mcl_fpDbl_add3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = load i64, i64* %r3
%r42 = zext i64 %r41 to i128
%r44 = getelementptr i64, i64* %r3, i32 1
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i128
%r47 = shl i128 %r46, 64
%r48 = or i128 %r42, %r47
%r49 = zext i128 %r48 to i192
%r51 = getelementptr i64, i64* %r3, i32 2
%r52 = load i64, i64* %r51
%r53 = zext i64 %r52 to i192
%r54 = shl i192 %r53, 128
%r55 = or i192 %r49, %r54
%r56 = zext i192 %r55 to i256
%r58 = getelementptr i64, i64* %r3, i32 3
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i256
%r61 = shl i256 %r60, 192
%r62 = or i256 %r56, %r61
%r63 = zext i256 %r62 to i320
%r65 = getelementptr i64, i64* %r3, i32 4
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i320
%r68 = shl i320 %r67, 256
%r69 = or i320 %r63, %r68
%r70 = zext i320 %r69 to i384
%r72 = getelementptr i64, i64* %r3, i32 5
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i384
%r75 = shl i384 %r74, 320
%r76 = or i384 %r70, %r75
%r77 = zext i384 %r40 to i448
%r78 = zext i384 %r76 to i448
%r79 = add i448 %r77, %r78
%r80 = trunc i448 %r79 to i192
%r82 = getelementptr i64, i64* %r1, i32 0
%r83 = trunc i192 %r80 to i64
store i64 %r83, i64* %r82
%r84 = lshr i192 %r80, 64
%r86 = getelementptr i64, i64* %r1, i32 1
%r87 = trunc i192 %r84 to i64
store i64 %r87, i64* %r86
%r88 = lshr i192 %r84, 64
%r90 = getelementptr i64, i64* %r1, i32 2
%r91 = trunc i192 %r88 to i64
store i64 %r91, i64* %r90
%r92 = lshr i448 %r79, 192
%r93 = trunc i448 %r92 to i256
%r94 = load i64, i64* %r4
%r95 = zext i64 %r94 to i128
%r97 = getelementptr i64, i64* %r4, i32 1
%r98 = load i64, i64* %r97
%r99 = zext i64 %r98 to i128
%r100 = shl i128 %r99, 64
%r101 = or i128 %r95, %r100
%r102 = zext i128 %r101 to i192
%r104 = getelementptr i64, i64* %r4, i32 2
%r105 = load i64, i64* %r104
%r106 = zext i64 %r105 to i192
%r107 = shl i192 %r106, 128
%r108 = or i192 %r102, %r107
%r109 = zext i192 %r108 to i256
%r110 = sub i256 %r93, %r109
%r111 = lshr i256 %r110, 192
%r112 = trunc i256 %r111 to i1
%r113 = select i1 %r112, i256 %r93, i256 %r110
%r114 = trunc i256 %r113 to i192
%r116 = getelementptr i64, i64* %r1, i32 3
%r118 = getelementptr i64, i64* %r116, i32 0
%r119 = trunc i192 %r114 to i64
store i64 %r119, i64* %r118
%r120 = lshr i192 %r114, 64
%r122 = getelementptr i64, i64* %r116, i32 1
%r123 = trunc i192 %r120 to i64
store i64 %r123, i64* %r122
%r124 = lshr i192 %r120, 64
%r126 = getelementptr i64, i64* %r116, i32 2
%r127 = trunc i192 %r124 to i64
store i64 %r127, i64* %r126
ret void
}
define void @mcl_fpDbl_sub3L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = load i64, i64* %r3
%r42 = zext i64 %r41 to i128
%r44 = getelementptr i64, i64* %r3, i32 1
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i128
%r47 = shl i128 %r46, 64
%r48 = or i128 %r42, %r47
%r49 = zext i128 %r48 to i192
%r51 = getelementptr i64, i64* %r3, i32 2
%r52 = load i64, i64* %r51
%r53 = zext i64 %r52 to i192
%r54 = shl i192 %r53, 128
%r55 = or i192 %r49, %r54
%r56 = zext i192 %r55 to i256
%r58 = getelementptr i64, i64* %r3, i32 3
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i256
%r61 = shl i256 %r60, 192
%r62 = or i256 %r56, %r61
%r63 = zext i256 %r62 to i320
%r65 = getelementptr i64, i64* %r3, i32 4
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i320
%r68 = shl i320 %r67, 256
%r69 = or i320 %r63, %r68
%r70 = zext i320 %r69 to i384
%r72 = getelementptr i64, i64* %r3, i32 5
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i384
%r75 = shl i384 %r74, 320
%r76 = or i384 %r70, %r75
%r77 = zext i384 %r40 to i448
%r78 = zext i384 %r76 to i448
%r79 = sub i448 %r77, %r78
%r80 = trunc i448 %r79 to i192
%r82 = getelementptr i64, i64* %r1, i32 0
%r83 = trunc i192 %r80 to i64
store i64 %r83, i64* %r82
%r84 = lshr i192 %r80, 64
%r86 = getelementptr i64, i64* %r1, i32 1
%r87 = trunc i192 %r84 to i64
store i64 %r87, i64* %r86
%r88 = lshr i192 %r84, 64
%r90 = getelementptr i64, i64* %r1, i32 2
%r91 = trunc i192 %r88 to i64
store i64 %r91, i64* %r90
%r92 = lshr i448 %r79, 192
%r93 = trunc i448 %r92 to i192
%r94 = lshr i448 %r79, 384
%r95 = trunc i448 %r94 to i1
%r96 = load i64, i64* %r4
%r97 = zext i64 %r96 to i128
%r99 = getelementptr i64, i64* %r4, i32 1
%r100 = load i64, i64* %r99
%r101 = zext i64 %r100 to i128
%r102 = shl i128 %r101, 64
%r103 = or i128 %r97, %r102
%r104 = zext i128 %r103 to i192
%r106 = getelementptr i64, i64* %r4, i32 2
%r107 = load i64, i64* %r106
%r108 = zext i64 %r107 to i192
%r109 = shl i192 %r108, 128
%r110 = or i192 %r104, %r109
%r112 = select i1 %r95, i192 %r110, i192 0
%r113 = add i192 %r93, %r112
%r115 = getelementptr i64, i64* %r1, i32 3
%r117 = getelementptr i64, i64* %r115, i32 0
%r118 = trunc i192 %r113 to i64
store i64 %r118, i64* %r117
%r119 = lshr i192 %r113, 64
%r121 = getelementptr i64, i64* %r115, i32 1
%r122 = trunc i192 %r119 to i64
store i64 %r122, i64* %r121
%r123 = lshr i192 %r119, 64
%r125 = getelementptr i64, i64* %r115, i32 2
%r126 = trunc i192 %r123 to i64
store i64 %r126, i64* %r125
ret void
}
define i320 @mulPv256x64(i64* noalias  %r2, i64 %r3)
{
%r5 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 0)
%r6 = trunc i128 %r5 to i64
%r7 = call i64 @extractHigh64(i128 %r5)
%r9 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 1)
%r10 = trunc i128 %r9 to i64
%r11 = call i64 @extractHigh64(i128 %r9)
%r13 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 2)
%r14 = trunc i128 %r13 to i64
%r15 = call i64 @extractHigh64(i128 %r13)
%r17 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 3)
%r18 = trunc i128 %r17 to i64
%r19 = call i64 @extractHigh64(i128 %r17)
%r20 = zext i64 %r6 to i128
%r21 = zext i64 %r10 to i128
%r22 = shl i128 %r21, 64
%r23 = or i128 %r20, %r22
%r24 = zext i128 %r23 to i192
%r25 = zext i64 %r14 to i192
%r26 = shl i192 %r25, 128
%r27 = or i192 %r24, %r26
%r28 = zext i192 %r27 to i256
%r29 = zext i64 %r18 to i256
%r30 = shl i256 %r29, 192
%r31 = or i256 %r28, %r30
%r32 = zext i64 %r7 to i128
%r33 = zext i64 %r11 to i128
%r34 = shl i128 %r33, 64
%r35 = or i128 %r32, %r34
%r36 = zext i128 %r35 to i192
%r37 = zext i64 %r15 to i192
%r38 = shl i192 %r37, 128
%r39 = or i192 %r36, %r38
%r40 = zext i192 %r39 to i256
%r41 = zext i64 %r19 to i256
%r42 = shl i256 %r41, 192
%r43 = or i256 %r40, %r42
%r44 = zext i256 %r31 to i320
%r45 = zext i256 %r43 to i320
%r46 = shl i320 %r45, 64
%r47 = add i320 %r44, %r46
ret i320 %r47
}
define void @mcl_fp_mulUnitPre4L(i64* noalias  %r1, i64* noalias  %r2, i64 %r3)
{
%r4 = call i320 @mulPv256x64(i64* %r2, i64 %r3)
%r6 = getelementptr i64, i64* %r1, i32 0
%r7 = trunc i320 %r4 to i64
store i64 %r7, i64* %r6
%r8 = lshr i320 %r4, 64
%r10 = getelementptr i64, i64* %r1, i32 1
%r11 = trunc i320 %r8 to i64
store i64 %r11, i64* %r10
%r12 = lshr i320 %r8, 64
%r14 = getelementptr i64, i64* %r1, i32 2
%r15 = trunc i320 %r12 to i64
store i64 %r15, i64* %r14
%r16 = lshr i320 %r12, 64
%r18 = getelementptr i64, i64* %r1, i32 3
%r19 = trunc i320 %r16 to i64
store i64 %r19, i64* %r18
%r20 = lshr i320 %r16, 64
%r22 = getelementptr i64, i64* %r1, i32 4
%r23 = trunc i320 %r20 to i64
store i64 %r23, i64* %r22
ret void
}
define void @mcl_fpDbl_mulPre4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r4 = load i64, i64* %r3
%r5 = call i320 @mulPv256x64(i64* %r2, i64 %r4)
%r6 = trunc i320 %r5 to i64
store i64 %r6, i64* %r1
%r7 = lshr i320 %r5, 64
%r9 = getelementptr i64, i64* %r3, i32 1
%r10 = load i64, i64* %r9
%r11 = call i320 @mulPv256x64(i64* %r2, i64 %r10)
%r12 = add i320 %r7, %r11
%r13 = trunc i320 %r12 to i64
%r15 = getelementptr i64, i64* %r1, i32 1
store i64 %r13, i64* %r15
%r16 = lshr i320 %r12, 64
%r18 = getelementptr i64, i64* %r3, i32 2
%r19 = load i64, i64* %r18
%r20 = call i320 @mulPv256x64(i64* %r2, i64 %r19)
%r21 = add i320 %r16, %r20
%r22 = trunc i320 %r21 to i64
%r24 = getelementptr i64, i64* %r1, i32 2
store i64 %r22, i64* %r24
%r25 = lshr i320 %r21, 64
%r27 = getelementptr i64, i64* %r3, i32 3
%r28 = load i64, i64* %r27
%r29 = call i320 @mulPv256x64(i64* %r2, i64 %r28)
%r30 = add i320 %r25, %r29
%r32 = getelementptr i64, i64* %r1, i32 3
%r34 = getelementptr i64, i64* %r32, i32 0
%r35 = trunc i320 %r30 to i64
store i64 %r35, i64* %r34
%r36 = lshr i320 %r30, 64
%r38 = getelementptr i64, i64* %r32, i32 1
%r39 = trunc i320 %r36 to i64
store i64 %r39, i64* %r38
%r40 = lshr i320 %r36, 64
%r42 = getelementptr i64, i64* %r32, i32 2
%r43 = trunc i320 %r40 to i64
store i64 %r43, i64* %r42
%r44 = lshr i320 %r40, 64
%r46 = getelementptr i64, i64* %r32, i32 3
%r47 = trunc i320 %r44 to i64
store i64 %r47, i64* %r46
%r48 = lshr i320 %r44, 64
%r50 = getelementptr i64, i64* %r32, i32 4
%r51 = trunc i320 %r48 to i64
store i64 %r51, i64* %r50
ret void
}
define void @mcl_fpDbl_sqrPre4L(i64* noalias  %r1, i64* noalias  %r2)
{
%r3 = load i64, i64* %r2
%r4 = call i320 @mulPv256x64(i64* %r2, i64 %r3)
%r5 = trunc i320 %r4 to i64
store i64 %r5, i64* %r1
%r6 = lshr i320 %r4, 64
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = call i320 @mulPv256x64(i64* %r2, i64 %r9)
%r11 = add i320 %r6, %r10
%r12 = trunc i320 %r11 to i64
%r14 = getelementptr i64, i64* %r1, i32 1
store i64 %r12, i64* %r14
%r15 = lshr i320 %r11, 64
%r17 = getelementptr i64, i64* %r2, i32 2
%r18 = load i64, i64* %r17
%r19 = call i320 @mulPv256x64(i64* %r2, i64 %r18)
%r20 = add i320 %r15, %r19
%r21 = trunc i320 %r20 to i64
%r23 = getelementptr i64, i64* %r1, i32 2
store i64 %r21, i64* %r23
%r24 = lshr i320 %r20, 64
%r26 = getelementptr i64, i64* %r2, i32 3
%r27 = load i64, i64* %r26
%r28 = call i320 @mulPv256x64(i64* %r2, i64 %r27)
%r29 = add i320 %r24, %r28
%r31 = getelementptr i64, i64* %r1, i32 3
%r33 = getelementptr i64, i64* %r31, i32 0
%r34 = trunc i320 %r29 to i64
store i64 %r34, i64* %r33
%r35 = lshr i320 %r29, 64
%r37 = getelementptr i64, i64* %r31, i32 1
%r38 = trunc i320 %r35 to i64
store i64 %r38, i64* %r37
%r39 = lshr i320 %r35, 64
%r41 = getelementptr i64, i64* %r31, i32 2
%r42 = trunc i320 %r39 to i64
store i64 %r42, i64* %r41
%r43 = lshr i320 %r39, 64
%r45 = getelementptr i64, i64* %r31, i32 3
%r46 = trunc i320 %r43 to i64
store i64 %r46, i64* %r45
%r47 = lshr i320 %r43, 64
%r49 = getelementptr i64, i64* %r31, i32 4
%r50 = trunc i320 %r47 to i64
store i64 %r50, i64* %r49
ret void
}
define void @mcl_fp_mont4L(i64* %r1, i64* %r2, i64* %r3, i64* %r4)
{
%r6 = getelementptr i64, i64* %r4, i32 -1
%r7 = load i64, i64* %r6
%r9 = getelementptr i64, i64* %r3, i32 0
%r10 = load i64, i64* %r9
%r11 = call i320 @mulPv256x64(i64* %r2, i64 %r10)
%r12 = zext i320 %r11 to i384
%r13 = trunc i320 %r11 to i64
%r14 = mul i64 %r13, %r7
%r15 = call i320 @mulPv256x64(i64* %r4, i64 %r14)
%r16 = zext i320 %r15 to i384
%r17 = add i384 %r12, %r16
%r18 = lshr i384 %r17, 64
%r20 = getelementptr i64, i64* %r3, i32 1
%r21 = load i64, i64* %r20
%r22 = call i320 @mulPv256x64(i64* %r2, i64 %r21)
%r23 = zext i320 %r22 to i384
%r24 = add i384 %r18, %r23
%r25 = trunc i384 %r24 to i64
%r26 = mul i64 %r25, %r7
%r27 = call i320 @mulPv256x64(i64* %r4, i64 %r26)
%r28 = zext i320 %r27 to i384
%r29 = add i384 %r24, %r28
%r30 = lshr i384 %r29, 64
%r32 = getelementptr i64, i64* %r3, i32 2
%r33 = load i64, i64* %r32
%r34 = call i320 @mulPv256x64(i64* %r2, i64 %r33)
%r35 = zext i320 %r34 to i384
%r36 = add i384 %r30, %r35
%r37 = trunc i384 %r36 to i64
%r38 = mul i64 %r37, %r7
%r39 = call i320 @mulPv256x64(i64* %r4, i64 %r38)
%r40 = zext i320 %r39 to i384
%r41 = add i384 %r36, %r40
%r42 = lshr i384 %r41, 64
%r44 = getelementptr i64, i64* %r3, i32 3
%r45 = load i64, i64* %r44
%r46 = call i320 @mulPv256x64(i64* %r2, i64 %r45)
%r47 = zext i320 %r46 to i384
%r48 = add i384 %r42, %r47
%r49 = trunc i384 %r48 to i64
%r50 = mul i64 %r49, %r7
%r51 = call i320 @mulPv256x64(i64* %r4, i64 %r50)
%r52 = zext i320 %r51 to i384
%r53 = add i384 %r48, %r52
%r54 = lshr i384 %r53, 64
%r55 = trunc i384 %r54 to i320
%r56 = load i64, i64* %r4
%r57 = zext i64 %r56 to i128
%r59 = getelementptr i64, i64* %r4, i32 1
%r60 = load i64, i64* %r59
%r61 = zext i64 %r60 to i128
%r62 = shl i128 %r61, 64
%r63 = or i128 %r57, %r62
%r64 = zext i128 %r63 to i192
%r66 = getelementptr i64, i64* %r4, i32 2
%r67 = load i64, i64* %r66
%r68 = zext i64 %r67 to i192
%r69 = shl i192 %r68, 128
%r70 = or i192 %r64, %r69
%r71 = zext i192 %r70 to i256
%r73 = getelementptr i64, i64* %r4, i32 3
%r74 = load i64, i64* %r73
%r75 = zext i64 %r74 to i256
%r76 = shl i256 %r75, 192
%r77 = or i256 %r71, %r76
%r78 = zext i256 %r77 to i320
%r79 = sub i320 %r55, %r78
%r80 = lshr i320 %r79, 256
%r81 = trunc i320 %r80 to i1
%r82 = select i1 %r81, i320 %r55, i320 %r79
%r83 = trunc i320 %r82 to i256
%r85 = getelementptr i64, i64* %r1, i32 0
%r86 = trunc i256 %r83 to i64
store i64 %r86, i64* %r85
%r87 = lshr i256 %r83, 64
%r89 = getelementptr i64, i64* %r1, i32 1
%r90 = trunc i256 %r87 to i64
store i64 %r90, i64* %r89
%r91 = lshr i256 %r87, 64
%r93 = getelementptr i64, i64* %r1, i32 2
%r94 = trunc i256 %r91 to i64
store i64 %r94, i64* %r93
%r95 = lshr i256 %r91, 64
%r97 = getelementptr i64, i64* %r1, i32 3
%r98 = trunc i256 %r95 to i64
store i64 %r98, i64* %r97
ret void
}
define void @mcl_fp_montNF4L(i64* %r1, i64* %r2, i64* %r3, i64* %r4)
{
%r6 = getelementptr i64, i64* %r4, i32 -1
%r7 = load i64, i64* %r6
%r8 = load i64, i64* %r3
%r9 = call i320 @mulPv256x64(i64* %r2, i64 %r8)
%r10 = trunc i320 %r9 to i64
%r11 = mul i64 %r10, %r7
%r12 = call i320 @mulPv256x64(i64* %r4, i64 %r11)
%r13 = add i320 %r9, %r12
%r14 = lshr i320 %r13, 64
%r16 = getelementptr i64, i64* %r3, i32 1
%r17 = load i64, i64* %r16
%r18 = call i320 @mulPv256x64(i64* %r2, i64 %r17)
%r19 = add i320 %r14, %r18
%r20 = trunc i320 %r19 to i64
%r21 = mul i64 %r20, %r7
%r22 = call i320 @mulPv256x64(i64* %r4, i64 %r21)
%r23 = add i320 %r19, %r22
%r24 = lshr i320 %r23, 64
%r26 = getelementptr i64, i64* %r3, i32 2
%r27 = load i64, i64* %r26
%r28 = call i320 @mulPv256x64(i64* %r2, i64 %r27)
%r29 = add i320 %r24, %r28
%r30 = trunc i320 %r29 to i64
%r31 = mul i64 %r30, %r7
%r32 = call i320 @mulPv256x64(i64* %r4, i64 %r31)
%r33 = add i320 %r29, %r32
%r34 = lshr i320 %r33, 64
%r36 = getelementptr i64, i64* %r3, i32 3
%r37 = load i64, i64* %r36
%r38 = call i320 @mulPv256x64(i64* %r2, i64 %r37)
%r39 = add i320 %r34, %r38
%r40 = trunc i320 %r39 to i64
%r41 = mul i64 %r40, %r7
%r42 = call i320 @mulPv256x64(i64* %r4, i64 %r41)
%r43 = add i320 %r39, %r42
%r44 = lshr i320 %r43, 64
%r45 = trunc i320 %r44 to i256
%r46 = load i64, i64* %r4
%r47 = zext i64 %r46 to i128
%r49 = getelementptr i64, i64* %r4, i32 1
%r50 = load i64, i64* %r49
%r51 = zext i64 %r50 to i128
%r52 = shl i128 %r51, 64
%r53 = or i128 %r47, %r52
%r54 = zext i128 %r53 to i192
%r56 = getelementptr i64, i64* %r4, i32 2
%r57 = load i64, i64* %r56
%r58 = zext i64 %r57 to i192
%r59 = shl i192 %r58, 128
%r60 = or i192 %r54, %r59
%r61 = zext i192 %r60 to i256
%r63 = getelementptr i64, i64* %r4, i32 3
%r64 = load i64, i64* %r63
%r65 = zext i64 %r64 to i256
%r66 = shl i256 %r65, 192
%r67 = or i256 %r61, %r66
%r68 = sub i256 %r45, %r67
%r69 = lshr i256 %r68, 255
%r70 = trunc i256 %r69 to i1
%r71 = select i1 %r70, i256 %r45, i256 %r68
%r73 = getelementptr i64, i64* %r1, i32 0
%r74 = trunc i256 %r71 to i64
store i64 %r74, i64* %r73
%r75 = lshr i256 %r71, 64
%r77 = getelementptr i64, i64* %r1, i32 1
%r78 = trunc i256 %r75 to i64
store i64 %r78, i64* %r77
%r79 = lshr i256 %r75, 64
%r81 = getelementptr i64, i64* %r1, i32 2
%r82 = trunc i256 %r79 to i64
store i64 %r82, i64* %r81
%r83 = lshr i256 %r79, 64
%r85 = getelementptr i64, i64* %r1, i32 3
%r86 = trunc i256 %r83 to i64
store i64 %r86, i64* %r85
ret void
}
define void @mcl_fp_montRed4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = getelementptr i64, i64* %r3, i32 -1
%r6 = load i64, i64* %r5
%r7 = load i64, i64* %r3
%r8 = zext i64 %r7 to i128
%r10 = getelementptr i64, i64* %r3, i32 1
%r11 = load i64, i64* %r10
%r12 = zext i64 %r11 to i128
%r13 = shl i128 %r12, 64
%r14 = or i128 %r8, %r13
%r15 = zext i128 %r14 to i192
%r17 = getelementptr i64, i64* %r3, i32 2
%r18 = load i64, i64* %r17
%r19 = zext i64 %r18 to i192
%r20 = shl i192 %r19, 128
%r21 = or i192 %r15, %r20
%r22 = zext i192 %r21 to i256
%r24 = getelementptr i64, i64* %r3, i32 3
%r25 = load i64, i64* %r24
%r26 = zext i64 %r25 to i256
%r27 = shl i256 %r26, 192
%r28 = or i256 %r22, %r27
%r29 = load i64, i64* %r2
%r30 = zext i64 %r29 to i128
%r32 = getelementptr i64, i64* %r2, i32 1
%r33 = load i64, i64* %r32
%r34 = zext i64 %r33 to i128
%r35 = shl i128 %r34, 64
%r36 = or i128 %r30, %r35
%r37 = zext i128 %r36 to i192
%r39 = getelementptr i64, i64* %r2, i32 2
%r40 = load i64, i64* %r39
%r41 = zext i64 %r40 to i192
%r42 = shl i192 %r41, 128
%r43 = or i192 %r37, %r42
%r44 = zext i192 %r43 to i256
%r46 = getelementptr i64, i64* %r2, i32 3
%r47 = load i64, i64* %r46
%r48 = zext i64 %r47 to i256
%r49 = shl i256 %r48, 192
%r50 = or i256 %r44, %r49
%r51 = trunc i256 %r50 to i64
%r52 = mul i64 %r51, %r6
%r53 = call i320 @mulPv256x64(i64* %r3, i64 %r52)
%r55 = getelementptr i64, i64* %r2, i32 4
%r56 = load i64, i64* %r55
%r57 = zext i64 %r56 to i320
%r58 = shl i320 %r57, 256
%r59 = zext i256 %r50 to i320
%r60 = or i320 %r58, %r59
%r61 = zext i320 %r60 to i384
%r62 = zext i320 %r53 to i384
%r63 = add i384 %r61, %r62
%r64 = lshr i384 %r63, 64
%r65 = trunc i384 %r64 to i320
%r66 = lshr i320 %r65, 256
%r67 = trunc i320 %r66 to i64
%r68 = trunc i320 %r65 to i256
%r69 = trunc i256 %r68 to i64
%r70 = mul i64 %r69, %r6
%r71 = call i320 @mulPv256x64(i64* %r3, i64 %r70)
%r72 = zext i64 %r67 to i320
%r73 = shl i320 %r72, 256
%r74 = add i320 %r71, %r73
%r76 = getelementptr i64, i64* %r2, i32 5
%r77 = load i64, i64* %r76
%r78 = zext i64 %r77 to i320
%r79 = shl i320 %r78, 256
%r80 = zext i256 %r68 to i320
%r81 = or i320 %r79, %r80
%r82 = zext i320 %r81 to i384
%r83 = zext i320 %r74 to i384
%r84 = add i384 %r82, %r83
%r85 = lshr i384 %r84, 64
%r86 = trunc i384 %r85 to i320
%r87 = lshr i320 %r86, 256
%r88 = trunc i320 %r87 to i64
%r89 = trunc i320 %r86 to i256
%r90 = trunc i256 %r89 to i64
%r91 = mul i64 %r90, %r6
%r92 = call i320 @mulPv256x64(i64* %r3, i64 %r91)
%r93 = zext i64 %r88 to i320
%r94 = shl i320 %r93, 256
%r95 = add i320 %r92, %r94
%r97 = getelementptr i64, i64* %r2, i32 6
%r98 = load i64, i64* %r97
%r99 = zext i64 %r98 to i320
%r100 = shl i320 %r99, 256
%r101 = zext i256 %r89 to i320
%r102 = or i320 %r100, %r101
%r103 = zext i320 %r102 to i384
%r104 = zext i320 %r95 to i384
%r105 = add i384 %r103, %r104
%r106 = lshr i384 %r105, 64
%r107 = trunc i384 %r106 to i320
%r108 = lshr i320 %r107, 256
%r109 = trunc i320 %r108 to i64
%r110 = trunc i320 %r107 to i256
%r111 = trunc i256 %r110 to i64
%r112 = mul i64 %r111, %r6
%r113 = call i320 @mulPv256x64(i64* %r3, i64 %r112)
%r114 = zext i64 %r109 to i320
%r115 = shl i320 %r114, 256
%r116 = add i320 %r113, %r115
%r118 = getelementptr i64, i64* %r2, i32 7
%r119 = load i64, i64* %r118
%r120 = zext i64 %r119 to i320
%r121 = shl i320 %r120, 256
%r122 = zext i256 %r110 to i320
%r123 = or i320 %r121, %r122
%r124 = zext i320 %r123 to i384
%r125 = zext i320 %r116 to i384
%r126 = add i384 %r124, %r125
%r127 = lshr i384 %r126, 64
%r128 = trunc i384 %r127 to i320
%r129 = lshr i320 %r128, 256
%r130 = trunc i320 %r129 to i64
%r131 = trunc i320 %r128 to i256
%r132 = zext i256 %r28 to i320
%r133 = zext i256 %r131 to i320
%r134 = sub i320 %r133, %r132
%r135 = lshr i320 %r134, 256
%r136 = trunc i320 %r135 to i1
%r137 = select i1 %r136, i320 %r133, i320 %r134
%r138 = trunc i320 %r137 to i256
%r140 = getelementptr i64, i64* %r1, i32 0
%r141 = trunc i256 %r138 to i64
store i64 %r141, i64* %r140
%r142 = lshr i256 %r138, 64
%r144 = getelementptr i64, i64* %r1, i32 1
%r145 = trunc i256 %r142 to i64
store i64 %r145, i64* %r144
%r146 = lshr i256 %r142, 64
%r148 = getelementptr i64, i64* %r1, i32 2
%r149 = trunc i256 %r146 to i64
store i64 %r149, i64* %r148
%r150 = lshr i256 %r146, 64
%r152 = getelementptr i64, i64* %r1, i32 3
%r153 = trunc i256 %r150 to i64
store i64 %r153, i64* %r152
ret void
}
define void @mcl_fp_montRedNF4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = getelementptr i64, i64* %r3, i32 -1
%r6 = load i64, i64* %r5
%r7 = load i64, i64* %r3
%r8 = zext i64 %r7 to i128
%r10 = getelementptr i64, i64* %r3, i32 1
%r11 = load i64, i64* %r10
%r12 = zext i64 %r11 to i128
%r13 = shl i128 %r12, 64
%r14 = or i128 %r8, %r13
%r15 = zext i128 %r14 to i192
%r17 = getelementptr i64, i64* %r3, i32 2
%r18 = load i64, i64* %r17
%r19 = zext i64 %r18 to i192
%r20 = shl i192 %r19, 128
%r21 = or i192 %r15, %r20
%r22 = zext i192 %r21 to i256
%r24 = getelementptr i64, i64* %r3, i32 3
%r25 = load i64, i64* %r24
%r26 = zext i64 %r25 to i256
%r27 = shl i256 %r26, 192
%r28 = or i256 %r22, %r27
%r29 = load i64, i64* %r2
%r30 = zext i64 %r29 to i128
%r32 = getelementptr i64, i64* %r2, i32 1
%r33 = load i64, i64* %r32
%r34 = zext i64 %r33 to i128
%r35 = shl i128 %r34, 64
%r36 = or i128 %r30, %r35
%r37 = zext i128 %r36 to i192
%r39 = getelementptr i64, i64* %r2, i32 2
%r40 = load i64, i64* %r39
%r41 = zext i64 %r40 to i192
%r42 = shl i192 %r41, 128
%r43 = or i192 %r37, %r42
%r44 = zext i192 %r43 to i256
%r46 = getelementptr i64, i64* %r2, i32 3
%r47 = load i64, i64* %r46
%r48 = zext i64 %r47 to i256
%r49 = shl i256 %r48, 192
%r50 = or i256 %r44, %r49
%r51 = trunc i256 %r50 to i64
%r52 = mul i64 %r51, %r6
%r53 = call i320 @mulPv256x64(i64* %r3, i64 %r52)
%r55 = getelementptr i64, i64* %r2, i32 4
%r56 = load i64, i64* %r55
%r57 = zext i64 %r56 to i320
%r58 = shl i320 %r57, 256
%r59 = zext i256 %r50 to i320
%r60 = or i320 %r58, %r59
%r61 = zext i320 %r60 to i384
%r62 = zext i320 %r53 to i384
%r63 = add i384 %r61, %r62
%r64 = lshr i384 %r63, 64
%r65 = trunc i384 %r64 to i320
%r66 = lshr i320 %r65, 256
%r67 = trunc i320 %r66 to i64
%r68 = trunc i320 %r65 to i256
%r69 = trunc i256 %r68 to i64
%r70 = mul i64 %r69, %r6
%r71 = call i320 @mulPv256x64(i64* %r3, i64 %r70)
%r72 = zext i64 %r67 to i320
%r73 = shl i320 %r72, 256
%r74 = add i320 %r71, %r73
%r76 = getelementptr i64, i64* %r2, i32 5
%r77 = load i64, i64* %r76
%r78 = zext i64 %r77 to i320
%r79 = shl i320 %r78, 256
%r80 = zext i256 %r68 to i320
%r81 = or i320 %r79, %r80
%r82 = zext i320 %r81 to i384
%r83 = zext i320 %r74 to i384
%r84 = add i384 %r82, %r83
%r85 = lshr i384 %r84, 64
%r86 = trunc i384 %r85 to i320
%r87 = lshr i320 %r86, 256
%r88 = trunc i320 %r87 to i64
%r89 = trunc i320 %r86 to i256
%r90 = trunc i256 %r89 to i64
%r91 = mul i64 %r90, %r6
%r92 = call i320 @mulPv256x64(i64* %r3, i64 %r91)
%r93 = zext i64 %r88 to i320
%r94 = shl i320 %r93, 256
%r95 = add i320 %r92, %r94
%r97 = getelementptr i64, i64* %r2, i32 6
%r98 = load i64, i64* %r97
%r99 = zext i64 %r98 to i320
%r100 = shl i320 %r99, 256
%r101 = zext i256 %r89 to i320
%r102 = or i320 %r100, %r101
%r103 = zext i320 %r102 to i384
%r104 = zext i320 %r95 to i384
%r105 = add i384 %r103, %r104
%r106 = lshr i384 %r105, 64
%r107 = trunc i384 %r106 to i320
%r108 = lshr i320 %r107, 256
%r109 = trunc i320 %r108 to i64
%r110 = trunc i320 %r107 to i256
%r111 = trunc i256 %r110 to i64
%r112 = mul i64 %r111, %r6
%r113 = call i320 @mulPv256x64(i64* %r3, i64 %r112)
%r114 = zext i64 %r109 to i320
%r115 = shl i320 %r114, 256
%r116 = add i320 %r113, %r115
%r118 = getelementptr i64, i64* %r2, i32 7
%r119 = load i64, i64* %r118
%r120 = zext i64 %r119 to i320
%r121 = shl i320 %r120, 256
%r122 = zext i256 %r110 to i320
%r123 = or i320 %r121, %r122
%r124 = zext i320 %r123 to i384
%r125 = zext i320 %r116 to i384
%r126 = add i384 %r124, %r125
%r127 = lshr i384 %r126, 64
%r128 = trunc i384 %r127 to i320
%r129 = lshr i320 %r128, 256
%r130 = trunc i320 %r129 to i64
%r131 = trunc i320 %r128 to i256
%r132 = sub i256 %r131, %r28
%r133 = lshr i256 %r132, 255
%r134 = trunc i256 %r133 to i1
%r135 = select i1 %r134, i256 %r131, i256 %r132
%r137 = getelementptr i64, i64* %r1, i32 0
%r138 = trunc i256 %r135 to i64
store i64 %r138, i64* %r137
%r139 = lshr i256 %r135, 64
%r141 = getelementptr i64, i64* %r1, i32 1
%r142 = trunc i256 %r139 to i64
store i64 %r142, i64* %r141
%r143 = lshr i256 %r139, 64
%r145 = getelementptr i64, i64* %r1, i32 2
%r146 = trunc i256 %r143 to i64
store i64 %r146, i64* %r145
%r147 = lshr i256 %r143, 64
%r149 = getelementptr i64, i64* %r1, i32 3
%r150 = trunc i256 %r147 to i64
store i64 %r150, i64* %r149
ret void
}
define i64 @mcl_fp_addPre4L(i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r3
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r3, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r3, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r3, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r28 = load i64, i64* %r4
%r29 = zext i64 %r28 to i128
%r31 = getelementptr i64, i64* %r4, i32 1
%r32 = load i64, i64* %r31
%r33 = zext i64 %r32 to i128
%r34 = shl i128 %r33, 64
%r35 = or i128 %r29, %r34
%r36 = zext i128 %r35 to i192
%r38 = getelementptr i64, i64* %r4, i32 2
%r39 = load i64, i64* %r38
%r40 = zext i64 %r39 to i192
%r41 = shl i192 %r40, 128
%r42 = or i192 %r36, %r41
%r43 = zext i192 %r42 to i256
%r45 = getelementptr i64, i64* %r4, i32 3
%r46 = load i64, i64* %r45
%r47 = zext i64 %r46 to i256
%r48 = shl i256 %r47, 192
%r49 = or i256 %r43, %r48
%r50 = zext i256 %r49 to i320
%r51 = add i320 %r27, %r50
%r52 = trunc i320 %r51 to i256
%r54 = getelementptr i64, i64* %r2, i32 0
%r55 = trunc i256 %r52 to i64
store i64 %r55, i64* %r54
%r56 = lshr i256 %r52, 64
%r58 = getelementptr i64, i64* %r2, i32 1
%r59 = trunc i256 %r56 to i64
store i64 %r59, i64* %r58
%r60 = lshr i256 %r56, 64
%r62 = getelementptr i64, i64* %r2, i32 2
%r63 = trunc i256 %r60 to i64
store i64 %r63, i64* %r62
%r64 = lshr i256 %r60, 64
%r66 = getelementptr i64, i64* %r2, i32 3
%r67 = trunc i256 %r64 to i64
store i64 %r67, i64* %r66
%r68 = lshr i320 %r51, 256
%r69 = trunc i320 %r68 to i64
ret i64 %r69
}
define i64 @mcl_fp_subPre4L(i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r3
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r3, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r3, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r3, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r28 = load i64, i64* %r4
%r29 = zext i64 %r28 to i128
%r31 = getelementptr i64, i64* %r4, i32 1
%r32 = load i64, i64* %r31
%r33 = zext i64 %r32 to i128
%r34 = shl i128 %r33, 64
%r35 = or i128 %r29, %r34
%r36 = zext i128 %r35 to i192
%r38 = getelementptr i64, i64* %r4, i32 2
%r39 = load i64, i64* %r38
%r40 = zext i64 %r39 to i192
%r41 = shl i192 %r40, 128
%r42 = or i192 %r36, %r41
%r43 = zext i192 %r42 to i256
%r45 = getelementptr i64, i64* %r4, i32 3
%r46 = load i64, i64* %r45
%r47 = zext i64 %r46 to i256
%r48 = shl i256 %r47, 192
%r49 = or i256 %r43, %r48
%r50 = zext i256 %r49 to i320
%r51 = sub i320 %r27, %r50
%r52 = trunc i320 %r51 to i256
%r54 = getelementptr i64, i64* %r2, i32 0
%r55 = trunc i256 %r52 to i64
store i64 %r55, i64* %r54
%r56 = lshr i256 %r52, 64
%r58 = getelementptr i64, i64* %r2, i32 1
%r59 = trunc i256 %r56 to i64
store i64 %r59, i64* %r58
%r60 = lshr i256 %r56, 64
%r62 = getelementptr i64, i64* %r2, i32 2
%r63 = trunc i256 %r60 to i64
store i64 %r63, i64* %r62
%r64 = lshr i256 %r60, 64
%r66 = getelementptr i64, i64* %r2, i32 3
%r67 = trunc i256 %r64 to i64
store i64 %r67, i64* %r66
%r69 = lshr i320 %r51, 256
%r70 = trunc i320 %r69 to i64
%r71 = and i64 %r70, 1
ret i64 %r71
}
define void @mcl_fp_shr1_4L(i64* noalias  %r1, i64* noalias  %r2)
{
%r3 = load i64, i64* %r2
%r4 = zext i64 %r3 to i128
%r6 = getelementptr i64, i64* %r2, i32 1
%r7 = load i64, i64* %r6
%r8 = zext i64 %r7 to i128
%r9 = shl i128 %r8, 64
%r10 = or i128 %r4, %r9
%r11 = zext i128 %r10 to i192
%r13 = getelementptr i64, i64* %r2, i32 2
%r14 = load i64, i64* %r13
%r15 = zext i64 %r14 to i192
%r16 = shl i192 %r15, 128
%r17 = or i192 %r11, %r16
%r18 = zext i192 %r17 to i256
%r20 = getelementptr i64, i64* %r2, i32 3
%r21 = load i64, i64* %r20
%r22 = zext i64 %r21 to i256
%r23 = shl i256 %r22, 192
%r24 = or i256 %r18, %r23
%r25 = lshr i256 %r24, 1
%r27 = getelementptr i64, i64* %r1, i32 0
%r28 = trunc i256 %r25 to i64
store i64 %r28, i64* %r27
%r29 = lshr i256 %r25, 64
%r31 = getelementptr i64, i64* %r1, i32 1
%r32 = trunc i256 %r29 to i64
store i64 %r32, i64* %r31
%r33 = lshr i256 %r29, 64
%r35 = getelementptr i64, i64* %r1, i32 2
%r36 = trunc i256 %r33 to i64
store i64 %r36, i64* %r35
%r37 = lshr i256 %r33, 64
%r39 = getelementptr i64, i64* %r1, i32 3
%r40 = trunc i256 %r37 to i64
store i64 %r40, i64* %r39
ret void
}
define void @mcl_fp_add4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = load i64, i64* %r3
%r28 = zext i64 %r27 to i128
%r30 = getelementptr i64, i64* %r3, i32 1
%r31 = load i64, i64* %r30
%r32 = zext i64 %r31 to i128
%r33 = shl i128 %r32, 64
%r34 = or i128 %r28, %r33
%r35 = zext i128 %r34 to i192
%r37 = getelementptr i64, i64* %r3, i32 2
%r38 = load i64, i64* %r37
%r39 = zext i64 %r38 to i192
%r40 = shl i192 %r39, 128
%r41 = or i192 %r35, %r40
%r42 = zext i192 %r41 to i256
%r44 = getelementptr i64, i64* %r3, i32 3
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i256
%r47 = shl i256 %r46, 192
%r48 = or i256 %r42, %r47
%r49 = zext i256 %r26 to i320
%r50 = zext i256 %r48 to i320
%r51 = add i320 %r49, %r50
%r52 = trunc i320 %r51 to i256
%r54 = getelementptr i64, i64* %r1, i32 0
%r55 = trunc i256 %r52 to i64
store i64 %r55, i64* %r54
%r56 = lshr i256 %r52, 64
%r58 = getelementptr i64, i64* %r1, i32 1
%r59 = trunc i256 %r56 to i64
store i64 %r59, i64* %r58
%r60 = lshr i256 %r56, 64
%r62 = getelementptr i64, i64* %r1, i32 2
%r63 = trunc i256 %r60 to i64
store i64 %r63, i64* %r62
%r64 = lshr i256 %r60, 64
%r66 = getelementptr i64, i64* %r1, i32 3
%r67 = trunc i256 %r64 to i64
store i64 %r67, i64* %r66
%r68 = load i64, i64* %r4
%r69 = zext i64 %r68 to i128
%r71 = getelementptr i64, i64* %r4, i32 1
%r72 = load i64, i64* %r71
%r73 = zext i64 %r72 to i128
%r74 = shl i128 %r73, 64
%r75 = or i128 %r69, %r74
%r76 = zext i128 %r75 to i192
%r78 = getelementptr i64, i64* %r4, i32 2
%r79 = load i64, i64* %r78
%r80 = zext i64 %r79 to i192
%r81 = shl i192 %r80, 128
%r82 = or i192 %r76, %r81
%r83 = zext i192 %r82 to i256
%r85 = getelementptr i64, i64* %r4, i32 3
%r86 = load i64, i64* %r85
%r87 = zext i64 %r86 to i256
%r88 = shl i256 %r87, 192
%r89 = or i256 %r83, %r88
%r90 = zext i256 %r89 to i320
%r91 = sub i320 %r51, %r90
%r92 = lshr i320 %r91, 256
%r93 = trunc i320 %r92 to i1
br i1%r93, label %carry, label %nocarry
nocarry:
%r94 = trunc i320 %r91 to i256
%r96 = getelementptr i64, i64* %r1, i32 0
%r97 = trunc i256 %r94 to i64
store i64 %r97, i64* %r96
%r98 = lshr i256 %r94, 64
%r100 = getelementptr i64, i64* %r1, i32 1
%r101 = trunc i256 %r98 to i64
store i64 %r101, i64* %r100
%r102 = lshr i256 %r98, 64
%r104 = getelementptr i64, i64* %r1, i32 2
%r105 = trunc i256 %r102 to i64
store i64 %r105, i64* %r104
%r106 = lshr i256 %r102, 64
%r108 = getelementptr i64, i64* %r1, i32 3
%r109 = trunc i256 %r106 to i64
store i64 %r109, i64* %r108
ret void
carry:
ret void
}
define void @mcl_fp_addNF4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = load i64, i64* %r3
%r28 = zext i64 %r27 to i128
%r30 = getelementptr i64, i64* %r3, i32 1
%r31 = load i64, i64* %r30
%r32 = zext i64 %r31 to i128
%r33 = shl i128 %r32, 64
%r34 = or i128 %r28, %r33
%r35 = zext i128 %r34 to i192
%r37 = getelementptr i64, i64* %r3, i32 2
%r38 = load i64, i64* %r37
%r39 = zext i64 %r38 to i192
%r40 = shl i192 %r39, 128
%r41 = or i192 %r35, %r40
%r42 = zext i192 %r41 to i256
%r44 = getelementptr i64, i64* %r3, i32 3
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i256
%r47 = shl i256 %r46, 192
%r48 = or i256 %r42, %r47
%r49 = add i256 %r26, %r48
%r50 = load i64, i64* %r4
%r51 = zext i64 %r50 to i128
%r53 = getelementptr i64, i64* %r4, i32 1
%r54 = load i64, i64* %r53
%r55 = zext i64 %r54 to i128
%r56 = shl i128 %r55, 64
%r57 = or i128 %r51, %r56
%r58 = zext i128 %r57 to i192
%r60 = getelementptr i64, i64* %r4, i32 2
%r61 = load i64, i64* %r60
%r62 = zext i64 %r61 to i192
%r63 = shl i192 %r62, 128
%r64 = or i192 %r58, %r63
%r65 = zext i192 %r64 to i256
%r67 = getelementptr i64, i64* %r4, i32 3
%r68 = load i64, i64* %r67
%r69 = zext i64 %r68 to i256
%r70 = shl i256 %r69, 192
%r71 = or i256 %r65, %r70
%r72 = sub i256 %r49, %r71
%r73 = lshr i256 %r72, 255
%r74 = trunc i256 %r73 to i1
%r75 = select i1 %r74, i256 %r49, i256 %r72
%r77 = getelementptr i64, i64* %r1, i32 0
%r78 = trunc i256 %r75 to i64
store i64 %r78, i64* %r77
%r79 = lshr i256 %r75, 64
%r81 = getelementptr i64, i64* %r1, i32 1
%r82 = trunc i256 %r79 to i64
store i64 %r82, i64* %r81
%r83 = lshr i256 %r79, 64
%r85 = getelementptr i64, i64* %r1, i32 2
%r86 = trunc i256 %r83 to i64
store i64 %r86, i64* %r85
%r87 = lshr i256 %r83, 64
%r89 = getelementptr i64, i64* %r1, i32 3
%r90 = trunc i256 %r87 to i64
store i64 %r90, i64* %r89
ret void
}
define void @mcl_fp_sub4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = load i64, i64* %r3
%r28 = zext i64 %r27 to i128
%r30 = getelementptr i64, i64* %r3, i32 1
%r31 = load i64, i64* %r30
%r32 = zext i64 %r31 to i128
%r33 = shl i128 %r32, 64
%r34 = or i128 %r28, %r33
%r35 = zext i128 %r34 to i192
%r37 = getelementptr i64, i64* %r3, i32 2
%r38 = load i64, i64* %r37
%r39 = zext i64 %r38 to i192
%r40 = shl i192 %r39, 128
%r41 = or i192 %r35, %r40
%r42 = zext i192 %r41 to i256
%r44 = getelementptr i64, i64* %r3, i32 3
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i256
%r47 = shl i256 %r46, 192
%r48 = or i256 %r42, %r47
%r49 = zext i256 %r26 to i320
%r50 = zext i256 %r48 to i320
%r51 = sub i320 %r49, %r50
%r52 = trunc i320 %r51 to i256
%r53 = lshr i320 %r51, 256
%r54 = trunc i320 %r53 to i1
%r56 = getelementptr i64, i64* %r1, i32 0
%r57 = trunc i256 %r52 to i64
store i64 %r57, i64* %r56
%r58 = lshr i256 %r52, 64
%r60 = getelementptr i64, i64* %r1, i32 1
%r61 = trunc i256 %r58 to i64
store i64 %r61, i64* %r60
%r62 = lshr i256 %r58, 64
%r64 = getelementptr i64, i64* %r1, i32 2
%r65 = trunc i256 %r62 to i64
store i64 %r65, i64* %r64
%r66 = lshr i256 %r62, 64
%r68 = getelementptr i64, i64* %r1, i32 3
%r69 = trunc i256 %r66 to i64
store i64 %r69, i64* %r68
br i1%r54, label %carry, label %nocarry
nocarry:
ret void
carry:
%r70 = load i64, i64* %r4
%r71 = zext i64 %r70 to i128
%r73 = getelementptr i64, i64* %r4, i32 1
%r74 = load i64, i64* %r73
%r75 = zext i64 %r74 to i128
%r76 = shl i128 %r75, 64
%r77 = or i128 %r71, %r76
%r78 = zext i128 %r77 to i192
%r80 = getelementptr i64, i64* %r4, i32 2
%r81 = load i64, i64* %r80
%r82 = zext i64 %r81 to i192
%r83 = shl i192 %r82, 128
%r84 = or i192 %r78, %r83
%r85 = zext i192 %r84 to i256
%r87 = getelementptr i64, i64* %r4, i32 3
%r88 = load i64, i64* %r87
%r89 = zext i64 %r88 to i256
%r90 = shl i256 %r89, 192
%r91 = or i256 %r85, %r90
%r92 = add i256 %r52, %r91
%r94 = getelementptr i64, i64* %r1, i32 0
%r95 = trunc i256 %r92 to i64
store i64 %r95, i64* %r94
%r96 = lshr i256 %r92, 64
%r98 = getelementptr i64, i64* %r1, i32 1
%r99 = trunc i256 %r96 to i64
store i64 %r99, i64* %r98
%r100 = lshr i256 %r96, 64
%r102 = getelementptr i64, i64* %r1, i32 2
%r103 = trunc i256 %r100 to i64
store i64 %r103, i64* %r102
%r104 = lshr i256 %r100, 64
%r106 = getelementptr i64, i64* %r1, i32 3
%r107 = trunc i256 %r104 to i64
store i64 %r107, i64* %r106
ret void
}
define void @mcl_fp_subNF4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = load i64, i64* %r3
%r28 = zext i64 %r27 to i128
%r30 = getelementptr i64, i64* %r3, i32 1
%r31 = load i64, i64* %r30
%r32 = zext i64 %r31 to i128
%r33 = shl i128 %r32, 64
%r34 = or i128 %r28, %r33
%r35 = zext i128 %r34 to i192
%r37 = getelementptr i64, i64* %r3, i32 2
%r38 = load i64, i64* %r37
%r39 = zext i64 %r38 to i192
%r40 = shl i192 %r39, 128
%r41 = or i192 %r35, %r40
%r42 = zext i192 %r41 to i256
%r44 = getelementptr i64, i64* %r3, i32 3
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i256
%r47 = shl i256 %r46, 192
%r48 = or i256 %r42, %r47
%r49 = sub i256 %r26, %r48
%r50 = lshr i256 %r49, 255
%r51 = trunc i256 %r50 to i1
%r52 = load i64, i64* %r4
%r53 = zext i64 %r52 to i128
%r55 = getelementptr i64, i64* %r4, i32 1
%r56 = load i64, i64* %r55
%r57 = zext i64 %r56 to i128
%r58 = shl i128 %r57, 64
%r59 = or i128 %r53, %r58
%r60 = zext i128 %r59 to i192
%r62 = getelementptr i64, i64* %r4, i32 2
%r63 = load i64, i64* %r62
%r64 = zext i64 %r63 to i192
%r65 = shl i192 %r64, 128
%r66 = or i192 %r60, %r65
%r67 = zext i192 %r66 to i256
%r69 = getelementptr i64, i64* %r4, i32 3
%r70 = load i64, i64* %r69
%r71 = zext i64 %r70 to i256
%r72 = shl i256 %r71, 192
%r73 = or i256 %r67, %r72
%r75 = select i1 %r51, i256 %r73, i256 0
%r76 = add i256 %r49, %r75
%r78 = getelementptr i64, i64* %r1, i32 0
%r79 = trunc i256 %r76 to i64
store i64 %r79, i64* %r78
%r80 = lshr i256 %r76, 64
%r82 = getelementptr i64, i64* %r1, i32 1
%r83 = trunc i256 %r80 to i64
store i64 %r83, i64* %r82
%r84 = lshr i256 %r80, 64
%r86 = getelementptr i64, i64* %r1, i32 2
%r87 = trunc i256 %r84 to i64
store i64 %r87, i64* %r86
%r88 = lshr i256 %r84, 64
%r90 = getelementptr i64, i64* %r1, i32 3
%r91 = trunc i256 %r88 to i64
store i64 %r91, i64* %r90
ret void
}
define void @mcl_fpDbl_add4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = load i64, i64* %r3
%r56 = zext i64 %r55 to i128
%r58 = getelementptr i64, i64* %r3, i32 1
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i128
%r61 = shl i128 %r60, 64
%r62 = or i128 %r56, %r61
%r63 = zext i128 %r62 to i192
%r65 = getelementptr i64, i64* %r3, i32 2
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i192
%r68 = shl i192 %r67, 128
%r69 = or i192 %r63, %r68
%r70 = zext i192 %r69 to i256
%r72 = getelementptr i64, i64* %r3, i32 3
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i256
%r75 = shl i256 %r74, 192
%r76 = or i256 %r70, %r75
%r77 = zext i256 %r76 to i320
%r79 = getelementptr i64, i64* %r3, i32 4
%r80 = load i64, i64* %r79
%r81 = zext i64 %r80 to i320
%r82 = shl i320 %r81, 256
%r83 = or i320 %r77, %r82
%r84 = zext i320 %r83 to i384
%r86 = getelementptr i64, i64* %r3, i32 5
%r87 = load i64, i64* %r86
%r88 = zext i64 %r87 to i384
%r89 = shl i384 %r88, 320
%r90 = or i384 %r84, %r89
%r91 = zext i384 %r90 to i448
%r93 = getelementptr i64, i64* %r3, i32 6
%r94 = load i64, i64* %r93
%r95 = zext i64 %r94 to i448
%r96 = shl i448 %r95, 384
%r97 = or i448 %r91, %r96
%r98 = zext i448 %r97 to i512
%r100 = getelementptr i64, i64* %r3, i32 7
%r101 = load i64, i64* %r100
%r102 = zext i64 %r101 to i512
%r103 = shl i512 %r102, 448
%r104 = or i512 %r98, %r103
%r105 = zext i512 %r54 to i576
%r106 = zext i512 %r104 to i576
%r107 = add i576 %r105, %r106
%r108 = trunc i576 %r107 to i256
%r110 = getelementptr i64, i64* %r1, i32 0
%r111 = trunc i256 %r108 to i64
store i64 %r111, i64* %r110
%r112 = lshr i256 %r108, 64
%r114 = getelementptr i64, i64* %r1, i32 1
%r115 = trunc i256 %r112 to i64
store i64 %r115, i64* %r114
%r116 = lshr i256 %r112, 64
%r118 = getelementptr i64, i64* %r1, i32 2
%r119 = trunc i256 %r116 to i64
store i64 %r119, i64* %r118
%r120 = lshr i256 %r116, 64
%r122 = getelementptr i64, i64* %r1, i32 3
%r123 = trunc i256 %r120 to i64
store i64 %r123, i64* %r122
%r124 = lshr i576 %r107, 256
%r125 = trunc i576 %r124 to i320
%r126 = load i64, i64* %r4
%r127 = zext i64 %r126 to i128
%r129 = getelementptr i64, i64* %r4, i32 1
%r130 = load i64, i64* %r129
%r131 = zext i64 %r130 to i128
%r132 = shl i128 %r131, 64
%r133 = or i128 %r127, %r132
%r134 = zext i128 %r133 to i192
%r136 = getelementptr i64, i64* %r4, i32 2
%r137 = load i64, i64* %r136
%r138 = zext i64 %r137 to i192
%r139 = shl i192 %r138, 128
%r140 = or i192 %r134, %r139
%r141 = zext i192 %r140 to i256
%r143 = getelementptr i64, i64* %r4, i32 3
%r144 = load i64, i64* %r143
%r145 = zext i64 %r144 to i256
%r146 = shl i256 %r145, 192
%r147 = or i256 %r141, %r146
%r148 = zext i256 %r147 to i320
%r149 = sub i320 %r125, %r148
%r150 = lshr i320 %r149, 256
%r151 = trunc i320 %r150 to i1
%r152 = select i1 %r151, i320 %r125, i320 %r149
%r153 = trunc i320 %r152 to i256
%r155 = getelementptr i64, i64* %r1, i32 4
%r157 = getelementptr i64, i64* %r155, i32 0
%r158 = trunc i256 %r153 to i64
store i64 %r158, i64* %r157
%r159 = lshr i256 %r153, 64
%r161 = getelementptr i64, i64* %r155, i32 1
%r162 = trunc i256 %r159 to i64
store i64 %r162, i64* %r161
%r163 = lshr i256 %r159, 64
%r165 = getelementptr i64, i64* %r155, i32 2
%r166 = trunc i256 %r163 to i64
store i64 %r166, i64* %r165
%r167 = lshr i256 %r163, 64
%r169 = getelementptr i64, i64* %r155, i32 3
%r170 = trunc i256 %r167 to i64
store i64 %r170, i64* %r169
ret void
}
define void @mcl_fpDbl_sub4L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = load i64, i64* %r3
%r56 = zext i64 %r55 to i128
%r58 = getelementptr i64, i64* %r3, i32 1
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i128
%r61 = shl i128 %r60, 64
%r62 = or i128 %r56, %r61
%r63 = zext i128 %r62 to i192
%r65 = getelementptr i64, i64* %r3, i32 2
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i192
%r68 = shl i192 %r67, 128
%r69 = or i192 %r63, %r68
%r70 = zext i192 %r69 to i256
%r72 = getelementptr i64, i64* %r3, i32 3
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i256
%r75 = shl i256 %r74, 192
%r76 = or i256 %r70, %r75
%r77 = zext i256 %r76 to i320
%r79 = getelementptr i64, i64* %r3, i32 4
%r80 = load i64, i64* %r79
%r81 = zext i64 %r80 to i320
%r82 = shl i320 %r81, 256
%r83 = or i320 %r77, %r82
%r84 = zext i320 %r83 to i384
%r86 = getelementptr i64, i64* %r3, i32 5
%r87 = load i64, i64* %r86
%r88 = zext i64 %r87 to i384
%r89 = shl i384 %r88, 320
%r90 = or i384 %r84, %r89
%r91 = zext i384 %r90 to i448
%r93 = getelementptr i64, i64* %r3, i32 6
%r94 = load i64, i64* %r93
%r95 = zext i64 %r94 to i448
%r96 = shl i448 %r95, 384
%r97 = or i448 %r91, %r96
%r98 = zext i448 %r97 to i512
%r100 = getelementptr i64, i64* %r3, i32 7
%r101 = load i64, i64* %r100
%r102 = zext i64 %r101 to i512
%r103 = shl i512 %r102, 448
%r104 = or i512 %r98, %r103
%r105 = zext i512 %r54 to i576
%r106 = zext i512 %r104 to i576
%r107 = sub i576 %r105, %r106
%r108 = trunc i576 %r107 to i256
%r110 = getelementptr i64, i64* %r1, i32 0
%r111 = trunc i256 %r108 to i64
store i64 %r111, i64* %r110
%r112 = lshr i256 %r108, 64
%r114 = getelementptr i64, i64* %r1, i32 1
%r115 = trunc i256 %r112 to i64
store i64 %r115, i64* %r114
%r116 = lshr i256 %r112, 64
%r118 = getelementptr i64, i64* %r1, i32 2
%r119 = trunc i256 %r116 to i64
store i64 %r119, i64* %r118
%r120 = lshr i256 %r116, 64
%r122 = getelementptr i64, i64* %r1, i32 3
%r123 = trunc i256 %r120 to i64
store i64 %r123, i64* %r122
%r124 = lshr i576 %r107, 256
%r125 = trunc i576 %r124 to i256
%r126 = lshr i576 %r107, 512
%r127 = trunc i576 %r126 to i1
%r128 = load i64, i64* %r4
%r129 = zext i64 %r128 to i128
%r131 = getelementptr i64, i64* %r4, i32 1
%r132 = load i64, i64* %r131
%r133 = zext i64 %r132 to i128
%r134 = shl i128 %r133, 64
%r135 = or i128 %r129, %r134
%r136 = zext i128 %r135 to i192
%r138 = getelementptr i64, i64* %r4, i32 2
%r139 = load i64, i64* %r138
%r140 = zext i64 %r139 to i192
%r141 = shl i192 %r140, 128
%r142 = or i192 %r136, %r141
%r143 = zext i192 %r142 to i256
%r145 = getelementptr i64, i64* %r4, i32 3
%r146 = load i64, i64* %r145
%r147 = zext i64 %r146 to i256
%r148 = shl i256 %r147, 192
%r149 = or i256 %r143, %r148
%r151 = select i1 %r127, i256 %r149, i256 0
%r152 = add i256 %r125, %r151
%r154 = getelementptr i64, i64* %r1, i32 4
%r156 = getelementptr i64, i64* %r154, i32 0
%r157 = trunc i256 %r152 to i64
store i64 %r157, i64* %r156
%r158 = lshr i256 %r152, 64
%r160 = getelementptr i64, i64* %r154, i32 1
%r161 = trunc i256 %r158 to i64
store i64 %r161, i64* %r160
%r162 = lshr i256 %r158, 64
%r164 = getelementptr i64, i64* %r154, i32 2
%r165 = trunc i256 %r162 to i64
store i64 %r165, i64* %r164
%r166 = lshr i256 %r162, 64
%r168 = getelementptr i64, i64* %r154, i32 3
%r169 = trunc i256 %r166 to i64
store i64 %r169, i64* %r168
ret void
}
define i448 @mulPv384x64(i64* noalias  %r2, i64 %r3)
{
%r5 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 0)
%r6 = trunc i128 %r5 to i64
%r7 = call i64 @extractHigh64(i128 %r5)
%r9 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 1)
%r10 = trunc i128 %r9 to i64
%r11 = call i64 @extractHigh64(i128 %r9)
%r13 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 2)
%r14 = trunc i128 %r13 to i64
%r15 = call i64 @extractHigh64(i128 %r13)
%r17 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 3)
%r18 = trunc i128 %r17 to i64
%r19 = call i64 @extractHigh64(i128 %r17)
%r21 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 4)
%r22 = trunc i128 %r21 to i64
%r23 = call i64 @extractHigh64(i128 %r21)
%r25 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 5)
%r26 = trunc i128 %r25 to i64
%r27 = call i64 @extractHigh64(i128 %r25)
%r28 = zext i64 %r6 to i128
%r29 = zext i64 %r10 to i128
%r30 = shl i128 %r29, 64
%r31 = or i128 %r28, %r30
%r32 = zext i128 %r31 to i192
%r33 = zext i64 %r14 to i192
%r34 = shl i192 %r33, 128
%r35 = or i192 %r32, %r34
%r36 = zext i192 %r35 to i256
%r37 = zext i64 %r18 to i256
%r38 = shl i256 %r37, 192
%r39 = or i256 %r36, %r38
%r40 = zext i256 %r39 to i320
%r41 = zext i64 %r22 to i320
%r42 = shl i320 %r41, 256
%r43 = or i320 %r40, %r42
%r44 = zext i320 %r43 to i384
%r45 = zext i64 %r26 to i384
%r46 = shl i384 %r45, 320
%r47 = or i384 %r44, %r46
%r48 = zext i64 %r7 to i128
%r49 = zext i64 %r11 to i128
%r50 = shl i128 %r49, 64
%r51 = or i128 %r48, %r50
%r52 = zext i128 %r51 to i192
%r53 = zext i64 %r15 to i192
%r54 = shl i192 %r53, 128
%r55 = or i192 %r52, %r54
%r56 = zext i192 %r55 to i256
%r57 = zext i64 %r19 to i256
%r58 = shl i256 %r57, 192
%r59 = or i256 %r56, %r58
%r60 = zext i256 %r59 to i320
%r61 = zext i64 %r23 to i320
%r62 = shl i320 %r61, 256
%r63 = or i320 %r60, %r62
%r64 = zext i320 %r63 to i384
%r65 = zext i64 %r27 to i384
%r66 = shl i384 %r65, 320
%r67 = or i384 %r64, %r66
%r68 = zext i384 %r47 to i448
%r69 = zext i384 %r67 to i448
%r70 = shl i448 %r69, 64
%r71 = add i448 %r68, %r70
ret i448 %r71
}
define void @mcl_fp_mulUnitPre6L(i64* noalias  %r1, i64* noalias  %r2, i64 %r3)
{
%r4 = call i448 @mulPv384x64(i64* %r2, i64 %r3)
%r6 = getelementptr i64, i64* %r1, i32 0
%r7 = trunc i448 %r4 to i64
store i64 %r7, i64* %r6
%r8 = lshr i448 %r4, 64
%r10 = getelementptr i64, i64* %r1, i32 1
%r11 = trunc i448 %r8 to i64
store i64 %r11, i64* %r10
%r12 = lshr i448 %r8, 64
%r14 = getelementptr i64, i64* %r1, i32 2
%r15 = trunc i448 %r12 to i64
store i64 %r15, i64* %r14
%r16 = lshr i448 %r12, 64
%r18 = getelementptr i64, i64* %r1, i32 3
%r19 = trunc i448 %r16 to i64
store i64 %r19, i64* %r18
%r20 = lshr i448 %r16, 64
%r22 = getelementptr i64, i64* %r1, i32 4
%r23 = trunc i448 %r20 to i64
store i64 %r23, i64* %r22
%r24 = lshr i448 %r20, 64
%r26 = getelementptr i64, i64* %r1, i32 5
%r27 = trunc i448 %r24 to i64
store i64 %r27, i64* %r26
%r28 = lshr i448 %r24, 64
%r30 = getelementptr i64, i64* %r1, i32 6
%r31 = trunc i448 %r28 to i64
store i64 %r31, i64* %r30
ret void
}
define void @mcl_fpDbl_mulPre6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r4 = load i64, i64* %r3
%r5 = call i448 @mulPv384x64(i64* %r2, i64 %r4)
%r6 = trunc i448 %r5 to i64
store i64 %r6, i64* %r1
%r7 = lshr i448 %r5, 64
%r9 = getelementptr i64, i64* %r3, i32 1
%r10 = load i64, i64* %r9
%r11 = call i448 @mulPv384x64(i64* %r2, i64 %r10)
%r12 = add i448 %r7, %r11
%r13 = trunc i448 %r12 to i64
%r15 = getelementptr i64, i64* %r1, i32 1
store i64 %r13, i64* %r15
%r16 = lshr i448 %r12, 64
%r18 = getelementptr i64, i64* %r3, i32 2
%r19 = load i64, i64* %r18
%r20 = call i448 @mulPv384x64(i64* %r2, i64 %r19)
%r21 = add i448 %r16, %r20
%r22 = trunc i448 %r21 to i64
%r24 = getelementptr i64, i64* %r1, i32 2
store i64 %r22, i64* %r24
%r25 = lshr i448 %r21, 64
%r27 = getelementptr i64, i64* %r3, i32 3
%r28 = load i64, i64* %r27
%r29 = call i448 @mulPv384x64(i64* %r2, i64 %r28)
%r30 = add i448 %r25, %r29
%r31 = trunc i448 %r30 to i64
%r33 = getelementptr i64, i64* %r1, i32 3
store i64 %r31, i64* %r33
%r34 = lshr i448 %r30, 64
%r36 = getelementptr i64, i64* %r3, i32 4
%r37 = load i64, i64* %r36
%r38 = call i448 @mulPv384x64(i64* %r2, i64 %r37)
%r39 = add i448 %r34, %r38
%r40 = trunc i448 %r39 to i64
%r42 = getelementptr i64, i64* %r1, i32 4
store i64 %r40, i64* %r42
%r43 = lshr i448 %r39, 64
%r45 = getelementptr i64, i64* %r3, i32 5
%r46 = load i64, i64* %r45
%r47 = call i448 @mulPv384x64(i64* %r2, i64 %r46)
%r48 = add i448 %r43, %r47
%r50 = getelementptr i64, i64* %r1, i32 5
%r52 = getelementptr i64, i64* %r50, i32 0
%r53 = trunc i448 %r48 to i64
store i64 %r53, i64* %r52
%r54 = lshr i448 %r48, 64
%r56 = getelementptr i64, i64* %r50, i32 1
%r57 = trunc i448 %r54 to i64
store i64 %r57, i64* %r56
%r58 = lshr i448 %r54, 64
%r60 = getelementptr i64, i64* %r50, i32 2
%r61 = trunc i448 %r58 to i64
store i64 %r61, i64* %r60
%r62 = lshr i448 %r58, 64
%r64 = getelementptr i64, i64* %r50, i32 3
%r65 = trunc i448 %r62 to i64
store i64 %r65, i64* %r64
%r66 = lshr i448 %r62, 64
%r68 = getelementptr i64, i64* %r50, i32 4
%r69 = trunc i448 %r66 to i64
store i64 %r69, i64* %r68
%r70 = lshr i448 %r66, 64
%r72 = getelementptr i64, i64* %r50, i32 5
%r73 = trunc i448 %r70 to i64
store i64 %r73, i64* %r72
%r74 = lshr i448 %r70, 64
%r76 = getelementptr i64, i64* %r50, i32 6
%r77 = trunc i448 %r74 to i64
store i64 %r77, i64* %r76
ret void
}
define void @mcl_fpDbl_sqrPre6L(i64* noalias  %r1, i64* noalias  %r2)
{
%r3 = load i64, i64* %r2
%r4 = call i448 @mulPv384x64(i64* %r2, i64 %r3)
%r5 = trunc i448 %r4 to i64
store i64 %r5, i64* %r1
%r6 = lshr i448 %r4, 64
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = call i448 @mulPv384x64(i64* %r2, i64 %r9)
%r11 = add i448 %r6, %r10
%r12 = trunc i448 %r11 to i64
%r14 = getelementptr i64, i64* %r1, i32 1
store i64 %r12, i64* %r14
%r15 = lshr i448 %r11, 64
%r17 = getelementptr i64, i64* %r2, i32 2
%r18 = load i64, i64* %r17
%r19 = call i448 @mulPv384x64(i64* %r2, i64 %r18)
%r20 = add i448 %r15, %r19
%r21 = trunc i448 %r20 to i64
%r23 = getelementptr i64, i64* %r1, i32 2
store i64 %r21, i64* %r23
%r24 = lshr i448 %r20, 64
%r26 = getelementptr i64, i64* %r2, i32 3
%r27 = load i64, i64* %r26
%r28 = call i448 @mulPv384x64(i64* %r2, i64 %r27)
%r29 = add i448 %r24, %r28
%r30 = trunc i448 %r29 to i64
%r32 = getelementptr i64, i64* %r1, i32 3
store i64 %r30, i64* %r32
%r33 = lshr i448 %r29, 64
%r35 = getelementptr i64, i64* %r2, i32 4
%r36 = load i64, i64* %r35
%r37 = call i448 @mulPv384x64(i64* %r2, i64 %r36)
%r38 = add i448 %r33, %r37
%r39 = trunc i448 %r38 to i64
%r41 = getelementptr i64, i64* %r1, i32 4
store i64 %r39, i64* %r41
%r42 = lshr i448 %r38, 64
%r44 = getelementptr i64, i64* %r2, i32 5
%r45 = load i64, i64* %r44
%r46 = call i448 @mulPv384x64(i64* %r2, i64 %r45)
%r47 = add i448 %r42, %r46
%r49 = getelementptr i64, i64* %r1, i32 5
%r51 = getelementptr i64, i64* %r49, i32 0
%r52 = trunc i448 %r47 to i64
store i64 %r52, i64* %r51
%r53 = lshr i448 %r47, 64
%r55 = getelementptr i64, i64* %r49, i32 1
%r56 = trunc i448 %r53 to i64
store i64 %r56, i64* %r55
%r57 = lshr i448 %r53, 64
%r59 = getelementptr i64, i64* %r49, i32 2
%r60 = trunc i448 %r57 to i64
store i64 %r60, i64* %r59
%r61 = lshr i448 %r57, 64
%r63 = getelementptr i64, i64* %r49, i32 3
%r64 = trunc i448 %r61 to i64
store i64 %r64, i64* %r63
%r65 = lshr i448 %r61, 64
%r67 = getelementptr i64, i64* %r49, i32 4
%r68 = trunc i448 %r65 to i64
store i64 %r68, i64* %r67
%r69 = lshr i448 %r65, 64
%r71 = getelementptr i64, i64* %r49, i32 5
%r72 = trunc i448 %r69 to i64
store i64 %r72, i64* %r71
%r73 = lshr i448 %r69, 64
%r75 = getelementptr i64, i64* %r49, i32 6
%r76 = trunc i448 %r73 to i64
store i64 %r76, i64* %r75
ret void
}
define void @mcl_fp_mont6L(i64* %r1, i64* %r2, i64* %r3, i64* %r4)
{
%r6 = getelementptr i64, i64* %r4, i32 -1
%r7 = load i64, i64* %r6
%r9 = getelementptr i64, i64* %r3, i32 0
%r10 = load i64, i64* %r9
%r11 = call i448 @mulPv384x64(i64* %r2, i64 %r10)
%r12 = zext i448 %r11 to i512
%r13 = trunc i448 %r11 to i64
%r14 = mul i64 %r13, %r7
%r15 = call i448 @mulPv384x64(i64* %r4, i64 %r14)
%r16 = zext i448 %r15 to i512
%r17 = add i512 %r12, %r16
%r18 = lshr i512 %r17, 64
%r20 = getelementptr i64, i64* %r3, i32 1
%r21 = load i64, i64* %r20
%r22 = call i448 @mulPv384x64(i64* %r2, i64 %r21)
%r23 = zext i448 %r22 to i512
%r24 = add i512 %r18, %r23
%r25 = trunc i512 %r24 to i64
%r26 = mul i64 %r25, %r7
%r27 = call i448 @mulPv384x64(i64* %r4, i64 %r26)
%r28 = zext i448 %r27 to i512
%r29 = add i512 %r24, %r28
%r30 = lshr i512 %r29, 64
%r32 = getelementptr i64, i64* %r3, i32 2
%r33 = load i64, i64* %r32
%r34 = call i448 @mulPv384x64(i64* %r2, i64 %r33)
%r35 = zext i448 %r34 to i512
%r36 = add i512 %r30, %r35
%r37 = trunc i512 %r36 to i64
%r38 = mul i64 %r37, %r7
%r39 = call i448 @mulPv384x64(i64* %r4, i64 %r38)
%r40 = zext i448 %r39 to i512
%r41 = add i512 %r36, %r40
%r42 = lshr i512 %r41, 64
%r44 = getelementptr i64, i64* %r3, i32 3
%r45 = load i64, i64* %r44
%r46 = call i448 @mulPv384x64(i64* %r2, i64 %r45)
%r47 = zext i448 %r46 to i512
%r48 = add i512 %r42, %r47
%r49 = trunc i512 %r48 to i64
%r50 = mul i64 %r49, %r7
%r51 = call i448 @mulPv384x64(i64* %r4, i64 %r50)
%r52 = zext i448 %r51 to i512
%r53 = add i512 %r48, %r52
%r54 = lshr i512 %r53, 64
%r56 = getelementptr i64, i64* %r3, i32 4
%r57 = load i64, i64* %r56
%r58 = call i448 @mulPv384x64(i64* %r2, i64 %r57)
%r59 = zext i448 %r58 to i512
%r60 = add i512 %r54, %r59
%r61 = trunc i512 %r60 to i64
%r62 = mul i64 %r61, %r7
%r63 = call i448 @mulPv384x64(i64* %r4, i64 %r62)
%r64 = zext i448 %r63 to i512
%r65 = add i512 %r60, %r64
%r66 = lshr i512 %r65, 64
%r68 = getelementptr i64, i64* %r3, i32 5
%r69 = load i64, i64* %r68
%r70 = call i448 @mulPv384x64(i64* %r2, i64 %r69)
%r71 = zext i448 %r70 to i512
%r72 = add i512 %r66, %r71
%r73 = trunc i512 %r72 to i64
%r74 = mul i64 %r73, %r7
%r75 = call i448 @mulPv384x64(i64* %r4, i64 %r74)
%r76 = zext i448 %r75 to i512
%r77 = add i512 %r72, %r76
%r78 = lshr i512 %r77, 64
%r79 = trunc i512 %r78 to i448
%r80 = load i64, i64* %r4
%r81 = zext i64 %r80 to i128
%r83 = getelementptr i64, i64* %r4, i32 1
%r84 = load i64, i64* %r83
%r85 = zext i64 %r84 to i128
%r86 = shl i128 %r85, 64
%r87 = or i128 %r81, %r86
%r88 = zext i128 %r87 to i192
%r90 = getelementptr i64, i64* %r4, i32 2
%r91 = load i64, i64* %r90
%r92 = zext i64 %r91 to i192
%r93 = shl i192 %r92, 128
%r94 = or i192 %r88, %r93
%r95 = zext i192 %r94 to i256
%r97 = getelementptr i64, i64* %r4, i32 3
%r98 = load i64, i64* %r97
%r99 = zext i64 %r98 to i256
%r100 = shl i256 %r99, 192
%r101 = or i256 %r95, %r100
%r102 = zext i256 %r101 to i320
%r104 = getelementptr i64, i64* %r4, i32 4
%r105 = load i64, i64* %r104
%r106 = zext i64 %r105 to i320
%r107 = shl i320 %r106, 256
%r108 = or i320 %r102, %r107
%r109 = zext i320 %r108 to i384
%r111 = getelementptr i64, i64* %r4, i32 5
%r112 = load i64, i64* %r111
%r113 = zext i64 %r112 to i384
%r114 = shl i384 %r113, 320
%r115 = or i384 %r109, %r114
%r116 = zext i384 %r115 to i448
%r117 = sub i448 %r79, %r116
%r118 = lshr i448 %r117, 384
%r119 = trunc i448 %r118 to i1
%r120 = select i1 %r119, i448 %r79, i448 %r117
%r121 = trunc i448 %r120 to i384
%r123 = getelementptr i64, i64* %r1, i32 0
%r124 = trunc i384 %r121 to i64
store i64 %r124, i64* %r123
%r125 = lshr i384 %r121, 64
%r127 = getelementptr i64, i64* %r1, i32 1
%r128 = trunc i384 %r125 to i64
store i64 %r128, i64* %r127
%r129 = lshr i384 %r125, 64
%r131 = getelementptr i64, i64* %r1, i32 2
%r132 = trunc i384 %r129 to i64
store i64 %r132, i64* %r131
%r133 = lshr i384 %r129, 64
%r135 = getelementptr i64, i64* %r1, i32 3
%r136 = trunc i384 %r133 to i64
store i64 %r136, i64* %r135
%r137 = lshr i384 %r133, 64
%r139 = getelementptr i64, i64* %r1, i32 4
%r140 = trunc i384 %r137 to i64
store i64 %r140, i64* %r139
%r141 = lshr i384 %r137, 64
%r143 = getelementptr i64, i64* %r1, i32 5
%r144 = trunc i384 %r141 to i64
store i64 %r144, i64* %r143
ret void
}
define void @mcl_fp_montNF6L(i64* %r1, i64* %r2, i64* %r3, i64* %r4)
{
%r6 = getelementptr i64, i64* %r4, i32 -1
%r7 = load i64, i64* %r6
%r8 = load i64, i64* %r3
%r9 = call i448 @mulPv384x64(i64* %r2, i64 %r8)
%r10 = trunc i448 %r9 to i64
%r11 = mul i64 %r10, %r7
%r12 = call i448 @mulPv384x64(i64* %r4, i64 %r11)
%r13 = add i448 %r9, %r12
%r14 = lshr i448 %r13, 64
%r16 = getelementptr i64, i64* %r3, i32 1
%r17 = load i64, i64* %r16
%r18 = call i448 @mulPv384x64(i64* %r2, i64 %r17)
%r19 = add i448 %r14, %r18
%r20 = trunc i448 %r19 to i64
%r21 = mul i64 %r20, %r7
%r22 = call i448 @mulPv384x64(i64* %r4, i64 %r21)
%r23 = add i448 %r19, %r22
%r24 = lshr i448 %r23, 64
%r26 = getelementptr i64, i64* %r3, i32 2
%r27 = load i64, i64* %r26
%r28 = call i448 @mulPv384x64(i64* %r2, i64 %r27)
%r29 = add i448 %r24, %r28
%r30 = trunc i448 %r29 to i64
%r31 = mul i64 %r30, %r7
%r32 = call i448 @mulPv384x64(i64* %r4, i64 %r31)
%r33 = add i448 %r29, %r32
%r34 = lshr i448 %r33, 64
%r36 = getelementptr i64, i64* %r3, i32 3
%r37 = load i64, i64* %r36
%r38 = call i448 @mulPv384x64(i64* %r2, i64 %r37)
%r39 = add i448 %r34, %r38
%r40 = trunc i448 %r39 to i64
%r41 = mul i64 %r40, %r7
%r42 = call i448 @mulPv384x64(i64* %r4, i64 %r41)
%r43 = add i448 %r39, %r42
%r44 = lshr i448 %r43, 64
%r46 = getelementptr i64, i64* %r3, i32 4
%r47 = load i64, i64* %r46
%r48 = call i448 @mulPv384x64(i64* %r2, i64 %r47)
%r49 = add i448 %r44, %r48
%r50 = trunc i448 %r49 to i64
%r51 = mul i64 %r50, %r7
%r52 = call i448 @mulPv384x64(i64* %r4, i64 %r51)
%r53 = add i448 %r49, %r52
%r54 = lshr i448 %r53, 64
%r56 = getelementptr i64, i64* %r3, i32 5
%r57 = load i64, i64* %r56
%r58 = call i448 @mulPv384x64(i64* %r2, i64 %r57)
%r59 = add i448 %r54, %r58
%r60 = trunc i448 %r59 to i64
%r61 = mul i64 %r60, %r7
%r62 = call i448 @mulPv384x64(i64* %r4, i64 %r61)
%r63 = add i448 %r59, %r62
%r64 = lshr i448 %r63, 64
%r65 = trunc i448 %r64 to i384
%r66 = load i64, i64* %r4
%r67 = zext i64 %r66 to i128
%r69 = getelementptr i64, i64* %r4, i32 1
%r70 = load i64, i64* %r69
%r71 = zext i64 %r70 to i128
%r72 = shl i128 %r71, 64
%r73 = or i128 %r67, %r72
%r74 = zext i128 %r73 to i192
%r76 = getelementptr i64, i64* %r4, i32 2
%r77 = load i64, i64* %r76
%r78 = zext i64 %r77 to i192
%r79 = shl i192 %r78, 128
%r80 = or i192 %r74, %r79
%r81 = zext i192 %r80 to i256
%r83 = getelementptr i64, i64* %r4, i32 3
%r84 = load i64, i64* %r83
%r85 = zext i64 %r84 to i256
%r86 = shl i256 %r85, 192
%r87 = or i256 %r81, %r86
%r88 = zext i256 %r87 to i320
%r90 = getelementptr i64, i64* %r4, i32 4
%r91 = load i64, i64* %r90
%r92 = zext i64 %r91 to i320
%r93 = shl i320 %r92, 256
%r94 = or i320 %r88, %r93
%r95 = zext i320 %r94 to i384
%r97 = getelementptr i64, i64* %r4, i32 5
%r98 = load i64, i64* %r97
%r99 = zext i64 %r98 to i384
%r100 = shl i384 %r99, 320
%r101 = or i384 %r95, %r100
%r102 = sub i384 %r65, %r101
%r103 = lshr i384 %r102, 383
%r104 = trunc i384 %r103 to i1
%r105 = select i1 %r104, i384 %r65, i384 %r102
%r107 = getelementptr i64, i64* %r1, i32 0
%r108 = trunc i384 %r105 to i64
store i64 %r108, i64* %r107
%r109 = lshr i384 %r105, 64
%r111 = getelementptr i64, i64* %r1, i32 1
%r112 = trunc i384 %r109 to i64
store i64 %r112, i64* %r111
%r113 = lshr i384 %r109, 64
%r115 = getelementptr i64, i64* %r1, i32 2
%r116 = trunc i384 %r113 to i64
store i64 %r116, i64* %r115
%r117 = lshr i384 %r113, 64
%r119 = getelementptr i64, i64* %r1, i32 3
%r120 = trunc i384 %r117 to i64
store i64 %r120, i64* %r119
%r121 = lshr i384 %r117, 64
%r123 = getelementptr i64, i64* %r1, i32 4
%r124 = trunc i384 %r121 to i64
store i64 %r124, i64* %r123
%r125 = lshr i384 %r121, 64
%r127 = getelementptr i64, i64* %r1, i32 5
%r128 = trunc i384 %r125 to i64
store i64 %r128, i64* %r127
ret void
}
define void @mcl_fp_montRed6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = getelementptr i64, i64* %r3, i32 -1
%r6 = load i64, i64* %r5
%r7 = load i64, i64* %r3
%r8 = zext i64 %r7 to i128
%r10 = getelementptr i64, i64* %r3, i32 1
%r11 = load i64, i64* %r10
%r12 = zext i64 %r11 to i128
%r13 = shl i128 %r12, 64
%r14 = or i128 %r8, %r13
%r15 = zext i128 %r14 to i192
%r17 = getelementptr i64, i64* %r3, i32 2
%r18 = load i64, i64* %r17
%r19 = zext i64 %r18 to i192
%r20 = shl i192 %r19, 128
%r21 = or i192 %r15, %r20
%r22 = zext i192 %r21 to i256
%r24 = getelementptr i64, i64* %r3, i32 3
%r25 = load i64, i64* %r24
%r26 = zext i64 %r25 to i256
%r27 = shl i256 %r26, 192
%r28 = or i256 %r22, %r27
%r29 = zext i256 %r28 to i320
%r31 = getelementptr i64, i64* %r3, i32 4
%r32 = load i64, i64* %r31
%r33 = zext i64 %r32 to i320
%r34 = shl i320 %r33, 256
%r35 = or i320 %r29, %r34
%r36 = zext i320 %r35 to i384
%r38 = getelementptr i64, i64* %r3, i32 5
%r39 = load i64, i64* %r38
%r40 = zext i64 %r39 to i384
%r41 = shl i384 %r40, 320
%r42 = or i384 %r36, %r41
%r43 = load i64, i64* %r2
%r44 = zext i64 %r43 to i128
%r46 = getelementptr i64, i64* %r2, i32 1
%r47 = load i64, i64* %r46
%r48 = zext i64 %r47 to i128
%r49 = shl i128 %r48, 64
%r50 = or i128 %r44, %r49
%r51 = zext i128 %r50 to i192
%r53 = getelementptr i64, i64* %r2, i32 2
%r54 = load i64, i64* %r53
%r55 = zext i64 %r54 to i192
%r56 = shl i192 %r55, 128
%r57 = or i192 %r51, %r56
%r58 = zext i192 %r57 to i256
%r60 = getelementptr i64, i64* %r2, i32 3
%r61 = load i64, i64* %r60
%r62 = zext i64 %r61 to i256
%r63 = shl i256 %r62, 192
%r64 = or i256 %r58, %r63
%r65 = zext i256 %r64 to i320
%r67 = getelementptr i64, i64* %r2, i32 4
%r68 = load i64, i64* %r67
%r69 = zext i64 %r68 to i320
%r70 = shl i320 %r69, 256
%r71 = or i320 %r65, %r70
%r72 = zext i320 %r71 to i384
%r74 = getelementptr i64, i64* %r2, i32 5
%r75 = load i64, i64* %r74
%r76 = zext i64 %r75 to i384
%r77 = shl i384 %r76, 320
%r78 = or i384 %r72, %r77
%r79 = trunc i384 %r78 to i64
%r80 = mul i64 %r79, %r6
%r81 = call i448 @mulPv384x64(i64* %r3, i64 %r80)
%r83 = getelementptr i64, i64* %r2, i32 6
%r84 = load i64, i64* %r83
%r85 = zext i64 %r84 to i448
%r86 = shl i448 %r85, 384
%r87 = zext i384 %r78 to i448
%r88 = or i448 %r86, %r87
%r89 = zext i448 %r88 to i512
%r90 = zext i448 %r81 to i512
%r91 = add i512 %r89, %r90
%r92 = lshr i512 %r91, 64
%r93 = trunc i512 %r92 to i448
%r94 = lshr i448 %r93, 384
%r95 = trunc i448 %r94 to i64
%r96 = trunc i448 %r93 to i384
%r97 = trunc i384 %r96 to i64
%r98 = mul i64 %r97, %r6
%r99 = call i448 @mulPv384x64(i64* %r3, i64 %r98)
%r100 = zext i64 %r95 to i448
%r101 = shl i448 %r100, 384
%r102 = add i448 %r99, %r101
%r104 = getelementptr i64, i64* %r2, i32 7
%r105 = load i64, i64* %r104
%r106 = zext i64 %r105 to i448
%r107 = shl i448 %r106, 384
%r108 = zext i384 %r96 to i448
%r109 = or i448 %r107, %r108
%r110 = zext i448 %r109 to i512
%r111 = zext i448 %r102 to i512
%r112 = add i512 %r110, %r111
%r113 = lshr i512 %r112, 64
%r114 = trunc i512 %r113 to i448
%r115 = lshr i448 %r114, 384
%r116 = trunc i448 %r115 to i64
%r117 = trunc i448 %r114 to i384
%r118 = trunc i384 %r117 to i64
%r119 = mul i64 %r118, %r6
%r120 = call i448 @mulPv384x64(i64* %r3, i64 %r119)
%r121 = zext i64 %r116 to i448
%r122 = shl i448 %r121, 384
%r123 = add i448 %r120, %r122
%r125 = getelementptr i64, i64* %r2, i32 8
%r126 = load i64, i64* %r125
%r127 = zext i64 %r126 to i448
%r128 = shl i448 %r127, 384
%r129 = zext i384 %r117 to i448
%r130 = or i448 %r128, %r129
%r131 = zext i448 %r130 to i512
%r132 = zext i448 %r123 to i512
%r133 = add i512 %r131, %r132
%r134 = lshr i512 %r133, 64
%r135 = trunc i512 %r134 to i448
%r136 = lshr i448 %r135, 384
%r137 = trunc i448 %r136 to i64
%r138 = trunc i448 %r135 to i384
%r139 = trunc i384 %r138 to i64
%r140 = mul i64 %r139, %r6
%r141 = call i448 @mulPv384x64(i64* %r3, i64 %r140)
%r142 = zext i64 %r137 to i448
%r143 = shl i448 %r142, 384
%r144 = add i448 %r141, %r143
%r146 = getelementptr i64, i64* %r2, i32 9
%r147 = load i64, i64* %r146
%r148 = zext i64 %r147 to i448
%r149 = shl i448 %r148, 384
%r150 = zext i384 %r138 to i448
%r151 = or i448 %r149, %r150
%r152 = zext i448 %r151 to i512
%r153 = zext i448 %r144 to i512
%r154 = add i512 %r152, %r153
%r155 = lshr i512 %r154, 64
%r156 = trunc i512 %r155 to i448
%r157 = lshr i448 %r156, 384
%r158 = trunc i448 %r157 to i64
%r159 = trunc i448 %r156 to i384
%r160 = trunc i384 %r159 to i64
%r161 = mul i64 %r160, %r6
%r162 = call i448 @mulPv384x64(i64* %r3, i64 %r161)
%r163 = zext i64 %r158 to i448
%r164 = shl i448 %r163, 384
%r165 = add i448 %r162, %r164
%r167 = getelementptr i64, i64* %r2, i32 10
%r168 = load i64, i64* %r167
%r169 = zext i64 %r168 to i448
%r170 = shl i448 %r169, 384
%r171 = zext i384 %r159 to i448
%r172 = or i448 %r170, %r171
%r173 = zext i448 %r172 to i512
%r174 = zext i448 %r165 to i512
%r175 = add i512 %r173, %r174
%r176 = lshr i512 %r175, 64
%r177 = trunc i512 %r176 to i448
%r178 = lshr i448 %r177, 384
%r179 = trunc i448 %r178 to i64
%r180 = trunc i448 %r177 to i384
%r181 = trunc i384 %r180 to i64
%r182 = mul i64 %r181, %r6
%r183 = call i448 @mulPv384x64(i64* %r3, i64 %r182)
%r184 = zext i64 %r179 to i448
%r185 = shl i448 %r184, 384
%r186 = add i448 %r183, %r185
%r188 = getelementptr i64, i64* %r2, i32 11
%r189 = load i64, i64* %r188
%r190 = zext i64 %r189 to i448
%r191 = shl i448 %r190, 384
%r192 = zext i384 %r180 to i448
%r193 = or i448 %r191, %r192
%r194 = zext i448 %r193 to i512
%r195 = zext i448 %r186 to i512
%r196 = add i512 %r194, %r195
%r197 = lshr i512 %r196, 64
%r198 = trunc i512 %r197 to i448
%r199 = lshr i448 %r198, 384
%r200 = trunc i448 %r199 to i64
%r201 = trunc i448 %r198 to i384
%r202 = zext i384 %r42 to i448
%r203 = zext i384 %r201 to i448
%r204 = sub i448 %r203, %r202
%r205 = lshr i448 %r204, 384
%r206 = trunc i448 %r205 to i1
%r207 = select i1 %r206, i448 %r203, i448 %r204
%r208 = trunc i448 %r207 to i384
%r210 = getelementptr i64, i64* %r1, i32 0
%r211 = trunc i384 %r208 to i64
store i64 %r211, i64* %r210
%r212 = lshr i384 %r208, 64
%r214 = getelementptr i64, i64* %r1, i32 1
%r215 = trunc i384 %r212 to i64
store i64 %r215, i64* %r214
%r216 = lshr i384 %r212, 64
%r218 = getelementptr i64, i64* %r1, i32 2
%r219 = trunc i384 %r216 to i64
store i64 %r219, i64* %r218
%r220 = lshr i384 %r216, 64
%r222 = getelementptr i64, i64* %r1, i32 3
%r223 = trunc i384 %r220 to i64
store i64 %r223, i64* %r222
%r224 = lshr i384 %r220, 64
%r226 = getelementptr i64, i64* %r1, i32 4
%r227 = trunc i384 %r224 to i64
store i64 %r227, i64* %r226
%r228 = lshr i384 %r224, 64
%r230 = getelementptr i64, i64* %r1, i32 5
%r231 = trunc i384 %r228 to i64
store i64 %r231, i64* %r230
ret void
}
define void @mcl_fp_montRedNF6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = getelementptr i64, i64* %r3, i32 -1
%r6 = load i64, i64* %r5
%r7 = load i64, i64* %r3
%r8 = zext i64 %r7 to i128
%r10 = getelementptr i64, i64* %r3, i32 1
%r11 = load i64, i64* %r10
%r12 = zext i64 %r11 to i128
%r13 = shl i128 %r12, 64
%r14 = or i128 %r8, %r13
%r15 = zext i128 %r14 to i192
%r17 = getelementptr i64, i64* %r3, i32 2
%r18 = load i64, i64* %r17
%r19 = zext i64 %r18 to i192
%r20 = shl i192 %r19, 128
%r21 = or i192 %r15, %r20
%r22 = zext i192 %r21 to i256
%r24 = getelementptr i64, i64* %r3, i32 3
%r25 = load i64, i64* %r24
%r26 = zext i64 %r25 to i256
%r27 = shl i256 %r26, 192
%r28 = or i256 %r22, %r27
%r29 = zext i256 %r28 to i320
%r31 = getelementptr i64, i64* %r3, i32 4
%r32 = load i64, i64* %r31
%r33 = zext i64 %r32 to i320
%r34 = shl i320 %r33, 256
%r35 = or i320 %r29, %r34
%r36 = zext i320 %r35 to i384
%r38 = getelementptr i64, i64* %r3, i32 5
%r39 = load i64, i64* %r38
%r40 = zext i64 %r39 to i384
%r41 = shl i384 %r40, 320
%r42 = or i384 %r36, %r41
%r43 = load i64, i64* %r2
%r44 = zext i64 %r43 to i128
%r46 = getelementptr i64, i64* %r2, i32 1
%r47 = load i64, i64* %r46
%r48 = zext i64 %r47 to i128
%r49 = shl i128 %r48, 64
%r50 = or i128 %r44, %r49
%r51 = zext i128 %r50 to i192
%r53 = getelementptr i64, i64* %r2, i32 2
%r54 = load i64, i64* %r53
%r55 = zext i64 %r54 to i192
%r56 = shl i192 %r55, 128
%r57 = or i192 %r51, %r56
%r58 = zext i192 %r57 to i256
%r60 = getelementptr i64, i64* %r2, i32 3
%r61 = load i64, i64* %r60
%r62 = zext i64 %r61 to i256
%r63 = shl i256 %r62, 192
%r64 = or i256 %r58, %r63
%r65 = zext i256 %r64 to i320
%r67 = getelementptr i64, i64* %r2, i32 4
%r68 = load i64, i64* %r67
%r69 = zext i64 %r68 to i320
%r70 = shl i320 %r69, 256
%r71 = or i320 %r65, %r70
%r72 = zext i320 %r71 to i384
%r74 = getelementptr i64, i64* %r2, i32 5
%r75 = load i64, i64* %r74
%r76 = zext i64 %r75 to i384
%r77 = shl i384 %r76, 320
%r78 = or i384 %r72, %r77
%r79 = trunc i384 %r78 to i64
%r80 = mul i64 %r79, %r6
%r81 = call i448 @mulPv384x64(i64* %r3, i64 %r80)
%r83 = getelementptr i64, i64* %r2, i32 6
%r84 = load i64, i64* %r83
%r85 = zext i64 %r84 to i448
%r86 = shl i448 %r85, 384
%r87 = zext i384 %r78 to i448
%r88 = or i448 %r86, %r87
%r89 = zext i448 %r88 to i512
%r90 = zext i448 %r81 to i512
%r91 = add i512 %r89, %r90
%r92 = lshr i512 %r91, 64
%r93 = trunc i512 %r92 to i448
%r94 = lshr i448 %r93, 384
%r95 = trunc i448 %r94 to i64
%r96 = trunc i448 %r93 to i384
%r97 = trunc i384 %r96 to i64
%r98 = mul i64 %r97, %r6
%r99 = call i448 @mulPv384x64(i64* %r3, i64 %r98)
%r100 = zext i64 %r95 to i448
%r101 = shl i448 %r100, 384
%r102 = add i448 %r99, %r101
%r104 = getelementptr i64, i64* %r2, i32 7
%r105 = load i64, i64* %r104
%r106 = zext i64 %r105 to i448
%r107 = shl i448 %r106, 384
%r108 = zext i384 %r96 to i448
%r109 = or i448 %r107, %r108
%r110 = zext i448 %r109 to i512
%r111 = zext i448 %r102 to i512
%r112 = add i512 %r110, %r111
%r113 = lshr i512 %r112, 64
%r114 = trunc i512 %r113 to i448
%r115 = lshr i448 %r114, 384
%r116 = trunc i448 %r115 to i64
%r117 = trunc i448 %r114 to i384
%r118 = trunc i384 %r117 to i64
%r119 = mul i64 %r118, %r6
%r120 = call i448 @mulPv384x64(i64* %r3, i64 %r119)
%r121 = zext i64 %r116 to i448
%r122 = shl i448 %r121, 384
%r123 = add i448 %r120, %r122
%r125 = getelementptr i64, i64* %r2, i32 8
%r126 = load i64, i64* %r125
%r127 = zext i64 %r126 to i448
%r128 = shl i448 %r127, 384
%r129 = zext i384 %r117 to i448
%r130 = or i448 %r128, %r129
%r131 = zext i448 %r130 to i512
%r132 = zext i448 %r123 to i512
%r133 = add i512 %r131, %r132
%r134 = lshr i512 %r133, 64
%r135 = trunc i512 %r134 to i448
%r136 = lshr i448 %r135, 384
%r137 = trunc i448 %r136 to i64
%r138 = trunc i448 %r135 to i384
%r139 = trunc i384 %r138 to i64
%r140 = mul i64 %r139, %r6
%r141 = call i448 @mulPv384x64(i64* %r3, i64 %r140)
%r142 = zext i64 %r137 to i448
%r143 = shl i448 %r142, 384
%r144 = add i448 %r141, %r143
%r146 = getelementptr i64, i64* %r2, i32 9
%r147 = load i64, i64* %r146
%r148 = zext i64 %r147 to i448
%r149 = shl i448 %r148, 384
%r150 = zext i384 %r138 to i448
%r151 = or i448 %r149, %r150
%r152 = zext i448 %r151 to i512
%r153 = zext i448 %r144 to i512
%r154 = add i512 %r152, %r153
%r155 = lshr i512 %r154, 64
%r156 = trunc i512 %r155 to i448
%r157 = lshr i448 %r156, 384
%r158 = trunc i448 %r157 to i64
%r159 = trunc i448 %r156 to i384
%r160 = trunc i384 %r159 to i64
%r161 = mul i64 %r160, %r6
%r162 = call i448 @mulPv384x64(i64* %r3, i64 %r161)
%r163 = zext i64 %r158 to i448
%r164 = shl i448 %r163, 384
%r165 = add i448 %r162, %r164
%r167 = getelementptr i64, i64* %r2, i32 10
%r168 = load i64, i64* %r167
%r169 = zext i64 %r168 to i448
%r170 = shl i448 %r169, 384
%r171 = zext i384 %r159 to i448
%r172 = or i448 %r170, %r171
%r173 = zext i448 %r172 to i512
%r174 = zext i448 %r165 to i512
%r175 = add i512 %r173, %r174
%r176 = lshr i512 %r175, 64
%r177 = trunc i512 %r176 to i448
%r178 = lshr i448 %r177, 384
%r179 = trunc i448 %r178 to i64
%r180 = trunc i448 %r177 to i384
%r181 = trunc i384 %r180 to i64
%r182 = mul i64 %r181, %r6
%r183 = call i448 @mulPv384x64(i64* %r3, i64 %r182)
%r184 = zext i64 %r179 to i448
%r185 = shl i448 %r184, 384
%r186 = add i448 %r183, %r185
%r188 = getelementptr i64, i64* %r2, i32 11
%r189 = load i64, i64* %r188
%r190 = zext i64 %r189 to i448
%r191 = shl i448 %r190, 384
%r192 = zext i384 %r180 to i448
%r193 = or i448 %r191, %r192
%r194 = zext i448 %r193 to i512
%r195 = zext i448 %r186 to i512
%r196 = add i512 %r194, %r195
%r197 = lshr i512 %r196, 64
%r198 = trunc i512 %r197 to i448
%r199 = lshr i448 %r198, 384
%r200 = trunc i448 %r199 to i64
%r201 = trunc i448 %r198 to i384
%r202 = sub i384 %r201, %r42
%r203 = lshr i384 %r202, 383
%r204 = trunc i384 %r203 to i1
%r205 = select i1 %r204, i384 %r201, i384 %r202
%r207 = getelementptr i64, i64* %r1, i32 0
%r208 = trunc i384 %r205 to i64
store i64 %r208, i64* %r207
%r209 = lshr i384 %r205, 64
%r211 = getelementptr i64, i64* %r1, i32 1
%r212 = trunc i384 %r209 to i64
store i64 %r212, i64* %r211
%r213 = lshr i384 %r209, 64
%r215 = getelementptr i64, i64* %r1, i32 2
%r216 = trunc i384 %r213 to i64
store i64 %r216, i64* %r215
%r217 = lshr i384 %r213, 64
%r219 = getelementptr i64, i64* %r1, i32 3
%r220 = trunc i384 %r217 to i64
store i64 %r220, i64* %r219
%r221 = lshr i384 %r217, 64
%r223 = getelementptr i64, i64* %r1, i32 4
%r224 = trunc i384 %r221 to i64
store i64 %r224, i64* %r223
%r225 = lshr i384 %r221, 64
%r227 = getelementptr i64, i64* %r1, i32 5
%r228 = trunc i384 %r225 to i64
store i64 %r228, i64* %r227
ret void
}
define i64 @mcl_fp_addPre6L(i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r3
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r3, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r3, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r3, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r3, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r3, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r42 = load i64, i64* %r4
%r43 = zext i64 %r42 to i128
%r45 = getelementptr i64, i64* %r4, i32 1
%r46 = load i64, i64* %r45
%r47 = zext i64 %r46 to i128
%r48 = shl i128 %r47, 64
%r49 = or i128 %r43, %r48
%r50 = zext i128 %r49 to i192
%r52 = getelementptr i64, i64* %r4, i32 2
%r53 = load i64, i64* %r52
%r54 = zext i64 %r53 to i192
%r55 = shl i192 %r54, 128
%r56 = or i192 %r50, %r55
%r57 = zext i192 %r56 to i256
%r59 = getelementptr i64, i64* %r4, i32 3
%r60 = load i64, i64* %r59
%r61 = zext i64 %r60 to i256
%r62 = shl i256 %r61, 192
%r63 = or i256 %r57, %r62
%r64 = zext i256 %r63 to i320
%r66 = getelementptr i64, i64* %r4, i32 4
%r67 = load i64, i64* %r66
%r68 = zext i64 %r67 to i320
%r69 = shl i320 %r68, 256
%r70 = or i320 %r64, %r69
%r71 = zext i320 %r70 to i384
%r73 = getelementptr i64, i64* %r4, i32 5
%r74 = load i64, i64* %r73
%r75 = zext i64 %r74 to i384
%r76 = shl i384 %r75, 320
%r77 = or i384 %r71, %r76
%r78 = zext i384 %r77 to i448
%r79 = add i448 %r41, %r78
%r80 = trunc i448 %r79 to i384
%r82 = getelementptr i64, i64* %r2, i32 0
%r83 = trunc i384 %r80 to i64
store i64 %r83, i64* %r82
%r84 = lshr i384 %r80, 64
%r86 = getelementptr i64, i64* %r2, i32 1
%r87 = trunc i384 %r84 to i64
store i64 %r87, i64* %r86
%r88 = lshr i384 %r84, 64
%r90 = getelementptr i64, i64* %r2, i32 2
%r91 = trunc i384 %r88 to i64
store i64 %r91, i64* %r90
%r92 = lshr i384 %r88, 64
%r94 = getelementptr i64, i64* %r2, i32 3
%r95 = trunc i384 %r92 to i64
store i64 %r95, i64* %r94
%r96 = lshr i384 %r92, 64
%r98 = getelementptr i64, i64* %r2, i32 4
%r99 = trunc i384 %r96 to i64
store i64 %r99, i64* %r98
%r100 = lshr i384 %r96, 64
%r102 = getelementptr i64, i64* %r2, i32 5
%r103 = trunc i384 %r100 to i64
store i64 %r103, i64* %r102
%r104 = lshr i448 %r79, 384
%r105 = trunc i448 %r104 to i64
ret i64 %r105
}
define i64 @mcl_fp_subPre6L(i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r3
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r3, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r3, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r3, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r3, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r3, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r42 = load i64, i64* %r4
%r43 = zext i64 %r42 to i128
%r45 = getelementptr i64, i64* %r4, i32 1
%r46 = load i64, i64* %r45
%r47 = zext i64 %r46 to i128
%r48 = shl i128 %r47, 64
%r49 = or i128 %r43, %r48
%r50 = zext i128 %r49 to i192
%r52 = getelementptr i64, i64* %r4, i32 2
%r53 = load i64, i64* %r52
%r54 = zext i64 %r53 to i192
%r55 = shl i192 %r54, 128
%r56 = or i192 %r50, %r55
%r57 = zext i192 %r56 to i256
%r59 = getelementptr i64, i64* %r4, i32 3
%r60 = load i64, i64* %r59
%r61 = zext i64 %r60 to i256
%r62 = shl i256 %r61, 192
%r63 = or i256 %r57, %r62
%r64 = zext i256 %r63 to i320
%r66 = getelementptr i64, i64* %r4, i32 4
%r67 = load i64, i64* %r66
%r68 = zext i64 %r67 to i320
%r69 = shl i320 %r68, 256
%r70 = or i320 %r64, %r69
%r71 = zext i320 %r70 to i384
%r73 = getelementptr i64, i64* %r4, i32 5
%r74 = load i64, i64* %r73
%r75 = zext i64 %r74 to i384
%r76 = shl i384 %r75, 320
%r77 = or i384 %r71, %r76
%r78 = zext i384 %r77 to i448
%r79 = sub i448 %r41, %r78
%r80 = trunc i448 %r79 to i384
%r82 = getelementptr i64, i64* %r2, i32 0
%r83 = trunc i384 %r80 to i64
store i64 %r83, i64* %r82
%r84 = lshr i384 %r80, 64
%r86 = getelementptr i64, i64* %r2, i32 1
%r87 = trunc i384 %r84 to i64
store i64 %r87, i64* %r86
%r88 = lshr i384 %r84, 64
%r90 = getelementptr i64, i64* %r2, i32 2
%r91 = trunc i384 %r88 to i64
store i64 %r91, i64* %r90
%r92 = lshr i384 %r88, 64
%r94 = getelementptr i64, i64* %r2, i32 3
%r95 = trunc i384 %r92 to i64
store i64 %r95, i64* %r94
%r96 = lshr i384 %r92, 64
%r98 = getelementptr i64, i64* %r2, i32 4
%r99 = trunc i384 %r96 to i64
store i64 %r99, i64* %r98
%r100 = lshr i384 %r96, 64
%r102 = getelementptr i64, i64* %r2, i32 5
%r103 = trunc i384 %r100 to i64
store i64 %r103, i64* %r102
%r105 = lshr i448 %r79, 384
%r106 = trunc i448 %r105 to i64
%r107 = and i64 %r106, 1
ret i64 %r107
}
define void @mcl_fp_shr1_6L(i64* noalias  %r1, i64* noalias  %r2)
{
%r3 = load i64, i64* %r2
%r4 = zext i64 %r3 to i128
%r6 = getelementptr i64, i64* %r2, i32 1
%r7 = load i64, i64* %r6
%r8 = zext i64 %r7 to i128
%r9 = shl i128 %r8, 64
%r10 = or i128 %r4, %r9
%r11 = zext i128 %r10 to i192
%r13 = getelementptr i64, i64* %r2, i32 2
%r14 = load i64, i64* %r13
%r15 = zext i64 %r14 to i192
%r16 = shl i192 %r15, 128
%r17 = or i192 %r11, %r16
%r18 = zext i192 %r17 to i256
%r20 = getelementptr i64, i64* %r2, i32 3
%r21 = load i64, i64* %r20
%r22 = zext i64 %r21 to i256
%r23 = shl i256 %r22, 192
%r24 = or i256 %r18, %r23
%r25 = zext i256 %r24 to i320
%r27 = getelementptr i64, i64* %r2, i32 4
%r28 = load i64, i64* %r27
%r29 = zext i64 %r28 to i320
%r30 = shl i320 %r29, 256
%r31 = or i320 %r25, %r30
%r32 = zext i320 %r31 to i384
%r34 = getelementptr i64, i64* %r2, i32 5
%r35 = load i64, i64* %r34
%r36 = zext i64 %r35 to i384
%r37 = shl i384 %r36, 320
%r38 = or i384 %r32, %r37
%r39 = lshr i384 %r38, 1
%r41 = getelementptr i64, i64* %r1, i32 0
%r42 = trunc i384 %r39 to i64
store i64 %r42, i64* %r41
%r43 = lshr i384 %r39, 64
%r45 = getelementptr i64, i64* %r1, i32 1
%r46 = trunc i384 %r43 to i64
store i64 %r46, i64* %r45
%r47 = lshr i384 %r43, 64
%r49 = getelementptr i64, i64* %r1, i32 2
%r50 = trunc i384 %r47 to i64
store i64 %r50, i64* %r49
%r51 = lshr i384 %r47, 64
%r53 = getelementptr i64, i64* %r1, i32 3
%r54 = trunc i384 %r51 to i64
store i64 %r54, i64* %r53
%r55 = lshr i384 %r51, 64
%r57 = getelementptr i64, i64* %r1, i32 4
%r58 = trunc i384 %r55 to i64
store i64 %r58, i64* %r57
%r59 = lshr i384 %r55, 64
%r61 = getelementptr i64, i64* %r1, i32 5
%r62 = trunc i384 %r59 to i64
store i64 %r62, i64* %r61
ret void
}
define void @mcl_fp_add6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = load i64, i64* %r3
%r42 = zext i64 %r41 to i128
%r44 = getelementptr i64, i64* %r3, i32 1
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i128
%r47 = shl i128 %r46, 64
%r48 = or i128 %r42, %r47
%r49 = zext i128 %r48 to i192
%r51 = getelementptr i64, i64* %r3, i32 2
%r52 = load i64, i64* %r51
%r53 = zext i64 %r52 to i192
%r54 = shl i192 %r53, 128
%r55 = or i192 %r49, %r54
%r56 = zext i192 %r55 to i256
%r58 = getelementptr i64, i64* %r3, i32 3
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i256
%r61 = shl i256 %r60, 192
%r62 = or i256 %r56, %r61
%r63 = zext i256 %r62 to i320
%r65 = getelementptr i64, i64* %r3, i32 4
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i320
%r68 = shl i320 %r67, 256
%r69 = or i320 %r63, %r68
%r70 = zext i320 %r69 to i384
%r72 = getelementptr i64, i64* %r3, i32 5
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i384
%r75 = shl i384 %r74, 320
%r76 = or i384 %r70, %r75
%r77 = zext i384 %r40 to i448
%r78 = zext i384 %r76 to i448
%r79 = add i448 %r77, %r78
%r80 = trunc i448 %r79 to i384
%r82 = getelementptr i64, i64* %r1, i32 0
%r83 = trunc i384 %r80 to i64
store i64 %r83, i64* %r82
%r84 = lshr i384 %r80, 64
%r86 = getelementptr i64, i64* %r1, i32 1
%r87 = trunc i384 %r84 to i64
store i64 %r87, i64* %r86
%r88 = lshr i384 %r84, 64
%r90 = getelementptr i64, i64* %r1, i32 2
%r91 = trunc i384 %r88 to i64
store i64 %r91, i64* %r90
%r92 = lshr i384 %r88, 64
%r94 = getelementptr i64, i64* %r1, i32 3
%r95 = trunc i384 %r92 to i64
store i64 %r95, i64* %r94
%r96 = lshr i384 %r92, 64
%r98 = getelementptr i64, i64* %r1, i32 4
%r99 = trunc i384 %r96 to i64
store i64 %r99, i64* %r98
%r100 = lshr i384 %r96, 64
%r102 = getelementptr i64, i64* %r1, i32 5
%r103 = trunc i384 %r100 to i64
store i64 %r103, i64* %r102
%r104 = load i64, i64* %r4
%r105 = zext i64 %r104 to i128
%r107 = getelementptr i64, i64* %r4, i32 1
%r108 = load i64, i64* %r107
%r109 = zext i64 %r108 to i128
%r110 = shl i128 %r109, 64
%r111 = or i128 %r105, %r110
%r112 = zext i128 %r111 to i192
%r114 = getelementptr i64, i64* %r4, i32 2
%r115 = load i64, i64* %r114
%r116 = zext i64 %r115 to i192
%r117 = shl i192 %r116, 128
%r118 = or i192 %r112, %r117
%r119 = zext i192 %r118 to i256
%r121 = getelementptr i64, i64* %r4, i32 3
%r122 = load i64, i64* %r121
%r123 = zext i64 %r122 to i256
%r124 = shl i256 %r123, 192
%r125 = or i256 %r119, %r124
%r126 = zext i256 %r125 to i320
%r128 = getelementptr i64, i64* %r4, i32 4
%r129 = load i64, i64* %r128
%r130 = zext i64 %r129 to i320
%r131 = shl i320 %r130, 256
%r132 = or i320 %r126, %r131
%r133 = zext i320 %r132 to i384
%r135 = getelementptr i64, i64* %r4, i32 5
%r136 = load i64, i64* %r135
%r137 = zext i64 %r136 to i384
%r138 = shl i384 %r137, 320
%r139 = or i384 %r133, %r138
%r140 = zext i384 %r139 to i448
%r141 = sub i448 %r79, %r140
%r142 = lshr i448 %r141, 384
%r143 = trunc i448 %r142 to i1
br i1%r143, label %carry, label %nocarry
nocarry:
%r144 = trunc i448 %r141 to i384
%r146 = getelementptr i64, i64* %r1, i32 0
%r147 = trunc i384 %r144 to i64
store i64 %r147, i64* %r146
%r148 = lshr i384 %r144, 64
%r150 = getelementptr i64, i64* %r1, i32 1
%r151 = trunc i384 %r148 to i64
store i64 %r151, i64* %r150
%r152 = lshr i384 %r148, 64
%r154 = getelementptr i64, i64* %r1, i32 2
%r155 = trunc i384 %r152 to i64
store i64 %r155, i64* %r154
%r156 = lshr i384 %r152, 64
%r158 = getelementptr i64, i64* %r1, i32 3
%r159 = trunc i384 %r156 to i64
store i64 %r159, i64* %r158
%r160 = lshr i384 %r156, 64
%r162 = getelementptr i64, i64* %r1, i32 4
%r163 = trunc i384 %r160 to i64
store i64 %r163, i64* %r162
%r164 = lshr i384 %r160, 64
%r166 = getelementptr i64, i64* %r1, i32 5
%r167 = trunc i384 %r164 to i64
store i64 %r167, i64* %r166
ret void
carry:
ret void
}
define void @mcl_fp_addNF6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = load i64, i64* %r3
%r42 = zext i64 %r41 to i128
%r44 = getelementptr i64, i64* %r3, i32 1
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i128
%r47 = shl i128 %r46, 64
%r48 = or i128 %r42, %r47
%r49 = zext i128 %r48 to i192
%r51 = getelementptr i64, i64* %r3, i32 2
%r52 = load i64, i64* %r51
%r53 = zext i64 %r52 to i192
%r54 = shl i192 %r53, 128
%r55 = or i192 %r49, %r54
%r56 = zext i192 %r55 to i256
%r58 = getelementptr i64, i64* %r3, i32 3
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i256
%r61 = shl i256 %r60, 192
%r62 = or i256 %r56, %r61
%r63 = zext i256 %r62 to i320
%r65 = getelementptr i64, i64* %r3, i32 4
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i320
%r68 = shl i320 %r67, 256
%r69 = or i320 %r63, %r68
%r70 = zext i320 %r69 to i384
%r72 = getelementptr i64, i64* %r3, i32 5
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i384
%r75 = shl i384 %r74, 320
%r76 = or i384 %r70, %r75
%r77 = add i384 %r40, %r76
%r78 = load i64, i64* %r4
%r79 = zext i64 %r78 to i128
%r81 = getelementptr i64, i64* %r4, i32 1
%r82 = load i64, i64* %r81
%r83 = zext i64 %r82 to i128
%r84 = shl i128 %r83, 64
%r85 = or i128 %r79, %r84
%r86 = zext i128 %r85 to i192
%r88 = getelementptr i64, i64* %r4, i32 2
%r89 = load i64, i64* %r88
%r90 = zext i64 %r89 to i192
%r91 = shl i192 %r90, 128
%r92 = or i192 %r86, %r91
%r93 = zext i192 %r92 to i256
%r95 = getelementptr i64, i64* %r4, i32 3
%r96 = load i64, i64* %r95
%r97 = zext i64 %r96 to i256
%r98 = shl i256 %r97, 192
%r99 = or i256 %r93, %r98
%r100 = zext i256 %r99 to i320
%r102 = getelementptr i64, i64* %r4, i32 4
%r103 = load i64, i64* %r102
%r104 = zext i64 %r103 to i320
%r105 = shl i320 %r104, 256
%r106 = or i320 %r100, %r105
%r107 = zext i320 %r106 to i384
%r109 = getelementptr i64, i64* %r4, i32 5
%r110 = load i64, i64* %r109
%r111 = zext i64 %r110 to i384
%r112 = shl i384 %r111, 320
%r113 = or i384 %r107, %r112
%r114 = sub i384 %r77, %r113
%r115 = lshr i384 %r114, 383
%r116 = trunc i384 %r115 to i1
%r117 = select i1 %r116, i384 %r77, i384 %r114
%r119 = getelementptr i64, i64* %r1, i32 0
%r120 = trunc i384 %r117 to i64
store i64 %r120, i64* %r119
%r121 = lshr i384 %r117, 64
%r123 = getelementptr i64, i64* %r1, i32 1
%r124 = trunc i384 %r121 to i64
store i64 %r124, i64* %r123
%r125 = lshr i384 %r121, 64
%r127 = getelementptr i64, i64* %r1, i32 2
%r128 = trunc i384 %r125 to i64
store i64 %r128, i64* %r127
%r129 = lshr i384 %r125, 64
%r131 = getelementptr i64, i64* %r1, i32 3
%r132 = trunc i384 %r129 to i64
store i64 %r132, i64* %r131
%r133 = lshr i384 %r129, 64
%r135 = getelementptr i64, i64* %r1, i32 4
%r136 = trunc i384 %r133 to i64
store i64 %r136, i64* %r135
%r137 = lshr i384 %r133, 64
%r139 = getelementptr i64, i64* %r1, i32 5
%r140 = trunc i384 %r137 to i64
store i64 %r140, i64* %r139
ret void
}
define void @mcl_fp_sub6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = load i64, i64* %r3
%r42 = zext i64 %r41 to i128
%r44 = getelementptr i64, i64* %r3, i32 1
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i128
%r47 = shl i128 %r46, 64
%r48 = or i128 %r42, %r47
%r49 = zext i128 %r48 to i192
%r51 = getelementptr i64, i64* %r3, i32 2
%r52 = load i64, i64* %r51
%r53 = zext i64 %r52 to i192
%r54 = shl i192 %r53, 128
%r55 = or i192 %r49, %r54
%r56 = zext i192 %r55 to i256
%r58 = getelementptr i64, i64* %r3, i32 3
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i256
%r61 = shl i256 %r60, 192
%r62 = or i256 %r56, %r61
%r63 = zext i256 %r62 to i320
%r65 = getelementptr i64, i64* %r3, i32 4
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i320
%r68 = shl i320 %r67, 256
%r69 = or i320 %r63, %r68
%r70 = zext i320 %r69 to i384
%r72 = getelementptr i64, i64* %r3, i32 5
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i384
%r75 = shl i384 %r74, 320
%r76 = or i384 %r70, %r75
%r77 = zext i384 %r40 to i448
%r78 = zext i384 %r76 to i448
%r79 = sub i448 %r77, %r78
%r80 = trunc i448 %r79 to i384
%r81 = lshr i448 %r79, 384
%r82 = trunc i448 %r81 to i1
%r84 = getelementptr i64, i64* %r1, i32 0
%r85 = trunc i384 %r80 to i64
store i64 %r85, i64* %r84
%r86 = lshr i384 %r80, 64
%r88 = getelementptr i64, i64* %r1, i32 1
%r89 = trunc i384 %r86 to i64
store i64 %r89, i64* %r88
%r90 = lshr i384 %r86, 64
%r92 = getelementptr i64, i64* %r1, i32 2
%r93 = trunc i384 %r90 to i64
store i64 %r93, i64* %r92
%r94 = lshr i384 %r90, 64
%r96 = getelementptr i64, i64* %r1, i32 3
%r97 = trunc i384 %r94 to i64
store i64 %r97, i64* %r96
%r98 = lshr i384 %r94, 64
%r100 = getelementptr i64, i64* %r1, i32 4
%r101 = trunc i384 %r98 to i64
store i64 %r101, i64* %r100
%r102 = lshr i384 %r98, 64
%r104 = getelementptr i64, i64* %r1, i32 5
%r105 = trunc i384 %r102 to i64
store i64 %r105, i64* %r104
br i1%r82, label %carry, label %nocarry
nocarry:
ret void
carry:
%r106 = load i64, i64* %r4
%r107 = zext i64 %r106 to i128
%r109 = getelementptr i64, i64* %r4, i32 1
%r110 = load i64, i64* %r109
%r111 = zext i64 %r110 to i128
%r112 = shl i128 %r111, 64
%r113 = or i128 %r107, %r112
%r114 = zext i128 %r113 to i192
%r116 = getelementptr i64, i64* %r4, i32 2
%r117 = load i64, i64* %r116
%r118 = zext i64 %r117 to i192
%r119 = shl i192 %r118, 128
%r120 = or i192 %r114, %r119
%r121 = zext i192 %r120 to i256
%r123 = getelementptr i64, i64* %r4, i32 3
%r124 = load i64, i64* %r123
%r125 = zext i64 %r124 to i256
%r126 = shl i256 %r125, 192
%r127 = or i256 %r121, %r126
%r128 = zext i256 %r127 to i320
%r130 = getelementptr i64, i64* %r4, i32 4
%r131 = load i64, i64* %r130
%r132 = zext i64 %r131 to i320
%r133 = shl i320 %r132, 256
%r134 = or i320 %r128, %r133
%r135 = zext i320 %r134 to i384
%r137 = getelementptr i64, i64* %r4, i32 5
%r138 = load i64, i64* %r137
%r139 = zext i64 %r138 to i384
%r140 = shl i384 %r139, 320
%r141 = or i384 %r135, %r140
%r142 = add i384 %r80, %r141
%r144 = getelementptr i64, i64* %r1, i32 0
%r145 = trunc i384 %r142 to i64
store i64 %r145, i64* %r144
%r146 = lshr i384 %r142, 64
%r148 = getelementptr i64, i64* %r1, i32 1
%r149 = trunc i384 %r146 to i64
store i64 %r149, i64* %r148
%r150 = lshr i384 %r146, 64
%r152 = getelementptr i64, i64* %r1, i32 2
%r153 = trunc i384 %r150 to i64
store i64 %r153, i64* %r152
%r154 = lshr i384 %r150, 64
%r156 = getelementptr i64, i64* %r1, i32 3
%r157 = trunc i384 %r154 to i64
store i64 %r157, i64* %r156
%r158 = lshr i384 %r154, 64
%r160 = getelementptr i64, i64* %r1, i32 4
%r161 = trunc i384 %r158 to i64
store i64 %r161, i64* %r160
%r162 = lshr i384 %r158, 64
%r164 = getelementptr i64, i64* %r1, i32 5
%r165 = trunc i384 %r162 to i64
store i64 %r165, i64* %r164
ret void
}
define void @mcl_fp_subNF6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = load i64, i64* %r3
%r42 = zext i64 %r41 to i128
%r44 = getelementptr i64, i64* %r3, i32 1
%r45 = load i64, i64* %r44
%r46 = zext i64 %r45 to i128
%r47 = shl i128 %r46, 64
%r48 = or i128 %r42, %r47
%r49 = zext i128 %r48 to i192
%r51 = getelementptr i64, i64* %r3, i32 2
%r52 = load i64, i64* %r51
%r53 = zext i64 %r52 to i192
%r54 = shl i192 %r53, 128
%r55 = or i192 %r49, %r54
%r56 = zext i192 %r55 to i256
%r58 = getelementptr i64, i64* %r3, i32 3
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i256
%r61 = shl i256 %r60, 192
%r62 = or i256 %r56, %r61
%r63 = zext i256 %r62 to i320
%r65 = getelementptr i64, i64* %r3, i32 4
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i320
%r68 = shl i320 %r67, 256
%r69 = or i320 %r63, %r68
%r70 = zext i320 %r69 to i384
%r72 = getelementptr i64, i64* %r3, i32 5
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i384
%r75 = shl i384 %r74, 320
%r76 = or i384 %r70, %r75
%r77 = sub i384 %r40, %r76
%r78 = lshr i384 %r77, 383
%r79 = trunc i384 %r78 to i1
%r80 = load i64, i64* %r4
%r81 = zext i64 %r80 to i128
%r83 = getelementptr i64, i64* %r4, i32 1
%r84 = load i64, i64* %r83
%r85 = zext i64 %r84 to i128
%r86 = shl i128 %r85, 64
%r87 = or i128 %r81, %r86
%r88 = zext i128 %r87 to i192
%r90 = getelementptr i64, i64* %r4, i32 2
%r91 = load i64, i64* %r90
%r92 = zext i64 %r91 to i192
%r93 = shl i192 %r92, 128
%r94 = or i192 %r88, %r93
%r95 = zext i192 %r94 to i256
%r97 = getelementptr i64, i64* %r4, i32 3
%r98 = load i64, i64* %r97
%r99 = zext i64 %r98 to i256
%r100 = shl i256 %r99, 192
%r101 = or i256 %r95, %r100
%r102 = zext i256 %r101 to i320
%r104 = getelementptr i64, i64* %r4, i32 4
%r105 = load i64, i64* %r104
%r106 = zext i64 %r105 to i320
%r107 = shl i320 %r106, 256
%r108 = or i320 %r102, %r107
%r109 = zext i320 %r108 to i384
%r111 = getelementptr i64, i64* %r4, i32 5
%r112 = load i64, i64* %r111
%r113 = zext i64 %r112 to i384
%r114 = shl i384 %r113, 320
%r115 = or i384 %r109, %r114
%r117 = select i1 %r79, i384 %r115, i384 0
%r118 = add i384 %r77, %r117
%r120 = getelementptr i64, i64* %r1, i32 0
%r121 = trunc i384 %r118 to i64
store i64 %r121, i64* %r120
%r122 = lshr i384 %r118, 64
%r124 = getelementptr i64, i64* %r1, i32 1
%r125 = trunc i384 %r122 to i64
store i64 %r125, i64* %r124
%r126 = lshr i384 %r122, 64
%r128 = getelementptr i64, i64* %r1, i32 2
%r129 = trunc i384 %r126 to i64
store i64 %r129, i64* %r128
%r130 = lshr i384 %r126, 64
%r132 = getelementptr i64, i64* %r1, i32 3
%r133 = trunc i384 %r130 to i64
store i64 %r133, i64* %r132
%r134 = lshr i384 %r130, 64
%r136 = getelementptr i64, i64* %r1, i32 4
%r137 = trunc i384 %r134 to i64
store i64 %r137, i64* %r136
%r138 = lshr i384 %r134, 64
%r140 = getelementptr i64, i64* %r1, i32 5
%r141 = trunc i384 %r138 to i64
store i64 %r141, i64* %r140
ret void
}
define void @mcl_fpDbl_add6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = zext i512 %r54 to i576
%r57 = getelementptr i64, i64* %r2, i32 8
%r58 = load i64, i64* %r57
%r59 = zext i64 %r58 to i576
%r60 = shl i576 %r59, 512
%r61 = or i576 %r55, %r60
%r62 = zext i576 %r61 to i640
%r64 = getelementptr i64, i64* %r2, i32 9
%r65 = load i64, i64* %r64
%r66 = zext i64 %r65 to i640
%r67 = shl i640 %r66, 576
%r68 = or i640 %r62, %r67
%r69 = zext i640 %r68 to i704
%r71 = getelementptr i64, i64* %r2, i32 10
%r72 = load i64, i64* %r71
%r73 = zext i64 %r72 to i704
%r74 = shl i704 %r73, 640
%r75 = or i704 %r69, %r74
%r76 = zext i704 %r75 to i768
%r78 = getelementptr i64, i64* %r2, i32 11
%r79 = load i64, i64* %r78
%r80 = zext i64 %r79 to i768
%r81 = shl i768 %r80, 704
%r82 = or i768 %r76, %r81
%r83 = load i64, i64* %r3
%r84 = zext i64 %r83 to i128
%r86 = getelementptr i64, i64* %r3, i32 1
%r87 = load i64, i64* %r86
%r88 = zext i64 %r87 to i128
%r89 = shl i128 %r88, 64
%r90 = or i128 %r84, %r89
%r91 = zext i128 %r90 to i192
%r93 = getelementptr i64, i64* %r3, i32 2
%r94 = load i64, i64* %r93
%r95 = zext i64 %r94 to i192
%r96 = shl i192 %r95, 128
%r97 = or i192 %r91, %r96
%r98 = zext i192 %r97 to i256
%r100 = getelementptr i64, i64* %r3, i32 3
%r101 = load i64, i64* %r100
%r102 = zext i64 %r101 to i256
%r103 = shl i256 %r102, 192
%r104 = or i256 %r98, %r103
%r105 = zext i256 %r104 to i320
%r107 = getelementptr i64, i64* %r3, i32 4
%r108 = load i64, i64* %r107
%r109 = zext i64 %r108 to i320
%r110 = shl i320 %r109, 256
%r111 = or i320 %r105, %r110
%r112 = zext i320 %r111 to i384
%r114 = getelementptr i64, i64* %r3, i32 5
%r115 = load i64, i64* %r114
%r116 = zext i64 %r115 to i384
%r117 = shl i384 %r116, 320
%r118 = or i384 %r112, %r117
%r119 = zext i384 %r118 to i448
%r121 = getelementptr i64, i64* %r3, i32 6
%r122 = load i64, i64* %r121
%r123 = zext i64 %r122 to i448
%r124 = shl i448 %r123, 384
%r125 = or i448 %r119, %r124
%r126 = zext i448 %r125 to i512
%r128 = getelementptr i64, i64* %r3, i32 7
%r129 = load i64, i64* %r128
%r130 = zext i64 %r129 to i512
%r131 = shl i512 %r130, 448
%r132 = or i512 %r126, %r131
%r133 = zext i512 %r132 to i576
%r135 = getelementptr i64, i64* %r3, i32 8
%r136 = load i64, i64* %r135
%r137 = zext i64 %r136 to i576
%r138 = shl i576 %r137, 512
%r139 = or i576 %r133, %r138
%r140 = zext i576 %r139 to i640
%r142 = getelementptr i64, i64* %r3, i32 9
%r143 = load i64, i64* %r142
%r144 = zext i64 %r143 to i640
%r145 = shl i640 %r144, 576
%r146 = or i640 %r140, %r145
%r147 = zext i640 %r146 to i704
%r149 = getelementptr i64, i64* %r3, i32 10
%r150 = load i64, i64* %r149
%r151 = zext i64 %r150 to i704
%r152 = shl i704 %r151, 640
%r153 = or i704 %r147, %r152
%r154 = zext i704 %r153 to i768
%r156 = getelementptr i64, i64* %r3, i32 11
%r157 = load i64, i64* %r156
%r158 = zext i64 %r157 to i768
%r159 = shl i768 %r158, 704
%r160 = or i768 %r154, %r159
%r161 = zext i768 %r82 to i832
%r162 = zext i768 %r160 to i832
%r163 = add i832 %r161, %r162
%r164 = trunc i832 %r163 to i384
%r166 = getelementptr i64, i64* %r1, i32 0
%r167 = trunc i384 %r164 to i64
store i64 %r167, i64* %r166
%r168 = lshr i384 %r164, 64
%r170 = getelementptr i64, i64* %r1, i32 1
%r171 = trunc i384 %r168 to i64
store i64 %r171, i64* %r170
%r172 = lshr i384 %r168, 64
%r174 = getelementptr i64, i64* %r1, i32 2
%r175 = trunc i384 %r172 to i64
store i64 %r175, i64* %r174
%r176 = lshr i384 %r172, 64
%r178 = getelementptr i64, i64* %r1, i32 3
%r179 = trunc i384 %r176 to i64
store i64 %r179, i64* %r178
%r180 = lshr i384 %r176, 64
%r182 = getelementptr i64, i64* %r1, i32 4
%r183 = trunc i384 %r180 to i64
store i64 %r183, i64* %r182
%r184 = lshr i384 %r180, 64
%r186 = getelementptr i64, i64* %r1, i32 5
%r187 = trunc i384 %r184 to i64
store i64 %r187, i64* %r186
%r188 = lshr i832 %r163, 384
%r189 = trunc i832 %r188 to i448
%r190 = load i64, i64* %r4
%r191 = zext i64 %r190 to i128
%r193 = getelementptr i64, i64* %r4, i32 1
%r194 = load i64, i64* %r193
%r195 = zext i64 %r194 to i128
%r196 = shl i128 %r195, 64
%r197 = or i128 %r191, %r196
%r198 = zext i128 %r197 to i192
%r200 = getelementptr i64, i64* %r4, i32 2
%r201 = load i64, i64* %r200
%r202 = zext i64 %r201 to i192
%r203 = shl i192 %r202, 128
%r204 = or i192 %r198, %r203
%r205 = zext i192 %r204 to i256
%r207 = getelementptr i64, i64* %r4, i32 3
%r208 = load i64, i64* %r207
%r209 = zext i64 %r208 to i256
%r210 = shl i256 %r209, 192
%r211 = or i256 %r205, %r210
%r212 = zext i256 %r211 to i320
%r214 = getelementptr i64, i64* %r4, i32 4
%r215 = load i64, i64* %r214
%r216 = zext i64 %r215 to i320
%r217 = shl i320 %r216, 256
%r218 = or i320 %r212, %r217
%r219 = zext i320 %r218 to i384
%r221 = getelementptr i64, i64* %r4, i32 5
%r222 = load i64, i64* %r221
%r223 = zext i64 %r222 to i384
%r224 = shl i384 %r223, 320
%r225 = or i384 %r219, %r224
%r226 = zext i384 %r225 to i448
%r227 = sub i448 %r189, %r226
%r228 = lshr i448 %r227, 384
%r229 = trunc i448 %r228 to i1
%r230 = select i1 %r229, i448 %r189, i448 %r227
%r231 = trunc i448 %r230 to i384
%r233 = getelementptr i64, i64* %r1, i32 6
%r235 = getelementptr i64, i64* %r233, i32 0
%r236 = trunc i384 %r231 to i64
store i64 %r236, i64* %r235
%r237 = lshr i384 %r231, 64
%r239 = getelementptr i64, i64* %r233, i32 1
%r240 = trunc i384 %r237 to i64
store i64 %r240, i64* %r239
%r241 = lshr i384 %r237, 64
%r243 = getelementptr i64, i64* %r233, i32 2
%r244 = trunc i384 %r241 to i64
store i64 %r244, i64* %r243
%r245 = lshr i384 %r241, 64
%r247 = getelementptr i64, i64* %r233, i32 3
%r248 = trunc i384 %r245 to i64
store i64 %r248, i64* %r247
%r249 = lshr i384 %r245, 64
%r251 = getelementptr i64, i64* %r233, i32 4
%r252 = trunc i384 %r249 to i64
store i64 %r252, i64* %r251
%r253 = lshr i384 %r249, 64
%r255 = getelementptr i64, i64* %r233, i32 5
%r256 = trunc i384 %r253 to i64
store i64 %r256, i64* %r255
ret void
}
define void @mcl_fpDbl_sub6L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = zext i512 %r54 to i576
%r57 = getelementptr i64, i64* %r2, i32 8
%r58 = load i64, i64* %r57
%r59 = zext i64 %r58 to i576
%r60 = shl i576 %r59, 512
%r61 = or i576 %r55, %r60
%r62 = zext i576 %r61 to i640
%r64 = getelementptr i64, i64* %r2, i32 9
%r65 = load i64, i64* %r64
%r66 = zext i64 %r65 to i640
%r67 = shl i640 %r66, 576
%r68 = or i640 %r62, %r67
%r69 = zext i640 %r68 to i704
%r71 = getelementptr i64, i64* %r2, i32 10
%r72 = load i64, i64* %r71
%r73 = zext i64 %r72 to i704
%r74 = shl i704 %r73, 640
%r75 = or i704 %r69, %r74
%r76 = zext i704 %r75 to i768
%r78 = getelementptr i64, i64* %r2, i32 11
%r79 = load i64, i64* %r78
%r80 = zext i64 %r79 to i768
%r81 = shl i768 %r80, 704
%r82 = or i768 %r76, %r81
%r83 = load i64, i64* %r3
%r84 = zext i64 %r83 to i128
%r86 = getelementptr i64, i64* %r3, i32 1
%r87 = load i64, i64* %r86
%r88 = zext i64 %r87 to i128
%r89 = shl i128 %r88, 64
%r90 = or i128 %r84, %r89
%r91 = zext i128 %r90 to i192
%r93 = getelementptr i64, i64* %r3, i32 2
%r94 = load i64, i64* %r93
%r95 = zext i64 %r94 to i192
%r96 = shl i192 %r95, 128
%r97 = or i192 %r91, %r96
%r98 = zext i192 %r97 to i256
%r100 = getelementptr i64, i64* %r3, i32 3
%r101 = load i64, i64* %r100
%r102 = zext i64 %r101 to i256
%r103 = shl i256 %r102, 192
%r104 = or i256 %r98, %r103
%r105 = zext i256 %r104 to i320
%r107 = getelementptr i64, i64* %r3, i32 4
%r108 = load i64, i64* %r107
%r109 = zext i64 %r108 to i320
%r110 = shl i320 %r109, 256
%r111 = or i320 %r105, %r110
%r112 = zext i320 %r111 to i384
%r114 = getelementptr i64, i64* %r3, i32 5
%r115 = load i64, i64* %r114
%r116 = zext i64 %r115 to i384
%r117 = shl i384 %r116, 320
%r118 = or i384 %r112, %r117
%r119 = zext i384 %r118 to i448
%r121 = getelementptr i64, i64* %r3, i32 6
%r122 = load i64, i64* %r121
%r123 = zext i64 %r122 to i448
%r124 = shl i448 %r123, 384
%r125 = or i448 %r119, %r124
%r126 = zext i448 %r125 to i512
%r128 = getelementptr i64, i64* %r3, i32 7
%r129 = load i64, i64* %r128
%r130 = zext i64 %r129 to i512
%r131 = shl i512 %r130, 448
%r132 = or i512 %r126, %r131
%r133 = zext i512 %r132 to i576
%r135 = getelementptr i64, i64* %r3, i32 8
%r136 = load i64, i64* %r135
%r137 = zext i64 %r136 to i576
%r138 = shl i576 %r137, 512
%r139 = or i576 %r133, %r138
%r140 = zext i576 %r139 to i640
%r142 = getelementptr i64, i64* %r3, i32 9
%r143 = load i64, i64* %r142
%r144 = zext i64 %r143 to i640
%r145 = shl i640 %r144, 576
%r146 = or i640 %r140, %r145
%r147 = zext i640 %r146 to i704
%r149 = getelementptr i64, i64* %r3, i32 10
%r150 = load i64, i64* %r149
%r151 = zext i64 %r150 to i704
%r152 = shl i704 %r151, 640
%r153 = or i704 %r147, %r152
%r154 = zext i704 %r153 to i768
%r156 = getelementptr i64, i64* %r3, i32 11
%r157 = load i64, i64* %r156
%r158 = zext i64 %r157 to i768
%r159 = shl i768 %r158, 704
%r160 = or i768 %r154, %r159
%r161 = zext i768 %r82 to i832
%r162 = zext i768 %r160 to i832
%r163 = sub i832 %r161, %r162
%r164 = trunc i832 %r163 to i384
%r166 = getelementptr i64, i64* %r1, i32 0
%r167 = trunc i384 %r164 to i64
store i64 %r167, i64* %r166
%r168 = lshr i384 %r164, 64
%r170 = getelementptr i64, i64* %r1, i32 1
%r171 = trunc i384 %r168 to i64
store i64 %r171, i64* %r170
%r172 = lshr i384 %r168, 64
%r174 = getelementptr i64, i64* %r1, i32 2
%r175 = trunc i384 %r172 to i64
store i64 %r175, i64* %r174
%r176 = lshr i384 %r172, 64
%r178 = getelementptr i64, i64* %r1, i32 3
%r179 = trunc i384 %r176 to i64
store i64 %r179, i64* %r178
%r180 = lshr i384 %r176, 64
%r182 = getelementptr i64, i64* %r1, i32 4
%r183 = trunc i384 %r180 to i64
store i64 %r183, i64* %r182
%r184 = lshr i384 %r180, 64
%r186 = getelementptr i64, i64* %r1, i32 5
%r187 = trunc i384 %r184 to i64
store i64 %r187, i64* %r186
%r188 = lshr i832 %r163, 384
%r189 = trunc i832 %r188 to i384
%r190 = lshr i832 %r163, 768
%r191 = trunc i832 %r190 to i1
%r192 = load i64, i64* %r4
%r193 = zext i64 %r192 to i128
%r195 = getelementptr i64, i64* %r4, i32 1
%r196 = load i64, i64* %r195
%r197 = zext i64 %r196 to i128
%r198 = shl i128 %r197, 64
%r199 = or i128 %r193, %r198
%r200 = zext i128 %r199 to i192
%r202 = getelementptr i64, i64* %r4, i32 2
%r203 = load i64, i64* %r202
%r204 = zext i64 %r203 to i192
%r205 = shl i192 %r204, 128
%r206 = or i192 %r200, %r205
%r207 = zext i192 %r206 to i256
%r209 = getelementptr i64, i64* %r4, i32 3
%r210 = load i64, i64* %r209
%r211 = zext i64 %r210 to i256
%r212 = shl i256 %r211, 192
%r213 = or i256 %r207, %r212
%r214 = zext i256 %r213 to i320
%r216 = getelementptr i64, i64* %r4, i32 4
%r217 = load i64, i64* %r216
%r218 = zext i64 %r217 to i320
%r219 = shl i320 %r218, 256
%r220 = or i320 %r214, %r219
%r221 = zext i320 %r220 to i384
%r223 = getelementptr i64, i64* %r4, i32 5
%r224 = load i64, i64* %r223
%r225 = zext i64 %r224 to i384
%r226 = shl i384 %r225, 320
%r227 = or i384 %r221, %r226
%r229 = select i1 %r191, i384 %r227, i384 0
%r230 = add i384 %r189, %r229
%r232 = getelementptr i64, i64* %r1, i32 6
%r234 = getelementptr i64, i64* %r232, i32 0
%r235 = trunc i384 %r230 to i64
store i64 %r235, i64* %r234
%r236 = lshr i384 %r230, 64
%r238 = getelementptr i64, i64* %r232, i32 1
%r239 = trunc i384 %r236 to i64
store i64 %r239, i64* %r238
%r240 = lshr i384 %r236, 64
%r242 = getelementptr i64, i64* %r232, i32 2
%r243 = trunc i384 %r240 to i64
store i64 %r243, i64* %r242
%r244 = lshr i384 %r240, 64
%r246 = getelementptr i64, i64* %r232, i32 3
%r247 = trunc i384 %r244 to i64
store i64 %r247, i64* %r246
%r248 = lshr i384 %r244, 64
%r250 = getelementptr i64, i64* %r232, i32 4
%r251 = trunc i384 %r248 to i64
store i64 %r251, i64* %r250
%r252 = lshr i384 %r248, 64
%r254 = getelementptr i64, i64* %r232, i32 5
%r255 = trunc i384 %r252 to i64
store i64 %r255, i64* %r254
ret void
}
define i576 @mulPv512x64(i64* noalias  %r2, i64 %r3)
{
%r5 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 0)
%r6 = trunc i128 %r5 to i64
%r7 = call i64 @extractHigh64(i128 %r5)
%r9 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 1)
%r10 = trunc i128 %r9 to i64
%r11 = call i64 @extractHigh64(i128 %r9)
%r13 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 2)
%r14 = trunc i128 %r13 to i64
%r15 = call i64 @extractHigh64(i128 %r13)
%r17 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 3)
%r18 = trunc i128 %r17 to i64
%r19 = call i64 @extractHigh64(i128 %r17)
%r21 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 4)
%r22 = trunc i128 %r21 to i64
%r23 = call i64 @extractHigh64(i128 %r21)
%r25 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 5)
%r26 = trunc i128 %r25 to i64
%r27 = call i64 @extractHigh64(i128 %r25)
%r29 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 6)
%r30 = trunc i128 %r29 to i64
%r31 = call i64 @extractHigh64(i128 %r29)
%r33 = call i128 @mulPos64x64(i64* %r2, i64 %r3, i64 7)
%r34 = trunc i128 %r33 to i64
%r35 = call i64 @extractHigh64(i128 %r33)
%r36 = zext i64 %r6 to i128
%r37 = zext i64 %r10 to i128
%r38 = shl i128 %r37, 64
%r39 = or i128 %r36, %r38
%r40 = zext i128 %r39 to i192
%r41 = zext i64 %r14 to i192
%r42 = shl i192 %r41, 128
%r43 = or i192 %r40, %r42
%r44 = zext i192 %r43 to i256
%r45 = zext i64 %r18 to i256
%r46 = shl i256 %r45, 192
%r47 = or i256 %r44, %r46
%r48 = zext i256 %r47 to i320
%r49 = zext i64 %r22 to i320
%r50 = shl i320 %r49, 256
%r51 = or i320 %r48, %r50
%r52 = zext i320 %r51 to i384
%r53 = zext i64 %r26 to i384
%r54 = shl i384 %r53, 320
%r55 = or i384 %r52, %r54
%r56 = zext i384 %r55 to i448
%r57 = zext i64 %r30 to i448
%r58 = shl i448 %r57, 384
%r59 = or i448 %r56, %r58
%r60 = zext i448 %r59 to i512
%r61 = zext i64 %r34 to i512
%r62 = shl i512 %r61, 448
%r63 = or i512 %r60, %r62
%r64 = zext i64 %r7 to i128
%r65 = zext i64 %r11 to i128
%r66 = shl i128 %r65, 64
%r67 = or i128 %r64, %r66
%r68 = zext i128 %r67 to i192
%r69 = zext i64 %r15 to i192
%r70 = shl i192 %r69, 128
%r71 = or i192 %r68, %r70
%r72 = zext i192 %r71 to i256
%r73 = zext i64 %r19 to i256
%r74 = shl i256 %r73, 192
%r75 = or i256 %r72, %r74
%r76 = zext i256 %r75 to i320
%r77 = zext i64 %r23 to i320
%r78 = shl i320 %r77, 256
%r79 = or i320 %r76, %r78
%r80 = zext i320 %r79 to i384
%r81 = zext i64 %r27 to i384
%r82 = shl i384 %r81, 320
%r83 = or i384 %r80, %r82
%r84 = zext i384 %r83 to i448
%r85 = zext i64 %r31 to i448
%r86 = shl i448 %r85, 384
%r87 = or i448 %r84, %r86
%r88 = zext i448 %r87 to i512
%r89 = zext i64 %r35 to i512
%r90 = shl i512 %r89, 448
%r91 = or i512 %r88, %r90
%r92 = zext i512 %r63 to i576
%r93 = zext i512 %r91 to i576
%r94 = shl i576 %r93, 64
%r95 = add i576 %r92, %r94
ret i576 %r95
}
define void @mcl_fp_mulUnitPre8L(i64* noalias  %r1, i64* noalias  %r2, i64 %r3)
{
%r4 = call i576 @mulPv512x64(i64* %r2, i64 %r3)
%r6 = getelementptr i64, i64* %r1, i32 0
%r7 = trunc i576 %r4 to i64
store i64 %r7, i64* %r6
%r8 = lshr i576 %r4, 64
%r10 = getelementptr i64, i64* %r1, i32 1
%r11 = trunc i576 %r8 to i64
store i64 %r11, i64* %r10
%r12 = lshr i576 %r8, 64
%r14 = getelementptr i64, i64* %r1, i32 2
%r15 = trunc i576 %r12 to i64
store i64 %r15, i64* %r14
%r16 = lshr i576 %r12, 64
%r18 = getelementptr i64, i64* %r1, i32 3
%r19 = trunc i576 %r16 to i64
store i64 %r19, i64* %r18
%r20 = lshr i576 %r16, 64
%r22 = getelementptr i64, i64* %r1, i32 4
%r23 = trunc i576 %r20 to i64
store i64 %r23, i64* %r22
%r24 = lshr i576 %r20, 64
%r26 = getelementptr i64, i64* %r1, i32 5
%r27 = trunc i576 %r24 to i64
store i64 %r27, i64* %r26
%r28 = lshr i576 %r24, 64
%r30 = getelementptr i64, i64* %r1, i32 6
%r31 = trunc i576 %r28 to i64
store i64 %r31, i64* %r30
%r32 = lshr i576 %r28, 64
%r34 = getelementptr i64, i64* %r1, i32 7
%r35 = trunc i576 %r32 to i64
store i64 %r35, i64* %r34
%r36 = lshr i576 %r32, 64
%r38 = getelementptr i64, i64* %r1, i32 8
%r39 = trunc i576 %r36 to i64
store i64 %r39, i64* %r38
ret void
}
define void @mcl_fpDbl_mulPre8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r4 = load i64, i64* %r3
%r5 = call i576 @mulPv512x64(i64* %r2, i64 %r4)
%r6 = trunc i576 %r5 to i64
store i64 %r6, i64* %r1
%r7 = lshr i576 %r5, 64
%r9 = getelementptr i64, i64* %r3, i32 1
%r10 = load i64, i64* %r9
%r11 = call i576 @mulPv512x64(i64* %r2, i64 %r10)
%r12 = add i576 %r7, %r11
%r13 = trunc i576 %r12 to i64
%r15 = getelementptr i64, i64* %r1, i32 1
store i64 %r13, i64* %r15
%r16 = lshr i576 %r12, 64
%r18 = getelementptr i64, i64* %r3, i32 2
%r19 = load i64, i64* %r18
%r20 = call i576 @mulPv512x64(i64* %r2, i64 %r19)
%r21 = add i576 %r16, %r20
%r22 = trunc i576 %r21 to i64
%r24 = getelementptr i64, i64* %r1, i32 2
store i64 %r22, i64* %r24
%r25 = lshr i576 %r21, 64
%r27 = getelementptr i64, i64* %r3, i32 3
%r28 = load i64, i64* %r27
%r29 = call i576 @mulPv512x64(i64* %r2, i64 %r28)
%r30 = add i576 %r25, %r29
%r31 = trunc i576 %r30 to i64
%r33 = getelementptr i64, i64* %r1, i32 3
store i64 %r31, i64* %r33
%r34 = lshr i576 %r30, 64
%r36 = getelementptr i64, i64* %r3, i32 4
%r37 = load i64, i64* %r36
%r38 = call i576 @mulPv512x64(i64* %r2, i64 %r37)
%r39 = add i576 %r34, %r38
%r40 = trunc i576 %r39 to i64
%r42 = getelementptr i64, i64* %r1, i32 4
store i64 %r40, i64* %r42
%r43 = lshr i576 %r39, 64
%r45 = getelementptr i64, i64* %r3, i32 5
%r46 = load i64, i64* %r45
%r47 = call i576 @mulPv512x64(i64* %r2, i64 %r46)
%r48 = add i576 %r43, %r47
%r49 = trunc i576 %r48 to i64
%r51 = getelementptr i64, i64* %r1, i32 5
store i64 %r49, i64* %r51
%r52 = lshr i576 %r48, 64
%r54 = getelementptr i64, i64* %r3, i32 6
%r55 = load i64, i64* %r54
%r56 = call i576 @mulPv512x64(i64* %r2, i64 %r55)
%r57 = add i576 %r52, %r56
%r58 = trunc i576 %r57 to i64
%r60 = getelementptr i64, i64* %r1, i32 6
store i64 %r58, i64* %r60
%r61 = lshr i576 %r57, 64
%r63 = getelementptr i64, i64* %r3, i32 7
%r64 = load i64, i64* %r63
%r65 = call i576 @mulPv512x64(i64* %r2, i64 %r64)
%r66 = add i576 %r61, %r65
%r68 = getelementptr i64, i64* %r1, i32 7
%r70 = getelementptr i64, i64* %r68, i32 0
%r71 = trunc i576 %r66 to i64
store i64 %r71, i64* %r70
%r72 = lshr i576 %r66, 64
%r74 = getelementptr i64, i64* %r68, i32 1
%r75 = trunc i576 %r72 to i64
store i64 %r75, i64* %r74
%r76 = lshr i576 %r72, 64
%r78 = getelementptr i64, i64* %r68, i32 2
%r79 = trunc i576 %r76 to i64
store i64 %r79, i64* %r78
%r80 = lshr i576 %r76, 64
%r82 = getelementptr i64, i64* %r68, i32 3
%r83 = trunc i576 %r80 to i64
store i64 %r83, i64* %r82
%r84 = lshr i576 %r80, 64
%r86 = getelementptr i64, i64* %r68, i32 4
%r87 = trunc i576 %r84 to i64
store i64 %r87, i64* %r86
%r88 = lshr i576 %r84, 64
%r90 = getelementptr i64, i64* %r68, i32 5
%r91 = trunc i576 %r88 to i64
store i64 %r91, i64* %r90
%r92 = lshr i576 %r88, 64
%r94 = getelementptr i64, i64* %r68, i32 6
%r95 = trunc i576 %r92 to i64
store i64 %r95, i64* %r94
%r96 = lshr i576 %r92, 64
%r98 = getelementptr i64, i64* %r68, i32 7
%r99 = trunc i576 %r96 to i64
store i64 %r99, i64* %r98
%r100 = lshr i576 %r96, 64
%r102 = getelementptr i64, i64* %r68, i32 8
%r103 = trunc i576 %r100 to i64
store i64 %r103, i64* %r102
ret void
}
define void @mcl_fpDbl_sqrPre8L(i64* noalias  %r1, i64* noalias  %r2)
{
%r3 = load i64, i64* %r2
%r4 = call i576 @mulPv512x64(i64* %r2, i64 %r3)
%r5 = trunc i576 %r4 to i64
store i64 %r5, i64* %r1
%r6 = lshr i576 %r4, 64
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = call i576 @mulPv512x64(i64* %r2, i64 %r9)
%r11 = add i576 %r6, %r10
%r12 = trunc i576 %r11 to i64
%r14 = getelementptr i64, i64* %r1, i32 1
store i64 %r12, i64* %r14
%r15 = lshr i576 %r11, 64
%r17 = getelementptr i64, i64* %r2, i32 2
%r18 = load i64, i64* %r17
%r19 = call i576 @mulPv512x64(i64* %r2, i64 %r18)
%r20 = add i576 %r15, %r19
%r21 = trunc i576 %r20 to i64
%r23 = getelementptr i64, i64* %r1, i32 2
store i64 %r21, i64* %r23
%r24 = lshr i576 %r20, 64
%r26 = getelementptr i64, i64* %r2, i32 3
%r27 = load i64, i64* %r26
%r28 = call i576 @mulPv512x64(i64* %r2, i64 %r27)
%r29 = add i576 %r24, %r28
%r30 = trunc i576 %r29 to i64
%r32 = getelementptr i64, i64* %r1, i32 3
store i64 %r30, i64* %r32
%r33 = lshr i576 %r29, 64
%r35 = getelementptr i64, i64* %r2, i32 4
%r36 = load i64, i64* %r35
%r37 = call i576 @mulPv512x64(i64* %r2, i64 %r36)
%r38 = add i576 %r33, %r37
%r39 = trunc i576 %r38 to i64
%r41 = getelementptr i64, i64* %r1, i32 4
store i64 %r39, i64* %r41
%r42 = lshr i576 %r38, 64
%r44 = getelementptr i64, i64* %r2, i32 5
%r45 = load i64, i64* %r44
%r46 = call i576 @mulPv512x64(i64* %r2, i64 %r45)
%r47 = add i576 %r42, %r46
%r48 = trunc i576 %r47 to i64
%r50 = getelementptr i64, i64* %r1, i32 5
store i64 %r48, i64* %r50
%r51 = lshr i576 %r47, 64
%r53 = getelementptr i64, i64* %r2, i32 6
%r54 = load i64, i64* %r53
%r55 = call i576 @mulPv512x64(i64* %r2, i64 %r54)
%r56 = add i576 %r51, %r55
%r57 = trunc i576 %r56 to i64
%r59 = getelementptr i64, i64* %r1, i32 6
store i64 %r57, i64* %r59
%r60 = lshr i576 %r56, 64
%r62 = getelementptr i64, i64* %r2, i32 7
%r63 = load i64, i64* %r62
%r64 = call i576 @mulPv512x64(i64* %r2, i64 %r63)
%r65 = add i576 %r60, %r64
%r67 = getelementptr i64, i64* %r1, i32 7
%r69 = getelementptr i64, i64* %r67, i32 0
%r70 = trunc i576 %r65 to i64
store i64 %r70, i64* %r69
%r71 = lshr i576 %r65, 64
%r73 = getelementptr i64, i64* %r67, i32 1
%r74 = trunc i576 %r71 to i64
store i64 %r74, i64* %r73
%r75 = lshr i576 %r71, 64
%r77 = getelementptr i64, i64* %r67, i32 2
%r78 = trunc i576 %r75 to i64
store i64 %r78, i64* %r77
%r79 = lshr i576 %r75, 64
%r81 = getelementptr i64, i64* %r67, i32 3
%r82 = trunc i576 %r79 to i64
store i64 %r82, i64* %r81
%r83 = lshr i576 %r79, 64
%r85 = getelementptr i64, i64* %r67, i32 4
%r86 = trunc i576 %r83 to i64
store i64 %r86, i64* %r85
%r87 = lshr i576 %r83, 64
%r89 = getelementptr i64, i64* %r67, i32 5
%r90 = trunc i576 %r87 to i64
store i64 %r90, i64* %r89
%r91 = lshr i576 %r87, 64
%r93 = getelementptr i64, i64* %r67, i32 6
%r94 = trunc i576 %r91 to i64
store i64 %r94, i64* %r93
%r95 = lshr i576 %r91, 64
%r97 = getelementptr i64, i64* %r67, i32 7
%r98 = trunc i576 %r95 to i64
store i64 %r98, i64* %r97
%r99 = lshr i576 %r95, 64
%r101 = getelementptr i64, i64* %r67, i32 8
%r102 = trunc i576 %r99 to i64
store i64 %r102, i64* %r101
ret void
}
define void @mcl_fp_mont8L(i64* %r1, i64* %r2, i64* %r3, i64* %r4)
{
%r6 = getelementptr i64, i64* %r4, i32 -1
%r7 = load i64, i64* %r6
%r9 = getelementptr i64, i64* %r3, i32 0
%r10 = load i64, i64* %r9
%r11 = call i576 @mulPv512x64(i64* %r2, i64 %r10)
%r12 = zext i576 %r11 to i640
%r13 = trunc i576 %r11 to i64
%r14 = mul i64 %r13, %r7
%r15 = call i576 @mulPv512x64(i64* %r4, i64 %r14)
%r16 = zext i576 %r15 to i640
%r17 = add i640 %r12, %r16
%r18 = lshr i640 %r17, 64
%r20 = getelementptr i64, i64* %r3, i32 1
%r21 = load i64, i64* %r20
%r22 = call i576 @mulPv512x64(i64* %r2, i64 %r21)
%r23 = zext i576 %r22 to i640
%r24 = add i640 %r18, %r23
%r25 = trunc i640 %r24 to i64
%r26 = mul i64 %r25, %r7
%r27 = call i576 @mulPv512x64(i64* %r4, i64 %r26)
%r28 = zext i576 %r27 to i640
%r29 = add i640 %r24, %r28
%r30 = lshr i640 %r29, 64
%r32 = getelementptr i64, i64* %r3, i32 2
%r33 = load i64, i64* %r32
%r34 = call i576 @mulPv512x64(i64* %r2, i64 %r33)
%r35 = zext i576 %r34 to i640
%r36 = add i640 %r30, %r35
%r37 = trunc i640 %r36 to i64
%r38 = mul i64 %r37, %r7
%r39 = call i576 @mulPv512x64(i64* %r4, i64 %r38)
%r40 = zext i576 %r39 to i640
%r41 = add i640 %r36, %r40
%r42 = lshr i640 %r41, 64
%r44 = getelementptr i64, i64* %r3, i32 3
%r45 = load i64, i64* %r44
%r46 = call i576 @mulPv512x64(i64* %r2, i64 %r45)
%r47 = zext i576 %r46 to i640
%r48 = add i640 %r42, %r47
%r49 = trunc i640 %r48 to i64
%r50 = mul i64 %r49, %r7
%r51 = call i576 @mulPv512x64(i64* %r4, i64 %r50)
%r52 = zext i576 %r51 to i640
%r53 = add i640 %r48, %r52
%r54 = lshr i640 %r53, 64
%r56 = getelementptr i64, i64* %r3, i32 4
%r57 = load i64, i64* %r56
%r58 = call i576 @mulPv512x64(i64* %r2, i64 %r57)
%r59 = zext i576 %r58 to i640
%r60 = add i640 %r54, %r59
%r61 = trunc i640 %r60 to i64
%r62 = mul i64 %r61, %r7
%r63 = call i576 @mulPv512x64(i64* %r4, i64 %r62)
%r64 = zext i576 %r63 to i640
%r65 = add i640 %r60, %r64
%r66 = lshr i640 %r65, 64
%r68 = getelementptr i64, i64* %r3, i32 5
%r69 = load i64, i64* %r68
%r70 = call i576 @mulPv512x64(i64* %r2, i64 %r69)
%r71 = zext i576 %r70 to i640
%r72 = add i640 %r66, %r71
%r73 = trunc i640 %r72 to i64
%r74 = mul i64 %r73, %r7
%r75 = call i576 @mulPv512x64(i64* %r4, i64 %r74)
%r76 = zext i576 %r75 to i640
%r77 = add i640 %r72, %r76
%r78 = lshr i640 %r77, 64
%r80 = getelementptr i64, i64* %r3, i32 6
%r81 = load i64, i64* %r80
%r82 = call i576 @mulPv512x64(i64* %r2, i64 %r81)
%r83 = zext i576 %r82 to i640
%r84 = add i640 %r78, %r83
%r85 = trunc i640 %r84 to i64
%r86 = mul i64 %r85, %r7
%r87 = call i576 @mulPv512x64(i64* %r4, i64 %r86)
%r88 = zext i576 %r87 to i640
%r89 = add i640 %r84, %r88
%r90 = lshr i640 %r89, 64
%r92 = getelementptr i64, i64* %r3, i32 7
%r93 = load i64, i64* %r92
%r94 = call i576 @mulPv512x64(i64* %r2, i64 %r93)
%r95 = zext i576 %r94 to i640
%r96 = add i640 %r90, %r95
%r97 = trunc i640 %r96 to i64
%r98 = mul i64 %r97, %r7
%r99 = call i576 @mulPv512x64(i64* %r4, i64 %r98)
%r100 = zext i576 %r99 to i640
%r101 = add i640 %r96, %r100
%r102 = lshr i640 %r101, 64
%r103 = trunc i640 %r102 to i576
%r104 = load i64, i64* %r4
%r105 = zext i64 %r104 to i128
%r107 = getelementptr i64, i64* %r4, i32 1
%r108 = load i64, i64* %r107
%r109 = zext i64 %r108 to i128
%r110 = shl i128 %r109, 64
%r111 = or i128 %r105, %r110
%r112 = zext i128 %r111 to i192
%r114 = getelementptr i64, i64* %r4, i32 2
%r115 = load i64, i64* %r114
%r116 = zext i64 %r115 to i192
%r117 = shl i192 %r116, 128
%r118 = or i192 %r112, %r117
%r119 = zext i192 %r118 to i256
%r121 = getelementptr i64, i64* %r4, i32 3
%r122 = load i64, i64* %r121
%r123 = zext i64 %r122 to i256
%r124 = shl i256 %r123, 192
%r125 = or i256 %r119, %r124
%r126 = zext i256 %r125 to i320
%r128 = getelementptr i64, i64* %r4, i32 4
%r129 = load i64, i64* %r128
%r130 = zext i64 %r129 to i320
%r131 = shl i320 %r130, 256
%r132 = or i320 %r126, %r131
%r133 = zext i320 %r132 to i384
%r135 = getelementptr i64, i64* %r4, i32 5
%r136 = load i64, i64* %r135
%r137 = zext i64 %r136 to i384
%r138 = shl i384 %r137, 320
%r139 = or i384 %r133, %r138
%r140 = zext i384 %r139 to i448
%r142 = getelementptr i64, i64* %r4, i32 6
%r143 = load i64, i64* %r142
%r144 = zext i64 %r143 to i448
%r145 = shl i448 %r144, 384
%r146 = or i448 %r140, %r145
%r147 = zext i448 %r146 to i512
%r149 = getelementptr i64, i64* %r4, i32 7
%r150 = load i64, i64* %r149
%r151 = zext i64 %r150 to i512
%r152 = shl i512 %r151, 448
%r153 = or i512 %r147, %r152
%r154 = zext i512 %r153 to i576
%r155 = sub i576 %r103, %r154
%r156 = lshr i576 %r155, 512
%r157 = trunc i576 %r156 to i1
%r158 = select i1 %r157, i576 %r103, i576 %r155
%r159 = trunc i576 %r158 to i512
%r161 = getelementptr i64, i64* %r1, i32 0
%r162 = trunc i512 %r159 to i64
store i64 %r162, i64* %r161
%r163 = lshr i512 %r159, 64
%r165 = getelementptr i64, i64* %r1, i32 1
%r166 = trunc i512 %r163 to i64
store i64 %r166, i64* %r165
%r167 = lshr i512 %r163, 64
%r169 = getelementptr i64, i64* %r1, i32 2
%r170 = trunc i512 %r167 to i64
store i64 %r170, i64* %r169
%r171 = lshr i512 %r167, 64
%r173 = getelementptr i64, i64* %r1, i32 3
%r174 = trunc i512 %r171 to i64
store i64 %r174, i64* %r173
%r175 = lshr i512 %r171, 64
%r177 = getelementptr i64, i64* %r1, i32 4
%r178 = trunc i512 %r175 to i64
store i64 %r178, i64* %r177
%r179 = lshr i512 %r175, 64
%r181 = getelementptr i64, i64* %r1, i32 5
%r182 = trunc i512 %r179 to i64
store i64 %r182, i64* %r181
%r183 = lshr i512 %r179, 64
%r185 = getelementptr i64, i64* %r1, i32 6
%r186 = trunc i512 %r183 to i64
store i64 %r186, i64* %r185
%r187 = lshr i512 %r183, 64
%r189 = getelementptr i64, i64* %r1, i32 7
%r190 = trunc i512 %r187 to i64
store i64 %r190, i64* %r189
ret void
}
define void @mcl_fp_montNF8L(i64* %r1, i64* %r2, i64* %r3, i64* %r4)
{
%r6 = getelementptr i64, i64* %r4, i32 -1
%r7 = load i64, i64* %r6
%r8 = load i64, i64* %r3
%r9 = call i576 @mulPv512x64(i64* %r2, i64 %r8)
%r10 = trunc i576 %r9 to i64
%r11 = mul i64 %r10, %r7
%r12 = call i576 @mulPv512x64(i64* %r4, i64 %r11)
%r13 = add i576 %r9, %r12
%r14 = lshr i576 %r13, 64
%r16 = getelementptr i64, i64* %r3, i32 1
%r17 = load i64, i64* %r16
%r18 = call i576 @mulPv512x64(i64* %r2, i64 %r17)
%r19 = add i576 %r14, %r18
%r20 = trunc i576 %r19 to i64
%r21 = mul i64 %r20, %r7
%r22 = call i576 @mulPv512x64(i64* %r4, i64 %r21)
%r23 = add i576 %r19, %r22
%r24 = lshr i576 %r23, 64
%r26 = getelementptr i64, i64* %r3, i32 2
%r27 = load i64, i64* %r26
%r28 = call i576 @mulPv512x64(i64* %r2, i64 %r27)
%r29 = add i576 %r24, %r28
%r30 = trunc i576 %r29 to i64
%r31 = mul i64 %r30, %r7
%r32 = call i576 @mulPv512x64(i64* %r4, i64 %r31)
%r33 = add i576 %r29, %r32
%r34 = lshr i576 %r33, 64
%r36 = getelementptr i64, i64* %r3, i32 3
%r37 = load i64, i64* %r36
%r38 = call i576 @mulPv512x64(i64* %r2, i64 %r37)
%r39 = add i576 %r34, %r38
%r40 = trunc i576 %r39 to i64
%r41 = mul i64 %r40, %r7
%r42 = call i576 @mulPv512x64(i64* %r4, i64 %r41)
%r43 = add i576 %r39, %r42
%r44 = lshr i576 %r43, 64
%r46 = getelementptr i64, i64* %r3, i32 4
%r47 = load i64, i64* %r46
%r48 = call i576 @mulPv512x64(i64* %r2, i64 %r47)
%r49 = add i576 %r44, %r48
%r50 = trunc i576 %r49 to i64
%r51 = mul i64 %r50, %r7
%r52 = call i576 @mulPv512x64(i64* %r4, i64 %r51)
%r53 = add i576 %r49, %r52
%r54 = lshr i576 %r53, 64
%r56 = getelementptr i64, i64* %r3, i32 5
%r57 = load i64, i64* %r56
%r58 = call i576 @mulPv512x64(i64* %r2, i64 %r57)
%r59 = add i576 %r54, %r58
%r60 = trunc i576 %r59 to i64
%r61 = mul i64 %r60, %r7
%r62 = call i576 @mulPv512x64(i64* %r4, i64 %r61)
%r63 = add i576 %r59, %r62
%r64 = lshr i576 %r63, 64
%r66 = getelementptr i64, i64* %r3, i32 6
%r67 = load i64, i64* %r66
%r68 = call i576 @mulPv512x64(i64* %r2, i64 %r67)
%r69 = add i576 %r64, %r68
%r70 = trunc i576 %r69 to i64
%r71 = mul i64 %r70, %r7
%r72 = call i576 @mulPv512x64(i64* %r4, i64 %r71)
%r73 = add i576 %r69, %r72
%r74 = lshr i576 %r73, 64
%r76 = getelementptr i64, i64* %r3, i32 7
%r77 = load i64, i64* %r76
%r78 = call i576 @mulPv512x64(i64* %r2, i64 %r77)
%r79 = add i576 %r74, %r78
%r80 = trunc i576 %r79 to i64
%r81 = mul i64 %r80, %r7
%r82 = call i576 @mulPv512x64(i64* %r4, i64 %r81)
%r83 = add i576 %r79, %r82
%r84 = lshr i576 %r83, 64
%r85 = trunc i576 %r84 to i512
%r86 = load i64, i64* %r4
%r87 = zext i64 %r86 to i128
%r89 = getelementptr i64, i64* %r4, i32 1
%r90 = load i64, i64* %r89
%r91 = zext i64 %r90 to i128
%r92 = shl i128 %r91, 64
%r93 = or i128 %r87, %r92
%r94 = zext i128 %r93 to i192
%r96 = getelementptr i64, i64* %r4, i32 2
%r97 = load i64, i64* %r96
%r98 = zext i64 %r97 to i192
%r99 = shl i192 %r98, 128
%r100 = or i192 %r94, %r99
%r101 = zext i192 %r100 to i256
%r103 = getelementptr i64, i64* %r4, i32 3
%r104 = load i64, i64* %r103
%r105 = zext i64 %r104 to i256
%r106 = shl i256 %r105, 192
%r107 = or i256 %r101, %r106
%r108 = zext i256 %r107 to i320
%r110 = getelementptr i64, i64* %r4, i32 4
%r111 = load i64, i64* %r110
%r112 = zext i64 %r111 to i320
%r113 = shl i320 %r112, 256
%r114 = or i320 %r108, %r113
%r115 = zext i320 %r114 to i384
%r117 = getelementptr i64, i64* %r4, i32 5
%r118 = load i64, i64* %r117
%r119 = zext i64 %r118 to i384
%r120 = shl i384 %r119, 320
%r121 = or i384 %r115, %r120
%r122 = zext i384 %r121 to i448
%r124 = getelementptr i64, i64* %r4, i32 6
%r125 = load i64, i64* %r124
%r126 = zext i64 %r125 to i448
%r127 = shl i448 %r126, 384
%r128 = or i448 %r122, %r127
%r129 = zext i448 %r128 to i512
%r131 = getelementptr i64, i64* %r4, i32 7
%r132 = load i64, i64* %r131
%r133 = zext i64 %r132 to i512
%r134 = shl i512 %r133, 448
%r135 = or i512 %r129, %r134
%r136 = sub i512 %r85, %r135
%r137 = lshr i512 %r136, 511
%r138 = trunc i512 %r137 to i1
%r139 = select i1 %r138, i512 %r85, i512 %r136
%r141 = getelementptr i64, i64* %r1, i32 0
%r142 = trunc i512 %r139 to i64
store i64 %r142, i64* %r141
%r143 = lshr i512 %r139, 64
%r145 = getelementptr i64, i64* %r1, i32 1
%r146 = trunc i512 %r143 to i64
store i64 %r146, i64* %r145
%r147 = lshr i512 %r143, 64
%r149 = getelementptr i64, i64* %r1, i32 2
%r150 = trunc i512 %r147 to i64
store i64 %r150, i64* %r149
%r151 = lshr i512 %r147, 64
%r153 = getelementptr i64, i64* %r1, i32 3
%r154 = trunc i512 %r151 to i64
store i64 %r154, i64* %r153
%r155 = lshr i512 %r151, 64
%r157 = getelementptr i64, i64* %r1, i32 4
%r158 = trunc i512 %r155 to i64
store i64 %r158, i64* %r157
%r159 = lshr i512 %r155, 64
%r161 = getelementptr i64, i64* %r1, i32 5
%r162 = trunc i512 %r159 to i64
store i64 %r162, i64* %r161
%r163 = lshr i512 %r159, 64
%r165 = getelementptr i64, i64* %r1, i32 6
%r166 = trunc i512 %r163 to i64
store i64 %r166, i64* %r165
%r167 = lshr i512 %r163, 64
%r169 = getelementptr i64, i64* %r1, i32 7
%r170 = trunc i512 %r167 to i64
store i64 %r170, i64* %r169
ret void
}
define void @mcl_fp_montRed8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = getelementptr i64, i64* %r3, i32 -1
%r6 = load i64, i64* %r5
%r7 = load i64, i64* %r3
%r8 = zext i64 %r7 to i128
%r10 = getelementptr i64, i64* %r3, i32 1
%r11 = load i64, i64* %r10
%r12 = zext i64 %r11 to i128
%r13 = shl i128 %r12, 64
%r14 = or i128 %r8, %r13
%r15 = zext i128 %r14 to i192
%r17 = getelementptr i64, i64* %r3, i32 2
%r18 = load i64, i64* %r17
%r19 = zext i64 %r18 to i192
%r20 = shl i192 %r19, 128
%r21 = or i192 %r15, %r20
%r22 = zext i192 %r21 to i256
%r24 = getelementptr i64, i64* %r3, i32 3
%r25 = load i64, i64* %r24
%r26 = zext i64 %r25 to i256
%r27 = shl i256 %r26, 192
%r28 = or i256 %r22, %r27
%r29 = zext i256 %r28 to i320
%r31 = getelementptr i64, i64* %r3, i32 4
%r32 = load i64, i64* %r31
%r33 = zext i64 %r32 to i320
%r34 = shl i320 %r33, 256
%r35 = or i320 %r29, %r34
%r36 = zext i320 %r35 to i384
%r38 = getelementptr i64, i64* %r3, i32 5
%r39 = load i64, i64* %r38
%r40 = zext i64 %r39 to i384
%r41 = shl i384 %r40, 320
%r42 = or i384 %r36, %r41
%r43 = zext i384 %r42 to i448
%r45 = getelementptr i64, i64* %r3, i32 6
%r46 = load i64, i64* %r45
%r47 = zext i64 %r46 to i448
%r48 = shl i448 %r47, 384
%r49 = or i448 %r43, %r48
%r50 = zext i448 %r49 to i512
%r52 = getelementptr i64, i64* %r3, i32 7
%r53 = load i64, i64* %r52
%r54 = zext i64 %r53 to i512
%r55 = shl i512 %r54, 448
%r56 = or i512 %r50, %r55
%r57 = load i64, i64* %r2
%r58 = zext i64 %r57 to i128
%r60 = getelementptr i64, i64* %r2, i32 1
%r61 = load i64, i64* %r60
%r62 = zext i64 %r61 to i128
%r63 = shl i128 %r62, 64
%r64 = or i128 %r58, %r63
%r65 = zext i128 %r64 to i192
%r67 = getelementptr i64, i64* %r2, i32 2
%r68 = load i64, i64* %r67
%r69 = zext i64 %r68 to i192
%r70 = shl i192 %r69, 128
%r71 = or i192 %r65, %r70
%r72 = zext i192 %r71 to i256
%r74 = getelementptr i64, i64* %r2, i32 3
%r75 = load i64, i64* %r74
%r76 = zext i64 %r75 to i256
%r77 = shl i256 %r76, 192
%r78 = or i256 %r72, %r77
%r79 = zext i256 %r78 to i320
%r81 = getelementptr i64, i64* %r2, i32 4
%r82 = load i64, i64* %r81
%r83 = zext i64 %r82 to i320
%r84 = shl i320 %r83, 256
%r85 = or i320 %r79, %r84
%r86 = zext i320 %r85 to i384
%r88 = getelementptr i64, i64* %r2, i32 5
%r89 = load i64, i64* %r88
%r90 = zext i64 %r89 to i384
%r91 = shl i384 %r90, 320
%r92 = or i384 %r86, %r91
%r93 = zext i384 %r92 to i448
%r95 = getelementptr i64, i64* %r2, i32 6
%r96 = load i64, i64* %r95
%r97 = zext i64 %r96 to i448
%r98 = shl i448 %r97, 384
%r99 = or i448 %r93, %r98
%r100 = zext i448 %r99 to i512
%r102 = getelementptr i64, i64* %r2, i32 7
%r103 = load i64, i64* %r102
%r104 = zext i64 %r103 to i512
%r105 = shl i512 %r104, 448
%r106 = or i512 %r100, %r105
%r107 = trunc i512 %r106 to i64
%r108 = mul i64 %r107, %r6
%r109 = call i576 @mulPv512x64(i64* %r3, i64 %r108)
%r111 = getelementptr i64, i64* %r2, i32 8
%r112 = load i64, i64* %r111
%r113 = zext i64 %r112 to i576
%r114 = shl i576 %r113, 512
%r115 = zext i512 %r106 to i576
%r116 = or i576 %r114, %r115
%r117 = zext i576 %r116 to i640
%r118 = zext i576 %r109 to i640
%r119 = add i640 %r117, %r118
%r120 = lshr i640 %r119, 64
%r121 = trunc i640 %r120 to i576
%r122 = lshr i576 %r121, 512
%r123 = trunc i576 %r122 to i64
%r124 = trunc i576 %r121 to i512
%r125 = trunc i512 %r124 to i64
%r126 = mul i64 %r125, %r6
%r127 = call i576 @mulPv512x64(i64* %r3, i64 %r126)
%r128 = zext i64 %r123 to i576
%r129 = shl i576 %r128, 512
%r130 = add i576 %r127, %r129
%r132 = getelementptr i64, i64* %r2, i32 9
%r133 = load i64, i64* %r132
%r134 = zext i64 %r133 to i576
%r135 = shl i576 %r134, 512
%r136 = zext i512 %r124 to i576
%r137 = or i576 %r135, %r136
%r138 = zext i576 %r137 to i640
%r139 = zext i576 %r130 to i640
%r140 = add i640 %r138, %r139
%r141 = lshr i640 %r140, 64
%r142 = trunc i640 %r141 to i576
%r143 = lshr i576 %r142, 512
%r144 = trunc i576 %r143 to i64
%r145 = trunc i576 %r142 to i512
%r146 = trunc i512 %r145 to i64
%r147 = mul i64 %r146, %r6
%r148 = call i576 @mulPv512x64(i64* %r3, i64 %r147)
%r149 = zext i64 %r144 to i576
%r150 = shl i576 %r149, 512
%r151 = add i576 %r148, %r150
%r153 = getelementptr i64, i64* %r2, i32 10
%r154 = load i64, i64* %r153
%r155 = zext i64 %r154 to i576
%r156 = shl i576 %r155, 512
%r157 = zext i512 %r145 to i576
%r158 = or i576 %r156, %r157
%r159 = zext i576 %r158 to i640
%r160 = zext i576 %r151 to i640
%r161 = add i640 %r159, %r160
%r162 = lshr i640 %r161, 64
%r163 = trunc i640 %r162 to i576
%r164 = lshr i576 %r163, 512
%r165 = trunc i576 %r164 to i64
%r166 = trunc i576 %r163 to i512
%r167 = trunc i512 %r166 to i64
%r168 = mul i64 %r167, %r6
%r169 = call i576 @mulPv512x64(i64* %r3, i64 %r168)
%r170 = zext i64 %r165 to i576
%r171 = shl i576 %r170, 512
%r172 = add i576 %r169, %r171
%r174 = getelementptr i64, i64* %r2, i32 11
%r175 = load i64, i64* %r174
%r176 = zext i64 %r175 to i576
%r177 = shl i576 %r176, 512
%r178 = zext i512 %r166 to i576
%r179 = or i576 %r177, %r178
%r180 = zext i576 %r179 to i640
%r181 = zext i576 %r172 to i640
%r182 = add i640 %r180, %r181
%r183 = lshr i640 %r182, 64
%r184 = trunc i640 %r183 to i576
%r185 = lshr i576 %r184, 512
%r186 = trunc i576 %r185 to i64
%r187 = trunc i576 %r184 to i512
%r188 = trunc i512 %r187 to i64
%r189 = mul i64 %r188, %r6
%r190 = call i576 @mulPv512x64(i64* %r3, i64 %r189)
%r191 = zext i64 %r186 to i576
%r192 = shl i576 %r191, 512
%r193 = add i576 %r190, %r192
%r195 = getelementptr i64, i64* %r2, i32 12
%r196 = load i64, i64* %r195
%r197 = zext i64 %r196 to i576
%r198 = shl i576 %r197, 512
%r199 = zext i512 %r187 to i576
%r200 = or i576 %r198, %r199
%r201 = zext i576 %r200 to i640
%r202 = zext i576 %r193 to i640
%r203 = add i640 %r201, %r202
%r204 = lshr i640 %r203, 64
%r205 = trunc i640 %r204 to i576
%r206 = lshr i576 %r205, 512
%r207 = trunc i576 %r206 to i64
%r208 = trunc i576 %r205 to i512
%r209 = trunc i512 %r208 to i64
%r210 = mul i64 %r209, %r6
%r211 = call i576 @mulPv512x64(i64* %r3, i64 %r210)
%r212 = zext i64 %r207 to i576
%r213 = shl i576 %r212, 512
%r214 = add i576 %r211, %r213
%r216 = getelementptr i64, i64* %r2, i32 13
%r217 = load i64, i64* %r216
%r218 = zext i64 %r217 to i576
%r219 = shl i576 %r218, 512
%r220 = zext i512 %r208 to i576
%r221 = or i576 %r219, %r220
%r222 = zext i576 %r221 to i640
%r223 = zext i576 %r214 to i640
%r224 = add i640 %r222, %r223
%r225 = lshr i640 %r224, 64
%r226 = trunc i640 %r225 to i576
%r227 = lshr i576 %r226, 512
%r228 = trunc i576 %r227 to i64
%r229 = trunc i576 %r226 to i512
%r230 = trunc i512 %r229 to i64
%r231 = mul i64 %r230, %r6
%r232 = call i576 @mulPv512x64(i64* %r3, i64 %r231)
%r233 = zext i64 %r228 to i576
%r234 = shl i576 %r233, 512
%r235 = add i576 %r232, %r234
%r237 = getelementptr i64, i64* %r2, i32 14
%r238 = load i64, i64* %r237
%r239 = zext i64 %r238 to i576
%r240 = shl i576 %r239, 512
%r241 = zext i512 %r229 to i576
%r242 = or i576 %r240, %r241
%r243 = zext i576 %r242 to i640
%r244 = zext i576 %r235 to i640
%r245 = add i640 %r243, %r244
%r246 = lshr i640 %r245, 64
%r247 = trunc i640 %r246 to i576
%r248 = lshr i576 %r247, 512
%r249 = trunc i576 %r248 to i64
%r250 = trunc i576 %r247 to i512
%r251 = trunc i512 %r250 to i64
%r252 = mul i64 %r251, %r6
%r253 = call i576 @mulPv512x64(i64* %r3, i64 %r252)
%r254 = zext i64 %r249 to i576
%r255 = shl i576 %r254, 512
%r256 = add i576 %r253, %r255
%r258 = getelementptr i64, i64* %r2, i32 15
%r259 = load i64, i64* %r258
%r260 = zext i64 %r259 to i576
%r261 = shl i576 %r260, 512
%r262 = zext i512 %r250 to i576
%r263 = or i576 %r261, %r262
%r264 = zext i576 %r263 to i640
%r265 = zext i576 %r256 to i640
%r266 = add i640 %r264, %r265
%r267 = lshr i640 %r266, 64
%r268 = trunc i640 %r267 to i576
%r269 = lshr i576 %r268, 512
%r270 = trunc i576 %r269 to i64
%r271 = trunc i576 %r268 to i512
%r272 = zext i512 %r56 to i576
%r273 = zext i512 %r271 to i576
%r274 = sub i576 %r273, %r272
%r275 = lshr i576 %r274, 512
%r276 = trunc i576 %r275 to i1
%r277 = select i1 %r276, i576 %r273, i576 %r274
%r278 = trunc i576 %r277 to i512
%r280 = getelementptr i64, i64* %r1, i32 0
%r281 = trunc i512 %r278 to i64
store i64 %r281, i64* %r280
%r282 = lshr i512 %r278, 64
%r284 = getelementptr i64, i64* %r1, i32 1
%r285 = trunc i512 %r282 to i64
store i64 %r285, i64* %r284
%r286 = lshr i512 %r282, 64
%r288 = getelementptr i64, i64* %r1, i32 2
%r289 = trunc i512 %r286 to i64
store i64 %r289, i64* %r288
%r290 = lshr i512 %r286, 64
%r292 = getelementptr i64, i64* %r1, i32 3
%r293 = trunc i512 %r290 to i64
store i64 %r293, i64* %r292
%r294 = lshr i512 %r290, 64
%r296 = getelementptr i64, i64* %r1, i32 4
%r297 = trunc i512 %r294 to i64
store i64 %r297, i64* %r296
%r298 = lshr i512 %r294, 64
%r300 = getelementptr i64, i64* %r1, i32 5
%r301 = trunc i512 %r298 to i64
store i64 %r301, i64* %r300
%r302 = lshr i512 %r298, 64
%r304 = getelementptr i64, i64* %r1, i32 6
%r305 = trunc i512 %r302 to i64
store i64 %r305, i64* %r304
%r306 = lshr i512 %r302, 64
%r308 = getelementptr i64, i64* %r1, i32 7
%r309 = trunc i512 %r306 to i64
store i64 %r309, i64* %r308
ret void
}
define void @mcl_fp_montRedNF8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3)
{
%r5 = getelementptr i64, i64* %r3, i32 -1
%r6 = load i64, i64* %r5
%r7 = load i64, i64* %r3
%r8 = zext i64 %r7 to i128
%r10 = getelementptr i64, i64* %r3, i32 1
%r11 = load i64, i64* %r10
%r12 = zext i64 %r11 to i128
%r13 = shl i128 %r12, 64
%r14 = or i128 %r8, %r13
%r15 = zext i128 %r14 to i192
%r17 = getelementptr i64, i64* %r3, i32 2
%r18 = load i64, i64* %r17
%r19 = zext i64 %r18 to i192
%r20 = shl i192 %r19, 128
%r21 = or i192 %r15, %r20
%r22 = zext i192 %r21 to i256
%r24 = getelementptr i64, i64* %r3, i32 3
%r25 = load i64, i64* %r24
%r26 = zext i64 %r25 to i256
%r27 = shl i256 %r26, 192
%r28 = or i256 %r22, %r27
%r29 = zext i256 %r28 to i320
%r31 = getelementptr i64, i64* %r3, i32 4
%r32 = load i64, i64* %r31
%r33 = zext i64 %r32 to i320
%r34 = shl i320 %r33, 256
%r35 = or i320 %r29, %r34
%r36 = zext i320 %r35 to i384
%r38 = getelementptr i64, i64* %r3, i32 5
%r39 = load i64, i64* %r38
%r40 = zext i64 %r39 to i384
%r41 = shl i384 %r40, 320
%r42 = or i384 %r36, %r41
%r43 = zext i384 %r42 to i448
%r45 = getelementptr i64, i64* %r3, i32 6
%r46 = load i64, i64* %r45
%r47 = zext i64 %r46 to i448
%r48 = shl i448 %r47, 384
%r49 = or i448 %r43, %r48
%r50 = zext i448 %r49 to i512
%r52 = getelementptr i64, i64* %r3, i32 7
%r53 = load i64, i64* %r52
%r54 = zext i64 %r53 to i512
%r55 = shl i512 %r54, 448
%r56 = or i512 %r50, %r55
%r57 = load i64, i64* %r2
%r58 = zext i64 %r57 to i128
%r60 = getelementptr i64, i64* %r2, i32 1
%r61 = load i64, i64* %r60
%r62 = zext i64 %r61 to i128
%r63 = shl i128 %r62, 64
%r64 = or i128 %r58, %r63
%r65 = zext i128 %r64 to i192
%r67 = getelementptr i64, i64* %r2, i32 2
%r68 = load i64, i64* %r67
%r69 = zext i64 %r68 to i192
%r70 = shl i192 %r69, 128
%r71 = or i192 %r65, %r70
%r72 = zext i192 %r71 to i256
%r74 = getelementptr i64, i64* %r2, i32 3
%r75 = load i64, i64* %r74
%r76 = zext i64 %r75 to i256
%r77 = shl i256 %r76, 192
%r78 = or i256 %r72, %r77
%r79 = zext i256 %r78 to i320
%r81 = getelementptr i64, i64* %r2, i32 4
%r82 = load i64, i64* %r81
%r83 = zext i64 %r82 to i320
%r84 = shl i320 %r83, 256
%r85 = or i320 %r79, %r84
%r86 = zext i320 %r85 to i384
%r88 = getelementptr i64, i64* %r2, i32 5
%r89 = load i64, i64* %r88
%r90 = zext i64 %r89 to i384
%r91 = shl i384 %r90, 320
%r92 = or i384 %r86, %r91
%r93 = zext i384 %r92 to i448
%r95 = getelementptr i64, i64* %r2, i32 6
%r96 = load i64, i64* %r95
%r97 = zext i64 %r96 to i448
%r98 = shl i448 %r97, 384
%r99 = or i448 %r93, %r98
%r100 = zext i448 %r99 to i512
%r102 = getelementptr i64, i64* %r2, i32 7
%r103 = load i64, i64* %r102
%r104 = zext i64 %r103 to i512
%r105 = shl i512 %r104, 448
%r106 = or i512 %r100, %r105
%r107 = trunc i512 %r106 to i64
%r108 = mul i64 %r107, %r6
%r109 = call i576 @mulPv512x64(i64* %r3, i64 %r108)
%r111 = getelementptr i64, i64* %r2, i32 8
%r112 = load i64, i64* %r111
%r113 = zext i64 %r112 to i576
%r114 = shl i576 %r113, 512
%r115 = zext i512 %r106 to i576
%r116 = or i576 %r114, %r115
%r117 = zext i576 %r116 to i640
%r118 = zext i576 %r109 to i640
%r119 = add i640 %r117, %r118
%r120 = lshr i640 %r119, 64
%r121 = trunc i640 %r120 to i576
%r122 = lshr i576 %r121, 512
%r123 = trunc i576 %r122 to i64
%r124 = trunc i576 %r121 to i512
%r125 = trunc i512 %r124 to i64
%r126 = mul i64 %r125, %r6
%r127 = call i576 @mulPv512x64(i64* %r3, i64 %r126)
%r128 = zext i64 %r123 to i576
%r129 = shl i576 %r128, 512
%r130 = add i576 %r127, %r129
%r132 = getelementptr i64, i64* %r2, i32 9
%r133 = load i64, i64* %r132
%r134 = zext i64 %r133 to i576
%r135 = shl i576 %r134, 512
%r136 = zext i512 %r124 to i576
%r137 = or i576 %r135, %r136
%r138 = zext i576 %r137 to i640
%r139 = zext i576 %r130 to i640
%r140 = add i640 %r138, %r139
%r141 = lshr i640 %r140, 64
%r142 = trunc i640 %r141 to i576
%r143 = lshr i576 %r142, 512
%r144 = trunc i576 %r143 to i64
%r145 = trunc i576 %r142 to i512
%r146 = trunc i512 %r145 to i64
%r147 = mul i64 %r146, %r6
%r148 = call i576 @mulPv512x64(i64* %r3, i64 %r147)
%r149 = zext i64 %r144 to i576
%r150 = shl i576 %r149, 512
%r151 = add i576 %r148, %r150
%r153 = getelementptr i64, i64* %r2, i32 10
%r154 = load i64, i64* %r153
%r155 = zext i64 %r154 to i576
%r156 = shl i576 %r155, 512
%r157 = zext i512 %r145 to i576
%r158 = or i576 %r156, %r157
%r159 = zext i576 %r158 to i640
%r160 = zext i576 %r151 to i640
%r161 = add i640 %r159, %r160
%r162 = lshr i640 %r161, 64
%r163 = trunc i640 %r162 to i576
%r164 = lshr i576 %r163, 512
%r165 = trunc i576 %r164 to i64
%r166 = trunc i576 %r163 to i512
%r167 = trunc i512 %r166 to i64
%r168 = mul i64 %r167, %r6
%r169 = call i576 @mulPv512x64(i64* %r3, i64 %r168)
%r170 = zext i64 %r165 to i576
%r171 = shl i576 %r170, 512
%r172 = add i576 %r169, %r171
%r174 = getelementptr i64, i64* %r2, i32 11
%r175 = load i64, i64* %r174
%r176 = zext i64 %r175 to i576
%r177 = shl i576 %r176, 512
%r178 = zext i512 %r166 to i576
%r179 = or i576 %r177, %r178
%r180 = zext i576 %r179 to i640
%r181 = zext i576 %r172 to i640
%r182 = add i640 %r180, %r181
%r183 = lshr i640 %r182, 64
%r184 = trunc i640 %r183 to i576
%r185 = lshr i576 %r184, 512
%r186 = trunc i576 %r185 to i64
%r187 = trunc i576 %r184 to i512
%r188 = trunc i512 %r187 to i64
%r189 = mul i64 %r188, %r6
%r190 = call i576 @mulPv512x64(i64* %r3, i64 %r189)
%r191 = zext i64 %r186 to i576
%r192 = shl i576 %r191, 512
%r193 = add i576 %r190, %r192
%r195 = getelementptr i64, i64* %r2, i32 12
%r196 = load i64, i64* %r195
%r197 = zext i64 %r196 to i576
%r198 = shl i576 %r197, 512
%r199 = zext i512 %r187 to i576
%r200 = or i576 %r198, %r199
%r201 = zext i576 %r200 to i640
%r202 = zext i576 %r193 to i640
%r203 = add i640 %r201, %r202
%r204 = lshr i640 %r203, 64
%r205 = trunc i640 %r204 to i576
%r206 = lshr i576 %r205, 512
%r207 = trunc i576 %r206 to i64
%r208 = trunc i576 %r205 to i512
%r209 = trunc i512 %r208 to i64
%r210 = mul i64 %r209, %r6
%r211 = call i576 @mulPv512x64(i64* %r3, i64 %r210)
%r212 = zext i64 %r207 to i576
%r213 = shl i576 %r212, 512
%r214 = add i576 %r211, %r213
%r216 = getelementptr i64, i64* %r2, i32 13
%r217 = load i64, i64* %r216
%r218 = zext i64 %r217 to i576
%r219 = shl i576 %r218, 512
%r220 = zext i512 %r208 to i576
%r221 = or i576 %r219, %r220
%r222 = zext i576 %r221 to i640
%r223 = zext i576 %r214 to i640
%r224 = add i640 %r222, %r223
%r225 = lshr i640 %r224, 64
%r226 = trunc i640 %r225 to i576
%r227 = lshr i576 %r226, 512
%r228 = trunc i576 %r227 to i64
%r229 = trunc i576 %r226 to i512
%r230 = trunc i512 %r229 to i64
%r231 = mul i64 %r230, %r6
%r232 = call i576 @mulPv512x64(i64* %r3, i64 %r231)
%r233 = zext i64 %r228 to i576
%r234 = shl i576 %r233, 512
%r235 = add i576 %r232, %r234
%r237 = getelementptr i64, i64* %r2, i32 14
%r238 = load i64, i64* %r237
%r239 = zext i64 %r238 to i576
%r240 = shl i576 %r239, 512
%r241 = zext i512 %r229 to i576
%r242 = or i576 %r240, %r241
%r243 = zext i576 %r242 to i640
%r244 = zext i576 %r235 to i640
%r245 = add i640 %r243, %r244
%r246 = lshr i640 %r245, 64
%r247 = trunc i640 %r246 to i576
%r248 = lshr i576 %r247, 512
%r249 = trunc i576 %r248 to i64
%r250 = trunc i576 %r247 to i512
%r251 = trunc i512 %r250 to i64
%r252 = mul i64 %r251, %r6
%r253 = call i576 @mulPv512x64(i64* %r3, i64 %r252)
%r254 = zext i64 %r249 to i576
%r255 = shl i576 %r254, 512
%r256 = add i576 %r253, %r255
%r258 = getelementptr i64, i64* %r2, i32 15
%r259 = load i64, i64* %r258
%r260 = zext i64 %r259 to i576
%r261 = shl i576 %r260, 512
%r262 = zext i512 %r250 to i576
%r263 = or i576 %r261, %r262
%r264 = zext i576 %r263 to i640
%r265 = zext i576 %r256 to i640
%r266 = add i640 %r264, %r265
%r267 = lshr i640 %r266, 64
%r268 = trunc i640 %r267 to i576
%r269 = lshr i576 %r268, 512
%r270 = trunc i576 %r269 to i64
%r271 = trunc i576 %r268 to i512
%r272 = sub i512 %r271, %r56
%r273 = lshr i512 %r272, 511
%r274 = trunc i512 %r273 to i1
%r275 = select i1 %r274, i512 %r271, i512 %r272
%r277 = getelementptr i64, i64* %r1, i32 0
%r278 = trunc i512 %r275 to i64
store i64 %r278, i64* %r277
%r279 = lshr i512 %r275, 64
%r281 = getelementptr i64, i64* %r1, i32 1
%r282 = trunc i512 %r279 to i64
store i64 %r282, i64* %r281
%r283 = lshr i512 %r279, 64
%r285 = getelementptr i64, i64* %r1, i32 2
%r286 = trunc i512 %r283 to i64
store i64 %r286, i64* %r285
%r287 = lshr i512 %r283, 64
%r289 = getelementptr i64, i64* %r1, i32 3
%r290 = trunc i512 %r287 to i64
store i64 %r290, i64* %r289
%r291 = lshr i512 %r287, 64
%r293 = getelementptr i64, i64* %r1, i32 4
%r294 = trunc i512 %r291 to i64
store i64 %r294, i64* %r293
%r295 = lshr i512 %r291, 64
%r297 = getelementptr i64, i64* %r1, i32 5
%r298 = trunc i512 %r295 to i64
store i64 %r298, i64* %r297
%r299 = lshr i512 %r295, 64
%r301 = getelementptr i64, i64* %r1, i32 6
%r302 = trunc i512 %r299 to i64
store i64 %r302, i64* %r301
%r303 = lshr i512 %r299, 64
%r305 = getelementptr i64, i64* %r1, i32 7
%r306 = trunc i512 %r303 to i64
store i64 %r306, i64* %r305
ret void
}
define i64 @mcl_fp_addPre8L(i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r3
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r3, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r3, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r3, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r3, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r3, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r3, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r3, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = zext i512 %r54 to i576
%r56 = load i64, i64* %r4
%r57 = zext i64 %r56 to i128
%r59 = getelementptr i64, i64* %r4, i32 1
%r60 = load i64, i64* %r59
%r61 = zext i64 %r60 to i128
%r62 = shl i128 %r61, 64
%r63 = or i128 %r57, %r62
%r64 = zext i128 %r63 to i192
%r66 = getelementptr i64, i64* %r4, i32 2
%r67 = load i64, i64* %r66
%r68 = zext i64 %r67 to i192
%r69 = shl i192 %r68, 128
%r70 = or i192 %r64, %r69
%r71 = zext i192 %r70 to i256
%r73 = getelementptr i64, i64* %r4, i32 3
%r74 = load i64, i64* %r73
%r75 = zext i64 %r74 to i256
%r76 = shl i256 %r75, 192
%r77 = or i256 %r71, %r76
%r78 = zext i256 %r77 to i320
%r80 = getelementptr i64, i64* %r4, i32 4
%r81 = load i64, i64* %r80
%r82 = zext i64 %r81 to i320
%r83 = shl i320 %r82, 256
%r84 = or i320 %r78, %r83
%r85 = zext i320 %r84 to i384
%r87 = getelementptr i64, i64* %r4, i32 5
%r88 = load i64, i64* %r87
%r89 = zext i64 %r88 to i384
%r90 = shl i384 %r89, 320
%r91 = or i384 %r85, %r90
%r92 = zext i384 %r91 to i448
%r94 = getelementptr i64, i64* %r4, i32 6
%r95 = load i64, i64* %r94
%r96 = zext i64 %r95 to i448
%r97 = shl i448 %r96, 384
%r98 = or i448 %r92, %r97
%r99 = zext i448 %r98 to i512
%r101 = getelementptr i64, i64* %r4, i32 7
%r102 = load i64, i64* %r101
%r103 = zext i64 %r102 to i512
%r104 = shl i512 %r103, 448
%r105 = or i512 %r99, %r104
%r106 = zext i512 %r105 to i576
%r107 = add i576 %r55, %r106
%r108 = trunc i576 %r107 to i512
%r110 = getelementptr i64, i64* %r2, i32 0
%r111 = trunc i512 %r108 to i64
store i64 %r111, i64* %r110
%r112 = lshr i512 %r108, 64
%r114 = getelementptr i64, i64* %r2, i32 1
%r115 = trunc i512 %r112 to i64
store i64 %r115, i64* %r114
%r116 = lshr i512 %r112, 64
%r118 = getelementptr i64, i64* %r2, i32 2
%r119 = trunc i512 %r116 to i64
store i64 %r119, i64* %r118
%r120 = lshr i512 %r116, 64
%r122 = getelementptr i64, i64* %r2, i32 3
%r123 = trunc i512 %r120 to i64
store i64 %r123, i64* %r122
%r124 = lshr i512 %r120, 64
%r126 = getelementptr i64, i64* %r2, i32 4
%r127 = trunc i512 %r124 to i64
store i64 %r127, i64* %r126
%r128 = lshr i512 %r124, 64
%r130 = getelementptr i64, i64* %r2, i32 5
%r131 = trunc i512 %r128 to i64
store i64 %r131, i64* %r130
%r132 = lshr i512 %r128, 64
%r134 = getelementptr i64, i64* %r2, i32 6
%r135 = trunc i512 %r132 to i64
store i64 %r135, i64* %r134
%r136 = lshr i512 %r132, 64
%r138 = getelementptr i64, i64* %r2, i32 7
%r139 = trunc i512 %r136 to i64
store i64 %r139, i64* %r138
%r140 = lshr i576 %r107, 512
%r141 = trunc i576 %r140 to i64
ret i64 %r141
}
define i64 @mcl_fp_subPre8L(i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r3
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r3, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r3, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r3, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r3, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r3, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r3, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r3, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = zext i512 %r54 to i576
%r56 = load i64, i64* %r4
%r57 = zext i64 %r56 to i128
%r59 = getelementptr i64, i64* %r4, i32 1
%r60 = load i64, i64* %r59
%r61 = zext i64 %r60 to i128
%r62 = shl i128 %r61, 64
%r63 = or i128 %r57, %r62
%r64 = zext i128 %r63 to i192
%r66 = getelementptr i64, i64* %r4, i32 2
%r67 = load i64, i64* %r66
%r68 = zext i64 %r67 to i192
%r69 = shl i192 %r68, 128
%r70 = or i192 %r64, %r69
%r71 = zext i192 %r70 to i256
%r73 = getelementptr i64, i64* %r4, i32 3
%r74 = load i64, i64* %r73
%r75 = zext i64 %r74 to i256
%r76 = shl i256 %r75, 192
%r77 = or i256 %r71, %r76
%r78 = zext i256 %r77 to i320
%r80 = getelementptr i64, i64* %r4, i32 4
%r81 = load i64, i64* %r80
%r82 = zext i64 %r81 to i320
%r83 = shl i320 %r82, 256
%r84 = or i320 %r78, %r83
%r85 = zext i320 %r84 to i384
%r87 = getelementptr i64, i64* %r4, i32 5
%r88 = load i64, i64* %r87
%r89 = zext i64 %r88 to i384
%r90 = shl i384 %r89, 320
%r91 = or i384 %r85, %r90
%r92 = zext i384 %r91 to i448
%r94 = getelementptr i64, i64* %r4, i32 6
%r95 = load i64, i64* %r94
%r96 = zext i64 %r95 to i448
%r97 = shl i448 %r96, 384
%r98 = or i448 %r92, %r97
%r99 = zext i448 %r98 to i512
%r101 = getelementptr i64, i64* %r4, i32 7
%r102 = load i64, i64* %r101
%r103 = zext i64 %r102 to i512
%r104 = shl i512 %r103, 448
%r105 = or i512 %r99, %r104
%r106 = zext i512 %r105 to i576
%r107 = sub i576 %r55, %r106
%r108 = trunc i576 %r107 to i512
%r110 = getelementptr i64, i64* %r2, i32 0
%r111 = trunc i512 %r108 to i64
store i64 %r111, i64* %r110
%r112 = lshr i512 %r108, 64
%r114 = getelementptr i64, i64* %r2, i32 1
%r115 = trunc i512 %r112 to i64
store i64 %r115, i64* %r114
%r116 = lshr i512 %r112, 64
%r118 = getelementptr i64, i64* %r2, i32 2
%r119 = trunc i512 %r116 to i64
store i64 %r119, i64* %r118
%r120 = lshr i512 %r116, 64
%r122 = getelementptr i64, i64* %r2, i32 3
%r123 = trunc i512 %r120 to i64
store i64 %r123, i64* %r122
%r124 = lshr i512 %r120, 64
%r126 = getelementptr i64, i64* %r2, i32 4
%r127 = trunc i512 %r124 to i64
store i64 %r127, i64* %r126
%r128 = lshr i512 %r124, 64
%r130 = getelementptr i64, i64* %r2, i32 5
%r131 = trunc i512 %r128 to i64
store i64 %r131, i64* %r130
%r132 = lshr i512 %r128, 64
%r134 = getelementptr i64, i64* %r2, i32 6
%r135 = trunc i512 %r132 to i64
store i64 %r135, i64* %r134
%r136 = lshr i512 %r132, 64
%r138 = getelementptr i64, i64* %r2, i32 7
%r139 = trunc i512 %r136 to i64
store i64 %r139, i64* %r138
%r141 = lshr i576 %r107, 512
%r142 = trunc i576 %r141 to i64
%r143 = and i64 %r142, 1
ret i64 %r143
}
define void @mcl_fp_shr1_8L(i64* noalias  %r1, i64* noalias  %r2)
{
%r3 = load i64, i64* %r2
%r4 = zext i64 %r3 to i128
%r6 = getelementptr i64, i64* %r2, i32 1
%r7 = load i64, i64* %r6
%r8 = zext i64 %r7 to i128
%r9 = shl i128 %r8, 64
%r10 = or i128 %r4, %r9
%r11 = zext i128 %r10 to i192
%r13 = getelementptr i64, i64* %r2, i32 2
%r14 = load i64, i64* %r13
%r15 = zext i64 %r14 to i192
%r16 = shl i192 %r15, 128
%r17 = or i192 %r11, %r16
%r18 = zext i192 %r17 to i256
%r20 = getelementptr i64, i64* %r2, i32 3
%r21 = load i64, i64* %r20
%r22 = zext i64 %r21 to i256
%r23 = shl i256 %r22, 192
%r24 = or i256 %r18, %r23
%r25 = zext i256 %r24 to i320
%r27 = getelementptr i64, i64* %r2, i32 4
%r28 = load i64, i64* %r27
%r29 = zext i64 %r28 to i320
%r30 = shl i320 %r29, 256
%r31 = or i320 %r25, %r30
%r32 = zext i320 %r31 to i384
%r34 = getelementptr i64, i64* %r2, i32 5
%r35 = load i64, i64* %r34
%r36 = zext i64 %r35 to i384
%r37 = shl i384 %r36, 320
%r38 = or i384 %r32, %r37
%r39 = zext i384 %r38 to i448
%r41 = getelementptr i64, i64* %r2, i32 6
%r42 = load i64, i64* %r41
%r43 = zext i64 %r42 to i448
%r44 = shl i448 %r43, 384
%r45 = or i448 %r39, %r44
%r46 = zext i448 %r45 to i512
%r48 = getelementptr i64, i64* %r2, i32 7
%r49 = load i64, i64* %r48
%r50 = zext i64 %r49 to i512
%r51 = shl i512 %r50, 448
%r52 = or i512 %r46, %r51
%r53 = lshr i512 %r52, 1
%r55 = getelementptr i64, i64* %r1, i32 0
%r56 = trunc i512 %r53 to i64
store i64 %r56, i64* %r55
%r57 = lshr i512 %r53, 64
%r59 = getelementptr i64, i64* %r1, i32 1
%r60 = trunc i512 %r57 to i64
store i64 %r60, i64* %r59
%r61 = lshr i512 %r57, 64
%r63 = getelementptr i64, i64* %r1, i32 2
%r64 = trunc i512 %r61 to i64
store i64 %r64, i64* %r63
%r65 = lshr i512 %r61, 64
%r67 = getelementptr i64, i64* %r1, i32 3
%r68 = trunc i512 %r65 to i64
store i64 %r68, i64* %r67
%r69 = lshr i512 %r65, 64
%r71 = getelementptr i64, i64* %r1, i32 4
%r72 = trunc i512 %r69 to i64
store i64 %r72, i64* %r71
%r73 = lshr i512 %r69, 64
%r75 = getelementptr i64, i64* %r1, i32 5
%r76 = trunc i512 %r73 to i64
store i64 %r76, i64* %r75
%r77 = lshr i512 %r73, 64
%r79 = getelementptr i64, i64* %r1, i32 6
%r80 = trunc i512 %r77 to i64
store i64 %r80, i64* %r79
%r81 = lshr i512 %r77, 64
%r83 = getelementptr i64, i64* %r1, i32 7
%r84 = trunc i512 %r81 to i64
store i64 %r84, i64* %r83
ret void
}
define void @mcl_fp_add8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = load i64, i64* %r3
%r56 = zext i64 %r55 to i128
%r58 = getelementptr i64, i64* %r3, i32 1
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i128
%r61 = shl i128 %r60, 64
%r62 = or i128 %r56, %r61
%r63 = zext i128 %r62 to i192
%r65 = getelementptr i64, i64* %r3, i32 2
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i192
%r68 = shl i192 %r67, 128
%r69 = or i192 %r63, %r68
%r70 = zext i192 %r69 to i256
%r72 = getelementptr i64, i64* %r3, i32 3
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i256
%r75 = shl i256 %r74, 192
%r76 = or i256 %r70, %r75
%r77 = zext i256 %r76 to i320
%r79 = getelementptr i64, i64* %r3, i32 4
%r80 = load i64, i64* %r79
%r81 = zext i64 %r80 to i320
%r82 = shl i320 %r81, 256
%r83 = or i320 %r77, %r82
%r84 = zext i320 %r83 to i384
%r86 = getelementptr i64, i64* %r3, i32 5
%r87 = load i64, i64* %r86
%r88 = zext i64 %r87 to i384
%r89 = shl i384 %r88, 320
%r90 = or i384 %r84, %r89
%r91 = zext i384 %r90 to i448
%r93 = getelementptr i64, i64* %r3, i32 6
%r94 = load i64, i64* %r93
%r95 = zext i64 %r94 to i448
%r96 = shl i448 %r95, 384
%r97 = or i448 %r91, %r96
%r98 = zext i448 %r97 to i512
%r100 = getelementptr i64, i64* %r3, i32 7
%r101 = load i64, i64* %r100
%r102 = zext i64 %r101 to i512
%r103 = shl i512 %r102, 448
%r104 = or i512 %r98, %r103
%r105 = zext i512 %r54 to i576
%r106 = zext i512 %r104 to i576
%r107 = add i576 %r105, %r106
%r108 = trunc i576 %r107 to i512
%r110 = getelementptr i64, i64* %r1, i32 0
%r111 = trunc i512 %r108 to i64
store i64 %r111, i64* %r110
%r112 = lshr i512 %r108, 64
%r114 = getelementptr i64, i64* %r1, i32 1
%r115 = trunc i512 %r112 to i64
store i64 %r115, i64* %r114
%r116 = lshr i512 %r112, 64
%r118 = getelementptr i64, i64* %r1, i32 2
%r119 = trunc i512 %r116 to i64
store i64 %r119, i64* %r118
%r120 = lshr i512 %r116, 64
%r122 = getelementptr i64, i64* %r1, i32 3
%r123 = trunc i512 %r120 to i64
store i64 %r123, i64* %r122
%r124 = lshr i512 %r120, 64
%r126 = getelementptr i64, i64* %r1, i32 4
%r127 = trunc i512 %r124 to i64
store i64 %r127, i64* %r126
%r128 = lshr i512 %r124, 64
%r130 = getelementptr i64, i64* %r1, i32 5
%r131 = trunc i512 %r128 to i64
store i64 %r131, i64* %r130
%r132 = lshr i512 %r128, 64
%r134 = getelementptr i64, i64* %r1, i32 6
%r135 = trunc i512 %r132 to i64
store i64 %r135, i64* %r134
%r136 = lshr i512 %r132, 64
%r138 = getelementptr i64, i64* %r1, i32 7
%r139 = trunc i512 %r136 to i64
store i64 %r139, i64* %r138
%r140 = load i64, i64* %r4
%r141 = zext i64 %r140 to i128
%r143 = getelementptr i64, i64* %r4, i32 1
%r144 = load i64, i64* %r143
%r145 = zext i64 %r144 to i128
%r146 = shl i128 %r145, 64
%r147 = or i128 %r141, %r146
%r148 = zext i128 %r147 to i192
%r150 = getelementptr i64, i64* %r4, i32 2
%r151 = load i64, i64* %r150
%r152 = zext i64 %r151 to i192
%r153 = shl i192 %r152, 128
%r154 = or i192 %r148, %r153
%r155 = zext i192 %r154 to i256
%r157 = getelementptr i64, i64* %r4, i32 3
%r158 = load i64, i64* %r157
%r159 = zext i64 %r158 to i256
%r160 = shl i256 %r159, 192
%r161 = or i256 %r155, %r160
%r162 = zext i256 %r161 to i320
%r164 = getelementptr i64, i64* %r4, i32 4
%r165 = load i64, i64* %r164
%r166 = zext i64 %r165 to i320
%r167 = shl i320 %r166, 256
%r168 = or i320 %r162, %r167
%r169 = zext i320 %r168 to i384
%r171 = getelementptr i64, i64* %r4, i32 5
%r172 = load i64, i64* %r171
%r173 = zext i64 %r172 to i384
%r174 = shl i384 %r173, 320
%r175 = or i384 %r169, %r174
%r176 = zext i384 %r175 to i448
%r178 = getelementptr i64, i64* %r4, i32 6
%r179 = load i64, i64* %r178
%r180 = zext i64 %r179 to i448
%r181 = shl i448 %r180, 384
%r182 = or i448 %r176, %r181
%r183 = zext i448 %r182 to i512
%r185 = getelementptr i64, i64* %r4, i32 7
%r186 = load i64, i64* %r185
%r187 = zext i64 %r186 to i512
%r188 = shl i512 %r187, 448
%r189 = or i512 %r183, %r188
%r190 = zext i512 %r189 to i576
%r191 = sub i576 %r107, %r190
%r192 = lshr i576 %r191, 512
%r193 = trunc i576 %r192 to i1
br i1%r193, label %carry, label %nocarry
nocarry:
%r194 = trunc i576 %r191 to i512
%r196 = getelementptr i64, i64* %r1, i32 0
%r197 = trunc i512 %r194 to i64
store i64 %r197, i64* %r196
%r198 = lshr i512 %r194, 64
%r200 = getelementptr i64, i64* %r1, i32 1
%r201 = trunc i512 %r198 to i64
store i64 %r201, i64* %r200
%r202 = lshr i512 %r198, 64
%r204 = getelementptr i64, i64* %r1, i32 2
%r205 = trunc i512 %r202 to i64
store i64 %r205, i64* %r204
%r206 = lshr i512 %r202, 64
%r208 = getelementptr i64, i64* %r1, i32 3
%r209 = trunc i512 %r206 to i64
store i64 %r209, i64* %r208
%r210 = lshr i512 %r206, 64
%r212 = getelementptr i64, i64* %r1, i32 4
%r213 = trunc i512 %r210 to i64
store i64 %r213, i64* %r212
%r214 = lshr i512 %r210, 64
%r216 = getelementptr i64, i64* %r1, i32 5
%r217 = trunc i512 %r214 to i64
store i64 %r217, i64* %r216
%r218 = lshr i512 %r214, 64
%r220 = getelementptr i64, i64* %r1, i32 6
%r221 = trunc i512 %r218 to i64
store i64 %r221, i64* %r220
%r222 = lshr i512 %r218, 64
%r224 = getelementptr i64, i64* %r1, i32 7
%r225 = trunc i512 %r222 to i64
store i64 %r225, i64* %r224
ret void
carry:
ret void
}
define void @mcl_fp_addNF8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = load i64, i64* %r3
%r56 = zext i64 %r55 to i128
%r58 = getelementptr i64, i64* %r3, i32 1
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i128
%r61 = shl i128 %r60, 64
%r62 = or i128 %r56, %r61
%r63 = zext i128 %r62 to i192
%r65 = getelementptr i64, i64* %r3, i32 2
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i192
%r68 = shl i192 %r67, 128
%r69 = or i192 %r63, %r68
%r70 = zext i192 %r69 to i256
%r72 = getelementptr i64, i64* %r3, i32 3
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i256
%r75 = shl i256 %r74, 192
%r76 = or i256 %r70, %r75
%r77 = zext i256 %r76 to i320
%r79 = getelementptr i64, i64* %r3, i32 4
%r80 = load i64, i64* %r79
%r81 = zext i64 %r80 to i320
%r82 = shl i320 %r81, 256
%r83 = or i320 %r77, %r82
%r84 = zext i320 %r83 to i384
%r86 = getelementptr i64, i64* %r3, i32 5
%r87 = load i64, i64* %r86
%r88 = zext i64 %r87 to i384
%r89 = shl i384 %r88, 320
%r90 = or i384 %r84, %r89
%r91 = zext i384 %r90 to i448
%r93 = getelementptr i64, i64* %r3, i32 6
%r94 = load i64, i64* %r93
%r95 = zext i64 %r94 to i448
%r96 = shl i448 %r95, 384
%r97 = or i448 %r91, %r96
%r98 = zext i448 %r97 to i512
%r100 = getelementptr i64, i64* %r3, i32 7
%r101 = load i64, i64* %r100
%r102 = zext i64 %r101 to i512
%r103 = shl i512 %r102, 448
%r104 = or i512 %r98, %r103
%r105 = add i512 %r54, %r104
%r106 = load i64, i64* %r4
%r107 = zext i64 %r106 to i128
%r109 = getelementptr i64, i64* %r4, i32 1
%r110 = load i64, i64* %r109
%r111 = zext i64 %r110 to i128
%r112 = shl i128 %r111, 64
%r113 = or i128 %r107, %r112
%r114 = zext i128 %r113 to i192
%r116 = getelementptr i64, i64* %r4, i32 2
%r117 = load i64, i64* %r116
%r118 = zext i64 %r117 to i192
%r119 = shl i192 %r118, 128
%r120 = or i192 %r114, %r119
%r121 = zext i192 %r120 to i256
%r123 = getelementptr i64, i64* %r4, i32 3
%r124 = load i64, i64* %r123
%r125 = zext i64 %r124 to i256
%r126 = shl i256 %r125, 192
%r127 = or i256 %r121, %r126
%r128 = zext i256 %r127 to i320
%r130 = getelementptr i64, i64* %r4, i32 4
%r131 = load i64, i64* %r130
%r132 = zext i64 %r131 to i320
%r133 = shl i320 %r132, 256
%r134 = or i320 %r128, %r133
%r135 = zext i320 %r134 to i384
%r137 = getelementptr i64, i64* %r4, i32 5
%r138 = load i64, i64* %r137
%r139 = zext i64 %r138 to i384
%r140 = shl i384 %r139, 320
%r141 = or i384 %r135, %r140
%r142 = zext i384 %r141 to i448
%r144 = getelementptr i64, i64* %r4, i32 6
%r145 = load i64, i64* %r144
%r146 = zext i64 %r145 to i448
%r147 = shl i448 %r146, 384
%r148 = or i448 %r142, %r147
%r149 = zext i448 %r148 to i512
%r151 = getelementptr i64, i64* %r4, i32 7
%r152 = load i64, i64* %r151
%r153 = zext i64 %r152 to i512
%r154 = shl i512 %r153, 448
%r155 = or i512 %r149, %r154
%r156 = sub i512 %r105, %r155
%r157 = lshr i512 %r156, 511
%r158 = trunc i512 %r157 to i1
%r159 = select i1 %r158, i512 %r105, i512 %r156
%r161 = getelementptr i64, i64* %r1, i32 0
%r162 = trunc i512 %r159 to i64
store i64 %r162, i64* %r161
%r163 = lshr i512 %r159, 64
%r165 = getelementptr i64, i64* %r1, i32 1
%r166 = trunc i512 %r163 to i64
store i64 %r166, i64* %r165
%r167 = lshr i512 %r163, 64
%r169 = getelementptr i64, i64* %r1, i32 2
%r170 = trunc i512 %r167 to i64
store i64 %r170, i64* %r169
%r171 = lshr i512 %r167, 64
%r173 = getelementptr i64, i64* %r1, i32 3
%r174 = trunc i512 %r171 to i64
store i64 %r174, i64* %r173
%r175 = lshr i512 %r171, 64
%r177 = getelementptr i64, i64* %r1, i32 4
%r178 = trunc i512 %r175 to i64
store i64 %r178, i64* %r177
%r179 = lshr i512 %r175, 64
%r181 = getelementptr i64, i64* %r1, i32 5
%r182 = trunc i512 %r179 to i64
store i64 %r182, i64* %r181
%r183 = lshr i512 %r179, 64
%r185 = getelementptr i64, i64* %r1, i32 6
%r186 = trunc i512 %r183 to i64
store i64 %r186, i64* %r185
%r187 = lshr i512 %r183, 64
%r189 = getelementptr i64, i64* %r1, i32 7
%r190 = trunc i512 %r187 to i64
store i64 %r190, i64* %r189
ret void
}
define void @mcl_fp_sub8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = load i64, i64* %r3
%r56 = zext i64 %r55 to i128
%r58 = getelementptr i64, i64* %r3, i32 1
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i128
%r61 = shl i128 %r60, 64
%r62 = or i128 %r56, %r61
%r63 = zext i128 %r62 to i192
%r65 = getelementptr i64, i64* %r3, i32 2
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i192
%r68 = shl i192 %r67, 128
%r69 = or i192 %r63, %r68
%r70 = zext i192 %r69 to i256
%r72 = getelementptr i64, i64* %r3, i32 3
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i256
%r75 = shl i256 %r74, 192
%r76 = or i256 %r70, %r75
%r77 = zext i256 %r76 to i320
%r79 = getelementptr i64, i64* %r3, i32 4
%r80 = load i64, i64* %r79
%r81 = zext i64 %r80 to i320
%r82 = shl i320 %r81, 256
%r83 = or i320 %r77, %r82
%r84 = zext i320 %r83 to i384
%r86 = getelementptr i64, i64* %r3, i32 5
%r87 = load i64, i64* %r86
%r88 = zext i64 %r87 to i384
%r89 = shl i384 %r88, 320
%r90 = or i384 %r84, %r89
%r91 = zext i384 %r90 to i448
%r93 = getelementptr i64, i64* %r3, i32 6
%r94 = load i64, i64* %r93
%r95 = zext i64 %r94 to i448
%r96 = shl i448 %r95, 384
%r97 = or i448 %r91, %r96
%r98 = zext i448 %r97 to i512
%r100 = getelementptr i64, i64* %r3, i32 7
%r101 = load i64, i64* %r100
%r102 = zext i64 %r101 to i512
%r103 = shl i512 %r102, 448
%r104 = or i512 %r98, %r103
%r105 = zext i512 %r54 to i576
%r106 = zext i512 %r104 to i576
%r107 = sub i576 %r105, %r106
%r108 = trunc i576 %r107 to i512
%r109 = lshr i576 %r107, 512
%r110 = trunc i576 %r109 to i1
%r112 = getelementptr i64, i64* %r1, i32 0
%r113 = trunc i512 %r108 to i64
store i64 %r113, i64* %r112
%r114 = lshr i512 %r108, 64
%r116 = getelementptr i64, i64* %r1, i32 1
%r117 = trunc i512 %r114 to i64
store i64 %r117, i64* %r116
%r118 = lshr i512 %r114, 64
%r120 = getelementptr i64, i64* %r1, i32 2
%r121 = trunc i512 %r118 to i64
store i64 %r121, i64* %r120
%r122 = lshr i512 %r118, 64
%r124 = getelementptr i64, i64* %r1, i32 3
%r125 = trunc i512 %r122 to i64
store i64 %r125, i64* %r124
%r126 = lshr i512 %r122, 64
%r128 = getelementptr i64, i64* %r1, i32 4
%r129 = trunc i512 %r126 to i64
store i64 %r129, i64* %r128
%r130 = lshr i512 %r126, 64
%r132 = getelementptr i64, i64* %r1, i32 5
%r133 = trunc i512 %r130 to i64
store i64 %r133, i64* %r132
%r134 = lshr i512 %r130, 64
%r136 = getelementptr i64, i64* %r1, i32 6
%r137 = trunc i512 %r134 to i64
store i64 %r137, i64* %r136
%r138 = lshr i512 %r134, 64
%r140 = getelementptr i64, i64* %r1, i32 7
%r141 = trunc i512 %r138 to i64
store i64 %r141, i64* %r140
br i1%r110, label %carry, label %nocarry
nocarry:
ret void
carry:
%r142 = load i64, i64* %r4
%r143 = zext i64 %r142 to i128
%r145 = getelementptr i64, i64* %r4, i32 1
%r146 = load i64, i64* %r145
%r147 = zext i64 %r146 to i128
%r148 = shl i128 %r147, 64
%r149 = or i128 %r143, %r148
%r150 = zext i128 %r149 to i192
%r152 = getelementptr i64, i64* %r4, i32 2
%r153 = load i64, i64* %r152
%r154 = zext i64 %r153 to i192
%r155 = shl i192 %r154, 128
%r156 = or i192 %r150, %r155
%r157 = zext i192 %r156 to i256
%r159 = getelementptr i64, i64* %r4, i32 3
%r160 = load i64, i64* %r159
%r161 = zext i64 %r160 to i256
%r162 = shl i256 %r161, 192
%r163 = or i256 %r157, %r162
%r164 = zext i256 %r163 to i320
%r166 = getelementptr i64, i64* %r4, i32 4
%r167 = load i64, i64* %r166
%r168 = zext i64 %r167 to i320
%r169 = shl i320 %r168, 256
%r170 = or i320 %r164, %r169
%r171 = zext i320 %r170 to i384
%r173 = getelementptr i64, i64* %r4, i32 5
%r174 = load i64, i64* %r173
%r175 = zext i64 %r174 to i384
%r176 = shl i384 %r175, 320
%r177 = or i384 %r171, %r176
%r178 = zext i384 %r177 to i448
%r180 = getelementptr i64, i64* %r4, i32 6
%r181 = load i64, i64* %r180
%r182 = zext i64 %r181 to i448
%r183 = shl i448 %r182, 384
%r184 = or i448 %r178, %r183
%r185 = zext i448 %r184 to i512
%r187 = getelementptr i64, i64* %r4, i32 7
%r188 = load i64, i64* %r187
%r189 = zext i64 %r188 to i512
%r190 = shl i512 %r189, 448
%r191 = or i512 %r185, %r190
%r192 = add i512 %r108, %r191
%r194 = getelementptr i64, i64* %r1, i32 0
%r195 = trunc i512 %r192 to i64
store i64 %r195, i64* %r194
%r196 = lshr i512 %r192, 64
%r198 = getelementptr i64, i64* %r1, i32 1
%r199 = trunc i512 %r196 to i64
store i64 %r199, i64* %r198
%r200 = lshr i512 %r196, 64
%r202 = getelementptr i64, i64* %r1, i32 2
%r203 = trunc i512 %r200 to i64
store i64 %r203, i64* %r202
%r204 = lshr i512 %r200, 64
%r206 = getelementptr i64, i64* %r1, i32 3
%r207 = trunc i512 %r204 to i64
store i64 %r207, i64* %r206
%r208 = lshr i512 %r204, 64
%r210 = getelementptr i64, i64* %r1, i32 4
%r211 = trunc i512 %r208 to i64
store i64 %r211, i64* %r210
%r212 = lshr i512 %r208, 64
%r214 = getelementptr i64, i64* %r1, i32 5
%r215 = trunc i512 %r212 to i64
store i64 %r215, i64* %r214
%r216 = lshr i512 %r212, 64
%r218 = getelementptr i64, i64* %r1, i32 6
%r219 = trunc i512 %r216 to i64
store i64 %r219, i64* %r218
%r220 = lshr i512 %r216, 64
%r222 = getelementptr i64, i64* %r1, i32 7
%r223 = trunc i512 %r220 to i64
store i64 %r223, i64* %r222
ret void
}
define void @mcl_fp_subNF8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = load i64, i64* %r3
%r56 = zext i64 %r55 to i128
%r58 = getelementptr i64, i64* %r3, i32 1
%r59 = load i64, i64* %r58
%r60 = zext i64 %r59 to i128
%r61 = shl i128 %r60, 64
%r62 = or i128 %r56, %r61
%r63 = zext i128 %r62 to i192
%r65 = getelementptr i64, i64* %r3, i32 2
%r66 = load i64, i64* %r65
%r67 = zext i64 %r66 to i192
%r68 = shl i192 %r67, 128
%r69 = or i192 %r63, %r68
%r70 = zext i192 %r69 to i256
%r72 = getelementptr i64, i64* %r3, i32 3
%r73 = load i64, i64* %r72
%r74 = zext i64 %r73 to i256
%r75 = shl i256 %r74, 192
%r76 = or i256 %r70, %r75
%r77 = zext i256 %r76 to i320
%r79 = getelementptr i64, i64* %r3, i32 4
%r80 = load i64, i64* %r79
%r81 = zext i64 %r80 to i320
%r82 = shl i320 %r81, 256
%r83 = or i320 %r77, %r82
%r84 = zext i320 %r83 to i384
%r86 = getelementptr i64, i64* %r3, i32 5
%r87 = load i64, i64* %r86
%r88 = zext i64 %r87 to i384
%r89 = shl i384 %r88, 320
%r90 = or i384 %r84, %r89
%r91 = zext i384 %r90 to i448
%r93 = getelementptr i64, i64* %r3, i32 6
%r94 = load i64, i64* %r93
%r95 = zext i64 %r94 to i448
%r96 = shl i448 %r95, 384
%r97 = or i448 %r91, %r96
%r98 = zext i448 %r97 to i512
%r100 = getelementptr i64, i64* %r3, i32 7
%r101 = load i64, i64* %r100
%r102 = zext i64 %r101 to i512
%r103 = shl i512 %r102, 448
%r104 = or i512 %r98, %r103
%r105 = sub i512 %r54, %r104
%r106 = lshr i512 %r105, 511
%r107 = trunc i512 %r106 to i1
%r108 = load i64, i64* %r4
%r109 = zext i64 %r108 to i128
%r111 = getelementptr i64, i64* %r4, i32 1
%r112 = load i64, i64* %r111
%r113 = zext i64 %r112 to i128
%r114 = shl i128 %r113, 64
%r115 = or i128 %r109, %r114
%r116 = zext i128 %r115 to i192
%r118 = getelementptr i64, i64* %r4, i32 2
%r119 = load i64, i64* %r118
%r120 = zext i64 %r119 to i192
%r121 = shl i192 %r120, 128
%r122 = or i192 %r116, %r121
%r123 = zext i192 %r122 to i256
%r125 = getelementptr i64, i64* %r4, i32 3
%r126 = load i64, i64* %r125
%r127 = zext i64 %r126 to i256
%r128 = shl i256 %r127, 192
%r129 = or i256 %r123, %r128
%r130 = zext i256 %r129 to i320
%r132 = getelementptr i64, i64* %r4, i32 4
%r133 = load i64, i64* %r132
%r134 = zext i64 %r133 to i320
%r135 = shl i320 %r134, 256
%r136 = or i320 %r130, %r135
%r137 = zext i320 %r136 to i384
%r139 = getelementptr i64, i64* %r4, i32 5
%r140 = load i64, i64* %r139
%r141 = zext i64 %r140 to i384
%r142 = shl i384 %r141, 320
%r143 = or i384 %r137, %r142
%r144 = zext i384 %r143 to i448
%r146 = getelementptr i64, i64* %r4, i32 6
%r147 = load i64, i64* %r146
%r148 = zext i64 %r147 to i448
%r149 = shl i448 %r148, 384
%r150 = or i448 %r144, %r149
%r151 = zext i448 %r150 to i512
%r153 = getelementptr i64, i64* %r4, i32 7
%r154 = load i64, i64* %r153
%r155 = zext i64 %r154 to i512
%r156 = shl i512 %r155, 448
%r157 = or i512 %r151, %r156
%r159 = select i1 %r107, i512 %r157, i512 0
%r160 = add i512 %r105, %r159
%r162 = getelementptr i64, i64* %r1, i32 0
%r163 = trunc i512 %r160 to i64
store i64 %r163, i64* %r162
%r164 = lshr i512 %r160, 64
%r166 = getelementptr i64, i64* %r1, i32 1
%r167 = trunc i512 %r164 to i64
store i64 %r167, i64* %r166
%r168 = lshr i512 %r164, 64
%r170 = getelementptr i64, i64* %r1, i32 2
%r171 = trunc i512 %r168 to i64
store i64 %r171, i64* %r170
%r172 = lshr i512 %r168, 64
%r174 = getelementptr i64, i64* %r1, i32 3
%r175 = trunc i512 %r172 to i64
store i64 %r175, i64* %r174
%r176 = lshr i512 %r172, 64
%r178 = getelementptr i64, i64* %r1, i32 4
%r179 = trunc i512 %r176 to i64
store i64 %r179, i64* %r178
%r180 = lshr i512 %r176, 64
%r182 = getelementptr i64, i64* %r1, i32 5
%r183 = trunc i512 %r180 to i64
store i64 %r183, i64* %r182
%r184 = lshr i512 %r180, 64
%r186 = getelementptr i64, i64* %r1, i32 6
%r187 = trunc i512 %r184 to i64
store i64 %r187, i64* %r186
%r188 = lshr i512 %r184, 64
%r190 = getelementptr i64, i64* %r1, i32 7
%r191 = trunc i512 %r188 to i64
store i64 %r191, i64* %r190
ret void
}
define void @mcl_fpDbl_add8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = zext i512 %r54 to i576
%r57 = getelementptr i64, i64* %r2, i32 8
%r58 = load i64, i64* %r57
%r59 = zext i64 %r58 to i576
%r60 = shl i576 %r59, 512
%r61 = or i576 %r55, %r60
%r62 = zext i576 %r61 to i640
%r64 = getelementptr i64, i64* %r2, i32 9
%r65 = load i64, i64* %r64
%r66 = zext i64 %r65 to i640
%r67 = shl i640 %r66, 576
%r68 = or i640 %r62, %r67
%r69 = zext i640 %r68 to i704
%r71 = getelementptr i64, i64* %r2, i32 10
%r72 = load i64, i64* %r71
%r73 = zext i64 %r72 to i704
%r74 = shl i704 %r73, 640
%r75 = or i704 %r69, %r74
%r76 = zext i704 %r75 to i768
%r78 = getelementptr i64, i64* %r2, i32 11
%r79 = load i64, i64* %r78
%r80 = zext i64 %r79 to i768
%r81 = shl i768 %r80, 704
%r82 = or i768 %r76, %r81
%r83 = zext i768 %r82 to i832
%r85 = getelementptr i64, i64* %r2, i32 12
%r86 = load i64, i64* %r85
%r87 = zext i64 %r86 to i832
%r88 = shl i832 %r87, 768
%r89 = or i832 %r83, %r88
%r90 = zext i832 %r89 to i896
%r92 = getelementptr i64, i64* %r2, i32 13
%r93 = load i64, i64* %r92
%r94 = zext i64 %r93 to i896
%r95 = shl i896 %r94, 832
%r96 = or i896 %r90, %r95
%r97 = zext i896 %r96 to i960
%r99 = getelementptr i64, i64* %r2, i32 14
%r100 = load i64, i64* %r99
%r101 = zext i64 %r100 to i960
%r102 = shl i960 %r101, 896
%r103 = or i960 %r97, %r102
%r104 = zext i960 %r103 to i1024
%r106 = getelementptr i64, i64* %r2, i32 15
%r107 = load i64, i64* %r106
%r108 = zext i64 %r107 to i1024
%r109 = shl i1024 %r108, 960
%r110 = or i1024 %r104, %r109
%r111 = load i64, i64* %r3
%r112 = zext i64 %r111 to i128
%r114 = getelementptr i64, i64* %r3, i32 1
%r115 = load i64, i64* %r114
%r116 = zext i64 %r115 to i128
%r117 = shl i128 %r116, 64
%r118 = or i128 %r112, %r117
%r119 = zext i128 %r118 to i192
%r121 = getelementptr i64, i64* %r3, i32 2
%r122 = load i64, i64* %r121
%r123 = zext i64 %r122 to i192
%r124 = shl i192 %r123, 128
%r125 = or i192 %r119, %r124
%r126 = zext i192 %r125 to i256
%r128 = getelementptr i64, i64* %r3, i32 3
%r129 = load i64, i64* %r128
%r130 = zext i64 %r129 to i256
%r131 = shl i256 %r130, 192
%r132 = or i256 %r126, %r131
%r133 = zext i256 %r132 to i320
%r135 = getelementptr i64, i64* %r3, i32 4
%r136 = load i64, i64* %r135
%r137 = zext i64 %r136 to i320
%r138 = shl i320 %r137, 256
%r139 = or i320 %r133, %r138
%r140 = zext i320 %r139 to i384
%r142 = getelementptr i64, i64* %r3, i32 5
%r143 = load i64, i64* %r142
%r144 = zext i64 %r143 to i384
%r145 = shl i384 %r144, 320
%r146 = or i384 %r140, %r145
%r147 = zext i384 %r146 to i448
%r149 = getelementptr i64, i64* %r3, i32 6
%r150 = load i64, i64* %r149
%r151 = zext i64 %r150 to i448
%r152 = shl i448 %r151, 384
%r153 = or i448 %r147, %r152
%r154 = zext i448 %r153 to i512
%r156 = getelementptr i64, i64* %r3, i32 7
%r157 = load i64, i64* %r156
%r158 = zext i64 %r157 to i512
%r159 = shl i512 %r158, 448
%r160 = or i512 %r154, %r159
%r161 = zext i512 %r160 to i576
%r163 = getelementptr i64, i64* %r3, i32 8
%r164 = load i64, i64* %r163
%r165 = zext i64 %r164 to i576
%r166 = shl i576 %r165, 512
%r167 = or i576 %r161, %r166
%r168 = zext i576 %r167 to i640
%r170 = getelementptr i64, i64* %r3, i32 9
%r171 = load i64, i64* %r170
%r172 = zext i64 %r171 to i640
%r173 = shl i640 %r172, 576
%r174 = or i640 %r168, %r173
%r175 = zext i640 %r174 to i704
%r177 = getelementptr i64, i64* %r3, i32 10
%r178 = load i64, i64* %r177
%r179 = zext i64 %r178 to i704
%r180 = shl i704 %r179, 640
%r181 = or i704 %r175, %r180
%r182 = zext i704 %r181 to i768
%r184 = getelementptr i64, i64* %r3, i32 11
%r185 = load i64, i64* %r184
%r186 = zext i64 %r185 to i768
%r187 = shl i768 %r186, 704
%r188 = or i768 %r182, %r187
%r189 = zext i768 %r188 to i832
%r191 = getelementptr i64, i64* %r3, i32 12
%r192 = load i64, i64* %r191
%r193 = zext i64 %r192 to i832
%r194 = shl i832 %r193, 768
%r195 = or i832 %r189, %r194
%r196 = zext i832 %r195 to i896
%r198 = getelementptr i64, i64* %r3, i32 13
%r199 = load i64, i64* %r198
%r200 = zext i64 %r199 to i896
%r201 = shl i896 %r200, 832
%r202 = or i896 %r196, %r201
%r203 = zext i896 %r202 to i960
%r205 = getelementptr i64, i64* %r3, i32 14
%r206 = load i64, i64* %r205
%r207 = zext i64 %r206 to i960
%r208 = shl i960 %r207, 896
%r209 = or i960 %r203, %r208
%r210 = zext i960 %r209 to i1024
%r212 = getelementptr i64, i64* %r3, i32 15
%r213 = load i64, i64* %r212
%r214 = zext i64 %r213 to i1024
%r215 = shl i1024 %r214, 960
%r216 = or i1024 %r210, %r215
%r217 = zext i1024 %r110 to i1088
%r218 = zext i1024 %r216 to i1088
%r219 = add i1088 %r217, %r218
%r220 = trunc i1088 %r219 to i512
%r222 = getelementptr i64, i64* %r1, i32 0
%r223 = trunc i512 %r220 to i64
store i64 %r223, i64* %r222
%r224 = lshr i512 %r220, 64
%r226 = getelementptr i64, i64* %r1, i32 1
%r227 = trunc i512 %r224 to i64
store i64 %r227, i64* %r226
%r228 = lshr i512 %r224, 64
%r230 = getelementptr i64, i64* %r1, i32 2
%r231 = trunc i512 %r228 to i64
store i64 %r231, i64* %r230
%r232 = lshr i512 %r228, 64
%r234 = getelementptr i64, i64* %r1, i32 3
%r235 = trunc i512 %r232 to i64
store i64 %r235, i64* %r234
%r236 = lshr i512 %r232, 64
%r238 = getelementptr i64, i64* %r1, i32 4
%r239 = trunc i512 %r236 to i64
store i64 %r239, i64* %r238
%r240 = lshr i512 %r236, 64
%r242 = getelementptr i64, i64* %r1, i32 5
%r243 = trunc i512 %r240 to i64
store i64 %r243, i64* %r242
%r244 = lshr i512 %r240, 64
%r246 = getelementptr i64, i64* %r1, i32 6
%r247 = trunc i512 %r244 to i64
store i64 %r247, i64* %r246
%r248 = lshr i512 %r244, 64
%r250 = getelementptr i64, i64* %r1, i32 7
%r251 = trunc i512 %r248 to i64
store i64 %r251, i64* %r250
%r252 = lshr i1088 %r219, 512
%r253 = trunc i1088 %r252 to i576
%r254 = load i64, i64* %r4
%r255 = zext i64 %r254 to i128
%r257 = getelementptr i64, i64* %r4, i32 1
%r258 = load i64, i64* %r257
%r259 = zext i64 %r258 to i128
%r260 = shl i128 %r259, 64
%r261 = or i128 %r255, %r260
%r262 = zext i128 %r261 to i192
%r264 = getelementptr i64, i64* %r4, i32 2
%r265 = load i64, i64* %r264
%r266 = zext i64 %r265 to i192
%r267 = shl i192 %r266, 128
%r268 = or i192 %r262, %r267
%r269 = zext i192 %r268 to i256
%r271 = getelementptr i64, i64* %r4, i32 3
%r272 = load i64, i64* %r271
%r273 = zext i64 %r272 to i256
%r274 = shl i256 %r273, 192
%r275 = or i256 %r269, %r274
%r276 = zext i256 %r275 to i320
%r278 = getelementptr i64, i64* %r4, i32 4
%r279 = load i64, i64* %r278
%r280 = zext i64 %r279 to i320
%r281 = shl i320 %r280, 256
%r282 = or i320 %r276, %r281
%r283 = zext i320 %r282 to i384
%r285 = getelementptr i64, i64* %r4, i32 5
%r286 = load i64, i64* %r285
%r287 = zext i64 %r286 to i384
%r288 = shl i384 %r287, 320
%r289 = or i384 %r283, %r288
%r290 = zext i384 %r289 to i448
%r292 = getelementptr i64, i64* %r4, i32 6
%r293 = load i64, i64* %r292
%r294 = zext i64 %r293 to i448
%r295 = shl i448 %r294, 384
%r296 = or i448 %r290, %r295
%r297 = zext i448 %r296 to i512
%r299 = getelementptr i64, i64* %r4, i32 7
%r300 = load i64, i64* %r299
%r301 = zext i64 %r300 to i512
%r302 = shl i512 %r301, 448
%r303 = or i512 %r297, %r302
%r304 = zext i512 %r303 to i576
%r305 = sub i576 %r253, %r304
%r306 = lshr i576 %r305, 512
%r307 = trunc i576 %r306 to i1
%r308 = select i1 %r307, i576 %r253, i576 %r305
%r309 = trunc i576 %r308 to i512
%r311 = getelementptr i64, i64* %r1, i32 8
%r313 = getelementptr i64, i64* %r311, i32 0
%r314 = trunc i512 %r309 to i64
store i64 %r314, i64* %r313
%r315 = lshr i512 %r309, 64
%r317 = getelementptr i64, i64* %r311, i32 1
%r318 = trunc i512 %r315 to i64
store i64 %r318, i64* %r317
%r319 = lshr i512 %r315, 64
%r321 = getelementptr i64, i64* %r311, i32 2
%r322 = trunc i512 %r319 to i64
store i64 %r322, i64* %r321
%r323 = lshr i512 %r319, 64
%r325 = getelementptr i64, i64* %r311, i32 3
%r326 = trunc i512 %r323 to i64
store i64 %r326, i64* %r325
%r327 = lshr i512 %r323, 64
%r329 = getelementptr i64, i64* %r311, i32 4
%r330 = trunc i512 %r327 to i64
store i64 %r330, i64* %r329
%r331 = lshr i512 %r327, 64
%r333 = getelementptr i64, i64* %r311, i32 5
%r334 = trunc i512 %r331 to i64
store i64 %r334, i64* %r333
%r335 = lshr i512 %r331, 64
%r337 = getelementptr i64, i64* %r311, i32 6
%r338 = trunc i512 %r335 to i64
store i64 %r338, i64* %r337
%r339 = lshr i512 %r335, 64
%r341 = getelementptr i64, i64* %r311, i32 7
%r342 = trunc i512 %r339 to i64
store i64 %r342, i64* %r341
ret void
}
define void @mcl_fpDbl_sub8L(i64* noalias  %r1, i64* noalias  %r2, i64* noalias  %r3, i64* noalias  %r4)
{
%r5 = load i64, i64* %r2
%r6 = zext i64 %r5 to i128
%r8 = getelementptr i64, i64* %r2, i32 1
%r9 = load i64, i64* %r8
%r10 = zext i64 %r9 to i128
%r11 = shl i128 %r10, 64
%r12 = or i128 %r6, %r11
%r13 = zext i128 %r12 to i192
%r15 = getelementptr i64, i64* %r2, i32 2
%r16 = load i64, i64* %r15
%r17 = zext i64 %r16 to i192
%r18 = shl i192 %r17, 128
%r19 = or i192 %r13, %r18
%r20 = zext i192 %r19 to i256
%r22 = getelementptr i64, i64* %r2, i32 3
%r23 = load i64, i64* %r22
%r24 = zext i64 %r23 to i256
%r25 = shl i256 %r24, 192
%r26 = or i256 %r20, %r25
%r27 = zext i256 %r26 to i320
%r29 = getelementptr i64, i64* %r2, i32 4
%r30 = load i64, i64* %r29
%r31 = zext i64 %r30 to i320
%r32 = shl i320 %r31, 256
%r33 = or i320 %r27, %r32
%r34 = zext i320 %r33 to i384
%r36 = getelementptr i64, i64* %r2, i32 5
%r37 = load i64, i64* %r36
%r38 = zext i64 %r37 to i384
%r39 = shl i384 %r38, 320
%r40 = or i384 %r34, %r39
%r41 = zext i384 %r40 to i448
%r43 = getelementptr i64, i64* %r2, i32 6
%r44 = load i64, i64* %r43
%r45 = zext i64 %r44 to i448
%r46 = shl i448 %r45, 384
%r47 = or i448 %r41, %r46
%r48 = zext i448 %r47 to i512
%r50 = getelementptr i64, i64* %r2, i32 7
%r51 = load i64, i64* %r50
%r52 = zext i64 %r51 to i512
%r53 = shl i512 %r52, 448
%r54 = or i512 %r48, %r53
%r55 = zext i512 %r54 to i576
%r57 = getelementptr i64, i64* %r2, i32 8
%r58 = load i64, i64* %r57
%r59 = zext i64 %r58 to i576
%r60 = shl i576 %r59, 512
%r61 = or i576 %r55, %r60
%r62 = zext i576 %r61 to i640
%r64 = getelementptr i64, i64* %r2, i32 9
%r65 = load i64, i64* %r64
%r66 = zext i64 %r65 to i640
%r67 = shl i640 %r66, 576
%r68 = or i640 %r62, %r67
%r69 = zext i640 %r68 to i704
%r71 = getelementptr i64, i64* %r2, i32 10
%r72 = load i64, i64* %r71
%r73 = zext i64 %r72 to i704
%r74 = shl i704 %r73, 640
%r75 = or i704 %r69, %r74
%r76 = zext i704 %r75 to i768
%r78 = getelementptr i64, i64* %r2, i32 11
%r79 = load i64, i64* %r78
%r80 = zext i64 %r79 to i768
%r81 = shl i768 %r80, 704
%r82 = or i768 %r76, %r81
%r83 = zext i768 %r82 to i832
%r85 = getelementptr i64, i64* %r2, i32 12
%r86 = load i64, i64* %r85
%r87 = zext i64 %r86 to i832
%r88 = shl i832 %r87, 768
%r89 = or i832 %r83, %r88
%r90 = zext i832 %r89 to i896
%r92 = getelementptr i64, i64* %r2, i32 13
%r93 = load i64, i64* %r92
%r94 = zext i64 %r93 to i896
%r95 = shl i896 %r94, 832
%r96 = or i896 %r90, %r95
%r97 = zext i896 %r96 to i960
%r99 = getelementptr i64, i64* %r2, i32 14
%r100 = load i64, i64* %r99
%r101 = zext i64 %r100 to i960
%r102 = shl i960 %r101, 896
%r103 = or i960 %r97, %r102
%r104 = zext i960 %r103 to i1024
%r106 = getelementptr i64, i64* %r2, i32 15
%r107 = load i64, i64* %r106
%r108 = zext i64 %r107 to i1024
%r109 = shl i1024 %r108, 960
%r110 = or i1024 %r104, %r109
%r111 = load i64, i64* %r3
%r112 = zext i64 %r111 to i128
%r114 = getelementptr i64, i64* %r3, i32 1
%r115 = load i64, i64* %r114
%r116 = zext i64 %r115 to i128
%r117 = shl i128 %r116, 64
%r118 = or i128 %r112, %r117
%r119 = zext i128 %r118 to i192
%r121 = getelementptr i64, i64* %r3, i32 2
%r122 = load i64, i64* %r121
%r123 = zext i64 %r122 to i192
%r124 = shl i192 %r123, 128
%r125 = or i192 %r119, %r124
%r126 = zext i192 %r125 to i256
%r128 = getelementptr i64, i64* %r3, i32 3
%r129 = load i64, i64* %r128
%r130 = zext i64 %r129 to i256
%r131 = shl i256 %r130, 192
%r132 = or i256 %r126, %r131
%r133 = zext i256 %r132 to i320
%r135 = getelementptr i64, i64* %r3, i32 4
%r136 = load i64, i64* %r135
%r137 = zext i64 %r136 to i320
%r138 = shl i320 %r137, 256
%r139 = or i320 %r133, %r138
%r140 = zext i320 %r139 to i384
%r142 = getelementptr i64, i64* %r3, i32 5
%r143 = load i64, i64* %r142
%r144 = zext i64 %r143 to i384
%r145 = shl i384 %r144, 320
%r146 = or i384 %r140, %r145
%r147 = zext i384 %r146 to i448
%r149 = getelementptr i64, i64* %r3, i32 6
%r150 = load i64, i64* %r149
%r151 = zext i64 %r150 to i448
%r152 = shl i448 %r151, 384
%r153 = or i448 %r147, %r152
%r154 = zext i448 %r153 to i512
%r156 = getelementptr i64, i64* %r3, i32 7
%r157 = load i64, i64* %r156
%r158 = zext i64 %r157 to i512
%r159 = shl i512 %r158, 448
%r160 = or i512 %r154, %r159
%r161 = zext i512 %r160 to i576
%r163 = getelementptr i64, i64* %r3, i32 8
%r164 = load i64, i64* %r163
%r165 = zext i64 %r164 to i576
%r166 = shl i576 %r165, 512
%r167 = or i576 %r161, %r166
%r168 = zext i576 %r167 to i640
%r170 = getelementptr i64, i64* %r3, i32 9
%r171 = load i64, i64* %r170
%r172 = zext i64 %r171 to i640
%r173 = shl i640 %r172, 576
%r174 = or i640 %r168, %r173
%r175 = zext i640 %r174 to i704
%r177 = getelementptr i64, i64* %r3, i32 10
%r178 = load i64, i64* %r177
%r179 = zext i64 %r178 to i704
%r180 = shl i704 %r179, 640
%r181 = or i704 %r175, %r180
%r182 = zext i704 %r181 to i768
%r184 = getelementptr i64, i64* %r3, i32 11
%r185 = load i64, i64* %r184
%r186 = zext i64 %r185 to i768
%r187 = shl i768 %r186, 704
%r188 = or i768 %r182, %r187
%r189 = zext i768 %r188 to i832
%r191 = getelementptr i64, i64* %r3, i32 12
%r192 = load i64, i64* %r191
%r193 = zext i64 %r192 to i832
%r194 = shl i832 %r193, 768
%r195 = or i832 %r189, %r194
%r196 = zext i832 %r195 to i896
%r198 = getelementptr i64, i64* %r3, i32 13
%r199 = load i64, i64* %r198
%r200 = zext i64 %r199 to i896
%r201 = shl i896 %r200, 832
%r202 = or i896 %r196, %r201
%r203 = zext i896 %r202 to i960
%r205 = getelementptr i64, i64* %r3, i32 14
%r206 = load i64, i64* %r205
%r207 = zext i64 %r206 to i960
%r208 = shl i960 %r207, 896
%r209 = or i960 %r203, %r208
%r210 = zext i960 %r209 to i1024
%r212 = getelementptr i64, i64* %r3, i32 15
%r213 = load i64, i64* %r212
%r214 = zext i64 %r213 to i1024
%r215 = shl i1024 %r214, 960
%r216 = or i1024 %r210, %r215
%r217 = zext i1024 %r110 to i1088
%r218 = zext i1024 %r216 to i1088
%r219 = sub i1088 %r217, %r218
%r220 = trunc i1088 %r219 to i512
%r222 = getelementptr i64, i64* %r1, i32 0
%r223 = trunc i512 %r220 to i64
store i64 %r223, i64* %r222
%r224 = lshr i512 %r220, 64
%r226 = getelementptr i64, i64* %r1, i32 1
%r227 = trunc i512 %r224 to i64
store i64 %r227, i64* %r226
%r228 = lshr i512 %r224, 64
%r230 = getelementptr i64, i64* %r1, i32 2
%r231 = trunc i512 %r228 to i64
store i64 %r231, i64* %r230
%r232 = lshr i512 %r228, 64
%r234 = getelementptr i64, i64* %r1, i32 3
%r235 = trunc i512 %r232 to i64
store i64 %r235, i64* %r234
%r236 = lshr i512 %r232, 64
%r238 = getelementptr i64, i64* %r1, i32 4
%r239 = trunc i512 %r236 to i64
store i64 %r239, i64* %r238
%r240 = lshr i512 %r236, 64
%r242 = getelementptr i64, i64* %r1, i32 5
%r243 = trunc i512 %r240 to i64
store i64 %r243, i64* %r242
%r244 = lshr i512 %r240, 64
%r246 = getelementptr i64, i64* %r1, i32 6
%r247 = trunc i512 %r244 to i64
store i64 %r247, i64* %r246
%r248 = lshr i512 %r244, 64
%r250 = getelementptr i64, i64* %r1, i32 7
%r251 = trunc i512 %r248 to i64
store i64 %r251, i64* %r250
%r252 = lshr i1088 %r219, 512
%r253 = trunc i1088 %r252 to i512
%r254 = lshr i1088 %r219, 1024
%r255 = trunc i1088 %r254 to i1
%r256 = load i64, i64* %r4
%r257 = zext i64 %r256 to i128
%r259 = getelementptr i64, i64* %r4, i32 1
%r260 = load i64, i64* %r259
%r261 = zext i64 %r260 to i128
%r262 = shl i128 %r261, 64
%r263 = or i128 %r257, %r262
%r264 = zext i128 %r263 to i192
%r266 = getelementptr i64, i64* %r4, i32 2
%r267 = load i64, i64* %r266
%r268 = zext i64 %r267 to i192
%r269 = shl i192 %r268, 128
%r270 = or i192 %r264, %r269
%r271 = zext i192 %r270 to i256
%r273 = getelementptr i64, i64* %r4, i32 3
%r274 = load i64, i64* %r273
%r275 = zext i64 %r274 to i256
%r276 = shl i256 %r275, 192
%r277 = or i256 %r271, %r276
%r278 = zext i256 %r277 to i320
%r280 = getelementptr i64, i64* %r4, i32 4
%r281 = load i64, i64* %r280
%r282 = zext i64 %r281 to i320
%r283 = shl i320 %r282, 256
%r284 = or i320 %r278, %r283
%r285 = zext i320 %r284 to i384
%r287 = getelementptr i64, i64* %r4, i32 5
%r288 = load i64, i64* %r287
%r289 = zext i64 %r288 to i384
%r290 = shl i384 %r289, 320
%r291 = or i384 %r285, %r290
%r292 = zext i384 %r291 to i448
%r294 = getelementptr i64, i64* %r4, i32 6
%r295 = load i64, i64* %r294
%r296 = zext i64 %r295 to i448
%r297 = shl i448 %r296, 384
%r298 = or i448 %r292, %r297
%r299 = zext i448 %r298 to i512
%r301 = getelementptr i64, i64* %r4, i32 7
%r302 = load i64, i64* %r301
%r303 = zext i64 %r302 to i512
%r304 = shl i512 %r303, 448
%r305 = or i512 %r299, %r304
%r307 = select i1 %r255, i512 %r305, i512 0
%r308 = add i512 %r253, %r307
%r310 = getelementptr i64, i64* %r1, i32 8
%r312 = getelementptr i64, i64* %r310, i32 0
%r313 = trunc i512 %r308 to i64
store i64 %r313, i64* %r312
%r314 = lshr i512 %r308, 64
%r316 = getelementptr i64, i64* %r310, i32 1
%r317 = trunc i512 %r314 to i64
store i64 %r317, i64* %r316
%r318 = lshr i512 %r314, 64
%r320 = getelementptr i64, i64* %r310, i32 2
%r321 = trunc i512 %r318 to i64
store i64 %r321, i64* %r320
%r322 = lshr i512 %r318, 64
%r324 = getelementptr i64, i64* %r310, i32 3
%r325 = trunc i512 %r322 to i64
store i64 %r325, i64* %r324
%r326 = lshr i512 %r322, 64
%r328 = getelementptr i64, i64* %r310, i32 4
%r329 = trunc i512 %r326 to i64
store i64 %r329, i64* %r328
%r330 = lshr i512 %r326, 64
%r332 = getelementptr i64, i64* %r310, i32 5
%r333 = trunc i512 %r330 to i64
store i64 %r333, i64* %r332
%r334 = lshr i512 %r330, 64
%r336 = getelementptr i64, i64* %r310, i32 6
%r337 = trunc i512 %r334 to i64
store i64 %r337, i64* %r336
%r338 = lshr i512 %r334, 64
%r340 = getelementptr i64, i64* %r310, i32 7
%r341 = trunc i512 %r338 to i64
store i64 %r341, i64* %r340
ret void
}
