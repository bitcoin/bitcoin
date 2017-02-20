/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.sleepycat.persist.evolve.Converter;
import com.sleepycat.persist.evolve.Deleter;
import com.sleepycat.persist.evolve.EntityConverter;
import com.sleepycat.persist.evolve.Mutations;
import com.sleepycat.persist.evolve.Renamer;
import com.sleepycat.persist.model.ClassMetadata;
import com.sleepycat.persist.model.DeleteAction;
import com.sleepycat.persist.model.EntityMetadata;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.model.FieldMetadata;
import com.sleepycat.persist.model.Relationship;
import com.sleepycat.persist.model.SecondaryKeyMetadata;
import com.sleepycat.persist.raw.RawField;
import com.sleepycat.persist.raw.RawObject;

/**
 * Format for persistent complex classes that are not composite key classes.
 * This includes entity classes and subclasses.
 *
 * @author Mark Hayes
 */
public class ComplexFormat extends Format {

    private static final long serialVersionUID = -2847843033590454917L;

    private ClassMetadata clsMeta;
    private EntityMetadata entityMeta;
    private FieldInfo priKeyField;
    private List<FieldInfo> secKeyFields;
    private List<FieldInfo> nonKeyFields;
    private FieldReader secKeyFieldReader;
    private FieldReader nonKeyFieldReader;
    private Map<String,String> oldToNewKeyMap;
    private Map<String,String> newToOldFieldMap;
    private boolean evolveNeeded;
    private transient Accessor objAccessor;
    private transient Accessor rawAccessor;
    private transient ComplexFormat entityFormat;
    private transient Map<String,FieldAddress> secKeyAddresses;
    private transient volatile Map<String,RawField> rawFields;
    private transient volatile FieldInfo[] rawInputFields;
    private transient volatile int[] rawInputLevels;
    private transient volatile int rawInputDepth;

    ComplexFormat(Class cls,
                  ClassMetadata clsMeta,
                  EntityMetadata entityMeta) {
        super(cls);
        this.clsMeta = clsMeta;
        this.entityMeta = entityMeta;
        secKeyFields = new ArrayList<FieldInfo>();
        nonKeyFields = FieldInfo.getInstanceFields(cls, clsMeta);

        /*
         * Validate primary key metadata and move primary key field from
         * nonKeyFields to priKeyField.
         */
        if (clsMeta.getPrimaryKey() != null) {
            String fieldName = clsMeta.getPrimaryKey().getName();
            FieldInfo field = FieldInfo.getField(nonKeyFields, fieldName);
            if (field == null) {
                throw new IllegalArgumentException
                    ("Primary key field does not exist: " +
                     getClassName() + '.' + fieldName);
            }
            nonKeyFields.remove(field);
            priKeyField = field;
        }

        /*
         * Validate secondary key metadata and move secondary key fields from
         * nonKeyFields to secKeyFields.
         */
        if (clsMeta.getSecondaryKeys() != null) {
            for (SecondaryKeyMetadata secKeyMeta :
                 clsMeta.getSecondaryKeys().values()) {
                String fieldName = secKeyMeta.getName();
                FieldInfo field = FieldInfo.getField(nonKeyFields, fieldName);
                if (field == null) {
                    throw new IllegalArgumentException
                        ("Secondary key field does not exist: " +
                         getClassName() + '.' + fieldName);
                }
                Class fieldCls = field.getFieldClass();
                Relationship rel = secKeyMeta.getRelationship();
                if (rel == Relationship.ONE_TO_MANY ||
                    rel == Relationship.MANY_TO_MANY) {
                    if (!PersistKeyCreator.isManyType(fieldCls)) {
                        throw new IllegalArgumentException
                            ("ONE_TO_MANY and MANY_TO_MANY keys must" +
                             " have an array or Collection type: " +
                             getClassName() + '.' + fieldName);
                    }
                } else {
                    if (PersistKeyCreator.isManyType(fieldCls)) {
                        throw new IllegalArgumentException
                            ("ONE_TO_ONE and MANY_TO_ONE keys must not" +
                             " have an array or Collection type: " +
                             getClassName() + '.' + fieldName);
                    }
                }
                if (fieldCls.isPrimitive() &&
                    secKeyMeta.getDeleteAction() == DeleteAction.NULLIFY) {
                    throw new IllegalArgumentException
                        ("NULLIFY may not be used with primitive fields: " +
                         getClassName() + '.' + fieldName);
                }
                nonKeyFields.remove(field);
                secKeyFields.add(field);
            }
        }

        /* Sort each group of fields by name. */
        Collections.sort(secKeyFields);
        Collections.sort(nonKeyFields);
    }

    @Override
    void migrateFromBeta(Map<String,Format> formatMap) {
        super.migrateFromBeta(formatMap);
        if (priKeyField != null) {
            priKeyField.migrateFromBeta(formatMap);
        }
        for (FieldInfo field : secKeyFields) {
            field.migrateFromBeta(formatMap);
        }
        for (FieldInfo field : nonKeyFields) {
            field.migrateFromBeta(formatMap);
        }
    }

    /**
     * Returns getSuperFormat cast to ComplexFormat.  It is guaranteed that all
     * super formats of a ComplexFormat are a ComplexFormat.
     */
    ComplexFormat getComplexSuper() {
        return (ComplexFormat) getSuperFormat();
    }

    /**
     * Returns getLatestVersion cast to ComplexFormat.  It is guaranteed that
     * all versions of a ComplexFormat are a ComplexFormat.
     */
    private ComplexFormat getComplexLatest() {
        return (ComplexFormat) getLatestVersion();
    }

    FieldInfo getPriKeyFieldInfo() {
        return priKeyField;
    }

    String getPriKeyField() {
        if (clsMeta.getPrimaryKey() != null) {
            return clsMeta.getPrimaryKey().getName();
        } else {
            return null;
        }
    }

    @Override
    boolean isEntity() {
        return clsMeta.isEntityClass();
    }

    @Override
    boolean isModelClass() {
        return true;
    }

    @Override
    public ClassMetadata getClassMetadata() {
        return clsMeta;
    }

    @Override
    public EntityMetadata getEntityMetadata() {
        return entityMeta;
    }

    @Override
    ComplexFormat getEntityFormat() {
        if (isInitialized()) {
            /* The transient entityFormat field is set by initialize(). */
            return entityFormat;
        } else {

            /*
             * If not initialized, the entity format can be found by traversing
             * the super formats.  However, this is only possible for an
             * existing format which has its superFormat field set.
             */
            if (isNew()) {
                throw new IllegalStateException(toString());
            }
            for (ComplexFormat format = this;
                 format != null;
                 format = format.getComplexSuper()) {
                if (format.isEntity()) {
                    return format;
                }
            }
            return null;
        }
    }

    @Override
    void setEvolveNeeded(boolean needed) {
        evolveNeeded = needed;
    }

    @Override
    boolean getEvolveNeeded() {
        return evolveNeeded;
    }

    @Override
    public Map<String,RawField> getFields() {

        /*
         * Synchronization is not required since rawFields is immutable.  If
         * by chance we create two maps when two threads execute this block, no
         * harm is done.  But be sure to assign the rawFields field only after
         * the map is fully populated.
         */
        if (rawFields == null) {
            Map<String,RawField> map = new HashMap<String,RawField>();
            if (priKeyField != null) {
                map.put(priKeyField.getName(), priKeyField);
            }
            for (RawField field : secKeyFields) {
                map.put(field.getName(), field);
            }
            for (RawField field : nonKeyFields) {
                map.put(field.getName(), field);
            }
            rawFields = map;
        }
        return rawFields;
    }

    @Override
    void collectRelatedFormats(Catalog catalog,
                               Map<String,Format> newFormats) {
        Class cls = getType();
        /* Collect field formats. */
        if (priKeyField != null) {
            priKeyField.collectRelatedFormats(catalog, newFormats);
        }
        for (FieldInfo field : secKeyFields) {
            field.collectRelatedFormats(catalog, newFormats);
        }
        for (FieldInfo field : nonKeyFields) {
            field.collectRelatedFormats(catalog, newFormats);
        }
        /* Collect TO_MANY secondary key field element class formats. */
        if (entityMeta != null) {
            for (SecondaryKeyMetadata secKeyMeta :
                 entityMeta.getSecondaryKeys().values()) {
                String elemClsName = secKeyMeta.getElementClassName();
                if (elemClsName != null) {
                    Class elemCls =
                        SimpleCatalog.keyClassForName(elemClsName);
                    catalog.createFormat(elemCls, newFormats);
                }
            }
        }
        /* Recursively collect superclass formats. */
        Class superCls = cls.getSuperclass();
        if (superCls != Object.class) {
            Format superFormat = catalog.createFormat(superCls, newFormats);
            if (!(superFormat instanceof ComplexFormat)) {
                throw new IllegalArgumentException
                    ("The superclass of a complex type must not be a" +
                     " composite key class or a simple type class: " +
                     superCls.getName());
            }
        }
        /* Collect proxied format. */
        String proxiedClsName = clsMeta.getProxiedClassName();
        if (proxiedClsName != null) {
            catalog.createFormat(proxiedClsName, newFormats);
        }
    }

