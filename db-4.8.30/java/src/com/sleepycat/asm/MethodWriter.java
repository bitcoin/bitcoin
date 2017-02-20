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
 * A {@link MethodVisitor} that generates methods in bytecode form. Each visit
 * method of this class appends the bytecode corresponding to the visited
 * instruction to a byte vector, in the order these methods are called.
 *
 * @author Eric Bruneton
 */
class MethodWriter implements MethodVisitor {

    /**
     * Next method writer (see {@link ClassWriter#firstMethod firstMethod}).
     */
    MethodWriter next;

    /**
     * The class writer to which this method must be added.
     */
    ClassWriter cw;

    /**
     * Access flags of this method.
     */
    private int access;

    /**
     * The index of the constant pool item that contains the name of this
     * method.
     */
    private int name;

    /**
     * The index of the constant pool item that contains the descriptor of this
     * method.
     */
    private int desc;

    /**
     * The descriptor of this method.
     */
    private String descriptor;

    /**
     * If not zero, indicates that the code of this method must be copied from
     * the ClassReader associated to this writer in <code>cw.cr</code>. More
     * precisely, this field gives the index of the first byte to copied from
     * <code>cw.cr.b</code>.
     */
    int classReaderOffset;

    /**
     * If not zero, indicates that the code of this method must be copied from
     * the ClassReader associated to this writer in <code>cw.cr</code>. More
     * precisely, this field gives the number of bytes to copied from
     * <code>cw.cr.b</code>.
     */
    int classReaderLength;

    /**
     * The signature of this method.
     */
    String signature;

    /**
     * Number of exceptions that can be thrown by this method.
     */
    int exceptionCount;

    /**
     * The exceptions that can be thrown by this method. More precisely, this
     * array contains the indexes of the constant pool items that contain the
     * internal names of these exception classes.
     */
    int[] exceptions;

    /**
     * The annotation default attribute of this method. May be <tt>null</tt>.
     */
    private ByteVector annd;

    /**
     * The runtime visible annotations of this method. May be <tt>null</tt>.
     */
    private AnnotationWriter anns;

    /**
     * The runtime invisible annotations of this method. May be <tt>null</tt>.
     */
    private AnnotationWriter ianns;

    /**
     * The runtime visible parameter annotations of this method. May be
     * <tt>null</tt>.
     */
    private AnnotationWriter[] panns;

    /**
     * The runtime invisible parameter annotations of this method. May be
     * <tt>null</tt>.
     */
    private AnnotationWriter[] ipanns;

    /**
     * The non standard attributes of the method.
     */
    private Attribute attrs;

    /**
     * The bytecode of this method.
     */
    private ByteVector code = new ByteVector();

    /**
     * Maximum stack size of this method.
     */
    private int maxStack;

    /**
     * Maximum number of local variables for this method.
     */
    private int maxLocals;

    /**
     * Number of entries in the catch table of this method.
     */
    private int catchCount;

    /**
     * The catch table of this method.
     */
    private Handler catchTable;

    /**
     * The last element in the catchTable handler list.
     */
    private Handler lastHandler;

    /**
     * Number of entries in the LocalVariableTable attribute.
     */
    private int localVarCount;

    /**
     * The LocalVariableTable attribute.
     */
    private ByteVector localVar;

    /**
     * Number of entries in the LocalVariableTypeTable attribute.
     */
    private int localVarTypeCount;

    /**
     * The LocalVariableTypeTable attribute.
     */
    private ByteVector localVarType;

    /**
     * Number of entries in the LineNumberTable attribute.
     */
    private int lineNumberCount;

    /**
     * The LineNumberTable attribute.
     */
    private ByteVector lineNumber;

    /**
     * The non standard attributes of the method's code.
     */
    private Attribute cattrs;

    /**
     * Indicates if some jump instructions are too small and need to be resized.
     */
    private boolean resize;

    /*
     * Fields for the control flow graph analysis algorithm (used to compute the
     * maximum stack size). A control flow graph contains one node per "basic
     * block", and one edge per "jump" from one basic block to another. Each
     * node (i.e., each basic block) is represented by the Label object that
     * corresponds to the first instruction of this basic block. Each node also
     * stores the list of its successors in the graph, as a linked list of Edge
     * objects.
     */

    /**
     * <tt>true</tt> if the maximum stack size and number of local variables
     * must be automatically computed.
     */
    private final boolean computeMaxs;

    /**
     * The (relative) stack size after the last visited instruction. This size
     * is relative to the beginning of the current basic block, i.e., the true
     * stack size after the last visited instruction is equal to the {@link
     * Label#beginStackSize beginStackSize} of the current basic block plus
     * <tt>stackSize</tt>.
     */
    private int stackSize;

    /**
     * The (relative) maximum stack size after the last visited instruction.
     * This size is relative to the beginning of the current basic block, i.e.,
     * the true maximum stack size after the last visited instruction is equal
     * to the {@link Label#beginStackSize beginStackSize} of the current basic
     * block plus <tt>stackSize</tt>.
     */
    private int maxStackSize;

    /**
     * The current basic block. This block is the basic block to which the next
     * instruction to be visited must be added.
     */
    private Label currentBlock;

    /**
     * The basic block stack used by the control flow analysis algorithm. This
     * stack is represented by a linked list of {@link Label Label} objects,
     * linked to each other by their {@link Label#next} field. This stack must
     * not be confused with the JVM stack used to execute the JVM instructions!
     */
    private Label blockStack;

    /**
     * The stack size variation corresponding to each JVM instruction. This
     * stack variation is equal to the size of the values produced by an
     * instruction, minus the size of the values consumed by this instruction.
     */
    private final static int[] SIZE;

    // ------------------------------------------------------------------------
    // Static initializer
    // ------------------------------------------------------------------------

