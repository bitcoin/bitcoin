/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.io.Serializable;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;

/**
 * The btree comparator for persistent key classes.  The serialized form of
 * this comparator is stored in the BDB JE database descriptor so that the
 * comparator can be re-created during recovery.
 *
 * @author Mark Hayes
 */
public class PersistComparator implements Comparator<byte[]>, Serializable {

    private static final long serialVersionUID = 5221576538843355317L;

    private String keyClassName;
    private String[] comositeFieldOrder;
    private Map<String, String[]> fieldFormatData;
    private transient PersistKeyBinding binding;

    public PersistComparator(PersistKeyBinding binding) {
        this.binding = binding;
        /* Save info necessary to recreate binding during deserialization. */
        final CompositeKeyFormat format =
            (CompositeKeyFormat) binding.keyFormat;
        keyClassName = format.getClassName();
        comositeFieldOrder = CompositeKeyFormat.getFieldNameArray
            (format.getClassMetadata().getCompositeKeyFields());
        /* Currently only enum formats have per-class data. */
        for (FieldInfo field : format.getFieldInfo()) {
            Format fieldFormat = field.getType();
            if (fieldFormat.isEnum()) {
                EnumFormat enumFormat = (EnumFormat) fieldFormat;
                if (fieldFormatData == null) {
                    fieldFormatData = new HashMap<String, String[]>();
                }
                fieldFormatData.put(enumFormat.getClassName(),
                                    enumFormat.getFormatData());
            }
        }
    }

    public int compare(byte[] b1, byte[] b2) {

        /*
         * The binding will be null after the comparator is deserialized, i.e.,
         * during BDB JE recovery.  We must construct it here, without access
         * to the stored catalog since recovery is not complete.
         */
        if (binding == null) {
            Catalog catalog = SimpleCatalog.getInstance();
            Map<String, Format> enumFormats = null;
            if (fieldFormatData != null) {
                enumFormats = new HashMap<String, Format>();
                for (Map.Entry<String, String[]> entry :
                     fieldFormatData.entrySet()) {
                    final String fldClassName = entry.getKey();
                    final String[] enumNames = entry.getValue();
                    final Class fldClass;
                    try {
                        fldClass = SimpleCatalog.classForName(fldClassName);
                    } catch (ClassNotFoundException e) {
                        throw new IllegalStateException(e);
                    }
                    enumFormats.put(fldClassName,
                                    new EnumFormat(fldClass, enumNames));
                }
                catalog = new ComparatorCatalog(enumFormats);
                for (Format fldFormat : enumFormats.values()) {
                    fldFormat.initializeIfNeeded(catalog, null /*model*/);
                }
            }
            final Class keyClass;
            try {
                keyClass = SimpleCatalog.classForName(keyClassName);
            } catch (ClassNotFoundException e) {
                throw new IllegalStateException(e);
            }
            binding = new PersistKeyBinding(catalog, keyClass,
                                            comositeFieldOrder);
        }

        Comparable k1 = (Comparable) binding.bytesToObject(b1, 0, b1.length);
        Comparable k2 = (Comparable) binding.bytesToObject(b2, 0, b2.length);

        return k1.compareTo(k2);
    }

    @Override
    public String toString() {
        StringBuilder b = new StringBuilder();
        b.append("[DPL comparator ");
        b.append(" keyClassName = ").append(keyClassName);
        b.append(" comositeFieldOrder = [");
        for (String s : comositeFieldOrder) {
            b.append(s).append(',');
        }
        b.append(']');
        b.append(" fieldFormatData = {");
        for (Map.Entry<String, String[]> entry : fieldFormatData.entrySet()) {
            b.append(entry.getKey()).append(": [");
            for (String s : entry.getValue()) {
                b.append(s).append(',');
            }
            b.append(']');
        }
        b.append('}');
        b.append(']');
        return b.toString();
    }
}