    @Override
    void initialize(Catalog catalog, EntityModel model, int initVersion) {
        Class type = getType();
        boolean useEnhanced = false;
        if (type != null) {
            useEnhanced = EnhancedAccessor.isEnhanced(type);
        }
        /* Initialize all fields. */
        if (priKeyField != null) {
            priKeyField.initialize(catalog, model, initVersion);
        }
        for (FieldInfo field : secKeyFields) {
            field.initialize(catalog, model, initVersion);
        }
        for (FieldInfo field : nonKeyFields) {
            field.initialize(catalog, model, initVersion);
        }
        /* Set the superclass format for a new (never initialized) format. */
        ComplexFormat superFormat = getComplexSuper();
        if (type != null && superFormat == null) {
            Class superCls = type.getSuperclass();
            if (superCls != Object.class) {
                superFormat =
                    (ComplexFormat) catalog.getFormat(superCls.getName());
                setSuperFormat(superFormat);
            }
        }
        /* Initialize the superclass format and validate the super accessor. */
        if (superFormat != null) {
            superFormat.initializeIfNeeded(catalog, model);
            Accessor superAccessor = superFormat.objAccessor;
            if (type != null && superAccessor != null) {
                if (useEnhanced) {
                    if (!(superAccessor instanceof EnhancedAccessor)) {
                        throw new IllegalStateException
                            ("The superclass of an enhanced class must also " +
                             "be enhanced: " + getClassName() +
                             " extends " + superFormat.getClassName());
                    }
                } else {
                    if (!(superAccessor instanceof ReflectionAccessor)) {
                        throw new IllegalStateException
                            ("The superclass of an unenhanced class must " +
                             "not be enhanced: " + getClassName() +
                             " extends " + superFormat.getClassName());
                    }
                }
            }
        }
        /* Find entity format, if any. */
        for (ComplexFormat format = this;
             format != null;
             format = format.getComplexSuper()) {
            if (format.isEntity()) {
                entityFormat = format;
                break;
            }
        }

        /*
         * Ensure that the current entity metadata is always referenced in
         * order to return it to the user and to properly construct secondary
         * key addresses.  Secondary key metadata can change in an entity
         * subclass or be created when a new subclass is used, but this will
         * not cause evolution of the entity class; instead, the metadata is
         * updated here.  [#16467]
         */
        if (isEntity() && isCurrentVersion()) {
            entityMeta = model.getEntityMetadata(getClassName());
        }

        /* Disallow proxy class that extends an entity class. [#15950] */
        if (clsMeta.getProxiedClassName() != null && entityFormat != null) {
            throw new IllegalArgumentException
                ("A proxy may not be an entity: " + getClassName());
        }
        /* Disallow primary keys on entity subclasses.  [#15757] */
        if (entityFormat != null &&
            entityFormat != this && 
            priKeyField != null) {
            throw new IllegalArgumentException
                ("A PrimaryKey may not appear on an Entity subclass: " +
                 getClassName() + " field: " + priKeyField.getName());
        }
        /* Create the accessors. */
        if (type != null) {
            if (useEnhanced) {
                objAccessor = new EnhancedAccessor(catalog, type, this);
            } else {
                Accessor superObjAccessor =
                    (superFormat != null) ?  superFormat.objAccessor : null;
                objAccessor = new ReflectionAccessor
                    (catalog, type, superObjAccessor, priKeyField,
                     secKeyFields, nonKeyFields);
            }
        }
        Accessor superRawAccessor =
            (superFormat != null) ? superFormat.rawAccessor : null;
        rawAccessor = new RawAccessor
            (this, superRawAccessor, priKeyField, secKeyFields, nonKeyFields);

        /* Initialize secondary key field addresses. */
        EntityMetadata latestEntityMeta = null;
        if (entityFormat != null) {
            latestEntityMeta =
                entityFormat.getLatestVersion().getEntityMetadata();
        }
        if (latestEntityMeta != null) {
            secKeyAddresses = new HashMap<String,FieldAddress>();
            ComplexFormat thisLatest = getComplexLatest();
            if (thisLatest != this) {
                thisLatest.initializeIfNeeded(catalog, model);
            }
            nextKeyLoop:
            for (SecondaryKeyMetadata secKeyMeta :
                 latestEntityMeta.getSecondaryKeys().values()) {
                String clsName = secKeyMeta.getDeclaringClassName();
                String fieldName = secKeyMeta.getName();
                int superLevel = 0;
                for (ComplexFormat format = this;
                     format != null;
                     format = format.getComplexSuper()) {
                    if (clsName.equals
                            (format.getLatestVersion().getClassName())) {
                        String useFieldName = null;
                        if (format.newToOldFieldMap != null &&
                            format.newToOldFieldMap.containsKey(fieldName)) {
                            useFieldName =
                                format.newToOldFieldMap.get(fieldName);
                        } else {
                            useFieldName = fieldName;
                        }
                        boolean isSecField;
                        int fieldNum;
                        FieldInfo info = FieldInfo.getField
                            (format.secKeyFields, useFieldName);
                        if (info != null) {
                            isSecField = true;
                            fieldNum = format.secKeyFields.indexOf(info);
                        } else {
                            isSecField = false;
                            info = FieldInfo.getField
                                (format.nonKeyFields, useFieldName);
                            if (info == null) {
                                /* Field not present in old format. */
                                assert thisLatest != this;
                                thisLatest.checkNewSecKeyInitializer
                                    (secKeyMeta);
                                continue nextKeyLoop;
                            }
                            fieldNum = format.nonKeyFields.indexOf(info);
                        }
                        FieldAddress addr = new FieldAddress
                            (isSecField, fieldNum, superLevel, format,
                             info.getType());
                        secKeyAddresses.put(secKeyMeta.getKeyName(), addr);
                    }
                    superLevel += 1;
                }
            }
        }
    }

    /**
     * Checks that the type of a new secondary key is not a primitive and that
     * the default contructor does not initialize it to a non-null value.
     */
    private void checkNewSecKeyInitializer(SecondaryKeyMetadata secKeyMeta) {
        if (objAccessor != null) {
            FieldAddress addr = secKeyAddresses.get(secKeyMeta.getKeyName());
            Object obj = objAccessor.newInstance();
            Object val = objAccessor.getField
                (obj, addr.fieldNum, addr.superLevel, addr.isSecField);
            if (val != null) {
                if (addr.keyFormat.isPrimitive()) {
                    throw new IllegalArgumentException
                        ("For a new secondary key field the field type must " +
                         "not be a primitive -- class: " +
                         secKeyMeta.getDeclaringClassName() + " field: " +
                         secKeyMeta.getName());
                } else {
                    throw new IllegalArgumentException
                        ("For a new secondary key field the default " +
                         "constructor must not initialize the field to a " +
                         "non-null value -- class: " +
                         secKeyMeta.getDeclaringClassName() + " field: " +
                         secKeyMeta.getName());
                }
            }
        }
    }

    private boolean nullOrEqual(Object o1, Object o2) {
        if (o1 == null) {
            return o2 == null;
        } else {
            return o1.equals(o2);
        }
    }

    @Override
    Object newArray(int len) {
        return objAccessor.newArray(len);
    }

    @Override
    public Object newInstance(EntityInput input, boolean rawAccess) {
        Accessor accessor = rawAccess ? rawAccessor : objAccessor;
        return accessor.newInstance();
    }

    @Override
    public Object readObject(Object o, EntityInput input, boolean rawAccess) {
        Accessor accessor = rawAccess ? rawAccessor : objAccessor;
        accessor.readSecKeyFields(o, input, 0, Accessor.MAX_FIELD_NUM, -1);
        accessor.readNonKeyFields(o, input, 0, Accessor.MAX_FIELD_NUM, -1);
        return o;
    }