    /**
     * Computes the stack size variation corresponding to each JVM instruction.
     */
    static {
        int i;
        int[] b = new int[202];
        String s = "EFFFFFFFFGGFFFGGFFFEEFGFGFEEEEEEEEEEEEEEEEEEEEDEDEDDDDD"
                + "CDCDEEEEEEEEEEEEEEEEEEEEBABABBBBDCFFFGGGEDCDCDCDCDCDCDCDCD"
                + "CDCEEEEDDDDDDDCDCDCEFEFDDEEFFDEDEEEBDDBBDDDDDDCCCCCCCCEFED"
                + "DDCDCDEEEEEEEEEEFEEEEEEDDEEDDEE";
        for (i = 0; i < b.length; ++i) {
            b[i] = s.charAt(i) - 'E';
        }
        SIZE = b;

        // code to generate the above string
        //
        // int NA = 0; // not applicable (unused opcode or variable size opcode)
        //
        // b = new int[] {
        // 0, //NOP, // visitInsn
        // 1, //ACONST_NULL, // -
        // 1, //ICONST_M1, // -
        // 1, //ICONST_0, // -
        // 1, //ICONST_1, // -
        // 1, //ICONST_2, // -
        // 1, //ICONST_3, // -
        // 1, //ICONST_4, // -
        // 1, //ICONST_5, // -
        // 2, //LCONST_0, // -
        // 2, //LCONST_1, // -
        // 1, //FCONST_0, // -
        // 1, //FCONST_1, // -
        // 1, //FCONST_2, // -
        // 2, //DCONST_0, // -
        // 2, //DCONST_1, // -
        // 1, //BIPUSH, // visitIntInsn
        // 1, //SIPUSH, // -
        // 1, //LDC, // visitLdcInsn
        // NA, //LDC_W, // -
        // NA, //LDC2_W, // -
        // 1, //ILOAD, // visitVarInsn
        // 2, //LLOAD, // -
        // 1, //FLOAD, // -
        // 2, //DLOAD, // -
        // 1, //ALOAD, // -
        // NA, //ILOAD_0, // -
        // NA, //ILOAD_1, // -
        // NA, //ILOAD_2, // -
        // NA, //ILOAD_3, // -
        // NA, //LLOAD_0, // -
        // NA, //LLOAD_1, // -
        // NA, //LLOAD_2, // -
        // NA, //LLOAD_3, // -
        // NA, //FLOAD_0, // -
        // NA, //FLOAD_1, // -
        // NA, //FLOAD_2, // -
        // NA, //FLOAD_3, // -
        // NA, //DLOAD_0, // -
        // NA, //DLOAD_1, // -
        // NA, //DLOAD_2, // -
        // NA, //DLOAD_3, // -
        // NA, //ALOAD_0, // -
        // NA, //ALOAD_1, // -
        // NA, //ALOAD_2, // -
        // NA, //ALOAD_3, // -
        // -1, //IALOAD, // visitInsn
        // 0, //LALOAD, // -
        // -1, //FALOAD, // -
        // 0, //DALOAD, // -
        // -1, //AALOAD, // -
        // -1, //BALOAD, // -
        // -1, //CALOAD, // -
        // -1, //SALOAD, // -
        // -1, //ISTORE, // visitVarInsn
        // -2, //LSTORE, // -
        // -1, //FSTORE, // -
        // -2, //DSTORE, // -
        // -1, //ASTORE, // -
        // NA, //ISTORE_0, // -
        // NA, //ISTORE_1, // -
        // NA, //ISTORE_2, // -
        // NA, //ISTORE_3, // -
        // NA, //LSTORE_0, // -
        // NA, //LSTORE_1, // -
        // NA, //LSTORE_2, // -
        // NA, //LSTORE_3, // -
        // NA, //FSTORE_0, // -
        // NA, //FSTORE_1, // -
        // NA, //FSTORE_2, // -
        // NA, //FSTORE_3, // -
        // NA, //DSTORE_0, // -
        // NA, //DSTORE_1, // -
        // NA, //DSTORE_2, // -
        // NA, //DSTORE_3, // -
        // NA, //ASTORE_0, // -
        // NA, //ASTORE_1, // -
        // NA, //ASTORE_2, // -
        // NA, //ASTORE_3, // -
        // -3, //IASTORE, // visitInsn
        // -4, //LASTORE, // -
        // -3, //FASTORE, // -
        // -4, //DASTORE, // -
        // -3, //AASTORE, // -
        // -3, //BASTORE, // -
        // -3, //CASTORE, // -
        // -3, //SASTORE, // -
        // -1, //POP, // -
        // -2, //POP2, // -
        // 1, //DUP, // -
        // 1, //DUP_X1, // -
        // 1, //DUP_X2, // -
        // 2, //DUP2, // -
        // 2, //DUP2_X1, // -
        // 2, //DUP2_X2, // -
        // 0, //SWAP, // -
        // -1, //IADD, // -
        // -2, //LADD, // -
        // -1, //FADD, // -
        // -2, //DADD, // -
        // -1, //ISUB, // -
        // -2, //LSUB, // -
        // -1, //FSUB, // -
        // -2, //DSUB, // -
        // -1, //IMUL, // -
        // -2, //LMUL, // -
        // -1, //FMUL, // -
        // -2, //DMUL, // -
        // -1, //IDIV, // -
        // -2, //LDIV, // -
        // -1, //FDIV, // -
        // -2, //DDIV, // -
        // -1, //IREM, // -
        // -2, //LREM, // -
        // -1, //FREM, // -
        // -2, //DREM, // -
        // 0, //INEG, // -
        // 0, //LNEG, // -
        // 0, //FNEG, // -
        // 0, //DNEG, // -
        // -1, //ISHL, // -
        // -1, //LSHL, // -
        // -1, //ISHR, // -
        // -1, //LSHR, // -
        // -1, //IUSHR, // -
        // -1, //LUSHR, // -
        // -1, //IAND, // -
        // -2, //LAND, // -
        // -1, //IOR, // -
        // -2, //LOR, // -
        // -1, //IXOR, // -
        // -2, //LXOR, // -
        // 0, //IINC, // visitIincInsn
        // 1, //I2L, // visitInsn
        // 0, //I2F, // -
        // 1, //I2D, // -
        // -1, //L2I, // -
        // -1, //L2F, // -
        // 0, //L2D, // -
        // 0, //F2I, // -
        // 1, //F2L, // -
        // 1, //F2D, // -
        // -1, //D2I, // -
        // 0, //D2L, // -
        // -1, //D2F, // -
        // 0, //I2B, // -
        // 0, //I2C, // -
        // 0, //I2S, // -
        // -3, //LCMP, // -
        // -1, //FCMPL, // -
        // -1, //FCMPG, // -
        // -3, //DCMPL, // -
        // -3, //DCMPG, // -
        // -1, //IFEQ, // visitJumpInsn
        // -1, //IFNE, // -
        // -1, //IFLT, // -
        // -1, //IFGE, // -
        // -1, //IFGT, // -
        // -1, //IFLE, // -
        // -2, //IF_ICMPEQ, // -
        // -2, //IF_ICMPNE, // -
        // -2, //IF_ICMPLT, // -
        // -2, //IF_ICMPGE, // -
        // -2, //IF_ICMPGT, // -
        // -2, //IF_ICMPLE, // -
        // -2, //IF_ACMPEQ, // -
        // -2, //IF_ACMPNE, // -
        // 0, //GOTO, // -
        // 1, //JSR, // -
        // 0, //RET, // visitVarInsn
        // -1, //TABLESWITCH, // visiTableSwitchInsn
        // -1, //LOOKUPSWITCH, // visitLookupSwitch
        // -1, //IRETURN, // visitInsn
        // -2, //LRETURN, // -
        // -1, //FRETURN, // -
        // -2, //DRETURN, // -
        // -1, //ARETURN, // -
        // 0, //RETURN, // -
        // NA, //GETSTATIC, // visitFieldInsn
        // NA, //PUTSTATIC, // -
        // NA, //GETFIELD, // -
        // NA, //PUTFIELD, // -
        // NA, //INVOKEVIRTUAL, // visitMethodInsn
        // NA, //INVOKESPECIAL, // -
        // NA, //INVOKESTATIC, // -
        // NA, //INVOKEINTERFACE, // -
        // NA, //UNUSED, // NOT VISITED
        // 1, //NEW, // visitTypeInsn
        // 0, //NEWARRAY, // visitIntInsn
        // 0, //ANEWARRAY, // visitTypeInsn
        // 0, //ARRAYLENGTH, // visitInsn
        // NA, //ATHROW, // -
        // 0, //CHECKCAST, // visitTypeInsn
        // 0, //INSTANCEOF, // -
        // -1, //MONITORENTER, // visitInsn
        // -1, //MONITOREXIT, // -
        // NA, //WIDE, // NOT VISITED
        // NA, //MULTIANEWARRAY, // visitMultiANewArrayInsn
        // -1, //IFNULL, // visitJumpInsn
        // -1, //IFNONNULL, // -
        // NA, //GOTO_W, // -
        // NA, //JSR_W, // -
        // };
        // for (i = 0; i < b.length; ++i) {
        // System.err.print((char)('E' + b[i]));
        // }
        // System.err.println();
    }

    // ------------------------------------------------------------------------
    // Constructor
    // ------------------------------------------------------------------------

    /**
     * Constructs a new {@link MethodWriter}.
     *
     * @param cw the class writer in which the method must be added.
     * @param access the method's access flags (see {@link Opcodes}).
     * @param name the method's name.
     * @param desc the method's descriptor (see {@link Type}).
     * @param signature the method's signature. May be <tt>null</tt>.
     * @param exceptions the internal names of the method's exceptions. May be
     *        <tt>null</tt>.
     * @param computeMaxs <tt>true</tt> if the maximum stack size and number
     *        of local variables must be automatically computed.
     */
    MethodWriter(
        final ClassWriter cw,
        final int access,
        final String name,
        final String desc,
        final String signature,
        final String[] exceptions,
        final boolean computeMaxs)
    {
        if (cw.firstMethod == null) {
            cw.firstMethod = this;
        } else {
            cw.lastMethod.next = this;
        }
        cw.lastMethod = this;
        this.cw = cw;
        this.access = access;
        this.name = cw.newUTF8(name);
        this.desc = cw.newUTF8(desc);
        this.descriptor = desc;
        this.signature = signature;
        if (exceptions != null && exceptions.length > 0) {
            exceptionCount = exceptions.length;
            this.exceptions = new int[exceptionCount];
            for (int i = 0; i < exceptionCount; ++i) {
                this.exceptions[i] = cw.newClass(exceptions[i]);
            }
        }
        this.computeMaxs = computeMaxs;
        if (computeMaxs) {
            // updates maxLocals
            int size = getArgumentsAndReturnSizes(desc) >> 2;
            if ((access & Opcodes.ACC_STATIC) != 0) {
                --size;
            }
            maxLocals = size;
            // pushes the first block onto the stack of blocks to be visited
            currentBlock = new Label();
            currentBlock.pushed = true;
            blockStack = currentBlock;
        }
    }

