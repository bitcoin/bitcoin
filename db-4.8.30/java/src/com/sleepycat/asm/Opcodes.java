/***
 * ASM: a very small and fast Java bytecode manipulation framework
 * Copyright (c) 2000-2005 INRIA, France Telecom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
package com.sleepycat.asm;

/**
 * Defines the JVM opcodes, access flags and array type codes. This interface
 * does not define all the JVM opcodes because some opcodes are automatically
 * handled. For example, the xLOAD and xSTORE opcodes are automatically replaced
 * by xLOAD_n and xSTORE_n opcodes when possible. The xLOAD_n and xSTORE_n
 * opcodes are therefore not defined in this interface. Likewise for LDC,
 * automatically replaced by LDC_W or LDC2_W when necessary, WIDE, GOTO_W and
 * JSR_W.
 *
 * @author Eric Bruneton
 * @author Eugene Kuleshov
 */
public interface Opcodes {

    // versions

    int V1_1 = 3 << 16 | 45;
    int V1_2 = 0 << 16 | 46;
    int V1_3 = 0 << 16 | 47;
    int V1_4 = 0 << 16 | 48;
    int V1_5 = 0 << 16 | 49;
    int V1_6 = 0 << 16 | 50;

    // access flags

    int ACC_PUBLIC = 0x0001; // class, field, method
    int ACC_PRIVATE = 0x0002; // class, field, method
    int ACC_PROTECTED = 0x0004; // class, field, method
    int ACC_STATIC = 0x0008; // field, method
    int ACC_FINAL = 0x0010; // class, field, method
    int ACC_SUPER = 0x0020; // class
    int ACC_SYNCHRONIZED = 0x0020; // method
    int ACC_VOLATILE = 0x0040; // field
    int ACC_BRIDGE = 0x0040; // method
    int ACC_VARARGS = 0x0080; // method
    int ACC_TRANSIENT = 0x0080; // field
    int ACC_NATIVE = 0x0100; // method
    int ACC_INTERFACE = 0x0200; // class
    int ACC_ABSTRACT = 0x0400; // class, method
    int ACC_STRICT = 0x0800; // method
    int ACC_SYNTHETIC = 0x1000; // class, field, method
    int ACC_ANNOTATION = 0x2000; // class
    int ACC_ENUM = 0x4000; // class(?) field inner

    // ASM specific pseudo access flags

    int ACC_DEPRECATED = 131072; // class, field, method

    // types for NEWARRAY

    int T_BOOLEAN = 4;
    int T_CHAR = 5;
    int T_FLOAT = 6;
    int T_DOUBLE = 7;
    int T_BYTE = 8;
    int T_SHORT = 9;
    int T_INT = 10;
    int T_LONG = 11;

    // opcodes // visit method (- = idem)

