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
 * A label represents a position in the bytecode of a method. Labels are used
 * for jump, goto, and switch instructions, and for try catch blocks.
 *
 * @author Eric Bruneton
 */
public class Label {

    /**
     * The line number corresponding to this label, if known.
     */
    int line;

    /**
     * Indicates if the position of this label is known.
     */
    boolean resolved;

    /**
     * The position of this label in the code, if known.
     */
    int position;

    /**
     * If the label position has been updated, after instruction resizing.
     */
    boolean resized;

    /**
     * Number of forward references to this label, times two.
     */
    private int referenceCount;

    /**
     * Informations about forward references. Each forward reference is
     * described by two consecutive integers in this array: the first one is the
     * position of the first byte of the bytecode instruction that contains the
     * forward reference, while the second is the position of the first byte of
     * the forward reference itself. In fact the sign of the first integer
     * indicates if this reference uses 2 or 4 bytes, and its absolute value
     * gives the position of the bytecode instruction.
     */
    private int[] srcAndRefPositions;

    /*
     * Fields for the control flow graph analysis algorithm (used to compute the
     * maximum stack size). A control flow graph contains one node per "basic
     * block", and one edge per "jump" from one basic block to another. Each
     * node (i.e., each basic block) is represented by the Label object that
     * corresponds to the first instruction of this basic block. Each node also
     * stores the list of it successors in the graph, as a linked list of Edge
     * objects.
     */

    /**
     * The stack size at the beginning of this basic block. This size is
     * initially unknown. It is computed by the control flow analysis algorithm
     * (see {@link MethodWriter#visitMaxs visitMaxs}).
     */
    int beginStackSize;

    /**
     * The (relative) maximum stack size corresponding to this basic block. This
     * size is relative to the stack size at the beginning of the basic block,
     * i.e., the true maximum stack size is equal to {@link #beginStackSize
     * beginStackSize} + {@link #maxStackSize maxStackSize}.
     */
    int maxStackSize;

    /**
     * The successors of this node in the control flow graph. These successors
     * are stored in a linked list of {@link Edge Edge} objects, linked to each
     * other by their {@link Edge#next} field.
     */
    Edge successors;

    /**
     * The next basic block in the basic block stack. See
     * {@link MethodWriter#visitMaxs visitMaxs}.
     */
    Label next;

    /**
     * <tt>true</tt> if this basic block has been pushed in the basic block
     * stack. See {@link MethodWriter#visitMaxs visitMaxs}.
     */
    boolean pushed;

    // ------------------------------------------------------------------------
    // Constructor
    // ------------------------------------------------------------------------

    /**
     * Constructs a new label.
     */
    public Label() {
    }

    // ------------------------------------------------------------------------
    // Methods to compute offsets and to manage forward references
    // ------------------------------------------------------------------------

    /**
     * Returns the offset corresponding to this label. This offset is computed
     * from the start of the method's bytecode. <i>This method is intended for
     * {@link Attribute} sub classes, and is normally not needed by class
     * generators or adapters.</i>
     *
     * @return the offset corresponding to this label.
     * @throws IllegalStateException if this label is not resolved yet.
     */
    public int getOffset() {
        if (!resolved) {
            throw new IllegalStateException("Label offset position has not been resolved yet");
        }
        return position;
    }

    /**
     * Puts a reference to this label in the bytecode of a method. If the
     * position of the label is known, the offset is computed and written
     * directly. Otherwise, a null offset is written and a new forward reference
     * is declared for this label.
     *
     * @param owner the code writer that calls this method.
     * @param out the bytecode of the method.
     * @param source the position of first byte of the bytecode instruction that
     *        contains this label.
     * @param wideOffset <tt>true</tt> if the reference must be stored in 4
     *        bytes, or <tt>false</tt> if it must be stored with 2 bytes.
     * @throws IllegalArgumentException if this label has not been created by
     *         the given code writer.
     */
    void put(
        final MethodWriter owner,
        final ByteVector out,
        final int source,
        final boolean wideOffset)
    {
        if (resolved) {
            if (wideOffset) {
                out.putInt(position - source);
            } else {
                out.putShort(position - source);
            }
        } else {
            if (wideOffset) {
                addReference(-1 - source, out.length);
                out.putInt(-1);
            } else {
                addReference(source, out.length);
                out.putShort(-1);
            }
        }
    }