    // ------------------------------------------------------------------------
    // Implementation of the MethodVisitor interface
    // ------------------------------------------------------------------------

    public AnnotationVisitor visitAnnotationDefault() {
        annd = new ByteVector();
        return new AnnotationWriter(cw, false, annd, null, 0);
    }

    public AnnotationVisitor visitAnnotation(
        final String desc,
        final boolean visible)
    {
        ByteVector bv = new ByteVector();
        // write type, and reserve space for values count
        bv.putShort(cw.newUTF8(desc)).putShort(0);
        AnnotationWriter aw = new AnnotationWriter(cw, true, bv, bv, 2);
        if (visible) {
            aw.next = anns;
            anns = aw;
        } else {
            aw.next = ianns;
            ianns = aw;
        }
        return aw;
    }

    public AnnotationVisitor visitParameterAnnotation(
        final int parameter,
        final String desc,
        final boolean visible)
    {
        ByteVector bv = new ByteVector();
        // write type, and reserve space for values count
        bv.putShort(cw.newUTF8(desc)).putShort(0);
        AnnotationWriter aw = new AnnotationWriter(cw, true, bv, bv, 2);
        if (visible) {
            if (panns == null) {
                panns = new AnnotationWriter[Type.getArgumentTypes(descriptor).length];
            }
            aw.next = panns[parameter];
            panns[parameter] = aw;
        } else {
            if (ipanns == null) {
                ipanns = new AnnotationWriter[Type.getArgumentTypes(descriptor).length];
            }
            aw.next = ipanns[parameter];
            ipanns[parameter] = aw;
        }
        return aw;
    }

    public void visitAttribute(final Attribute attr) {
        if (attr.isCodeAttribute()) {
            attr.next = cattrs;
            cattrs = attr;
        } else {
            attr.next = attrs;
            attrs = attr;
        }
    }

    public void visitCode() {
    }

    public void visitInsn(final int opcode) {
        if (computeMaxs) {
            // updates current and max stack sizes
            int size = stackSize + SIZE[opcode];
            if (size > maxStackSize) {
                maxStackSize = size;
            }
            stackSize = size;
            // if opcode == ATHROW or xRETURN, ends current block (no successor)
            if ((opcode >= Opcodes.IRETURN && opcode <= Opcodes.RETURN)
                    || opcode == Opcodes.ATHROW)
            {
                if (currentBlock != null) {
                    currentBlock.maxStackSize = maxStackSize;
                    currentBlock = null;
                }
            }
        }
        // adds the instruction to the bytecode of the method
        code.putByte(opcode);
    }

    public void visitIntInsn(final int opcode, final int operand) {
        if (computeMaxs && opcode != Opcodes.NEWARRAY) {
            // updates current and max stack sizes only if opcode == NEWARRAY
            // (stack size variation = 0 for BIPUSH or SIPUSH)
            int size = stackSize + 1;
            if (size > maxStackSize) {
                maxStackSize = size;
            }
            stackSize = size;
        }
        // adds the instruction to the bytecode of the method
        if (opcode == Opcodes.SIPUSH) {
            code.put12(opcode, operand);
        } else { // BIPUSH or NEWARRAY
            code.put11(opcode, operand);
        }
    }

    public void visitVarInsn(final int opcode, final int var) {
        if (computeMaxs) {
            // updates current and max stack sizes
            if (opcode == Opcodes.RET) {
                // no stack change, but end of current block (no successor)
                if (currentBlock != null) {
                    currentBlock.maxStackSize = maxStackSize;
                    currentBlock = null;
                }
            } else { // xLOAD or xSTORE
                int size = stackSize + SIZE[opcode];
                if (size > maxStackSize) {
                    maxStackSize = size;
                }
                stackSize = size;
            }
            // updates max locals
            int n;
            if (opcode == Opcodes.LLOAD || opcode == Opcodes.DLOAD
                    || opcode == Opcodes.LSTORE || opcode == Opcodes.DSTORE)
            {
                n = var + 2;
            } else {
                n = var + 1;
            }
            if (n > maxLocals) {
                maxLocals = n;
            }
        }
        // adds the instruction to the bytecode of the method
        if (var < 4 && opcode != Opcodes.RET) {
            int opt;
            if (opcode < Opcodes.ISTORE) {
                /* ILOAD_0 */
                opt = 26 + ((opcode - Opcodes.ILOAD) << 2) + var;
            } else {
                /* ISTORE_0 */
                opt = 59 + ((opcode - Opcodes.ISTORE) << 2) + var;
            }
            code.putByte(opt);
        } else if (var >= 256) {
            code.putByte(196 /* WIDE */).put12(opcode, var);
        } else {
            code.put11(opcode, var);
        }
    }

    public void visitTypeInsn(final int opcode, final String desc) {
        if (computeMaxs && opcode == Opcodes.NEW) {
            // updates current and max stack sizes only if opcode == NEW
            // (stack size variation = 0 for ANEWARRAY, CHECKCAST, INSTANCEOF)
            int size = stackSize + 1;
            if (size > maxStackSize) {
                maxStackSize = size;
            }
            stackSize = size;
        }
        // adds the instruction to the bytecode of the method
        code.put12(opcode, cw.newClass(desc));
    }

    public void visitFieldInsn(
        final int opcode,
        final String owner,
        final String name,
        final String desc)
    {
        if (computeMaxs) {
            int size;
            // computes the stack size variation
            char c = desc.charAt(0);
            switch (opcode) {
                case Opcodes.GETSTATIC:
                    size = stackSize + (c == 'D' || c == 'J' ? 2 : 1);
                    break;
                case Opcodes.PUTSTATIC:
                    size = stackSize + (c == 'D' || c == 'J' ? -2 : -1);
                    break;
                case Opcodes.GETFIELD:
                    size = stackSize + (c == 'D' || c == 'J' ? 1 : 0);
                    break;
                // case Constants.PUTFIELD:
                default:
                    size = stackSize + (c == 'D' || c == 'J' ? -3 : -2);
                    break;
            }
            // updates current and max stack sizes
            if (size > maxStackSize) {
                maxStackSize = size;
            }
            stackSize = size;
        }
        // adds the instruction to the bytecode of the method
        code.put12(opcode, cw.newField(owner, name, desc));
    }

    public void visitMethodInsn(
        final int opcode,
        final String owner,
        final String name,
        final String desc)
    {
        boolean itf = opcode == Opcodes.INVOKEINTERFACE;
        Item i = cw.newMethodItem(owner, name, desc, itf);
        int argSize = i.intVal;
        if (computeMaxs) {
            /*
             * computes the stack size variation. In order not to recompute
             * several times this variation for the same Item, we use the intVal
             * field of this item to store this variation, once it has been
             * computed. More precisely this intVal field stores the sizes of
             * the arguments and of the return value corresponding to desc.
             */
            if (argSize == 0) {
                // the above sizes have not been computed yet, so we compute
                // them...
                argSize = getArgumentsAndReturnSizes(desc);
                // ... and we save them in order not to recompute them in the
                // future
                i.intVal = argSize;
            }
            int size;
            if (opcode == Opcodes.INVOKESTATIC) {
                size = stackSize - (argSize >> 2) + (argSize & 0x03) + 1;
            } else {
                size = stackSize - (argSize >> 2) + (argSize & 0x03);
            }
            // updates current and max stack sizes
            if (size > maxStackSize) {
                maxStackSize = size;
            }
            stackSize = size;
        }
        // adds the instruction to the bytecode of the method
        if (itf) {
            if (!computeMaxs) {
                if (argSize == 0) {
                    argSize = getArgumentsAndReturnSizes(desc);
                    i.intVal = argSize;
                }
            }
            code.put12(Opcodes.INVOKEINTERFACE, i.index).put11(argSize >> 2, 0);
        } else {
            code.put12(opcode, i.index);
        }
    }