    int NOP = 0; // visitInsn
    int ACONST_NULL = 1; // -
    int ICONST_M1 = 2; // -
    int ICONST_0 = 3; // -
    int ICONST_1 = 4; // -
    int ICONST_2 = 5; // -
    int ICONST_3 = 6; // -
    int ICONST_4 = 7; // -
    int ICONST_5 = 8; // -
    int LCONST_0 = 9; // -
    int LCONST_1 = 10; // -
    int FCONST_0 = 11; // -
    int FCONST_1 = 12; // -
    int FCONST_2 = 13; // -
    int DCONST_0 = 14; // -
    int DCONST_1 = 15; // -
    int BIPUSH = 16; // visitIntInsn
    int SIPUSH = 17; // -
    int LDC = 18; // visitLdcInsn
    // int LDC_W = 19; // -
    // int LDC2_W = 20; // -
    int ILOAD = 21; // visitVarInsn
    int LLOAD = 22; // -
    int FLOAD = 23; // -
    int DLOAD = 24; // -
    int ALOAD = 25; // -
    // int ILOAD_0 = 26; // -
    // int ILOAD_1 = 27; // -
    // int ILOAD_2 = 28; // -
    // int ILOAD_3 = 29; // -
    // int LLOAD_0 = 30; // -
    // int LLOAD_1 = 31; // -
    // int LLOAD_2 = 32; // -
    // int LLOAD_3 = 33; // -
    // int FLOAD_0 = 34; // -
    // int FLOAD_1 = 35; // -
    // int FLOAD_2 = 36; // -
    // int FLOAD_3 = 37; // -
    // int DLOAD_0 = 38; // -
    // int DLOAD_1 = 39; // -
    // int DLOAD_2 = 40; // -
    // int DLOAD_3 = 41; // -
    // int ALOAD_0 = 42; // -
    // int ALOAD_1 = 43; // -
    // int ALOAD_2 = 44; // -
    // int ALOAD_3 = 45; // -
    int IALOAD = 46; // visitInsn
    int LALOAD = 47; // -
    int FALOAD = 48; // -
    int DALOAD = 49; // -
    int AALOAD = 50; // -
    int BALOAD = 51; // -
    int CALOAD = 52; // -
    int SALOAD = 53; // -
    int ISTORE = 54; // visitVarInsn
    int LSTORE = 55; // -
    int FSTORE = 56; // -
    int DSTORE = 57; // -
    int ASTORE = 58; // -
    // int ISTORE_0 = 59; // -
    // int ISTORE_1 = 60; // -
    // int ISTORE_2 = 61; // -
    // int ISTORE_3 = 62; // -
    // int LSTORE_0 = 63; // -
    // int LSTORE_1 = 64; // -
    // int LSTORE_2 = 65; // -
    // int LSTORE_3 = 66; // -
    // int FSTORE_0 = 67; // -
    // int FSTORE_1 = 68; // -
    // int FSTORE_2 = 69; // -
    // int FSTORE_3 = 70; // -
    // int DSTORE_0 = 71; // -
    // int DSTORE_1 = 72; // -
    // int DSTORE_2 = 73; // -
    // int DSTORE_3 = 74; // -
    // int ASTORE_0 = 75; // -
    // int ASTORE_1 = 76; // -
    // int ASTORE_2 = 77; // -
    // int ASTORE_3 = 78; // -
    int IASTORE = 79; // visitInsn
    int LASTORE = 80; // -
    int FASTORE = 81; // -
    int DASTORE = 82; // -
    int AASTORE = 83; // -
    int BASTORE = 84; // -
    int CASTORE = 85; // -
    int SASTORE = 86; // -
    int POP = 87; // -
    int POP2 = 88; // -
    int DUP = 89; // -
    int DUP_X1 = 90; // -
    int DUP_X2 = 91; // -
    int DUP2 = 92; // -
    int DUP2_X1 = 93; // -
    int DUP2_X2 = 94; // -
    int SWAP = 95; // -
    int IADD = 96; // -
    int LADD = 97; // -
    int FADD = 98; // -
    int DADD = 99; // -
    int ISUB = 100; // -
    int LSUB = 101; // -
    int FSUB = 102; // -
    int DSUB = 103; // -
    int IMUL = 104; // -
    int LMUL = 105; // -
    int FMUL = 106; // -
    int DMUL = 107; // -
    int IDIV = 108; // -
    int LDIV = 109; // -
    int FDIV = 110; // -
    int DDIV = 111; // -
    int IREM = 112; // -
    int LREM = 113; // -
    int FREM = 114; // -
    int DREM = 115; // -
    int INEG = 116; // -
    int LNEG = 117; // -
    int FNEG = 118; // -
    int DNEG = 119; // -
    int ISHL = 120; // -
    int LSHL = 121; // -
    int ISHR = 122; // -
    int LSHR = 123; // -
    int IUSHR = 124; // -
    int LUSHR = 125; // -
    int IAND = 126; // -
    int LAND = 127; // -
    int IOR = 128; // -
    int LOR = 129; // -
    int IXOR = 130; // -
    int LXOR = 131; // -
    int IINC = 132; // visitIincInsn
    int I2L = 133; // visitInsn
    int I2F = 134; // -
    int I2D = 135; // -
    int L2I = 136; // -
    int L2F = 137; // -
    int L2D = 138; // -
    int F2I = 139; // -
    int F2L = 140; // -
    int F2D = 141; // -
    int D2I = 142; // -
    int D2L = 143; // -
    int D2F = 144; // -
    int I2B = 145; // -
    int I2C = 146; // -
    int I2S = 147; // -
    int LCMP = 148; // -
    int FCMPL = 149; // -
    int FCMPG = 150; // -
    int DCMPL = 151; // -
    int DCMPG = 152; // -
    int IFEQ = 153; // visitJumpInsn
    int IFNE = 154; // -
    int IFLT = 155; // -
    int IFGE = 156; // -
    int IFGT = 157; // -
    int IFLE = 158; // -
    int IF_ICMPEQ = 159; // -
    int IF_ICMPNE = 160; // -
    int IF_ICMPLT = 161; // -
    int IF_ICMPGE = 162; // -
    int IF_ICMPGT = 163; // -
    int IF_ICMPLE = 164; // -
    int IF_ACMPEQ = 165; // -
    int IF_ACMPNE = 166; // -
    int GOTO = 167; // -
    int JSR = 168; // -
    int RET = 169; // visitVarInsn
    int TABLESWITCH = 170; // visiTableSwitchInsn
    int LOOKUPSWITCH = 171; // visitLookupSwitch
    int IRETURN = 172; // visitInsn
    int LRETURN = 173; // -
    int FRETURN = 174; // -
    int DRETURN = 175; // -
    int ARETURN = 176; // -
    int RETURN = 177; // -
    int GETSTATIC = 178; // visitFieldInsn
    int PUTSTATIC = 179; // -
    int GETFIELD = 180; // -
    int PUTFIELD = 181; // -
    int INVOKEVIRTUAL = 182; // visitMethodInsn
    int INVOKESPECIAL = 183; // -
    int INVOKESTATIC = 184; // -
    int INVOKEINTERFACE = 185; // -
    // int UNUSED = 186; // NOT VISITED
    int NEW = 187; // visitTypeInsn
    int NEWARRAY = 188; // visitIntInsn
    int ANEWARRAY = 189; // visitTypeInsn
    int ARRAYLENGTH = 190; // visitInsn
    int ATHROW = 191; // -
    int CHECKCAST = 192; // visitTypeInsn
    int INSTANCEOF = 193; // -
    int MONITORENTER = 194; // visitInsn
    int MONITOREXIT = 195; // -
    // int WIDE = 196; // NOT VISITED
    int MULTIANEWARRAY = 197; // visitMultiANewArrayInsn
    int IFNULL = 198; // visitJumpInsn
    int IFNONNULL = 199; // -
    // int GOTO_W = 200; // -
    // int JSR_W = 201; // -
}