    @Override
    void writeObject(Object o, EntityOutput output, boolean rawAccess) {
        Accessor accessor = rawAccess ? rawAccessor : objAccessor;
        accessor.writeSecKeyFields(o, output);
        accessor.writeNonKeyFields(o, output);
    }

    @Override
    Object convertRawObject(Catalog catalog,
                            boolean rawAccess,
                            RawObject rawObject,
                            IdentityHashMap converted) {
        /*
         * Synchronization is not required since rawInputFields, rawInputLevels
         * and rawInputDepth are immutable.  If by chance we create duplicate
         * values when two threads execute this block, no harm is done.  But be
         * sure to assign the fields only after the values are fully populated.
         */
        FieldInfo[] fields = rawInputFields;
        int[] levels = rawInputLevels;
        int depth = rawInputDepth;
        if (fields == null || levels == null || depth == 0) {

            /*
             * The volatile fields are not yet set.  Prepare to process the
             * class hierarchy, storing class formats in order from the highest
             * superclass down to the current class.
             */
            depth = 0;
            int nFields = 0;
            for (ComplexFormat format = this;
                 format != null;
                 format = format.getComplexSuper()) {
                nFields += format.getNFields();
                depth += 1;
            }
            ComplexFormat[] hierarchy = new ComplexFormat[depth];
            int level = depth;
            for (ComplexFormat format = this;
                 format != null;
                 format = format.getComplexSuper()) {
                level -= 1;
                hierarchy[level] = format;
            }
            assert level == 0;

            /* Populate levels and fields in parallel. */
            levels = new int[nFields];
            fields = new FieldInfo[nFields];
            int index = 0;

            /*
             * The primary key is the first field read/written.  We use the
             * first primary key field encountered going from this class upward
             * in the class hierarchy.
             */
            if (getEntityFormat() != null) {
                for (level = depth - 1; level >= 0; level -= 1) {
                    ComplexFormat format = hierarchy[level];
                    if (format.priKeyField != null) {
                        levels[index] = level;
                        fields[index] = format.priKeyField;
                        index += 1;
                        break;
                    }
                }
                assert index == 1;
            }

            /*
             * Secondary key fields are read/written next, from the highest
             * base class downward.
             */
            for (level = 0; level < depth; level += 1) {
                ComplexFormat format = hierarchy[level];
                for (FieldInfo field : format.secKeyFields) {
                    levels[index] = level;
                    fields[index] = field;
                    index += 1;
                }
            }

            /*
             * Other fields are read/written last, from the highest base class
             * downward.
             */
            for (level = 0; level < depth; level += 1) {
                ComplexFormat format = hierarchy[level];
                for (FieldInfo field : format.nonKeyFields) {
                    levels[index] = level;
                    fields[index] = field;
                    index += 1;
                }
            }

            /* We're finished -- update the volatile fields for next time. */
            assert index == fields.length;
            rawInputFields = fields;
            rawInputLevels = levels;
            rawInputDepth = depth;
        }

        /*
         * Create an objects array that is parallel to the fields and levels
         * arrays, but contains the RawObject for each slot from which the
         * field value can be retrieved.  The predetermined level for each
         * field determines which RawObject in the instance hierarchy to use.
         */
        RawObject[] objectsByLevel = new RawObject[depth];
        int level = depth;
        for (RawObject raw = rawObject; raw != null; raw = raw.getSuper()) {
            if (level == 0) {
                throw new IllegalArgumentException
                    ("RawObject has too many superclasses: " +
                     rawObject.getType().getClassName());
            }
            level -= 1;
            objectsByLevel[level] = raw;
        }
        if (level > 0) {
            throw new IllegalArgumentException
                ("RawObject has too few superclasses: " +
                 rawObject.getType().getClassName());
        }
        assert level == 0;
        RawObject[] objects = new RawObject[fields.length];
        for (int i = 0; i < objects.length; i += 1) {
            objects[i] = objectsByLevel[levels[i]];
        }

        /* Create the persistent object and convert all RawObject fields. */
        EntityInput in = new RawComplexInput
            (catalog, rawAccess, converted, fields, objects);
        Object o = newInstance(in, rawAccess);
        converted.put(rawObject, o);
        if (getEntityFormat() != null) {
            readPriKey(o, in, rawAccess);
        }
        return readObject(o, in, rawAccess);
    }

    @Override
    boolean isPriKeyNullOrZero(Object o, boolean rawAccess) {
        Accessor accessor = rawAccess ? rawAccessor : objAccessor;
        return accessor.isPriKeyFieldNullOrZero(o);
    }

    @Override
    void writePriKey(Object o, EntityOutput output, boolean rawAccess) {
        Accessor accessor = rawAccess ? rawAccessor : objAccessor;
        accessor.writePriKeyField(o, output);
    }

    @Override
    public void readPriKey(Object o, EntityInput input, boolean rawAccess) {
        Accessor accessor = rawAccess ? rawAccessor : objAccessor;
        accessor.readPriKeyField(o, input);
    }