    public void visitJumpInsn(final int opcode, final Label label) {
        if (computeMaxs) {
            if (opcode == Opcodes.GOTO) {
                // no stack change, but end of current block (with one new
                // successor)
                if (currentBlock != null) {
                    currentBlock.maxStackSize = maxStackSize;
                    addSuccessor(stackSize, label);
                    currentBlock = null;
                }
            } else if (opcode == Opcodes.JSR) {
                if (currentBlock != null) {
                    addSuccessor(stackSize + 1, label);
                }
            } else {
                // updates current stack size (max stack size unchanged because
                // stack size variation always negative in this case)
                stackSize += SIZE[opcode];
                if (currentBlock != null) {
                    addSuccessor(stackSize, label);
                }
            }
        }
        // adds the instruction to the bytecode of the method
        if (label.resolved && label.position - code.length < Short.MIN_VALUE) {
            /*
             * case of a backward jump with an offset < -32768. In this case we
             * automatically replace GOTO with GOTO_W, JSR with JSR_W and IFxxx
             * <l> with IFNOTxxx <l'> GOTO_W <l>, where IFNOTxxx is the
             * "opposite" opcode of IFxxx (i.e., IFNE for IFEQ) and where <l'>
             * designates the instruction just after the GOTO_W.
             */
            if (opcode == Opcodes.GOTO) {
                code.putByte(200); // GOTO_W
            } else if (opcode == Opcodes.JSR) {
                code.putByte(201); // JSR_W
            } else {
                code.putByte(opcode <= 166
                        ? ((opcode + 1) ^ 1) - 1
                        : opcode ^ 1);
                code.putShort(8); // jump offset
                code.putByte(200); // GOTO_W
            }
            label.put(this, code, code.length - 1, true);
        } else {
            /*
             * case of a backward jump with an offset >= -32768, or of a forward
             * jump with, of course, an unknown offset. In these cases we store
             * the offset in 2 bytes (which will be increased in
             * resizeInstructions, if needed).
             */
            code.putByte(opcode);
            label.put(this, code, code.length - 1, false);
        }
    }

    public void visitLabel(final Label label) {
        if (computeMaxs) {
            if (currentBlock != null) {
                // ends current block (with one new successor)
                currentBlock.maxStackSize = maxStackSize;
                addSuccessor(stackSize, label);
            }
            // begins a new current block,
            // resets the relative current and max stack sizes
            currentBlock = label;
            stackSize = 0;
            maxStackSize = 0;
        }
        // resolves previous forward references to label, if any
        resize |= label.resolve(this, code.length, code.data);
    }

    public void visitLdcInsn(final Object cst) {
        Item i = cw.newConstItem(cst);
        if (computeMaxs) {
            int size;
            // computes the stack size variation
            if (i.type == ClassWriter.LONG || i.type == ClassWriter.DOUBLE) {
                size = stackSize + 2;
            } else {
                size = stackSize + 1;
            }
            // updates current and max stack sizes
            if (size > maxStackSize) {
                maxStackSize = size;
            }
            stackSize = size;
        }
        // adds the instruction to the bytecode of the method
        int index = i.index;
        if (i.type == ClassWriter.LONG || i.type == ClassWriter.DOUBLE) {
            code.put12(20 /* LDC2_W */, index);
        } else if (index >= 256) {
            code.put12(19 /* LDC_W */, index);
        } else {
            code.put11(Opcodes.LDC, index);
        }
    }

    public void visitIincInsn(final int var, final int increment) {
        if (computeMaxs) {
            // updates max locals only (no stack change)
            int n = var + 1;
            if (n > maxLocals) {
                maxLocals = n;
            }
        }
        // adds the instruction to the bytecode of the method
        if ((var > 255) || (increment > 127) || (increment < -128)) {
            code.putByte(196 /* WIDE */)
                    .put12(Opcodes.IINC, var)
                    .putShort(increment);
        } else {
            code.putByte(Opcodes.IINC).put11(var, increment);
        }
    }

    public void visitTableSwitchInsn(
        final int min,
        final int max,
        final Label dflt,
        final Label labels[])
    {
        if (computeMaxs) {
            // updates current stack size (max stack size unchanged)
            --stackSize;
            // ends current block (with many new successors)
            if (currentBlock != null) {
                currentBlock.maxStackSize = maxStackSize;
                addSuccessor(stackSize, dflt);
                for (int i = 0; i < labels.length; ++i) {
                    addSuccessor(stackSize, labels[i]);
                }
                currentBlock = null;
            }
        }
        // adds the instruction to the bytecode of the method
        int source = code.length;
        code.putByte(Opcodes.TABLESWITCH);
        while (code.length % 4 != 0) {
            code.putByte(0);
        }
        dflt.put(this, code, source, true);
        code.putInt(min).putInt(max);
        for (int i = 0; i < labels.length; ++i) {
            labels[i].put(this, code, source, true);
        }
    }

    public void visitLookupSwitchInsn(
        final Label dflt,
        final int keys[],
        final Label labels[])
    {
        if (computeMaxs) {
            // updates current stack size (max stack size unchanged)
            --stackSize;
            // ends current block (with many new successors)
            if (currentBlock != null) {
                currentBlock.maxStackSize = maxStackSize;
                addSuccessor(stackSize, dflt);
                for (int i = 0; i < labels.length; ++i) {
                    addSuccessor(stackSize, labels[i]);
                }
                currentBlock = null;
            }
        }
        // adds the instruction to the bytecode of the method
        int source = code.length;
        code.putByte(Opcodes.LOOKUPSWITCH);
        while (code.length % 4 != 0) {
            code.putByte(0);
        }
        dflt.put(this, code, source, true);
        code.putInt(labels.length);
        for (int i = 0; i < labels.length; ++i) {
            code.putInt(keys[i]);
            labels[i].put(this, code, source, true);
        }
    }

    public void visitMultiANewArrayInsn(final String desc, final int dims) {
        if (computeMaxs) {
            // updates current stack size (max stack size unchanged because
            // stack size variation always negative or null)
            stackSize += 1 - dims;
        }
        // adds the instruction to the bytecode of the method
        code.put12(Opcodes.MULTIANEWARRAY, cw.newClass(desc)).putByte(dims);
    }

    public void visitTryCatchBlock(
        final Label start,
        final Label end,
        final Label handler,
        final String type)
    {
        if (computeMaxs) {
            // pushes handler block onto the stack of blocks to be visited
            if (!handler.pushed) {
                handler.beginStackSize = 1;
                handler.pushed = true;
                handler.next = blockStack;
                blockStack = handler;
            }
        }
        ++catchCount;
        Handler h = new Handler();
        h.start = start;
        h.end = end;
        h.handler = handler;
        h.desc = type;
        h.type = type != null ? cw.newClass(type) : 0;
        if (lastHandler == null) {
            catchTable = h;
        } else {
            lastHandler.next = h;
        }
        lastHandler = h;
    }

    public void visitLocalVariable(
        final String name,
        final String desc,
        final String signature,
        final Label start,
        final Label end,
        final int index)
    {
        if (signature != null) {
            if (localVarType == null) {
                localVarType = new ByteVector();
            }
            ++localVarTypeCount;
            localVarType.putShort(start.position)
                    .putShort(end.position - start.position)
                    .putShort(cw.newUTF8(name))
                    .putShort(cw.newUTF8(signature))
                    .putShort(index);
        }
        if (localVar == null) {
            localVar = new ByteVector();
        }
        ++localVarCount;
        localVar.putShort(start.position)
                .putShort(end.position - start.position)
                .putShort(cw.newUTF8(name))
                .putShort(cw.newUTF8(desc))
                .putShort(index);
    }

    public void visitLineNumber(final int line, final Label start) {
        if (lineNumber == null) {
            lineNumber = new ByteVector();
        }
        ++lineNumberCount;
        lineNumber.putShort(start.position);
        lineNumber.putShort(line);
    }

