/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import static com.sleepycat.asm.Opcodes.AALOAD;
import static com.sleepycat.asm.Opcodes.ACC_ABSTRACT;
import static com.sleepycat.asm.Opcodes.ACC_PRIVATE;
import static com.sleepycat.asm.Opcodes.ACC_PUBLIC;
import static com.sleepycat.asm.Opcodes.ACC_STATIC;
import static com.sleepycat.asm.Opcodes.ACC_TRANSIENT;
import static com.sleepycat.asm.Opcodes.ACONST_NULL;
import static com.sleepycat.asm.Opcodes.ALOAD;
import static com.sleepycat.asm.Opcodes.ANEWARRAY;
import static com.sleepycat.asm.Opcodes.ARETURN;
import static com.sleepycat.asm.Opcodes.BIPUSH;
import static com.sleepycat.asm.Opcodes.CHECKCAST;
import static com.sleepycat.asm.Opcodes.DCMPL;
import static com.sleepycat.asm.Opcodes.DCONST_0;
import static com.sleepycat.asm.Opcodes.DUP;
import static com.sleepycat.asm.Opcodes.FCMPL;
import static com.sleepycat.asm.Opcodes.FCONST_0;
import static com.sleepycat.asm.Opcodes.GETFIELD;
import static com.sleepycat.asm.Opcodes.GOTO;
import static com.sleepycat.asm.Opcodes.ICONST_0;
import static com.sleepycat.asm.Opcodes.ICONST_1;
import static com.sleepycat.asm.Opcodes.ICONST_2;
import static com.sleepycat.asm.Opcodes.ICONST_3;
import static com.sleepycat.asm.Opcodes.ICONST_4;
import static com.sleepycat.asm.Opcodes.ICONST_5;
import static com.sleepycat.asm.Opcodes.IFEQ;
import static com.sleepycat.asm.Opcodes.IFGT;
import static com.sleepycat.asm.Opcodes.IFLE;
import static com.sleepycat.asm.Opcodes.IFNE;
import static com.sleepycat.asm.Opcodes.IFNONNULL;
import static com.sleepycat.asm.Opcodes.IF_ICMPNE;
import static com.sleepycat.asm.Opcodes.ILOAD;
import static com.sleepycat.asm.Opcodes.INVOKEINTERFACE;
import static com.sleepycat.asm.Opcodes.INVOKESPECIAL;
import static com.sleepycat.asm.Opcodes.INVOKESTATIC;
import static com.sleepycat.asm.Opcodes.INVOKEVIRTUAL;
import static com.sleepycat.asm.Opcodes.IRETURN;
import static com.sleepycat.asm.Opcodes.ISUB;
import static com.sleepycat.asm.Opcodes.LCMP;
import static com.sleepycat.asm.Opcodes.LCONST_0;
import static com.sleepycat.asm.Opcodes.NEW;
import static com.sleepycat.asm.Opcodes.POP;
import static com.sleepycat.asm.Opcodes.PUTFIELD;
import static com.sleepycat.asm.Opcodes.RETURN;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.sleepycat.asm.AnnotationVisitor;
import com.sleepycat.asm.Attribute;
import com.sleepycat.asm.ClassAdapter;
import com.sleepycat.asm.ClassVisitor;
import com.sleepycat.asm.FieldVisitor;
import com.sleepycat.asm.Label;
import com.sleepycat.asm.MethodVisitor;
import com.sleepycat.asm.Type;

/**
 * An ASM ClassVisitor that examines a class, throws NotPersistentException if
 * it is not persistent, or enhances it if it is persistent.  A class is
 * persistent if it contains the @Entity or @Persistent annotations.  A
 * resulting enhanced class implements the com.sleepycat.persist.impl.Enhanced
 * interface.
 *
 * <p>NotPersistentException is thrown to abort the transformation in order to
 * avoid making two passes over the class file (one to look for the annotations
 * and another to enhance the bytecode) or outputing a class that isn't
 * enhanced.  By aborting the transformation as soon as we detect that the
 * annotations are missing, we make only one partial pass for a non-persistent
 * class.</p>
 *
 * @author Mark Hayes
 */
class BytecodeEnhancer extends ClassAdapter {

    /** Thrown when we determine that a class is not persistent. */
    @SuppressWarnings("serial")
    static class NotPersistentException extends RuntimeException {}

    /** A static instance is used to avoid fillInStaceTrace overhead. */
    private static final NotPersistentException NOT_PERSISTENT =
        new NotPersistentException();

    private static final Map<String,Integer> PRIMITIVE_WRAPPERS =
        new HashMap<String,Integer>();
    static {
        PRIMITIVE_WRAPPERS.put(Boolean.class.getName(), Type.BOOLEAN);
        PRIMITIVE_WRAPPERS.put(Character.class.getName(), Type.CHAR);
        PRIMITIVE_WRAPPERS.put(Byte.class.getName(), Type.BYTE);
        PRIMITIVE_WRAPPERS.put(Short.class.getName(), Type.SHORT);
        PRIMITIVE_WRAPPERS.put(Integer.class.getName(), Type.INT);
        PRIMITIVE_WRAPPERS.put(Long.class.getName(), Type.LONG);
        PRIMITIVE_WRAPPERS.put(Float.class.getName(), Type.FLOAT);
        PRIMITIVE_WRAPPERS.put(Double.class.getName(), Type.DOUBLE);
    }

    private String className;
    private String superclassName;
    private boolean isPersistent;
    private boolean isAbstract;
    private boolean hasDefaultConstructor;
    private boolean hasPersistentSuperclass;
    private boolean isCompositeKey;
    private FieldInfo priKeyField;
    private List<FieldInfo> secKeyFields;
    private List<FieldInfo> nonKeyFields;
    private String staticBlockMethod;

    BytecodeEnhancer(ClassVisitor parentVisitor) {
        super(parentVisitor);
        secKeyFields = new ArrayList<FieldInfo>();
        nonKeyFields = new ArrayList<FieldInfo>();
    }

    @Override
    public void visit(int version,
                      int access,
                      String name,
                      String sig,
                      String superName,
                      String[] interfaces) {
        className = name;
        superclassName = superName;
        final String ENHANCED = "com/sleepycat/persist/impl/Enhanced";
        if (containsString(interfaces, ENHANCED)) {
            throw abort();
        }
        interfaces = appendString(interfaces, ENHANCED);
        isAbstract = ((access & ACC_ABSTRACT) != 0);
        hasPersistentSuperclass =
            (superName != null && !superName.equals("java/lang/Object"));
        super.visit(version, access, name, sig, superName, interfaces);
    }

    @Override
    public void visitSource(String source, String debug) {
        super.visitSource(source, debug);
    }

    @Override
    public AnnotationVisitor visitAnnotation(String desc, boolean visible) {
        if (desc.equals("Lcom/sleepycat/persist/model/Entity;") ||
            desc.equals("Lcom/sleepycat/persist/model/Persistent;")) {
            isPersistent = true;
        }
        return super.visitAnnotation(desc, visible);
    }