    @Override
    boolean nullifySecKey(Catalog catalog,
                          Object entity,
                          String keyName,
                          Object keyElement) {
        if (secKeyAddresses == null) {
            throw new IllegalStateException();
        }
        FieldAddress addr = secKeyAddresses.get(keyName);
        if (addr != null) {
            Object oldVal = rawAccessor.getField
                (entity, addr.fieldNum, addr.superLevel, addr.isSecField);
            if (oldVal != null) {
                if (keyElement != null) {
                    RawObject container = (RawObject) oldVal;
                    Object[] a1 = container.getElements();
                    boolean isArray = (a1 != null);
                    if (!isArray) {
                        a1 = CollectionProxy.getElements(container);
                    }
                    if (a1 != null) {
                        for (int i = 0; i < a1.length; i += 1) {
                            if (keyElement.equals(a1[i])) {
                                int len = a1.length - 1;
                                Object[] a2 = new Object[len];
                                System.arraycopy(a1, 0, a2, 0, i);
                                System.arraycopy(a1, i + 1, a2, i, len - i);
                                if (isArray) {
                                    rawAccessor.setField
                                        (entity, addr.fieldNum,
                                         addr.superLevel, addr.isSecField,
                                         new RawObject
                                            (container.getType(), a2));
                                } else {
                                    CollectionProxy.setElements(container, a2);
                                }
                                return true;
                            }
                        }
                    }
                    return false;
                } else {
                    rawAccessor.setField
                        (entity, addr.fieldNum, addr.superLevel,
                         addr.isSecField, null);
                    return true;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    @Override
    void skipContents(RecordInput input) {
        skipToSecKeyField(input, Accessor.MAX_FIELD_NUM);
        skipToNonKeyField(input, Accessor.MAX_FIELD_NUM);
    }

    @Override
    void copySecMultiKey(RecordInput input, Format keyFormat, Set results) {
        CollectionProxy.copyElements(input, this, keyFormat, results);
    }

    @Override
    Format skipToSecKey(RecordInput input, String keyName) {
        if (secKeyAddresses == null) {
            throw new IllegalStateException();
        }
        FieldAddress addr = secKeyAddresses.get(keyName);
        if (addr != null) {
            if (addr.isSecField) {
                addr.clsFormat.skipToSecKeyField(input, addr.fieldNum);
            } else {
                skipToSecKeyField(input, Accessor.MAX_FIELD_NUM);
                addr.clsFormat.skipToNonKeyField(input, addr.fieldNum);
            }
            return addr.keyFormat;
        } else {
            return null;
        }
    }

    private int getNFields() {
        return ((priKeyField != null) ? 1 : 0) +
               secKeyFields.size() +
               nonKeyFields.size();
    }

    private void skipToSecKeyField(RecordInput input, int toFieldNum) {
        ComplexFormat superFormat = getComplexSuper();
        if (superFormat != null) {
            superFormat.skipToSecKeyField(input, Accessor.MAX_FIELD_NUM);
        }
        int maxNum = Math.min(secKeyFields.size(), toFieldNum);
        for (int i = 0; i < maxNum; i += 1) {
            input.skipField(secKeyFields.get(i).getType());
        }
    }

    private void skipToNonKeyField(RecordInput input, int toFieldNum) {
        ComplexFormat superFormat = getComplexSuper();
        if (superFormat != null) {
            superFormat.skipToNonKeyField(input, Accessor.MAX_FIELD_NUM);
        }
        int maxNum = Math.min(nonKeyFields.size(), toFieldNum);
        for (int i = 0; i < maxNum; i += 1) {
            input.skipField(nonKeyFields.get(i).getType());
        }
    }

    private static class FieldAddress {

        boolean isSecField;
        int fieldNum;
        int superLevel;
        ComplexFormat clsFormat;
        Format keyFormat;

        FieldAddress(boolean isSecField,
                     int fieldNum,
                     int superLevel,
                     ComplexFormat clsFormat,
                     Format keyFormat) {
            this.isSecField = isSecField;
            this.fieldNum = fieldNum;
            this.superLevel = superLevel;
            this.clsFormat = clsFormat;
            this.keyFormat = keyFormat;
        }
    }

    @Override
    boolean evolve(Format newFormatParam, Evolver evolver) {

        /* Disallow evolution to a non-complex format. */
        if (!(newFormatParam instanceof ComplexFormat)) {
            evolver.addMissingMutation
                (this, newFormatParam,
                 "Converter is required when a complex type is changed " +
                 "to a simple type or enum type");
            return false;
        }
        ComplexFormat newFormat = (ComplexFormat) newFormatParam;
        Mutations mutations = evolver.getMutations();
        boolean thisChanged = false;
        boolean hierarchyChanged = false;
        Map<String,String> allKeyNameMap = new HashMap<String,String>();

        /* The Evolver has already ensured that entities evolve to entities. */
        assert isEntity() == newFormat.isEntity();
        assert isEntity() == (entityMeta != null);
        assert newFormat.isEntity() == (newFormat.entityMeta != null);

        /*
         * Keep track of the old and new entity class names for use in deleting
         * and renaming secondary keys below.  If the oldEntityClass is
         * non-null this also signifies an entity class or subclass.  Note that
         * getEntityFormat cannot be called on a newly created format during
         * evolution because its super format property is not yet initialized.
         * [#16253]
         */
        String oldEntityClass;
        String newEntityClass;
        if (isEntity()) {
            oldEntityClass = getClassName();
            newEntityClass = newFormat.getClassName();
        } else {
            oldEntityClass = null;
            newEntityClass = null;
        }

        /*
         * Evolve all superclass formats, even when a deleted class appears in
         * the hierarchy.  This ensures that the super format's
         * getLatestVersion/getComplexLatest method can be used accurately
         * below.
         */
        for (ComplexFormat oldSuper = getComplexSuper();
             oldSuper != null;
             oldSuper = oldSuper.getComplexSuper()) {
            Converter converter = mutations.getConverter
                (oldSuper.getClassName(), oldSuper.getVersion(), null);
            if (converter != null) {
                evolver.addMissingMutation
                    (this, newFormatParam,
                     "Converter is required for this subclass when a " +
                     "Converter appears on its superclass: " + converter);
                return false;
            }
            if (!evolver.evolveFormat(oldSuper)) {
                return false;
            }
            if (!oldSuper.isCurrentVersion()) {
                if (oldSuper.isDeleted()) {
                    if (!oldSuper.evolveDeletedClass(evolver)) {
                        return false;
                    }
                }
                if (oldSuper.oldToNewKeyMap != null) {
                    allKeyNameMap.putAll(oldSuper.oldToNewKeyMap);
                }
                hierarchyChanged = true;
            }
        }

        /*
         * Compare the old and new class hierarhies and decide whether each
         * change is allowed or not:
         * + Old deleted and removed superclass -- allowed
         * + Old empty and removed superclass -- allowed
         * + Old non-empty and removed superclass -- not allowed
         * + Old superclass repositioned in the hierarchy -- not allowed
         * + New inserted superclass -- allowed
         */
        Class newFormatCls = newFormat.getExistingType();
        Class newSuper = newFormatCls;
        List<Integer> newLevels = new ArrayList<Integer>();
        int newLevel = 0;
        newLevels.add(newLevel);

        /*
         * When this format has a new superclass, we treat it as a change to
         * this format as well as to the superclass hierarchy.
         */
        if (getSuperFormat() == null) {
            if (newFormatCls.getSuperclass() != Object.class) {
                thisChanged = true;
                hierarchyChanged = true;
            }
        } else {
            if (!getSuperFormat().getLatestVersion().getClassName().equals
                    (newFormatCls.getSuperclass().getName())) {
                thisChanged = true;
                hierarchyChanged = true;
            }
        }

        for (ComplexFormat oldSuper = getComplexSuper();
             oldSuper != null;
             oldSuper = oldSuper.getComplexSuper()) {

            /* Find the matching superclass in the new hierarchy. */
            String oldSuperName = oldSuper.getLatestVersion().getClassName();
            Class foundNewSuper = null;
            int tryNewLevel = newLevel;
            for (Class newSuper2 = newSuper.getSuperclass();
                 newSuper2 != Object.class;
                 newSuper2 = newSuper2.getSuperclass()) {
                tryNewLevel += 1;
                if (oldSuperName.equals(newSuper2.getName())) {
                    foundNewSuper = newSuper2;
                    newLevel = tryNewLevel;
                    if (oldSuper.isEntity()) {
                        assert oldEntityClass == null;
                        assert newEntityClass == null;
                        oldEntityClass = oldSuper.getClassName();
                        newEntityClass = foundNewSuper.getName();
                    }
                    break;
                }
            }

            if (foundNewSuper != null) {

                /*
                 * We found the old superclass in the new hierarchy.  Traverse
                 * through the superclass formats that were skipped over above
                 * when finding it.
                 */
                for (Class newSuper2 = newSuper.getSuperclass();
                     newSuper2 != foundNewSuper;
                     newSuper2 = newSuper2.getSuperclass()) {

                    /*
                     * The class hierarchy changed -- a new class was inserted.
                     */
                    hierarchyChanged = true;

                    /*
                     * Check that the new formats skipped over above are not at
                     * a different position in the old hierarchy.
                     */
                    for (ComplexFormat oldSuper2 = oldSuper.getComplexSuper();
                         oldSuper2 != null;
                         oldSuper2 = oldSuper2.getComplexSuper()) {
                        String oldSuper2Name =
                            oldSuper2.getLatestVersion().getClassName();
                        if (oldSuper2Name.equals(newSuper2.getName())) {
                            evolver.addMissingMutation
                                (this, newFormatParam,
                                 "Class Converter is required when a " +
                                 "superclass is moved in the class " +
                                 "hierarchy: " + newSuper2.getName());
                            return false;
                        }
                    }
                }
                newSuper = foundNewSuper;
                newLevels.add(newLevel);
            } else {

                /*
                 * We did not find the old superclass in the new hierarchy.
                 * The class hierarchy changed, since an old class no longer
                 * appears.
                 */
                hierarchyChanged = true;

                /* Check that the old class can be safely removed. */
                if (!oldSuper.isDeleted()) {
                    ComplexFormat oldSuperLatest =
                        oldSuper.getComplexLatest();
                    if (oldSuperLatest.getNFields() != 0) {
                        evolver.addMissingMutation
                            (this, newFormatParam,
                             "When a superclass is removed from the class " +
                             "hierarchy, the superclass or all of its " +
                             "persistent fields must be deleted with a " +
                             "Deleter: " +
                             oldSuperLatest.getClassName());
                        return false;
                    }
                }

                if (oldEntityClass != null && isCurrentVersion()) {
                    Map<String,SecondaryKeyMetadata> secKeys =
                        oldSuper.clsMeta.getSecondaryKeys();
                    for (FieldInfo field : oldSuper.secKeyFields) {
                        SecondaryKeyMetadata meta =
                            getSecondaryKeyMetadataByFieldName
                                (secKeys, field.getName());
                        assert meta != null;
                        allKeyNameMap.put(meta.getKeyName(), null);
                    }
                }

                /*
                 * Add the DO_NOT_READ_ACCESSOR level to prevent an empty class
                 * (no persistent fields) from being read via the Accessor.
                 */
                newLevels.add(EvolveReader.DO_NOT_READ_ACCESSOR);
            }
        }

        /* Make FieldReaders for this format if needed. */
        int result = evolveAllFields(newFormat, evolver);
        if (result == Evolver.EVOLVE_FAILURE) {
            return false;
        }
        if (result == Evolver.EVOLVE_NEEDED) {
            thisChanged = true;
        }
        if (oldToNewKeyMap != null) {
            allKeyNameMap.putAll(oldToNewKeyMap);
        }

        /* Require new version number if this class was changed. */
        if (thisChanged &&
            !evolver.checkUpdatedVersion
                ("Changes to the fields or superclass were detected", this,
                 newFormat)) {
            return false;
        }

        /* Rename and delete the secondary databases. */
        if (allKeyNameMap.size() > 0 &&
            oldEntityClass != null &&
            newEntityClass != null &&
            isCurrentVersion()) {
            for (Map.Entry<String,String> entry : allKeyNameMap.entrySet()) {
                String oldKeyName = entry.getKey();
                String newKeyName = entry.getValue();
                if (newKeyName != null) {
                    evolver.renameSecondaryDatabase
                        (oldEntityClass, newEntityClass,
                         oldKeyName, newKeyName);
                } else {
                    evolver.deleteSecondaryDatabase
                        (oldEntityClass, oldKeyName);
                }
            }
        }

        /* Use an EvolveReader if needed. */
        if (hierarchyChanged || thisChanged) {
            Reader reader = new EvolveReader(newLevels);
            evolver.useEvolvedFormat(this, reader, newFormat);
        } else {
            evolver.useOldFormat(this, newFormat);
        }
        return true;
    }

    @Override
    boolean evolveMetadata(Format newFormatParam,
                           Converter converter,
                           Evolver evolver) {
        assert !isDeleted();
        assert isEntity();
        assert newFormatParam.isEntity();
        ComplexFormat newFormat = (ComplexFormat) newFormatParam;

        if (!checkKeyTypeChange
                (newFormat, entityMeta.getPrimaryKey(),
                 newFormat.entityMeta.getPrimaryKey(), "primary key",
                 evolver)) {
            return false;
        }

        Set<String> deletedKeys;
        if (converter instanceof EntityConverter) {
            EntityConverter entityConverter = (EntityConverter) converter;
            deletedKeys = entityConverter.getDeletedKeys();
        } else {
            deletedKeys = Collections.emptySet();
        }

        Map<String,SecondaryKeyMetadata> oldSecondaryKeys =
            entityMeta.getSecondaryKeys();
        Map<String,SecondaryKeyMetadata> newSecondaryKeys =
            newFormat.entityMeta.getSecondaryKeys();
        Set<String> insertedKeys =
            new HashSet<String>(newSecondaryKeys.keySet());

        for (SecondaryKeyMetadata oldMeta : oldSecondaryKeys.values()) {
            String keyName = oldMeta.getKeyName();
            if (deletedKeys.contains(keyName)) {
                if (isCurrentVersion()) {
                    evolver.deleteSecondaryDatabase(getClassName(), keyName);
                }
            } else {
                SecondaryKeyMetadata newMeta = newSecondaryKeys.get(keyName);
                if (newMeta == null) {
                    evolver.addInvalidMutation
                        (this, newFormat, converter,
                         "Existing key not found in new entity metadata: " +
                          keyName);
                    return false;
                }
                insertedKeys.remove(keyName);
                String keyLabel = "secondary key: " + keyName;
                if (!checkKeyTypeChange
                        (newFormat, oldMeta, newMeta, keyLabel, evolver)) {
                    return false;
                }
                if (!checkSecKeyMetadata
                        (newFormat, oldMeta, newMeta, evolver)) {
                    return false;
                }
            }
        }

        if (!insertedKeys.isEmpty()) {
            evolver.addEvolveError
                (this, newFormat, "Error",
                 "New keys " + insertedKeys +
                 " not allowed when using a Converter with an entity class");
        }

        return true;
    }

    /**
     * Checks that changes to secondary key metadata are legal.
     */
    private boolean checkSecKeyMetadata(Format newFormat,
                                        SecondaryKeyMetadata oldMeta,
                                        SecondaryKeyMetadata newMeta,
                                        Evolver evolver) {
        if (oldMeta.getRelationship() != newMeta.getRelationship()) {
            evolver.addEvolveError
                (this, newFormat,
                 "Change detected in the relate attribute (Relationship) " +
                 "of a secondary key",
                 "Old key: " + oldMeta.getKeyName() +
                 " relate: " + oldMeta.getRelationship() +
                 " new key: " + newMeta.getKeyName() +
                 " relate: " + newMeta.getRelationship());
            return false;
        }
        return true;
    }

    /**
     * Checks that the type of a key field did not change, as known from
     * metadata when a class conversion is used.
     */
    private boolean checkKeyTypeChange(Format newFormat,
                                       FieldMetadata oldMeta,
                                       FieldMetadata newMeta,
                                       String keyLabel,
                                       Evolver evolver) {
        String oldClass = oldMeta.getClassName();
        String newClass = newMeta.getClassName();
        if (!oldClass.equals(newClass)) {
            SimpleCatalog catalog = SimpleCatalog.getInstance();
            Format oldType = catalog.getFormat(oldClass);
            Format newType = catalog.getFormat(newClass);
            if (oldType == null || newType == null ||
                ((oldType.getWrapperFormat() == null ||
                  oldType.getWrapperFormat().getId() !=
                  newType.getId()) &&
                 (newType.getWrapperFormat() == null ||
                  newType.getWrapperFormat().getId() !=
                  oldType.getId()))) {
                evolver.addEvolveError
                    (this, newFormat,
                     "Type change detected for " + keyLabel,
                     "Old field type: " + oldClass +
                     " is not compatible with the new type: " +
                     newClass +
                     " old field: " + oldMeta.getName() +
                     " new field: " + newMeta.getName());
                return false;
            }
        }
        return true;
    }

    /**
     * Special case for creating FieldReaders for a deleted class when it
     * appears in the class hierarchy of its non-deleted subclass.
     */
    private boolean evolveDeletedClass(Evolver evolver) {
        assert isDeleted();
        if (secKeyFieldReader == null || nonKeyFieldReader == null) {
            if (priKeyField != null &&
                getEntityFormat() != null &&
                !getEntityFormat().isDeleted()) {
                evolver.addEvolveError
                    (this, this,
                     "Class containing primary key field was deleted ",
                     "Primary key is needed in an entity class hierarchy: " +
                     priKeyField.getName());
                return false;
            } else {
                secKeyFieldReader = new SkipFieldReader(0, secKeyFields);
                nonKeyFieldReader = new SkipFieldReader(0, nonKeyFields);
                return true;
            }
        } else {
            return true;
        }
    }

    /**
     * Creates a FieldReader for secondary key fields and non-key fields if
     * necessary.  Checks the primary key field if necessary.  Does not evolve
     * superclass format fields.
     */
    private int evolveAllFields(ComplexFormat newFormat, Evolver evolver) {

        assert !isDeleted();
        secKeyFieldReader = null;
        nonKeyFieldReader = null;
        oldToNewKeyMap = null;

        /* Evolve primary key field. */
        boolean evolveFailure = false;
        boolean localEvolveNeeded = false;
        if (priKeyField != null) {
            int result = evolver.evolveRequiredKeyField
                (this, newFormat, priKeyField, newFormat.priKeyField);
            if (result == Evolver.EVOLVE_FAILURE) {
                evolveFailure = true;
            } else if (result == Evolver.EVOLVE_NEEDED) {
                localEvolveNeeded = true;
            }
        }

        /* Evolve secondary key fields. */
        FieldReader reader = evolveFieldList
            (secKeyFields, newFormat.secKeyFields, true,
             newFormat.nonKeyFields, newFormat, evolver);
        if (reader == FieldReader.EVOLVE_FAILURE) {
            evolveFailure = true;
        } else if (reader != null) {
            localEvolveNeeded = true;
        }
        if (reader != FieldReader.EVOLVE_NEEDED) {
            secKeyFieldReader = reader;
        }

        /* Evolve non-key fields. */
        reader = evolveFieldList
            (nonKeyFields, newFormat.nonKeyFields, false,
             newFormat.secKeyFields, newFormat, evolver);
        if (reader == FieldReader.EVOLVE_FAILURE) {
            evolveFailure = true;
        } else if (reader != null) {
            localEvolveNeeded = true;
        }
        if (reader != FieldReader.EVOLVE_NEEDED) {
            nonKeyFieldReader = reader;
        }

        /* Return result. */
        if (evolveFailure) {
            return Evolver.EVOLVE_FAILURE;
        } else if (localEvolveNeeded) {
            return Evolver.EVOLVE_NEEDED;
        } else {
            return Evolver.EVOLVE_NONE;
        }
    }

    /**
     * Returns a FieldReader that reads no fields.
     *
     * Instead of adding a DoNothingFieldReader class, we use a
     * MultiFieldReader with an empty field list.  We do not add a new
     * FieldReader class to avoid changing the catalog format.  [#15524]
     */
    private FieldReader getDoNothingFieldReader() {
        List<FieldReader> emptyList = Collections.emptyList();
        return new MultiFieldReader(emptyList);
    }

    /**
     * Evolves a list of fields, either secondary key or non-key fields, for a
     * single class format.
     *
     * @return a FieldReader if field evolution is needed, null if no evolution
     * is needed, or FieldReader.EVOLVE_FAILURE if an evolution error occurs.
     */
    private FieldReader evolveFieldList(List<FieldInfo> oldFields,
                                        List<FieldInfo> newFields,
                                        boolean isOldSecKeyField,
                                        List<FieldInfo> otherNewFields,
                                        ComplexFormat newFormat,
                                        Evolver evolver) {
        Mutations mutations = evolver.getMutations();
        boolean evolveFailure = false;
        boolean localEvolveNeeded = false;
        boolean readerNeeded = false;
        List<FieldReader> fieldReaders = new ArrayList<FieldReader>();
        FieldReader currentReader = null;
        int newFieldsMatched = 0;

        /*
         * Add FieldReaders to the list in old field storage order, since that
         * is the order in which field values must be read.
         */
        fieldLoop:
        for (int oldFieldIndex = 0;
             oldFieldIndex < oldFields.size();
             oldFieldIndex += 1) {

            FieldInfo oldField = oldFields.get(oldFieldIndex);
            String oldName = oldField.getName();
            SecondaryKeyMetadata oldMeta = null;
            if (isOldSecKeyField) {
                oldMeta = getSecondaryKeyMetadataByFieldName
                    (clsMeta.getSecondaryKeys(), oldName);
                assert oldMeta != null;
            }

            /* Get field mutations. */
            Renamer renamer = mutations.getRenamer
                (getClassName(), getVersion(), oldName);
            Deleter deleter = mutations.getDeleter
                (getClassName(), getVersion(), oldName);
            Converter converter = mutations.getConverter
                (getClassName(), getVersion(), oldName);
            if (deleter != null && (converter != null || renamer != null)) {
                evolver.addInvalidMutation
                    (this, newFormat, deleter,
                     "Field Deleter is not allowed along with a Renamer or " +
                     "Converter for the same field: " + oldName);
                evolveFailure = true;
                continue fieldLoop;
            }

            /*
             * Match old and new field by name, taking into account the Renamer
             * mutation.  If the @SecondaryKey annotation was added or removed,
             * the field will have moved from one of the two field lists to the
             * other.
             */
            String newName = (renamer != null) ?
                renamer.getNewName() : oldName;
            if (!oldName.equals(newName)) {
                if (newToOldFieldMap == null) {
                    newToOldFieldMap = new HashMap<String,String>();
                }
                newToOldFieldMap.put(newName, oldName);
            }
            int newFieldIndex = FieldInfo.getFieldIndex(newFields, newName);
            FieldInfo newField = null;
            boolean isNewSecKeyField = isOldSecKeyField;
            if (newFieldIndex >= 0) {
                newField = newFields.get(newFieldIndex);
            } else {
                newFieldIndex = FieldInfo.getFieldIndex
                    (otherNewFields, newName);
                if (newFieldIndex >= 0) {
                    newField = otherNewFields.get(newFieldIndex);
                    isNewSecKeyField = !isOldSecKeyField;
                }
                localEvolveNeeded = true;
                readerNeeded = true;
            }

            /* Apply field Deleter and continue. */
            if (deleter != null) {
                if (newField != null) {
                    evolver.addInvalidMutation
                        (this, newFormat, deleter,
                         "Field Deleter is not allowed when the persistent " +
                         "field is still present: " + oldName);
                    evolveFailure = true;
                }
                /* A SkipFieldReader can read multiple sequential fields. */
                if (currentReader instanceof SkipFieldReader &&
                    currentReader.acceptField
                        (oldFieldIndex, newFieldIndex, isNewSecKeyField)) {
                    currentReader.addField(oldField);
                } else {
                    currentReader = new SkipFieldReader
                        (oldFieldIndex, oldField);
                    fieldReaders.add(currentReader);
                    readerNeeded = true;
                    localEvolveNeeded = true;
                }
                if (isOldSecKeyField) {
                    if (oldToNewKeyMap == null) {
                        oldToNewKeyMap = new HashMap<String,String>();
                    }
                    oldToNewKeyMap.put(oldMeta.getKeyName(), null);
                }
                continue fieldLoop;
            } else {
                if (newField == null) {
                    evolver.addMissingMutation
                        (this, newFormat,
                         "Field is not present or not persistent: " +
                         oldName);
                    evolveFailure = true;
                    continue fieldLoop;
                }
            }

            /*
             * The old field corresponds to a known new field, and no Deleter
             * mutation applies.
             */
            newFieldsMatched += 1;

            /* Get and process secondary key metadata changes. */
            SecondaryKeyMetadata newMeta = null;
            if (isOldSecKeyField && isNewSecKeyField) {
                newMeta = getSecondaryKeyMetadataByFieldName
                    (newFormat.clsMeta.getSecondaryKeys(), newName);
                assert newMeta != null;

                /* Validate metadata changes. */
                if (!checkSecKeyMetadata
                        (newFormat, oldMeta, newMeta, evolver)) {
                    evolveFailure = true;
                    continue fieldLoop;
                }

                /*
                 * Check for a renamed key and save the old-to-new mapping for
                 * use in renaming the secondary database and for key
                 * extraction.
                 */
                String oldKeyName = oldMeta.getKeyName();
                String newKeyName = newMeta.getKeyName();
                if (!oldKeyName.equals(newKeyName)) {
                    if (oldToNewKeyMap == null) {
                        oldToNewKeyMap = new HashMap<String,String>();
                    }
                    oldToNewKeyMap.put(oldName, newName);
                    localEvolveNeeded = true;
                }
            } else if (isOldSecKeyField && !isNewSecKeyField) {
                if (oldToNewKeyMap == null) {
                    oldToNewKeyMap = new HashMap<String,String>();
                }
                oldToNewKeyMap.put(oldMeta.getKeyName(), null);
            }

            /* Apply field Converter and continue. */
            if (converter != null) {
                if (isOldSecKeyField) {
                    evolver.addInvalidMutation
                        (this, newFormat, converter,
                         "Field Converter is not allowed for secondary key " +
                         "fields: " + oldName);
                    evolveFailure = true;
                } else {
                    currentReader = new ConvertFieldReader
                        (converter, oldFieldIndex, newFieldIndex,
                         isNewSecKeyField);
                    fieldReaders.add(currentReader);
                    readerNeeded = true;
                    localEvolveNeeded = true;
                }
                continue fieldLoop;
            }

            /*
             * Evolve the declared version of the field format and all versions
             * more recent, and the formats for all of their subclasses.  While
             * we're at it, check to see if all possible classes are converted.
             */
            boolean allClassesConverted = true;
            Format oldFieldFormat = oldField.getType();
            for (Format formatVersion = oldFieldFormat.getLatestVersion();
                 true;
                 formatVersion = formatVersion.getPreviousVersion()) {
                assert formatVersion != null;
                if (!evolver.evolveFormat(formatVersion)) {
                    evolveFailure = true;
                    continue fieldLoop;
                }
                if (!formatVersion.isNew() &&
                    !evolver.isClassConverted(formatVersion)) {
                    allClassesConverted = false;
                }
                Set<Format> subclassFormats =
                    evolver.getSubclassFormats(formatVersion);
                if (subclassFormats != null) {
                    for (Format format2 : subclassFormats) {
                        if (!evolver.evolveFormat(format2)) {
                            evolveFailure = true;
                            continue fieldLoop;
                        }
                        if (!format2.isNew() &&
                            !evolver.isClassConverted(format2)) {
                            allClassesConverted = false;
                        }
                    }
                }
                if (formatVersion == oldFieldFormat) {
                    break;
                }
            }

            /*
             * Check for compatible field types and apply a field widener if
             * needed.  If no widener is needed, fall through and apply a
             * PlainFieldReader.
             */
            Format oldLatestFormat = oldFieldFormat.getLatestVersion();
            Format newFieldFormat = newField.getType();
            if (oldLatestFormat.getClassName().equals
                    (newFieldFormat.getClassName()) &&
                !oldLatestFormat.isDeleted()) {
                /* Formats are identical.  Fall through. */
            } else if (allClassesConverted) {
                /* All old classes will be converted.  Fall through. */
                localEvolveNeeded = true;
            } else if (WidenerInput.isWideningSupported
                        (oldLatestFormat, newFieldFormat, isOldSecKeyField)) {
                /* Apply field widener and continue. */
                currentReader = new WidenFieldReader
                    (oldLatestFormat, newFieldFormat, newFieldIndex,
                     isNewSecKeyField);
                fieldReaders.add(currentReader);
                readerNeeded = true;
                localEvolveNeeded = true;
                continue fieldLoop;
            } else {
                boolean refWidened = false;
                if (!newFieldFormat.isPrimitive() &&
                    !oldLatestFormat.isPrimitive() &&
                    !oldLatestFormat.isDeleted() &&
                    !evolver.isClassConverted(oldLatestFormat)) {
                    Class oldCls = oldLatestFormat.getExistingType();
                    Class newCls = newFieldFormat.getExistingType();
                    if (newCls.isAssignableFrom(oldCls)) {
                        refWidened = true;
                    }
                }
                if (refWidened) {
                    /* A reference type has been widened.  Fall through. */
                    localEvolveNeeded = true;
                } else {
                    /* Types are not compatible. */
                    evolver.addMissingMutation
                        (this, newFormat,
                         "Old field type: " + oldLatestFormat.getClassName() +
                         " is not compatible with the new type: " +
                         newFieldFormat.getClassName() +
                         " for field: " + oldName);
                    evolveFailure = true;
                    continue fieldLoop;
                }
            }

            /*
             * Old to new field conversion is not needed or is automatic.  Read
             * fields as if no evolution is needed.  A PlainFieldReader can
             * read multiple sequential fields.
             */
            if (currentReader instanceof PlainFieldReader &&
                currentReader.acceptField
                    (oldFieldIndex, newFieldIndex, isNewSecKeyField)) {
                currentReader.addField(oldField);
            } else {
                currentReader = new PlainFieldReader
                    (oldFieldIndex, newFieldIndex, isNewSecKeyField);
                fieldReaders.add(currentReader);
            }
        }

        /*
         * If there are new fields, then the old fields must be read using a
         * reader, even if the old field list is empty.  Using the accessor
         * directly will read fields in the wrong order and will read fields
         * that were moved between lists (when adding and dropping
         * @SecondaryKey).  [#15524]
         */
        if (newFieldsMatched < newFields.size()) {
            localEvolveNeeded = true;
            readerNeeded = true;
        }

        if (evolveFailure) {
            return FieldReader.EVOLVE_FAILURE;
        } else if (readerNeeded) {
            if (fieldReaders.size() == 0) {
                return getDoNothingFieldReader();
            } else if (fieldReaders.size() == 1) {
                return fieldReaders.get(0);
            } else {
                return new MultiFieldReader(fieldReaders);
            }
        } else if (localEvolveNeeded) {
            return FieldReader.EVOLVE_NEEDED;
        } else {
            return null;
        }
    }

    /**
     * Base class for all FieldReader subclasses.  A FieldReader reads one or
     * more fields in the old format data, and may call the new format Accessor
     * to set the field values.
     */
    private static abstract class FieldReader implements Serializable {

        static final FieldReader EVOLVE_NEEDED =
            new PlainFieldReader(0, 0, false);
        static final FieldReader EVOLVE_FAILURE =
            new PlainFieldReader(0, 0, false);

        private static final long serialVersionUID = 866041475399255164L;

        FieldReader() {
        }

        void initialize(Catalog catalog,
                        int initVersion,
                        ComplexFormat oldParentFormat,
                        ComplexFormat newParentFormat,
                        boolean isOldSecKey) {
        }

        boolean acceptField(int oldFieldIndex,
                            int newFieldIndex,
                            boolean isNewSecKeyField) {
            return false;
        }

        void addField(FieldInfo oldField) {
            throw new UnsupportedOperationException();
        }

        abstract void readFields(Object o,
                                 EntityInput input,
                                 Accessor accessor,
                                 int superLevel);
    }

    /**
     * Reads a continguous block of fields that have the same format in the old
     * and new formats.
     */
    private static class PlainFieldReader extends FieldReader {

        private static final long serialVersionUID = 1795593463439931402L;

        private int startField;
        private int endField;
        private boolean secKeyField;
        private transient int endOldField;

        PlainFieldReader(int oldFieldIndex,
                         int newFieldIndex,
                         boolean isNewSecKeyField) {
            endOldField = oldFieldIndex;
            startField = newFieldIndex;
            endField = newFieldIndex;
            secKeyField = isNewSecKeyField;
        }

        @Override
        boolean acceptField(int oldFieldIndex,
                            int newFieldIndex,
                            boolean isNewSecKeyField) {
            return oldFieldIndex == endOldField + 1 &&
                   newFieldIndex == endField + 1 &&
                   secKeyField == isNewSecKeyField;
        }

        @Override
        void addField(FieldInfo oldField) {
            endField += 1;
            endOldField += 1;
        }

        @Override
        final void readFields(Object o,
                              EntityInput input,
                              Accessor accessor,
                              int superLevel) {
            if (secKeyField) {
                accessor.readSecKeyFields
                    (o, input, startField, endField, superLevel);
            } else {
                accessor.readNonKeyFields
                    (o, input, startField, endField, superLevel);
            }
        }
    }

    /**
     * Skips a continguous block of fields that exist in the old format but not
     * in the new format.
     */
    private static class SkipFieldReader extends FieldReader {

        private static final long serialVersionUID = -3060281692155253098L;

        private List<Format> fieldFormats;
        private transient int endField;

        SkipFieldReader(int startField, List<FieldInfo> fields) {
            endField = startField + fields.size() - 1;
            fieldFormats = new ArrayList<Format>(fields.size());
            for (FieldInfo field : fields) {
                fieldFormats.add(field.getType());
            }
        }

        SkipFieldReader(int startField, FieldInfo oldField) {
            endField = startField;
            fieldFormats = new ArrayList<Format>();
            fieldFormats.add(oldField.getType());
        }

        @Override
        boolean acceptField(int oldFieldIndex,
                            int newFieldIndex,
                            boolean isNewSecKeyField) {
            return oldFieldIndex == endField + 1;
        }

        @Override
        void addField(FieldInfo oldField) {
            endField += 1;
            fieldFormats.add(oldField.getType());
        }

        @Override
        final void readFields(Object o,
                              EntityInput input,
                              Accessor accessor,
                              int superLevel) {
            for (Format format : fieldFormats) {
                input.skipField(format);
            }
        }
    }

    /**
     * Converts a single field using a field Converter.
     */
    private static class ConvertFieldReader extends FieldReader {

        private static final long serialVersionUID = 8736410481633998710L;

        private Converter converter;
        private int oldFieldNum;
        private int fieldNum;
        private boolean secKeyField;
        private transient Format oldFormat;
        private transient Format newFormat;

        ConvertFieldReader(Converter converter,
                           int oldFieldIndex,
                           int newFieldIndex,
                           boolean isNewSecKeyField) {
            this.converter = converter;
            oldFieldNum = oldFieldIndex;
            fieldNum = newFieldIndex;
            secKeyField = isNewSecKeyField;
        }

        @Override
        void initialize(Catalog catalog,
                        int initVersion,
                        ComplexFormat oldParentFormat,
                        ComplexFormat newParentFormat,
                        boolean isOldSecKey) {
            
            /*
             * The oldFieldNum field was added as part of a bug fix.  If not
             * present in this version of the catalog, we assume it is equal to
             * the new field index.  The code prior to the bug fix assumes the
             * old and new fields have the same index. [#15797]
             */
            if (initVersion < 1) {
                oldFieldNum = fieldNum;
            }

            if (isOldSecKey) {
                oldFormat =
                    oldParentFormat.secKeyFields.get(oldFieldNum).getType();
            } else {
                oldFormat =
                    oldParentFormat.nonKeyFields.get(oldFieldNum).getType();
            }
            if (secKeyField) {
                newFormat =
                    newParentFormat.secKeyFields.get(fieldNum).getType();
            } else {
                newFormat =
                    newParentFormat.nonKeyFields.get(fieldNum).getType();
            }
        }

        @Override
        final void readFields(Object o,
                              EntityInput input,
                              Accessor accessor,
                              int superLevel) {

            /* Create and read the old format instance in raw mode. */
            boolean currentRawMode = input.setRawAccess(true);
            Object value;
            try {
                if (oldFormat.isPrimitive()) {
                    value = input.readKeyObject(oldFormat);
                } else {
                    value = input.readObject();
                }
            } finally {
                input.setRawAccess(currentRawMode);
            }

            /* Convert the raw instance to the current format. */
            Catalog catalog = input.getCatalog();
            value = converter.getConversion().convert(value);

            /* Use a RawSingleInput to convert and type-check the value. */
            EntityInput rawInput = new RawSingleInput
                (catalog, currentRawMode, null, value, newFormat);

            if (secKeyField) {
                accessor.readSecKeyFields
                    (o, rawInput, fieldNum, fieldNum, superLevel);
            } else {
                accessor.readNonKeyFields
                    (o, rawInput, fieldNum, fieldNum, superLevel);
            }
        }
    }

    /**
     * Widens a single field using a field Converter.
     */
    private static class WidenFieldReader extends FieldReader {

        private static final long serialVersionUID = -2054520670170407282L;

        private int fromFormatId;
        private int toFormatId;
        private int fieldNum;
        private boolean secKeyField;

        WidenFieldReader(Format oldFormat,
                         Format newFormat,
                         int newFieldIndex,
                         boolean isNewSecKeyField) {
            fromFormatId = oldFormat.getId();
            toFormatId = newFormat.getId();
            fieldNum = newFieldIndex;
            secKeyField = isNewSecKeyField;
        }

        @Override
        final void readFields(Object o,
                              EntityInput input,
                              Accessor accessor,
                              int superLevel) {

            /* The Accessor reads the field value from a WidenerInput. */
            EntityInput widenerInput = new WidenerInput
                (input, fromFormatId, toFormatId);

            if (secKeyField) {
                accessor.readSecKeyFields
                    (o, widenerInput, fieldNum, fieldNum, superLevel);
            } else {
                accessor.readNonKeyFields
                    (o, widenerInput, fieldNum, fieldNum, superLevel);
            }
        }
    }

    /**
     * A FieldReader composed of other FieldReaders, and that calls them in
     * sequence.  Used when more than one FieldReader is needed for a list of
     * fields.
     */
    private static class MultiFieldReader extends FieldReader {

        private static final long serialVersionUID = -6035976787562441473L;

        private List<FieldReader> subReaders;

        MultiFieldReader(List<FieldReader> subReaders) {
            this.subReaders = subReaders;
        }

        @Override
        void initialize(Catalog catalog,
                        int initVersion,
                        ComplexFormat oldParentFormat,
                        ComplexFormat newParentFormat,
                        boolean isOldSecKey) {
            for (FieldReader reader : subReaders) {
                reader.initialize
                    (catalog, initVersion, oldParentFormat, newParentFormat,
                     isOldSecKey);
            }
        }

        @Override
        final void readFields(Object o,
                              EntityInput input,
                              Accessor accessor,
                              int superLevel) {
            for (FieldReader reader : subReaders) {
                reader.readFields(o, input, accessor, superLevel);
            }
        }
    }

    /**
     * The Reader for evolving ComplexFormat instances.  Reads the old format
     * data one class (one level in the class hierarchy) at a time.  If an
     * Accessor is used at a given level, the Accessor is used for the
     * corresponding level in the new class hierarchy (classes may be
     * inserted/deleted during evolution).  At each level, a FieldReader is
     * called to evolve the secondary key and non-key lists of fields.
     */
    private static class EvolveReader implements Reader {

        static final int DO_NOT_READ_ACCESSOR = Integer.MAX_VALUE;

        private static final long serialVersionUID = -1016140948306913283L;

        private transient ComplexFormat newFormat;

        /**
         * oldHierarchy contains the formats of the old class hierarchy in most
         * to least derived class order.
         */
        private transient ComplexFormat[] oldHierarchy;

        /**
         * newHierarchyLevels contains the corresponding level in the new
         * hierarchy for each format in oldHierarchy. newHierarchyLevels is
         * indexed by the oldHierarchy index.
         */
        private int[] newHierarchyLevels;

        EvolveReader(List<Integer> newHierarchyLevelsList) {
            int oldDepth = newHierarchyLevelsList.size();
            newHierarchyLevels = new int[oldDepth];
            newHierarchyLevelsList.toArray();
            for (int i = 0; i < oldDepth; i += 1) {
                newHierarchyLevels[i] = newHierarchyLevelsList.get(i);
            }
        }

        public void initializeReader(Catalog catalog,
                                     EntityModel model,
                                     int initVersion,
                                     Format oldFormatParam) {

            ComplexFormat oldFormat = (ComplexFormat) oldFormatParam;
            newFormat = oldFormat.getComplexLatest();
            newFormat.initializeIfNeeded(catalog, model);

            /* Create newHierarchy array. */
            int newDepth = 0;
            for (Format format = newFormat;
                 format != null;
                 format = format.getSuperFormat()) {
                newDepth += 1;
            }
            ComplexFormat[] newHierarchy = new ComplexFormat[newDepth];
            int level = 0;
            for (ComplexFormat format = newFormat;
                 format != null;
                 format = format.getComplexSuper()) {
                newHierarchy[level] = format;
                level += 1;
            }
            assert level == newDepth;

            /* Create oldHierarchy array and initialize FieldReaders. */
            int oldDepth = newHierarchyLevels.length;
            oldHierarchy = new ComplexFormat[oldDepth];
            level = 0;
            for (ComplexFormat oldFormat2 = oldFormat;
                 oldFormat2 != null;
                 oldFormat2 = oldFormat2.getComplexSuper()) {
                oldHierarchy[level] = oldFormat2;
                int level2 = newHierarchyLevels[level];
                ComplexFormat newFormat2 = (level2 != DO_NOT_READ_ACCESSOR) ?
                    newHierarchy[level2] : null;
                level += 1;
                if (oldFormat2.secKeyFieldReader != null) {
                    oldFormat2.secKeyFieldReader.initialize
                        (catalog, initVersion, oldFormat2, newFormat2, true);
                }
                if (oldFormat2.nonKeyFieldReader != null) {
                    oldFormat2.nonKeyFieldReader.initialize
                        (catalog, initVersion, oldFormat2, newFormat2, false);
                }
            }
            assert level == oldDepth;
        }

        public Object newInstance(EntityInput input, boolean rawAccess) {
            return newFormat.newInstance(input, rawAccess);
        }

        public void readPriKey(Object o,
                               EntityInput input,
                               boolean rawAccess) {
            /* No conversion necessary for primary keys. */
            newFormat.readPriKey(o, input, rawAccess);
        }

        public Object readObject(Object o,
                                 EntityInput input,
                                 boolean rawAccess) {

            /* Use the Accessor for the new format. */
            Accessor accessor = rawAccess ? newFormat.rawAccessor
                                          : newFormat.objAccessor;

            /* Read old format fields from the top-most class downward. */
            int maxMinusOne = oldHierarchy.length - 1;

            /* Read secondary key fields with the adjusted superclass level. */
            for (int i = maxMinusOne; i >= 0; i -= 1) {
                FieldReader reader = oldHierarchy[i].secKeyFieldReader;
                int newLevel = newHierarchyLevels[i];
                if (reader != null) {
                    reader.readFields(o, input, accessor, newLevel);
                } else if (newLevel != DO_NOT_READ_ACCESSOR) {
                    accessor.readSecKeyFields
                        (o, input, 0, Accessor.MAX_FIELD_NUM, newLevel);
                }
            }

            /* Read non-key fields with the adjusted superclass level. */
            for (int i = maxMinusOne; i >= 0; i -= 1) {
                FieldReader reader = oldHierarchy[i].nonKeyFieldReader;
                int newLevel = newHierarchyLevels[i];
                if (reader != null) {
                    reader.readFields(o, input, accessor, newLevel);
                } else if (newLevel != DO_NOT_READ_ACCESSOR) {
                    accessor.readNonKeyFields
                        (o, input, 0, Accessor.MAX_FIELD_NUM, newLevel);
                }
            }
            return o;
        }
    }

    /**
     * The secondary key metadata map (ClassMetadata.getSecondaryKeys) is keyed
     * by key name, not field name.  Key name can be different than field name
     * when a @SecondaryKey name property is specified.  To look up metadata
     * by field name, we must do a linear search.  Luckily, the number of keys
     * per class is never very large. [#16819]
     */
    static SecondaryKeyMetadata
        getSecondaryKeyMetadataByFieldName(Map<String, SecondaryKeyMetadata>
                                             secKeys,
                                           String fieldName) {
        for (SecondaryKeyMetadata meta : secKeys.values()) {
            if (meta.getName().equals(fieldName)) {
                return meta;
            }
        }
        return null;
    }
}