    public void visitMaxs(final int maxStack, final int maxLocals) {
        if (computeMaxs) {
            // true (non relative) max stack size
            int max = 0;
            /*
             * control flow analysis algorithm: while the block stack is not
             * empty, pop a block from this stack, update the max stack size,
             * compute the true (non relative) begin stack size of the
             * successors of this block, and push these successors onto the
             * stack (unless they have already been pushed onto the stack).
             * Note: by hypothesis, the {@link Label#beginStackSize} of the
             * blocks in the block stack are the true (non relative) beginning
             * stack sizes of these blocks.
             */
            Label stack = blockStack;
            while (stack != null) {
                // pops a block from the stack
                Label l = stack;
                stack = stack.next;
                // computes the true (non relative) max stack size of this block
                int start = l.beginStackSize;
                int blockMax = start + l.maxStackSize;
                // updates the global max stack size
                if (blockMax > max) {
                    max = blockMax;
                }
                // analyses the successors of the block
                Edge b = l.successors;
                while (b != null) {
                    l = b.successor;
                    // if this successor has not already been pushed onto the
                    // stack...
                    if (!l.pushed) {
                        // computes the true beginning stack size of this
                        // successor block
                        l.beginStackSize = start + b.stackSize;
                        // pushes this successor onto the stack
                        l.pushed = true;
                        l.next = stack;
                        stack = l;
                    }
                    b = b.next;
                }
            }
            this.maxStack = max;
        } else {
            this.maxStack = maxStack;
            this.maxLocals = maxLocals;
        }
    }

    public void visitEnd() {
    }

    // ------------------------------------------------------------------------
    // Utility methods: control flow analysis algorithm
    // ------------------------------------------------------------------------

    /**
     * Computes the size of the arguments and of the return value of a method.
     *
     * @param desc the descriptor of a method.
     * @return the size of the arguments of the method (plus one for the
     *         implicit this argument), argSize, and the size of its return
     *         value, retSize, packed into a single int i =
     *         <tt>(argSize << 2) | retSize</tt> (argSize is therefore equal
     *         to <tt>i >> 2</tt>, and retSize to <tt>i & 0x03</tt>).
     */
    private static int getArgumentsAndReturnSizes(final String desc) {
        int n = 1;
        int c = 1;
        while (true) {
            char car = desc.charAt(c++);
            if (car == ')') {
                car = desc.charAt(c);
                return n << 2
                        | (car == 'V' ? 0 : (car == 'D' || car == 'J' ? 2 : 1));
            } else if (car == 'L') {
                while (desc.charAt(c++) != ';') {
                }
                n += 1;
            } else if (car == '[') {
                while ((car = desc.charAt(c)) == '[') {
                    ++c;
                }
                if (car == 'D' || car == 'J') {
                    n -= 1;
                }
            } else if (car == 'D' || car == 'J') {
                n += 2;
            } else {
                n += 1;
            }
        }
    }

    /**
     * Adds a successor to the {@link #currentBlock currentBlock} block.
     *
     * @param stackSize the current (relative) stack size in the current block.
     * @param successor the successor block to be added to the current block.
     */
    private void addSuccessor(final int stackSize, final Label successor) {
        Edge b = new Edge();
        // initializes the previous Edge object...
        b.stackSize = stackSize;
        b.successor = successor;
        // ...and adds it to the successor list of the currentBlock block
        b.next = currentBlock.successors;
        currentBlock.successors = b;
    }

    // ------------------------------------------------------------------------
    // Utility methods: dump bytecode array
    // ------------------------------------------------------------------------

    /**
     * Returns the size of the bytecode of this method.
     *
     * @return the size of the bytecode of this method.
     */
    final int getSize() {
        if (classReaderOffset != 0) {
            return 6 + classReaderLength;
        }
        if (resize) {
            // replaces the temporary jump opcodes introduced by Label.resolve.
            resizeInstructions(new int[0], new int[0], 0);
        }
        int size = 8;
        if (code.length > 0) {
            cw.newUTF8("Code");
            size += 18 + code.length + 8 * catchCount;
            if (localVar != null) {
                cw.newUTF8("LocalVariableTable");
                size += 8 + localVar.length;
            }
            if (localVarType != null) {
                cw.newUTF8("LocalVariableTypeTable");
                size += 8 + localVarType.length;
            }
            if (lineNumber != null) {
                cw.newUTF8("LineNumberTable");
                size += 8 + lineNumber.length;
            }
            if (cattrs != null) {
                size += cattrs.getSize(cw,
                        code.data,
                        code.length,
                        maxStack,
                        maxLocals);
            }
        }
        if (exceptionCount > 0) {
            cw.newUTF8("Exceptions");
            size += 8 + 2 * exceptionCount;
        }
        if ((access & Opcodes.ACC_SYNTHETIC) != 0
                && (cw.version & 0xffff) < Opcodes.V1_5)
        {
            cw.newUTF8("Synthetic");
            size += 6;
        }
        if ((access & Opcodes.ACC_DEPRECATED) != 0) {
            cw.newUTF8("Deprecated");
            size += 6;
        }
        if (cw.version == Opcodes.V1_4) {
            if ((access & Opcodes.ACC_VARARGS) != 0) {
                cw.newUTF8("Varargs");
                size += 6;
            }
            if ((access & Opcodes.ACC_BRIDGE) != 0) {
                cw.newUTF8("Bridge");
                size += 6;
            }
        }
        if (signature != null) {
            cw.newUTF8("Signature");
            cw.newUTF8(signature);
            size += 8;
        }
        if (annd != null) {
            cw.newUTF8("AnnotationDefault");
            size += 6 + annd.length;
        }
        if (anns != null) {
            cw.newUTF8("RuntimeVisibleAnnotations");
            size += 8 + anns.getSize();
        }
        if (ianns != null) {
            cw.newUTF8("RuntimeInvisibleAnnotations");
            size += 8 + ianns.getSize();
        }
        if (panns != null) {
            cw.newUTF8("RuntimeVisibleParameterAnnotations");
            size += 7 + 2 * panns.length;
            for (int i = panns.length - 1; i >= 0; --i) {
                size += panns[i] == null ? 0 : panns[i].getSize();
            }
        }
        if (ipanns != null) {
            cw.newUTF8("RuntimeInvisibleParameterAnnotations");
            size += 7 + 2 * ipanns.length;
            for (int i = ipanns.length - 1; i >= 0; --i) {
                size += ipanns[i] == null ? 0 : ipanns[i].getSize();
            }
        }
        if (attrs != null) {
            size += attrs.getSize(cw, null, 0, -1, -1);
        }
        return size;
    }