    @Override
    public FieldVisitor visitField(int access,
                                   String name,
                                   String desc,
                                   String sig,
                                   Object value) {
        if (!isPersistent) {
            throw abort();
        }
        FieldVisitor ret = super.visitField(access, name, desc, sig, value);
        if ((access & ACC_STATIC) == 0) {
            FieldInfo info = new FieldInfo(ret, name, desc,
                                           (access & ACC_TRANSIENT) != 0);
            nonKeyFields.add(info);
            ret = info;
        }
        return ret;
    }

    @Override
    public MethodVisitor visitMethod(int access,
                                     String name,
                                     String desc,
                                     String sig,
                                     String[] exceptions) {
        if (!isPersistent) {
            throw abort();
        }
        if ("<init>".equals(name) && "()V".equals(desc)) {
            hasDefaultConstructor = true;
        }
        if ("<clinit>".equals(name)) {
            if (staticBlockMethod != null) {
                throw new IllegalStateException();
            }
            staticBlockMethod = "bdbExistingStaticBlock";
            return cv.visitMethod
                (ACC_PRIVATE + ACC_STATIC, staticBlockMethod, "()V", null,
                 null);
        }
        return super.visitMethod(access, name, desc, sig, exceptions);
    }

    @Override
    public void visitEnd() {
        if (!isPersistent || !hasDefaultConstructor) {
            throw abort();
        }
        /* Generate new code at the end of the class. */
        sortFields();
        genBdbNewInstance();
        genBdbNewArray();
        genBdbIsPriKeyFieldNullOrZero();
        genBdbWritePriKeyField();
        genBdbReadPriKeyField();
        genBdbWriteSecKeyFields();
        genBdbReadSecKeyFields();
        genBdbWriteNonKeyFields();
        genBdbReadNonKeyFields();
        genBdbWriteCompositeKeyFields();
        genBdbReadCompositeKeyFields();
        genBdbGetField();
        genBdbSetField();
        genStaticBlock();
        super.visitEnd();
    }

    private void sortFields() {
        /*
        System.out.println("AllFields: " + nonKeyFields);
        //*/
        if (nonKeyFields.size() == 0) {
            return;
        }
        isCompositeKey = true;
        for (FieldInfo field : nonKeyFields) {
            if (field.order == null) {
                isCompositeKey = false;
            }
        }
        if (isCompositeKey) {
            Collections.sort(nonKeyFields, new Comparator<FieldInfo>() {
                public int compare(FieldInfo f1, FieldInfo f2) {
                    return f1.order.value - f2.order.value;
                }
            });
        } else {
            for (int i = 0; i < nonKeyFields.size();) {
                FieldInfo field = nonKeyFields.get(i);
                if (field.isTransient) {
                    nonKeyFields.remove(i);
                } else if (field.isPriKey) {
                    if (priKeyField == null) {
                        priKeyField = field;
                        nonKeyFields.remove(i);
                    }
                } else if (field.isSecKey) {
                    secKeyFields.add(field);
                    nonKeyFields.remove(i);
                } else {
                    i += 1;
                }
            }
            Comparator<FieldInfo> cmp = new Comparator<FieldInfo>() {
                public int compare(FieldInfo f1, FieldInfo f2) {
                    return f1.name.compareTo(f2.name);
                }
            };
            Collections.sort(secKeyFields, cmp);
            Collections.sort(nonKeyFields, cmp);
        }
        /*
        System.out.println("PriKey: " + priKeyField);
        System.out.println("SecKeys: " + secKeyFields);
        System.out.println("NonKeys: " + nonKeyFields);
        //*/
    }

    /**
     * Outputs code in a static block to register the prototype instance:
     *
     *  static {
     *      EnhancedAccessor.registerClass(TheClassName, new TheClass());
     *      // or for an abstract class:
     *      EnhancedAccessor.registerClass(TheClassName, null);
     *  }
     */
    private void genStaticBlock() {
        MethodVisitor mv =
            cv.visitMethod(ACC_STATIC, "<clinit>", "()V", null, null);
        mv.visitCode();
        if (staticBlockMethod != null) {
            mv.visitMethodInsn
                (INVOKESTATIC, className, staticBlockMethod, "()V");
        }
        mv.visitLdcInsn(className.replace('/', '.'));
        if (isAbstract) {
            mv.visitInsn(ACONST_NULL);
        } else {
            mv.visitTypeInsn(NEW, className);
            mv.visitInsn(DUP);
            mv.visitMethodInsn(INVOKESPECIAL, className, "<init>", "()V");
        }
        mv.visitMethodInsn
            (INVOKESTATIC, "com/sleepycat/persist/impl/EnhancedAccessor",
             "registerClass",
             "(Ljava/lang/String;Lcom/sleepycat/persist/impl/Enhanced;)V");
        mv.visitInsn(RETURN);
        mv.visitMaxs(3, 0);
        mv.visitEnd();
    }