    /**
     * Adds a forward reference to this label. This method must be called only
     * for a true forward reference, i.e. only if this label is not resolved
     * yet. For backward references, the offset of the reference can be, and
     * must be, computed and stored directly.
     *
     * @param sourcePosition the position of the referencing instruction. This
     *        position will be used to compute the offset of this forward
     *        reference.
     * @param referencePosition the position where the offset for this forward
     *        reference must be stored.
     */
    private void addReference(
        final int sourcePosition,
        final int referencePosition)
    {
        if (srcAndRefPositions == null) {
            srcAndRefPositions = new int[6];
        }
        if (referenceCount >= srcAndRefPositions.length) {
            int[] a = new int[srcAndRefPositions.length + 6];
            System.arraycopy(srcAndRefPositions,
                    0,
                    a,
                    0,
                    srcAndRefPositions.length);
            srcAndRefPositions = a;
        }
        srcAndRefPositions[referenceCount++] = sourcePosition;
        srcAndRefPositions[referenceCount++] = referencePosition;
    }

    /**
     * Resolves all forward references to this label. This method must be called
     * when this label is added to the bytecode of the method, i.e. when its
     * position becomes known. This method fills in the blanks that where left
     * in the bytecode by each forward reference previously added to this label.
     *
     * @param owner the code writer that calls this method.
     * @param position the position of this label in the bytecode.
     * @param data the bytecode of the method.
     * @return <tt>true</tt> if a blank that was left for this label was to
     *         small to store the offset. In such a case the corresponding jump
     *         instruction is replaced with a pseudo instruction (using unused
     *         opcodes) using an unsigned two bytes offset. These pseudo
     *         instructions will need to be replaced with true instructions with
     *         wider offsets (4 bytes instead of 2). This is done in
     *         {@link MethodWriter#resizeInstructions}.
     * @throws IllegalArgumentException if this label has already been resolved,
     *         or if it has not been created by the given code writer.
     */
    boolean resolve(
        final MethodWriter owner,
        final int position,
        final byte[] data)
    {
        boolean needUpdate = false;
        this.resolved = true;
        this.position = position;
        int i = 0;
        while (i < referenceCount) {
            int source = srcAndRefPositions[i++];
            int reference = srcAndRefPositions[i++];
            int offset;
            if (source >= 0) {
                offset = position - source;
                if (offset < Short.MIN_VALUE || offset > Short.MAX_VALUE) {
                    /*
                     * changes the opcode of the jump instruction, in order to
                     * be able to find it later (see resizeInstructions in
                     * MethodWriter). These temporary opcodes are similar to
                     * jump instruction opcodes, except that the 2 bytes offset
                     * is unsigned (and can therefore represent values from 0 to
                     * 65535, which is sufficient since the size of a method is
                     * limited to 65535 bytes).
                     */
                    int opcode = data[reference - 1] & 0xFF;
                    if (opcode <= Opcodes.JSR) {
                        // changes IFEQ ... JSR to opcodes 202 to 217
                        data[reference - 1] = (byte) (opcode + 49);
                    } else {
                        // changes IFNULL and IFNONNULL to opcodes 218 and 219
                        data[reference - 1] = (byte) (opcode + 20);
                    }
                    needUpdate = true;
                }
                data[reference++] = (byte) (offset >>> 8);
                data[reference] = (byte) offset;
            } else {
                offset = position + source + 1;
                data[reference++] = (byte) (offset >>> 24);
                data[reference++] = (byte) (offset >>> 16);
                data[reference++] = (byte) (offset >>> 8);
                data[reference] = (byte) offset;
            }
        }
        return needUpdate;
    }

    // ------------------------------------------------------------------------
    // Overriden Object methods
    // ------------------------------------------------------------------------

    /**
     * Returns a string representation of this label.
     *
     * @return a string representation of this label.
     */
    public String toString() {
        return "L" + System.identityHashCode(this);
    }
}