    /**
     * Puts the bytecode of this method in the given byte vector.
     *
     * @param out the byte vector into which the bytecode of this method must be
     *        copied.
     */
    final void put(final ByteVector out) {
        out.putShort(access).putShort(name).putShort(desc);
        if (classReaderOffset != 0) {
            out.putByteArray(cw.cr.b, classReaderOffset, classReaderLength);
            return;
        }
        int attributeCount = 0;
        if (code.length > 0) {
            ++attributeCount;
        }
        if (exceptionCount > 0) {
            ++attributeCount;
        }
        if ((access & Opcodes.ACC_SYNTHETIC) != 0
                && (cw.version & 0xffff) < Opcodes.V1_5)
        {
            ++attributeCount;
        }
        if ((access & Opcodes.ACC_DEPRECATED) != 0) {
            ++attributeCount;
        }
        if (cw.version == Opcodes.V1_4) {
            if ((access & Opcodes.ACC_VARARGS) != 0) {
                ++attributeCount;
            }
            if ((access & Opcodes.ACC_BRIDGE) != 0) {
                ++attributeCount;
            }
        }
        if (signature != null) {
            ++attributeCount;
        }
        if (annd != null) {
            ++attributeCount;
        }
        if (anns != null) {
            ++attributeCount;
        }
        if (ianns != null) {
            ++attributeCount;
        }
        if (panns != null) {
            ++attributeCount;
        }
        if (ipanns != null) {
            ++attributeCount;
        }
        if (attrs != null) {
            attributeCount += attrs.getCount();
        }
        out.putShort(attributeCount);
        if (code.length > 0) {
            int size = 12 + code.length + 8 * catchCount;
            if (localVar != null) {
                size += 8 + localVar.length;
            }
            if (localVarType != null) {
                size += 8 + localVarType.length;
            }
            if (lineNumber != null) {
                size += 8 + lineNumber.length;
            }
            if (cattrs != null) {
                size += cattrs.getSize(cw,
                        code.data,
                        code.length,
                        maxStack,
                        maxLocals);
            }
            out.putShort(cw.newUTF8("Code")).putInt(size);
            out.putShort(maxStack).putShort(maxLocals);
            out.putInt(code.length).putByteArray(code.data, 0, code.length);
            out.putShort(catchCount);
            if (catchCount > 0) {
                Handler h = catchTable;
                while (h != null) {
                    out.putShort(h.start.position)
                            .putShort(h.end.position)
                            .putShort(h.handler.position)
                            .putShort(h.type);
                    h = h.next;
                }
            }
            attributeCount = 0;
            if (localVar != null) {
                ++attributeCount;
            }
            if (localVarType != null) {
                ++attributeCount;
            }
            if (lineNumber != null) {
                ++attributeCount;
            }
            if (cattrs != null) {
                attributeCount += cattrs.getCount();
            }
            out.putShort(attributeCount);
            if (localVar != null) {
                out.putShort(cw.newUTF8("LocalVariableTable"));
                out.putInt(localVar.length + 2).putShort(localVarCount);
                out.putByteArray(localVar.data, 0, localVar.length);
            }
            if (localVarType != null) {
                out.putShort(cw.newUTF8("LocalVariableTypeTable"));
                out.putInt(localVarType.length + 2).putShort(localVarTypeCount);
                out.putByteArray(localVarType.data, 0, localVarType.length);
            }
            if (lineNumber != null) {
                out.putShort(cw.newUTF8("LineNumberTable"));
                out.putInt(lineNumber.length + 2).putShort(lineNumberCount);
                out.putByteArray(lineNumber.data, 0, lineNumber.length);
            }
            if (cattrs != null) {
                cattrs.put(cw, code.data, code.length, maxLocals, maxStack, out);
            }
        }
        if (exceptionCount > 0) {
            out.putShort(cw.newUTF8("Exceptions"))
                    .putInt(2 * exceptionCount + 2);
            out.putShort(exceptionCount);
            for (int i = 0; i < exceptionCount; ++i) {
                out.putShort(exceptions[i]);
            }
        }
        if ((access & Opcodes.ACC_SYNTHETIC) != 0
                && (cw.version & 0xffff) < Opcodes.V1_5)
        {
            out.putShort(cw.newUTF8("Synthetic")).putInt(0);
        }
        if ((access & Opcodes.ACC_DEPRECATED) != 0) {
            out.putShort(cw.newUTF8("Deprecated")).putInt(0);
        }
        if (cw.version == Opcodes.V1_4) {
            if ((access & Opcodes.ACC_VARARGS) != 0) {
                out.putShort(cw.newUTF8("Varargs")).putInt(0);
            }
            if ((access & Opcodes.ACC_BRIDGE) != 0) {
                out.putShort(cw.newUTF8("Bridge")).putInt(0);
            }
        }
        if (signature != null) {
            out.putShort(cw.newUTF8("Signature"))
                    .putInt(2)
                    .putShort(cw.newUTF8(signature));
        }
        if (annd != null) {
            out.putShort(cw.newUTF8("AnnotationDefault"));
            out.putInt(annd.length);
            out.putByteArray(annd.data, 0, annd.length);
        }
        if (anns != null) {
            out.putShort(cw.newUTF8("RuntimeVisibleAnnotations"));
            anns.put(out);
        }
        if (ianns != null) {
            out.putShort(cw.newUTF8("RuntimeInvisibleAnnotations"));
            ianns.put(out);
        }
        if (panns != null) {
            out.putShort(cw.newUTF8("RuntimeVisibleParameterAnnotations"));
            AnnotationWriter.put(panns, out);
        }
        if (ipanns != null) {
            out.putShort(cw.newUTF8("RuntimeInvisibleParameterAnnotations"));
            AnnotationWriter.put(ipanns, out);
        }
        if (attrs != null) {
            attrs.put(cw, null, 0, -1, -1, out);
        }
    }

    // ------------------------------------------------------------------------
    // Utility methods: instruction resizing (used to handle GOTO_W and JSR_W)
    // ------------------------------------------------------------------------