    /**
     *  public Object bdbNewInstance() {
     *      return new TheClass();
     *      // or if abstract:
     *      return null;
     *  }
     */
    private void genBdbNewInstance() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbNewInstance", "()Ljava/lang/Object;", null, null);
        mv.visitCode();
        if (isAbstract) {
            mv.visitInsn(ACONST_NULL);
            mv.visitInsn(ARETURN);
            mv.visitMaxs(1, 1);
        } else {
            mv.visitTypeInsn(NEW, className);
            mv.visitInsn(DUP);
            mv.visitMethodInsn(INVOKESPECIAL, className, "<init>", "()V");
            mv.visitInsn(ARETURN);
            mv.visitMaxs(2, 1);
        }
        mv.visitEnd();
    }

    /**
     *  public Object bdbNewArray(int len) {
     *      return new TheClass[len];
     *      // or if abstract:
     *      return null;
     *  }
     */
    private void genBdbNewArray() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbNewArray", "(I)Ljava/lang/Object;", null, null);
        mv.visitCode();
        if (isAbstract) {
            mv.visitInsn(ACONST_NULL);
            mv.visitInsn(ARETURN);
            mv.visitMaxs(1, 2);
        } else {
            mv.visitVarInsn(ILOAD, 1);
            mv.visitTypeInsn(ANEWARRAY, className);
            mv.visitInsn(ARETURN);
            mv.visitMaxs(1, 2);
            mv.visitEnd();
        }
    }

    /**
     *  public boolean bdbIsPriKeyFieldNullOrZero() {
     *      return theField == null; // or zero or false, as appropriate
     *      // or if no primary key but has superclass:
     *      return super.bdbIsPriKeyFieldNullOrZero();
     *  }
     */
    private void genBdbIsPriKeyFieldNullOrZero() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbIsPriKeyFieldNullOrZero", "()Z", null, null);
        mv.visitCode();
        if (priKeyField != null) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitFieldInsn
                (GETFIELD, className, priKeyField.name,
                 priKeyField.type.getDescriptor());
            Label l0 = new Label();
            if (isRefType(priKeyField.type)) {
                mv.visitJumpInsn(IFNONNULL, l0);
            } else {
                genBeforeCompareToZero(mv, priKeyField.type);
                mv.visitJumpInsn(IFNE, l0);
            }
            mv.visitInsn(ICONST_1);
            Label l1 = new Label();
            mv.visitJumpInsn(GOTO, l1);
            mv.visitLabel(l0);
            mv.visitInsn(ICONST_0);
            mv.visitLabel(l1);
        } else if (hasPersistentSuperclass) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitMethodInsn
                (INVOKESPECIAL, superclassName, "bdbIsPriKeyFieldNullOrZero",
                 "()Z");
        } else {
            mv.visitInsn(ICONST_0);
        }
        mv.visitInsn(IRETURN);
        mv.visitMaxs(1, 1);
        mv.visitEnd();
    }

    /**
     *  public void bdbWritePriKeyField(EntityOutput output, Format format) {
     *      output.writeKeyObject(theField, format);
     *      // or
     *      output.writeInt(theField); // and other simple types
     *      // or if no primary key but has superclass:
     *      return super.bdbWritePriKeyField(output, format);
     *  }
     */
    private void genBdbWritePriKeyField() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbWritePriKeyField",
             "(Lcom/sleepycat/persist/impl/EntityOutput;" +
              "Lcom/sleepycat/persist/impl/Format;)V",
             null, null);
        mv.visitCode();
        if (priKeyField != null) {
            if (!genWriteSimpleKeyField(mv, priKeyField)) {
                /* For a non-simple type, call EntityOutput.writeKeyObject. */
                mv.visitVarInsn(ALOAD, 1);
                mv.visitVarInsn(ALOAD, 0);
                mv.visitFieldInsn
                    (GETFIELD, className, priKeyField.name,
                     priKeyField.type.getDescriptor());
                mv.visitVarInsn(ALOAD, 2);
                mv.visitMethodInsn
                    (INVOKEINTERFACE,
                     "com/sleepycat/persist/impl/EntityOutput",
                     "writeKeyObject",
                     "(Ljava/lang/Object;" +
                      "Lcom/sleepycat/persist/impl/Format;)V");
            }
        } else if (hasPersistentSuperclass) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            mv.visitVarInsn(ALOAD, 2);
            mv.visitMethodInsn
                (INVOKESPECIAL, superclassName, "bdbWritePriKeyField",
                 "(Lcom/sleepycat/persist/impl/EntityOutput;" +
                  "Lcom/sleepycat/persist/impl/Format;)V");
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(3, 3);
        mv.visitEnd();
    }

    /**
     *  public void bdbReadPriKeyField(EntityInput input, Format format) {
     *      theField = (TheFieldClass) input.readKeyObject(format);
     *      // or
     *      theField = input.readInt(); // and other simple types
     *      // or if no primary key but has superclass:
     *      super.bdbReadPriKeyField(input, format);
     *  }
     */
    private void genBdbReadPriKeyField() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbReadPriKeyField",
             "(Lcom/sleepycat/persist/impl/EntityInput;" +
              "Lcom/sleepycat/persist/impl/Format;)V",
             null, null);
        mv.visitCode();
        if (priKeyField != null) {
            if (!genReadSimpleKeyField(mv, priKeyField)) {
                /* For a non-simple type, call EntityInput.readKeyObject. */
                mv.visitVarInsn(ALOAD, 0);
                mv.visitVarInsn(ALOAD, 1);
                mv.visitVarInsn(ALOAD, 2);
                mv.visitMethodInsn
                    (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                     "readKeyObject",
                     "(Lcom/sleepycat/persist/impl/Format;)" +
                     "Ljava/lang/Object;");
                mv.visitTypeInsn(CHECKCAST, getTypeInstName(priKeyField.type));
                mv.visitFieldInsn
                    (PUTFIELD, className, priKeyField.name,
                     priKeyField.type.getDescriptor());
            }
        } else if (hasPersistentSuperclass) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            mv.visitVarInsn(ALOAD, 2);
            mv.visitMethodInsn
                (INVOKESPECIAL, superclassName, "bdbReadPriKeyField",
                 "(Lcom/sleepycat/persist/impl/EntityInput;" +
                  "Lcom/sleepycat/persist/impl/Format;)V");
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(3, 3);
        mv.visitEnd();
    }

    /**
     *  public void bdbWriteSecKeyFields(EntityOutput output) {
     *      output.registerPriKeyObject(priKeyField); // if an object
     *      super.bdbWriteSecKeyFields(EntityOutput output); // if has super
     *      output.writeInt(secKeyField1);
     *      output.writeObject(secKeyField2, null);
     *      // etc
     *  }
     */
    private void genBdbWriteSecKeyFields() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbWriteSecKeyFields",
             "(Lcom/sleepycat/persist/impl/EntityOutput;)V", null, null);
        mv.visitCode();
        if (priKeyField != null && isRefType(priKeyField.type)) {
            genRegisterPrimaryKey(mv, false);
        }
        if (hasPersistentSuperclass) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            mv.visitMethodInsn
                (INVOKESPECIAL, superclassName, "bdbWriteSecKeyFields",
                 "(Lcom/sleepycat/persist/impl/EntityOutput;)V");
        }
        for (FieldInfo field : secKeyFields) {
            genWriteField(mv, field);
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(2, 2);
        mv.visitEnd();
    }

    /**
     *  public void bdbReadSecKeyFields(EntityInput input,
     *                                  int startField,
     *                                  int endField,
     *                                  int superLevel) {
     *      input.registerPriKeyObject(priKeyField); // if an object
     *      // if has super:
     *      if (superLevel != 0) {
     *          super.bdbReadSecKeyFields(..., superLevel - 1);
     *      }
     *      if (superLevel <= 0) {
     *          switch (startField) {
     *          case 0:
     *              secKeyField1 = input.readInt();
     *              if (endField == 0) break;
     *          case 1:
     *              secKeyField2 = (String) input.readObject();
     *              if (endField == 1) break;
     *          case 2:
     *              secKeyField3 = input.readInt();
     *          }
     *      }
     *  }
     */
    private void genBdbReadSecKeyFields() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbReadSecKeyFields",
             "(Lcom/sleepycat/persist/impl/EntityInput;III)V", null, null);
        mv.visitCode();
        if (priKeyField != null && isRefType(priKeyField.type)) {
            genRegisterPrimaryKey(mv, true);
        }
        genReadSuperKeyFields(mv, true);
        genReadFieldSwitch(mv, secKeyFields);
        mv.visitInsn(RETURN);
        mv.visitMaxs(5, 5);
        mv.visitEnd();
    }

    /**
     *      output.registerPriKeyObject(priKeyField);
     *      // or
     *      input.registerPriKeyObject(priKeyField);
     */
    private void genRegisterPrimaryKey(MethodVisitor mv, boolean input) {
        String entityInputOrOutputClass =
            input ? "com/sleepycat/persist/impl/EntityInput"
                  : "com/sleepycat/persist/impl/EntityOutput";
        mv.visitVarInsn(ALOAD, 1);
        mv.visitVarInsn(ALOAD, 0);
        mv.visitFieldInsn
            (GETFIELD, className, priKeyField.name,
             priKeyField.type.getDescriptor());
        mv.visitMethodInsn
            (INVOKEINTERFACE, entityInputOrOutputClass, "registerPriKeyObject",
             "(Ljava/lang/Object;)V");
    }

    /**
     *  public void bdbWriteNonKeyFields(EntityOutput output) {
     *      // like bdbWriteSecKeyFields but does not call registerPriKeyObject
     *  }
     */
    private void genBdbWriteNonKeyFields() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbWriteNonKeyFields",
             "(Lcom/sleepycat/persist/impl/EntityOutput;)V", null, null);
        mv.visitCode();
        if (!isCompositeKey) {
            if (hasPersistentSuperclass) {
                mv.visitVarInsn(ALOAD, 0);
                mv.visitVarInsn(ALOAD, 1);
                mv.visitMethodInsn
                    (INVOKESPECIAL, superclassName, "bdbWriteNonKeyFields",
                     "(Lcom/sleepycat/persist/impl/EntityOutput;)V");
            }
            for (FieldInfo field : nonKeyFields) {
                genWriteField(mv, field);
            }
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(2, 2);
        mv.visitEnd();
    }

    /**
     *  public void bdbReadNonKeyFields(EntityInput input,
     *                                  int startField,
     *                                  int endField,
     *                                  int superLevel) {
     *      // like bdbReadSecKeyFields but does not call registerPriKeyObject
     *  }
     */
    private void genBdbReadNonKeyFields() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbReadNonKeyFields",
             "(Lcom/sleepycat/persist/impl/EntityInput;III)V", null, null);
        mv.visitCode();
        if (!isCompositeKey) {
            genReadSuperKeyFields(mv, false);
            genReadFieldSwitch(mv, nonKeyFields);
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(5, 5);
        mv.visitEnd();
    }

    /**
     *  public void bdbWriteCompositeKeyFields(EntityOutput output,
     *                                         Format[] formats) {
     *      output.writeInt(compositeKeyField1);
     *      output.writeKeyObject(compositeKeyField2, formats[1]);
     *      // etc
     *  }
     */
    private void genBdbWriteCompositeKeyFields() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbWriteCompositeKeyFields",
             "(Lcom/sleepycat/persist/impl/EntityOutput;" +
             "[Lcom/sleepycat/persist/impl/Format;)V",
             null, null);
        mv.visitCode();
        if (isCompositeKey) {
            for (int i = 0; i < nonKeyFields.size(); i += 1) {
                FieldInfo field = nonKeyFields.get(i);
                if (!genWriteSimpleKeyField(mv, field)) {
                    /* For a non-simple type, call writeKeyObject. */
                    mv.visitVarInsn(ALOAD, 1);
                    mv.visitVarInsn(ALOAD, 0);
                    mv.visitFieldInsn
                        (GETFIELD, className, field.name,
                         field.type.getDescriptor());
                    mv.visitVarInsn(ALOAD, 2);
                    if (i <= Byte.MAX_VALUE) {
                        mv.visitIntInsn(BIPUSH, i);
                    } else {
                        mv.visitLdcInsn(new Integer(i));
                    }
                    mv.visitInsn(AALOAD);
                    mv.visitMethodInsn
                        (INVOKEINTERFACE,
                         "com/sleepycat/persist/impl/EntityOutput",
                         "writeKeyObject",
                         "(Ljava/lang/Object;" +
                          "Lcom/sleepycat/persist/impl/Format;)V");
                }
            }
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(3, 3);
        mv.visitEnd();
    }

    /**
     *  public void bdbReadCompositeKeyFields(EntityInput input,
     *                                        Format[] formats) {
     *      compositeKeyField1 = input.readInt();
     *      compositeKeyField2 = input.readKeyObject(formats[1]);
     *  }
     */
    private void genBdbReadCompositeKeyFields() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbReadCompositeKeyFields",
             "(Lcom/sleepycat/persist/impl/EntityInput;" +
             "[Lcom/sleepycat/persist/impl/Format;)V",
             null, null);
        mv.visitCode();
        if (isCompositeKey) {
            for (int i = 0; i < nonKeyFields.size(); i += 1) {
                FieldInfo field = nonKeyFields.get(i);
                /* Ignore non-simple (illegal) types for composite key. */
                if (!genReadSimpleKeyField(mv, field)) {
                    /* For a non-simple type, call readKeyObject. */
                    mv.visitVarInsn(ALOAD, 0);
                    mv.visitVarInsn(ALOAD, 1);
                    mv.visitVarInsn(ALOAD, 2);
                    if (i <= Byte.MAX_VALUE) {
                        mv.visitIntInsn(BIPUSH, i);
                    } else {
                        mv.visitLdcInsn(new Integer(i));
                    }
                    mv.visitInsn(AALOAD);
                    mv.visitMethodInsn
                        (INVOKEINTERFACE,
                         "com/sleepycat/persist/impl/EntityInput",
                         "readKeyObject",
                         "(Lcom/sleepycat/persist/impl/Format;)" +
                         "Ljava/lang/Object;");
                    mv.visitTypeInsn(CHECKCAST, getTypeInstName(field.type));
                    mv.visitFieldInsn
                        (PUTFIELD, className, field.name,
                         field.type.getDescriptor());
                }
            }
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(5, 5);
        mv.visitEnd();
    }

    /**
     *      output.writeInt(field); // and other primitives
     *      // or
     *      output.writeObject(field, null);
     */
    private void genWriteField(MethodVisitor mv, FieldInfo field) {
        mv.visitVarInsn(ALOAD, 1);
        mv.visitVarInsn(ALOAD, 0);
        mv.visitFieldInsn
            (GETFIELD, className, field.name, field.type.getDescriptor());
        int sort = field.type.getSort();
        if (sort == Type.OBJECT || sort == Type.ARRAY) {
            mv.visitInsn(ACONST_NULL);
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeObject",
                 "(Ljava/lang/Object;Lcom/sleepycat/persist/impl/Format;)V");
        } else {
            genWritePrimitive(mv, sort);
        }
    }

    /**
     * Generates writing of a simple type key field, or returns false if the
     * key field is not a simple type (i.e., it is a composite key type).
     *
     *      output.writeInt(theField); // and other primitives
     *      // or
     *      output.writeInt(theField.intValue()); // and other simple types
     *      // or returns false
     */
    private boolean genWriteSimpleKeyField(MethodVisitor mv, FieldInfo field) {
        if (genWritePrimitiveField(mv, field)) {
            return true;
        }
        String fieldClassName = field.type.getClassName();
        if (!isSimpleRefType(fieldClassName)) {
            return false;
        }
        mv.visitVarInsn(ALOAD, 1);
        mv.visitVarInsn(ALOAD, 0);
        mv.visitFieldInsn
            (GETFIELD, className, field.name, field.type.getDescriptor());
        Integer sort = PRIMITIVE_WRAPPERS.get(fieldClassName);
        if (sort != null) {
            genUnwrapPrimitive(mv, sort);
            genWritePrimitive(mv, sort);
        } else if (fieldClassName.equals(Date.class.getName())) {
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/util/Date", "getTime", "()J");
            genWritePrimitive(mv, Type.LONG);
        } else if (fieldClassName.equals(String.class.getName())) {
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeString",
                 "(Ljava/lang/String;)Lcom/sleepycat/bind/tuple/TupleOutput;");
            mv.visitInsn(POP);
        } else if (fieldClassName.equals(BigInteger.class.getName())) {
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeBigInteger",
             "(Ljava/math/BigInteger;)Lcom/sleepycat/bind/tuple/TupleOutput;");
            mv.visitInsn(POP);
        } else {
            throw new IllegalStateException(fieldClassName);
        }
        return true;
    }

    private boolean genWritePrimitiveField(MethodVisitor mv, FieldInfo field) {
        int sort = field.type.getSort();
        if (sort == Type.OBJECT || sort == Type.ARRAY) {
            return false;
        }
        mv.visitVarInsn(ALOAD, 1);
        mv.visitVarInsn(ALOAD, 0);
        mv.visitFieldInsn
            (GETFIELD, className, field.name, field.type.getDescriptor());
        genWritePrimitive(mv, sort);
        return true;
    }

    /**
     *      // if has super:
     *      if (superLevel != 0) {
     *          super.bdbReadXxxKeyFields(..., superLevel - 1);
     *      }
     */
    private void genReadSuperKeyFields(MethodVisitor mv,
                                       boolean areSecKeyFields) {
        if (hasPersistentSuperclass) {
            Label next = new Label();
            mv.visitVarInsn(ILOAD, 4);
            mv.visitJumpInsn(IFEQ, next);
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            mv.visitVarInsn(ILOAD, 2);
            mv.visitVarInsn(ILOAD, 3);
            mv.visitVarInsn(ILOAD, 4);
            mv.visitInsn(ICONST_1);
            mv.visitInsn(ISUB);
            String name = areSecKeyFields ? "bdbReadSecKeyFields"
                                          : "bdbReadNonKeyFields";
            mv.visitMethodInsn
                (INVOKESPECIAL, superclassName, name,
                 "(Lcom/sleepycat/persist/impl/EntityInput;III)V");
            mv.visitLabel(next);
        }
    }

    /**
     *  public void bdbReadXxxKeyFields(EntityInput input,
     *                                  int startField,
     *                                  int endField,
     *                                  int superLevel) {
     *      // ...
     *      if (superLevel <= 0) {
     *          switch (startField) {
     *          case 0:
     *              keyField1 = input.readInt();
     *              if (endField == 0) break;
     *          case 1:
     *              keyField2 = (String) input.readObject();
     *              if (endField == 1) break;
     *          case 2:
     *              keyField3 = input.readInt();
     *          }
     *      }
     */
    private void genReadFieldSwitch(MethodVisitor mv, List<FieldInfo> fields) {
        int nFields = fields.size();
        if (nFields > 0) {
            mv.visitVarInsn(ILOAD, 4);
            Label pastSwitch = new Label();
            mv.visitJumpInsn(IFGT, pastSwitch);
            Label[] labels = new Label[nFields];
            for (int i = 0; i < nFields; i += 1) {
                labels[i] = new Label();
            }
            mv.visitVarInsn(ILOAD, 2);
            mv.visitTableSwitchInsn(0, nFields - 1, pastSwitch, labels);
            for (int i = 0; i < nFields; i += 1) {
                FieldInfo field = fields.get(i);
                mv.visitLabel(labels[i]);
                genReadField(mv, field);
                if (i < nFields - 1) {
                    Label nextCase = labels[i + 1];
                    mv.visitVarInsn(ILOAD, 3);
                    if (i == 0) {
                        mv.visitJumpInsn(IFNE, nextCase);
                    } else {
                        switch (i) {
                        case 1:
                            mv.visitInsn(ICONST_1);
                            break;
                        case 2:
                            mv.visitInsn(ICONST_2);
                            break;
                        case 3:
                            mv.visitInsn(ICONST_3);
                            break;
                        case 4:
                            mv.visitInsn(ICONST_4);
                            break;
                        case 5:
                            mv.visitInsn(ICONST_5);
                            break;
                        default:
                            mv.visitIntInsn(BIPUSH, i);
                        }
                        mv.visitJumpInsn(IF_ICMPNE, nextCase);
                    }
                    mv.visitJumpInsn(GOTO, pastSwitch);
                }
            }
            mv.visitLabel(pastSwitch);
        }
    }

    /**
     *      field = input.readInt(); // and other primitives
     *      // or
     *      field = (FieldClass) input.readObject();
     */
    private void genReadField(MethodVisitor mv, FieldInfo field) {
        mv.visitVarInsn(ALOAD, 0);
        mv.visitVarInsn(ALOAD, 1);
        if (isRefType(field.type)) {
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readObject", "()Ljava/lang/Object;");
            mv.visitTypeInsn(CHECKCAST, getTypeInstName(field.type));
        } else {
            genReadPrimitive(mv, field.type.getSort());
        }
        mv.visitFieldInsn
            (PUTFIELD, className, field.name, field.type.getDescriptor());
    }

    /**
     * Generates reading of a simple type key field, or returns false if the
     * key field is not a simple type (i.e., it is a composite key type).
     *
     *      field = input.readInt(); // and other primitives
     *      // or
     *      field = Integer.valueOf(input.readInt()); // and other simple types
     *      // or returns false
     */
    private boolean genReadSimpleKeyField(MethodVisitor mv, FieldInfo field) {
        if (genReadPrimitiveField(mv, field)) {
            return true;
        }
        String fieldClassName = field.type.getClassName();
        if (!isSimpleRefType(fieldClassName)) {
            return false;
        }
        Integer sort = PRIMITIVE_WRAPPERS.get(fieldClassName);
        if (sort != null) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            genReadPrimitive(mv, sort);
            genWrapPrimitive(mv, sort);
        } else if (fieldClassName.equals(Date.class.getName())) {
            /* Date is a special case because we use NEW instead of valueOf. */
            mv.visitVarInsn(ALOAD, 0);
            mv.visitTypeInsn(NEW, "java/util/Date");
            mv.visitInsn(DUP);
            mv.visitVarInsn(ALOAD, 1);
            genReadPrimitive(mv, Type.LONG);
            mv.visitMethodInsn
                (INVOKESPECIAL, "java/util/Date", "<init>", "(J)V");
        } else if (fieldClassName.equals(String.class.getName())) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readString", "()Ljava/lang/String;");
        } else if (fieldClassName.equals(BigInteger.class.getName())) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readBigInteger", "()Ljava/math/BigInteger;");
        } else {
            throw new IllegalStateException(fieldClassName);
        }
        mv.visitFieldInsn
            (PUTFIELD, className, field.name, field.type.getDescriptor());
        return true;
    }

    private boolean genReadPrimitiveField(MethodVisitor mv, FieldInfo field) {
        int sort = field.type.getSort();
        if (sort == Type.OBJECT || sort == Type.ARRAY) {
            return false;
        }
        mv.visitVarInsn(ALOAD, 0);
        mv.visitVarInsn(ALOAD, 1);
        genReadPrimitive(mv, sort);
        mv.visitFieldInsn
            (PUTFIELD, className, field.name, field.type.getDescriptor());
        return true;
    }

    /**
     *  public Object bdbGetField(Object o,
     *                            int field,
     *                            int superLevel,
     *                            boolean isSecField) {
     *      if (superLevel > 0) {
     *          // if has superclass:
     *          return super.bdbGetField
     *              (o, field, superLevel - 1, isSecField);
     *      } else if (isSecField) {
     *          switch (field) {
     *          case 0:
     *              return Integer.valueOf(f2);
     *          case 1:
     *              return f3;
     *          case 2:
     *              return f4;
     *          }
     *      } else {
     *          switch (field) {
     *          case 0:
     *              return Integer.valueOf(f5);
     *          case 1:
     *              return f6;
     *          case 2:
     *              return f7;
     *          }
     *      }
     *      return null;
     *  }
     */
    private void genBdbGetField() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbGetField",
             "(Ljava/lang/Object;IIZ)Ljava/lang/Object;", null, null);
        mv.visitCode();
        mv.visitVarInsn(ILOAD, 3);
        Label l0 = new Label();
        mv.visitJumpInsn(IFLE, l0);
        Label l1 = new Label();
        if (hasPersistentSuperclass) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            mv.visitVarInsn(ILOAD, 2);
            mv.visitVarInsn(ILOAD, 3);
            mv.visitInsn(ICONST_1);
            mv.visitInsn(ISUB);
            mv.visitVarInsn(ILOAD, 4);
            mv.visitMethodInsn
                (INVOKESPECIAL, className, "bdbGetField",
                 "(Ljava/lang/Object;IIZ)Ljava/lang/Object;");
            mv.visitInsn(ARETURN);
        } else {
            mv.visitJumpInsn(GOTO, l1);
        }
        mv.visitLabel(l0);
        mv.visitVarInsn(ILOAD, 4);
        Label l2 = new Label();
        mv.visitJumpInsn(IFEQ, l2);
        genGetFieldSwitch(mv, secKeyFields, l1);
        mv.visitLabel(l2);
        genGetFieldSwitch(mv, nonKeyFields, l1);
        mv.visitLabel(l1);
        mv.visitInsn(ACONST_NULL);
        mv.visitInsn(ARETURN);
        mv.visitMaxs(1, 5);
        mv.visitEnd();
    }

    /**
     *  mv.visitVarInsn(ILOAD, 2);
     *  Label l0 = new Label();
     *  Label l1 = new Label();
     *  Label l2 = new Label();
     *  mv.visitTableSwitchInsn(0, 2, TheDefLabel, new Label[] { l0, l1, l2 });
     *  mv.visitLabel(l0);
     *  mv.visitVarInsn(ALOAD, 0);
     *  mv.visitFieldInsn(GETFIELD, TheClassName, "f2", "I");
     *  mv.visitMethodInsn(INVOKESTATIC, "java/lang/Integer", "valueOf",
     *                     "(I)Ljava/lang/Integer;");
     *  mv.visitInsn(ARETURN);
     *  mv.visitLabel(l1);
     *  mv.visitVarInsn(ALOAD, 0);
     *  mv.visitFieldInsn(GETFIELD, TheClassName, "f3", "Ljava/lang/String;");
     *  mv.visitInsn(ARETURN);
     *  mv.visitLabel(l2);
     *  mv.visitVarInsn(ALOAD, 0);
     *  mv.visitFieldInsn(GETFIELD, TheClassName, "f4", "Ljava/lang/String;");
     *  mv.visitInsn(ARETURN);
     */
    private void genGetFieldSwitch(MethodVisitor mv,
                                   List<FieldInfo> fields,
                                   Label defaultLabel) {
        int nFields = fields.size();
        if (nFields == 0) {
            mv.visitJumpInsn(GOTO, defaultLabel);
            return;
        }
        Label[] labels = new Label[nFields];
        for (int i = 0; i < nFields; i += 1) {
            labels[i] = new Label();
        }
        mv.visitVarInsn(ILOAD, 2);
        mv.visitTableSwitchInsn(0, nFields - 1, defaultLabel, labels);
        for (int i = 0; i < nFields; i += 1) {
            FieldInfo field = fields.get(i);
            mv.visitLabel(labels[i]);
            mv.visitVarInsn(ALOAD, 0);
            mv.visitFieldInsn
                (GETFIELD, className, field.name, field.type.getDescriptor());
            if (!isRefType(field.type)) {
                genWrapPrimitive(mv, field.type.getSort());
            }
            mv.visitInsn(ARETURN);
        }
    }

    /**
     *  public void bdbSetField(Object o,
     *                          int field,
     *                          int superLevel,
     *                          boolean isSecField,
     *                          Object value) {
     *      if (superLevel > 0) {
     *          // if has superclass:
     *          super.bdbSetField
     *              (o, field, superLevel - 1, isSecField, value);
     *      } else if (isSecField) {
     *          switch (field) {
     *          case 0:
     *              f2 = ((Integer) value).intValue();
     *          case 1:
     *              f3 = (String) value;
     *          case 2:
     *              f4 = (String) value;
     *          }
     *      } else {
     *          switch (field) {
     *          case 0:
     *              f5 = ((Integer) value).intValue();
     *          case 1:
     *              f6 = (String) value;
     *          case 2:
     *              f7 = (String) value;
     *          }
     *      }
     *  }
     */
    private void genBdbSetField() {
        MethodVisitor mv = cv.visitMethod
            (ACC_PUBLIC, "bdbSetField",
             "(Ljava/lang/Object;IIZLjava/lang/Object;)V", null, null);
        mv.visitCode();
        mv.visitVarInsn(ILOAD, 3);
        Label l0 = new Label();
        mv.visitJumpInsn(IFLE, l0);
        if (hasPersistentSuperclass) {
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 1);
            mv.visitVarInsn(ILOAD, 2);
            mv.visitVarInsn(ILOAD, 3);
            mv.visitInsn(ICONST_1);
            mv.visitInsn(ISUB);
            mv.visitVarInsn(ILOAD, 4);
            mv.visitVarInsn(ALOAD, 5);
            mv.visitMethodInsn
                (INVOKESPECIAL, className, "bdbSetField",
                 "(Ljava/lang/Object;IIZLjava/lang/Object;)V");
        }
        mv.visitInsn(RETURN);
        mv.visitLabel(l0);
        mv.visitVarInsn(ILOAD, 4);
        Label l2 = new Label();
        mv.visitJumpInsn(IFEQ, l2);
        Label l1 = new Label();
        genSetFieldSwitch(mv, secKeyFields, l1);
        mv.visitLabel(l2);
        genSetFieldSwitch(mv, nonKeyFields, l1);
        mv.visitLabel(l1);
        mv.visitInsn(RETURN);
        mv.visitMaxs(2, 6);
        mv.visitEnd();
    }

    /**
     *  mv.visitVarInsn(ILOAD, 2);
     *  Label l0 = new Label();
     *  Label l1 = new Label();
     *  Label l2 = new Label();
     *  mv.visitTableSwitchInsn(0, 2, TheDefLabel, new Label[] { l0, l1, l2 });
     *  mv.visitLabel(l0);
     *  mv.visitVarInsn(ALOAD, 0);
     *  mv.visitVarInsn(ALOAD, 5);
     *  mv.visitTypeInsn(CHECKCAST, "java/lang/Integer");
     *  mv.visitMethodInsn(INVOKEVIRTUAL, "java/lang/Integer", "intValue",
     *                     "()I");
     *  mv.visitFieldInsn(PUTFIELD, TheClassName, "f2", "I");
     *  mv.visitLabel(l1);
     *  mv.visitVarInsn(ALOAD, 0);
     *  mv.visitVarInsn(ALOAD, 5);
     *  mv.visitTypeInsn(CHECKCAST, "java/lang/String");
     *  mv.visitFieldInsn(PUTFIELD, TheClassName, "f3", "Ljava/lang/String;");
     *  mv.visitLabel(l2);
     *  mv.visitVarInsn(ALOAD, 0);
     *  mv.visitVarInsn(ALOAD, 5);
     *  mv.visitTypeInsn(CHECKCAST, "java/lang/String");
     *  mv.visitFieldInsn(PUTFIELD, TheClassName, "f4", "Ljava/lang/String;");
     */
    private void genSetFieldSwitch(MethodVisitor mv,
                                   List<FieldInfo> fields,
                                   Label defaultLabel) {
        int nFields = fields.size();
        if (nFields == 0) {
            mv.visitJumpInsn(GOTO, defaultLabel);
            return;
        }
        Label[] labels = new Label[nFields];
        for (int i = 0; i < nFields; i += 1) {
            labels[i] = new Label();
        }
        mv.visitVarInsn(ILOAD, 2);
        mv.visitTableSwitchInsn(0, nFields - 1, defaultLabel, labels);
        for (int i = 0; i < nFields; i += 1) {
            FieldInfo field = fields.get(i);
            mv.visitLabel(labels[i]);
            mv.visitVarInsn(ALOAD, 0);
            mv.visitVarInsn(ALOAD, 5);
            if (isRefType(field.type)) {
                mv.visitTypeInsn(CHECKCAST, getTypeInstName(field.type));
            } else {
                int sort = field.type.getSort();
                mv.visitTypeInsn
                    (CHECKCAST,
                     getPrimitiveWrapperClass(sort).replace('.', '/'));
                genUnwrapPrimitive(mv, sort);
            }
            mv.visitFieldInsn
                (PUTFIELD, className, field.name, field.type.getDescriptor());
            mv.visitInsn(RETURN);
        }
    }

    private void genWritePrimitive(MethodVisitor mv, int sort) {
        switch (sort) {
        case Type.BOOLEAN:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeBoolean", "(Z)Lcom/sleepycat/bind/tuple/TupleOutput;");
            break;
        case Type.CHAR:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeChar", "(I)Lcom/sleepycat/bind/tuple/TupleOutput;");
            break;
        case Type.BYTE:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeByte", "(I)Lcom/sleepycat/bind/tuple/TupleOutput;");
            break;
        case Type.SHORT:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeShort", "(I)Lcom/sleepycat/bind/tuple/TupleOutput;");
            break;
        case Type.INT:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeInt", "(I)Lcom/sleepycat/bind/tuple/TupleOutput;");
            break;
        case Type.LONG:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeLong", "(J)Lcom/sleepycat/bind/tuple/TupleOutput;");
            break;
        case Type.FLOAT:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeSortedFloat",
                 "(F)Lcom/sleepycat/bind/tuple/TupleOutput;");
            break;
        case Type.DOUBLE:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityOutput",
                 "writeSortedDouble",
                 "(D)Lcom/sleepycat/bind/tuple/TupleOutput;");
            break;
        default:
            throw new IllegalStateException(String.valueOf(sort));
        }
        /* The write methods always return 'this' and we always discard it. */
        mv.visitInsn(POP);
    }

    private void genReadPrimitive(MethodVisitor mv, int sort) {
        switch (sort) {
        case Type.BOOLEAN:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readBoolean", "()Z");
            break;
        case Type.CHAR:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readChar", "()C");
            break;
        case Type.BYTE:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readByte", "()B");
            break;
        case Type.SHORT:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readShort", "()S");
            break;
        case Type.INT:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readInt", "()I");
            break;
        case Type.LONG:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readLong", "()J");
            break;
        case Type.FLOAT:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readSortedFloat", "()F");
            break;
        case Type.DOUBLE:
            mv.visitMethodInsn
                (INVOKEINTERFACE, "com/sleepycat/persist/impl/EntityInput",
                 "readSortedDouble", "()D");
            break;
        default:
            throw new IllegalStateException(String.valueOf(sort));
        }
    }

    private void genWrapPrimitive(MethodVisitor mv, int sort) {
        switch (sort) {
        case Type.BOOLEAN:
            mv.visitMethodInsn
                (INVOKESTATIC, "java/lang/Boolean", "valueOf",
                 "(Z)Ljava/lang/Boolean;");
            break;
        case Type.CHAR:
            mv.visitMethodInsn
                (INVOKESTATIC, "java/lang/Character", "valueOf",
                 "(C)Ljava/lang/Character;");
            break;
        case Type.BYTE:
            mv.visitMethodInsn
                (INVOKESTATIC, "java/lang/Byte", "valueOf",
                 "(B)Ljava/lang/Byte;");
            break;
        case Type.SHORT:
            mv.visitMethodInsn
                (INVOKESTATIC, "java/lang/Short", "valueOf",
                 "(S)Ljava/lang/Short;");
            break;
        case Type.INT:
            mv.visitMethodInsn
                (INVOKESTATIC, "java/lang/Integer", "valueOf",
                 "(I)Ljava/lang/Integer;");
            break;
        case Type.LONG:
            mv.visitMethodInsn
                (INVOKESTATIC, "java/lang/Long", "valueOf",
                 "(J)Ljava/lang/Long;");
            break;
        case Type.FLOAT:
            mv.visitMethodInsn
                (INVOKESTATIC, "java/lang/Float", "valueOf",
                 "(F)Ljava/lang/Float;");
            break;
        case Type.DOUBLE:
            mv.visitMethodInsn
                (INVOKESTATIC, "java/lang/Double", "valueOf",
                 "(D)Ljava/lang/Double;");
            break;
        default:
            throw new IllegalStateException(String.valueOf(sort));
        }
    }

    private void genUnwrapPrimitive(MethodVisitor mv, int sort) {
        switch (sort) {
        case Type.BOOLEAN:
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/lang/Boolean", "booleanValue", "()Z");
            break;
        case Type.CHAR:
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/lang/Character", "charValue", "()C");
            break;
        case Type.BYTE:
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/lang/Byte", "byteValue", "()B");
            break;
        case Type.SHORT:
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/lang/Short", "shortValue", "()S");
            break;
        case Type.INT:
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/lang/Integer", "intValue", "()I");
            break;
        case Type.LONG:
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/lang/Long", "longValue", "()J");
            break;
        case Type.FLOAT:
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/lang/Float", "floatValue", "()F");
            break;
        case Type.DOUBLE:
            mv.visitMethodInsn
                (INVOKEVIRTUAL, "java/lang/Double", "doubleValue", "()D");
            break;
        default:
            throw new IllegalStateException(String.valueOf(sort));
        }
    }

    /**
     * Returns the type name for a visitTypeInsn operand, which is the internal
     * name for an object type and the descriptor for an array type.  Must not
     * be called for a non-reference type.
     */
    private static String getTypeInstName(Type type) {
        if (type.getSort() == Type.OBJECT) {
            return type.getInternalName();
        } else if (type.getSort() == Type.ARRAY) {
            return type.getDescriptor();
        } else {
            throw new IllegalStateException();
        }
    }

    /**
     * Call this method before comparing a non-reference operand to zero as an
     * int, for example, with IFNE, IFEQ, IFLT, etc.  If the operand is a long,
     * float or double, this method will compare it to zero and leave the
     * result as an int operand.
     */
    private static void genBeforeCompareToZero(MethodVisitor mv, Type type) {
        switch (type.getSort()) {
        case Type.LONG:
            mv.visitInsn(LCONST_0);
            mv.visitInsn(LCMP);
            break;
        case Type.FLOAT:
            mv.visitInsn(FCONST_0);
            mv.visitInsn(FCMPL);
            break;
        case Type.DOUBLE:
            mv.visitInsn(DCONST_0);
            mv.visitInsn(DCMPL);
            break;
        }
    }

    /**
     * Returns true if the given class is a primitive wrapper, Date or String.
     */
    static boolean isSimpleRefType(String className) {
        return (PRIMITIVE_WRAPPERS.containsKey(className) ||
                className.equals(BigInteger.class.getName()) ||
                className.equals(Date.class.getName()) ||
                className.equals(String.class.getName()));
    }

    /**
     * Returns the wrapper class for a primitive.
     */
    static String getPrimitiveWrapperClass(int primitiveSort) {
        for (Map.Entry<String,Integer> entry : PRIMITIVE_WRAPPERS.entrySet()) {
            if (entry.getValue() == primitiveSort) {
                return entry.getKey();
            }
        }
        throw new IllegalStateException(String.valueOf(primitiveSort));
    }

    /**
     * Returns true if the given type is an object or array.
     */
    private static boolean isRefType(Type type) {
        int sort = type.getSort();
        return (sort == Type.OBJECT || sort == Type.ARRAY);
    }

    /**
     * Returns whether a string array contains a given string.
     */
    private static boolean containsString(String[] a, String s) {
        if (a != null) {
            for (String t : a) {
                if (s.equals(t)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Appends a string to a string array.
     */
    private static String[] appendString(String[] a, String s) {
        if (a != null) {
            int len = a.length;
            String[] a2 = new String[len + 1];
            System.arraycopy(a, 0, a2, 0, len);
            a2[len] = s;
            return a2;
        } else {
            return new String[] { s };
        }
    }

    /**
     * Aborts the enhancement process when we determine that enhancement is
     * unnecessary or not possible.
     */
    private NotPersistentException abort() {
        return NOT_PERSISTENT;
    }

    private static class FieldInfo implements FieldVisitor {

        FieldVisitor parent;
        String name;
        Type type;
        OrderInfo order;
        boolean isPriKey;
        boolean isSecKey;
        boolean isTransient;

        FieldInfo(FieldVisitor parent,
                  String name,
                  String desc,
                  boolean isTransient) {
            this.parent = parent;
            this.name = name;
            this.isTransient = isTransient;
            type = Type.getType(desc);
        }

        public AnnotationVisitor visitAnnotation(String desc,
                                                 boolean visible) {
            AnnotationVisitor ret = parent.visitAnnotation(desc, visible);
            if (desc.equals
                    ("Lcom/sleepycat/persist/model/KeyField;")) {
                order = new OrderInfo(ret);
                ret = order;
            } else if (desc.equals
                    ("Lcom/sleepycat/persist/model/PrimaryKey;")) {
                isPriKey = true;
            } else if (desc.equals
                    ("Lcom/sleepycat/persist/model/SecondaryKey;")) {
                isSecKey = true;
            } else if (desc.equals
                    ("Lcom/sleepycat/persist/model/NotPersistent;")) {
                isTransient = true;
            } else if (desc.equals
                    ("Lcom/sleepycat/persist/model/NotTransient;")) {
                isTransient = false;
            }
            return ret;
        }

        public void visitAttribute(Attribute attr) {
            parent.visitAttribute(attr);
        }

        public void visitEnd() {
            parent.visitEnd();
        }

        @Override
        public String toString() {
            String label;
            if (isPriKey) {
                label = "PrimaryKey";
            } else if (isSecKey) {
                label = "SecondaryKey";
            } else if (order != null) {
                label = "CompositeKeyField " + order.value;
            } else {
                label = "NonKeyField";
            }
            return "[" + label + ' ' + name + ' ' + type + ']';
        }
    }

    private static class OrderInfo extends AnnotationInfo {

        int value;

        OrderInfo(AnnotationVisitor parent) {
            super(parent);
        }

        @Override
        public void visit(String name, Object value) {
            if (name.equals("value")) {
                this.value = (Integer) value;
            }
            parent.visit(name, value);
        }
    }

    private static abstract class AnnotationInfo implements AnnotationVisitor {

        AnnotationVisitor parent;

        AnnotationInfo(AnnotationVisitor parent) {
            this.parent = parent;
        }

        public void visit(String name, Object value) {
            parent.visit(name, value);
        }

        public AnnotationVisitor visitAnnotation(String name, String desc) {
            return parent.visitAnnotation(name, desc);
        }

        public AnnotationVisitor visitArray(String name) {
            return parent.visitArray(name);
        }

        public void visitEnum(String name, String desc, String value) {
            parent.visitEnum(name, desc, value);
        }

        public void visitEnd() {
            parent.visitEnd();
        }
    }
}