    /**
     * Resizes the designated instructions, while keeping jump offsets and
     * instruction addresses consistent. This may require to resize other
     * existing instructions, or even to introduce new instructions: for
     * example, increasing the size of an instruction by 2 at the middle of a
     * method can increases the offset of an IFEQ instruction from 32766 to
     * 32768, in which case IFEQ 32766 must be replaced with IFNEQ 8 GOTO_W
     * 32765. This, in turn, may require to increase the size of another jump
     * instruction, and so on... All these operations are handled automatically
     * by this method. <p> <i>This method must be called after all the method
     * that is being built has been visited</i>. In particular, the
     * {@link Label Label} objects used to construct the method are no longer
     * valid after this method has been called.
     *
     * @param indexes current positions of the instructions to be resized. Each
     *        instruction must be designated by the index of its <i>last</i>
     *        byte, plus one (or, in other words, by the index of the <i>first</i>
     *        byte of the <i>next</i> instruction).
     * @param sizes the number of bytes to be <i>added</i> to the above
     *        instructions. More precisely, for each i &lt; <tt>len</tt>,
     *        <tt>sizes</tt>[i] bytes will be added at the end of the
     *        instruction designated by <tt>indexes</tt>[i] or, if
     *        <tt>sizes</tt>[i] is negative, the <i>last</i> |<tt>sizes[i]</tt>|
     *        bytes of the instruction will be removed (the instruction size
     *        <i>must not</i> become negative or null). The gaps introduced by
     *        this method must be filled in "manually" in {@link #code code}
     *        method.
     * @param len the number of instruction to be resized. Must be smaller than
     *        or equal to <tt>indexes</tt>.length and <tt>sizes</tt>.length.
     * @return the <tt>indexes</tt> array, which now contains the new
     *         positions of the resized instructions (designated as above).
     */
    private int[] resizeInstructions(
        final int[] indexes,
        final int[] sizes,
        final int len)
    {
        byte[] b = code.data; // bytecode of the method
        int u, v, label; // indexes in b
        int i, j; // loop indexes

        /*
         * 1st step: As explained above, resizing an instruction may require to
         * resize another one, which may require to resize yet another one, and
         * so on. The first step of the algorithm consists in finding all the
         * instructions that need to be resized, without modifying the code.
         * This is done by the following "fix point" algorithm:
         *
         * Parse the code to find the jump instructions whose offset will need
         * more than 2 bytes to be stored (the future offset is computed from
         * the current offset and from the number of bytes that will be inserted
         * or removed between the source and target instructions). For each such
         * instruction, adds an entry in (a copy of) the indexes and sizes
         * arrays (if this has not already been done in a previous iteration!).
         *
         * If at least one entry has been added during the previous step, go
         * back to the beginning, otherwise stop.
         *
         * In fact the real algorithm is complicated by the fact that the size
         * of TABLESWITCH and LOOKUPSWITCH instructions depends on their
         * position in the bytecode (because of padding). In order to ensure the
         * convergence of the algorithm, the number of bytes to be added or
         * removed from these instructions is over estimated during the previous
         * loop, and computed exactly only after the loop is finished (this
         * requires another pass to parse the bytecode of the method).
         */
        int[] allIndexes = new int[len]; // copy of indexes
        int[] allSizes = new int[len]; // copy of sizes
        boolean[] resize; // instructions to be resized
        int newOffset; // future offset of a jump instruction

        System.arraycopy(indexes, 0, allIndexes, 0, len);
        System.arraycopy(sizes, 0, allSizes, 0, len);
        resize = new boolean[code.length];

        // 3 = loop again, 2 = loop ended, 1 = last pass, 0 = done
        int state = 3;
        do {
            if (state == 3) {
                state = 2;
            }
            u = 0;
            while (u < b.length) {
                int opcode = b[u] & 0xFF; // opcode of current instruction
                int insert = 0; // bytes to be added after this instruction

                switch (ClassWriter.TYPE[opcode]) {
                    case ClassWriter.NOARG_INSN:
                    case ClassWriter.IMPLVAR_INSN:
                        u += 1;
                        break;
                    case ClassWriter.LABEL_INSN:
                        if (opcode > 201) {
                            // converts temporary opcodes 202 to 217, 218 and
                            // 219 to IFEQ ... JSR (inclusive), IFNULL and
                            // IFNONNULL
                            opcode = opcode < 218 ? opcode - 49 : opcode - 20;
                            label = u + readUnsignedShort(b, u + 1);
                        } else {
                            label = u + readShort(b, u + 1);
                        }
                        newOffset = getNewOffset(allIndexes, allSizes, u, label);
                        if (newOffset < Short.MIN_VALUE
                                || newOffset > Short.MAX_VALUE)
                        {
                            if (!resize[u]) {
                                if (opcode == Opcodes.GOTO
                                        || opcode == Opcodes.JSR)
                                {
                                    // two additional bytes will be required to
                                    // replace this GOTO or JSR instruction with
                                    // a GOTO_W or a JSR_W
                                    insert = 2;
                                } else {
                                    // five additional bytes will be required to
                                    // replace this IFxxx <l> instruction with
                                    // IFNOTxxx <l'> GOTO_W <l>, where IFNOTxxx
                                    // is the "opposite" opcode of IFxxx (i.e.,
                                    // IFNE for IFEQ) and where <l'> designates
                                    // the instruction just after the GOTO_W.
                                    insert = 5;
                                }
                                resize[u] = true;
                            }
                        }
                        u += 3;
                        break;
                    case ClassWriter.LABELW_INSN:
                        u += 5;
                        break;
                    case ClassWriter.TABL_INSN:
                        if (state == 1) {
                            // true number of bytes to be added (or removed)
                            // from this instruction = (future number of padding
                            // bytes - current number of padding byte) -
                            // previously over estimated variation =
                            // = ((3 - newOffset%4) - (3 - u%4)) - u%4
                            // = (-newOffset%4 + u%4) - u%4
                            // = -(newOffset & 3)
                            newOffset = getNewOffset(allIndexes, allSizes, 0, u);
                            insert = -(newOffset & 3);
                        } else if (!resize[u]) {
                            // over estimation of the number of bytes to be
                            // added to this instruction = 3 - current number
                            // of padding bytes = 3 - (3 - u%4) = u%4 = u & 3
                            insert = u & 3;
                            resize[u] = true;
                        }
                        // skips instruction
                        u = u + 4 - (u & 3);
                        u += 4 * (readInt(b, u + 8) - readInt(b, u + 4) + 1) + 12;
                        break;
                    case ClassWriter.LOOK_INSN:
                        if (state == 1) {
                            // like TABL_INSN
                            newOffset = getNewOffset(allIndexes, allSizes, 0, u);
                            insert = -(newOffset & 3);
                        } else if (!resize[u]) {
                            // like TABL_INSN
                            insert = u & 3;
                            resize[u] = true;
                        }
                        // skips instruction
                        u = u + 4 - (u & 3);
                        u += 8 * readInt(b, u + 4) + 8;
                        break;
                    case ClassWriter.WIDE_INSN:
                        opcode = b[u + 1] & 0xFF;
                        if (opcode == Opcodes.IINC) {
                            u += 6;
                        } else {
                            u += 4;
                        }
                        break;
                    case ClassWriter.VAR_INSN:
                    case ClassWriter.SBYTE_INSN:
                    case ClassWriter.LDC_INSN:
                        u += 2;
                        break;
                    case ClassWriter.SHORT_INSN:
                    case ClassWriter.LDCW_INSN:
                    case ClassWriter.FIELDORMETH_INSN:
                    case ClassWriter.TYPE_INSN:
                    case ClassWriter.IINC_INSN:
                        u += 3;
                        break;
                    case ClassWriter.ITFMETH_INSN:
                        u += 5;
                        break;
                    // case ClassWriter.MANA_INSN:
                    default:
                        u += 4;
                        break;
                }
                if (insert != 0) {
                    // adds a new (u, insert) entry in the allIndexes and
                    // allSizes arrays
                    int[] newIndexes = new int[allIndexes.length + 1];
                    int[] newSizes = new int[allSizes.length + 1];
                    System.arraycopy(allIndexes,
                            0,
                            newIndexes,
                            0,
                            allIndexes.length);
                    System.arraycopy(allSizes, 0, newSizes, 0, allSizes.length);
                    newIndexes[allIndexes.length] = u;
                    newSizes[allSizes.length] = insert;
                    allIndexes = newIndexes;
                    allSizes = newSizes;
                    if (insert > 0) {
                        state = 3;
                    }
                }
            }
            if (state < 3) {
                --state;
            }
        } while (state != 0);

        // 2nd step:
        // copies the bytecode of the method into a new bytevector, updates the
        // offsets, and inserts (or removes) bytes as requested.

        ByteVector newCode = new ByteVector(code.length);

        u = 0;
        while (u < code.length) {
            for (i = allIndexes.length - 1; i >= 0; --i) {
                if (allIndexes[i] == u) {
                    if (i < len) {
                        if (sizes[i] > 0) {
                            newCode.putByteArray(null, 0, sizes[i]);
                        } else {
                            newCode.length += sizes[i];
                        }
                        indexes[i] = newCode.length;
                    }
                }
            }
            int opcode = b[u] & 0xFF;
            switch (ClassWriter.TYPE[opcode]) {
                case ClassWriter.NOARG_INSN:
                case ClassWriter.IMPLVAR_INSN:
                    newCode.putByte(opcode);
                    u += 1;
                    break;
                case ClassWriter.LABEL_INSN:
                    if (opcode > 201) {
                        // changes temporary opcodes 202 to 217 (inclusive), 218
                        // and 219 to IFEQ ... JSR (inclusive), IFNULL and
                        // IFNONNULL
                        opcode = opcode < 218 ? opcode - 49 : opcode - 20;
                        label = u + readUnsignedShort(b, u + 1);
                    } else {
                        label = u + readShort(b, u + 1);
                    }
                    newOffset = getNewOffset(allIndexes, allSizes, u, label);
                    if (resize[u]) {
                        // replaces GOTO with GOTO_W, JSR with JSR_W and IFxxx
                        // <l> with IFNOTxxx <l'> GOTO_W <l>, where IFNOTxxx is
                        // the "opposite" opcode of IFxxx (i.e., IFNE for IFEQ)
                        // and where <l'> designates the instruction just after
                        // the GOTO_W.
                        if (opcode == Opcodes.GOTO) {
                            newCode.putByte(200); // GOTO_W
                        } else if (opcode == Opcodes.JSR) {
                            newCode.putByte(201); // JSR_W
                        } else {
                            newCode.putByte(opcode <= 166
                                    ? ((opcode + 1) ^ 1) - 1
                                    : opcode ^ 1);
                            newCode.putShort(8); // jump offset
                            newCode.putByte(200); // GOTO_W
                            // newOffset now computed from start of GOTO_W
                            newOffset -= 3;
                        }
                        newCode.putInt(newOffset);
                    } else {
                        newCode.putByte(opcode);
                        newCode.putShort(newOffset);
                    }
                    u += 3;
                    break;
                case ClassWriter.LABELW_INSN:
                    label = u + readInt(b, u + 1);
                    newOffset = getNewOffset(allIndexes, allSizes, u, label);
                    newCode.putByte(opcode);
                    newCode.putInt(newOffset);
                    u += 5;
                    break;
                case ClassWriter.TABL_INSN:
                    // skips 0 to 3 padding bytes
                    v = u;
                    u = u + 4 - (v & 3);
                    // reads and copies instruction
                    newCode.putByte(Opcodes.TABLESWITCH);
                    while (newCode.length % 4 != 0) {
                        newCode.putByte(0);
                    }
                    label = v + readInt(b, u);
                    u += 4;
                    newOffset = getNewOffset(allIndexes, allSizes, v, label);
                    newCode.putInt(newOffset);
                    j = readInt(b, u);
                    u += 4;
                    newCode.putInt(j);
                    j = readInt(b, u) - j + 1;
                    u += 4;
                    newCode.putInt(readInt(b, u - 4));
                    for (; j > 0; --j) {
                        label = v + readInt(b, u);
                        u += 4;
                        newOffset = getNewOffset(allIndexes, allSizes, v, label);
                        newCode.putInt(newOffset);
                    }
                    break;
                case ClassWriter.LOOK_INSN:
                    // skips 0 to 3 padding bytes
                    v = u;
                    u = u + 4 - (v & 3);
                    // reads and copies instruction
                    newCode.putByte(Opcodes.LOOKUPSWITCH);
                    while (newCode.length % 4 != 0) {
                        newCode.putByte(0);
                    }
                    label = v + readInt(b, u);
                    u += 4;
                    newOffset = getNewOffset(allIndexes, allSizes, v, label);
                    newCode.putInt(newOffset);
                    j = readInt(b, u);
                    u += 4;
                    newCode.putInt(j);
                    for (; j > 0; --j) {
                        newCode.putInt(readInt(b, u));
                        u += 4;
                        label = v + readInt(b, u);
                        u += 4;
                        newOffset = getNewOffset(allIndexes, allSizes, v, label);
                        newCode.putInt(newOffset);
                    }
                    break;
                case ClassWriter.WIDE_INSN:
                    opcode = b[u + 1] & 0xFF;
                    if (opcode == Opcodes.IINC) {
                        newCode.putByteArray(b, u, 6);
                        u += 6;
                    } else {
                        newCode.putByteArray(b, u, 4);
                        u += 4;
                    }
                    break;
                case ClassWriter.VAR_INSN:
                case ClassWriter.SBYTE_INSN:
                case ClassWriter.LDC_INSN:
                    newCode.putByteArray(b, u, 2);
                    u += 2;
                    break;
                case ClassWriter.SHORT_INSN:
                case ClassWriter.LDCW_INSN:
                case ClassWriter.FIELDORMETH_INSN:
                case ClassWriter.TYPE_INSN:
                case ClassWriter.IINC_INSN:
                    newCode.putByteArray(b, u, 3);
                    u += 3;
                    break;
                case ClassWriter.ITFMETH_INSN:
                    newCode.putByteArray(b, u, 5);
                    u += 5;
                    break;
                // case MANA_INSN:
                default:
                    newCode.putByteArray(b, u, 4);
                    u += 4;
                    break;
            }
        }

        // updates the exception handler block labels
        Handler h = catchTable;
        while (h != null) {
            getNewOffset(allIndexes, allSizes, h.start);
            getNewOffset(allIndexes, allSizes, h.end);
            getNewOffset(allIndexes, allSizes, h.handler);
            h = h.next;
        }
        for (i = 0; i < 2; ++i) {
            ByteVector bv = i == 0 ? localVar : localVarType;
            if (bv != null) {
                b = bv.data;
                u = 0;
                while (u < bv.length) {
                    label = readUnsignedShort(b, u);
                    newOffset = getNewOffset(allIndexes, allSizes, 0, label);
                    writeShort(b, u, newOffset);
                    label += readUnsignedShort(b, u + 2);
                    newOffset = getNewOffset(allIndexes, allSizes, 0, label)
                            - newOffset;
                    writeShort(b, u + 2, newOffset);
                    u += 10;
                }
            }
        }
        if (lineNumber != null) {
            b = lineNumber.data;
            u = 0;
            while (u < lineNumber.length) {
                writeShort(b, u, getNewOffset(allIndexes,
                        allSizes,
                        0,
                        readUnsignedShort(b, u)));
                u += 4;
            }
        }
        // updates the labels of the other attributes
        while (cattrs != null) {
            Label[] labels = cattrs.getLabels();
            if (labels != null) {
                for (i = labels.length - 1; i >= 0; --i) {
                    if (!labels[i].resized) {
                        labels[i].position = getNewOffset(allIndexes,
                                allSizes,
                                0,
                                labels[i].position);
                        labels[i].resized = true;
                    }
                }
            }
        }

        // replaces old bytecodes with new ones
        code = newCode;

        // returns the positions of the resized instructions
        return indexes;
    }

    /**
     * Reads an unsigned short value in the given byte array.
     *
     * @param b a byte array.
     * @param index the start index of the value to be read.
     * @return the read value.
     */
    static int readUnsignedShort(final byte[] b, final int index) {
        return ((b[index] & 0xFF) << 8) | (b[index + 1] & 0xFF);
    }

    /**
     * Reads a signed short value in the given byte array.
     *
     * @param b a byte array.
     * @param index the start index of the value to be read.
     * @return the read value.
     */
    static short readShort(final byte[] b, final int index) {
        return (short) (((b[index] & 0xFF) << 8) | (b[index + 1] & 0xFF));
    }

    /**
     * Reads a signed int value in the given byte array.
     *
     * @param b a byte array.
     * @param index the start index of the value to be read.
     * @return the read value.
     */
    static int readInt(final byte[] b, final int index) {
        return ((b[index] & 0xFF) << 24) | ((b[index + 1] & 0xFF) << 16)
                | ((b[index + 2] & 0xFF) << 8) | (b[index + 3] & 0xFF);
    }

    /**
     * Writes a short value in the given byte array.
     *
     * @param b a byte array.
     * @param index where the first byte of the short value must be written.
     * @param s the value to be written in the given byte array.
     */
    static void writeShort(final byte[] b, final int index, final int s) {
        b[index] = (byte) (s >>> 8);
        b[index + 1] = (byte) s;
    }

    /**
     * Computes the future value of a bytecode offset. <p> Note: it is possible
     * to have several entries for the same instruction in the <tt>indexes</tt>
     * and <tt>sizes</tt>: two entries (index=a,size=b) and (index=a,size=b')
     * are equivalent to a single entry (index=a,size=b+b').
     *
     * @param indexes current positions of the instructions to be resized. Each
     *        instruction must be designated by the index of its <i>last</i>
     *        byte, plus one (or, in other words, by the index of the <i>first</i>
     *        byte of the <i>next</i> instruction).
     * @param sizes the number of bytes to be <i>added</i> to the above
     *        instructions. More precisely, for each i < <tt>len</tt>,
     *        <tt>sizes</tt>[i] bytes will be added at the end of the
     *        instruction designated by <tt>indexes</tt>[i] or, if
     *        <tt>sizes</tt>[i] is negative, the <i>last</i> |<tt>sizes[i]</tt>|
     *        bytes of the instruction will be removed (the instruction size
     *        <i>must not</i> become negative or null).
     * @param begin index of the first byte of the source instruction.
     * @param end index of the first byte of the target instruction.
     * @return the future value of the given bytecode offset.
     */
    static int getNewOffset(
        final int[] indexes,
        final int[] sizes,
        final int begin,
        final int end)
    {
        int offset = end - begin;
        for (int i = 0; i < indexes.length; ++i) {
            if (begin < indexes[i] && indexes[i] <= end) {
                // forward jump
                offset += sizes[i];
            } else if (end < indexes[i] && indexes[i] <= begin) {
                // backward jump
                offset -= sizes[i];
            }
        }
        return offset;
    }

    /**
     * Updates the offset of the given label.
     *
     * @param indexes current positions of the instructions to be resized. Each
     *        instruction must be designated by the index of its <i>last</i>
     *        byte, plus one (or, in other words, by the index of the <i>first</i>
     *        byte of the <i>next</i> instruction).
     * @param sizes the number of bytes to be <i>added</i> to the above
     *        instructions. More precisely, for each i < <tt>len</tt>,
     *        <tt>sizes</tt>[i] bytes will be added at the end of the
     *        instruction designated by <tt>indexes</tt>[i] or, if
     *        <tt>sizes</tt>[i] is negative, the <i>last</i> |<tt>sizes[i]</tt>|
     *        bytes of the instruction will be removed (the instruction size
     *        <i>must not</i> become negative or null).
     * @param label the label whose offset must be updated.
     */
    static void getNewOffset(
        final int[] indexes,
        final int[] sizes,
        final Label label)
    {
        if (!label.resized) {
            label.position = getNewOffset(indexes, sizes, 0, label.position);
            label.resized = true;
        }
    }
}
