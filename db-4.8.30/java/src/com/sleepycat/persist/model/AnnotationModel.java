/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The default annotation-based entity model.  An <code>AnnotationModel</code>
 * is based on annotations that are specified for entity classes and their key
 * fields.
 *
 * <p>{@code AnnotationModel} objects are thread-safe.  Multiple threads may
 * safely call the methods of a shared {@code AnnotationModel} object.</p>
 *
 * <p>The set of persistent classes in the annotation model is the set of all
 * classes with the {@link Persistent} or {@link Entity} annotation.</p>
 *
 * <p>The annotations used to define persistent classes are: {@link Entity},
 * {@link Persistent}, {@link PrimaryKey}, {@link SecondaryKey} and {@link
 * KeyField}.  A good starting point is {@link Entity}.</p>
 *
 * @author Mark Hayes
 */
public class AnnotationModel extends EntityModel {

    private static class EntityInfo {
        PrimaryKeyMetadata priKey;
        Map<String,SecondaryKeyMetadata> secKeys =
            new HashMap<String,SecondaryKeyMetadata>();
    }

    private Map<String,ClassMetadata> classMap;
    private Map<String,EntityInfo> entityMap;

    /**
     * Constructs a model for annotated entity classes.
     */
    public AnnotationModel() {
        super();
        classMap = new HashMap<String,ClassMetadata>();
        entityMap = new HashMap<String,EntityInfo>();
    }

    /* EntityModel methods */

    @Override
    public synchronized Set<String> getKnownClasses() {
        return Collections.unmodifiableSet
            (new HashSet<String>(classMap.keySet()));
    }

    @Override
    public synchronized EntityMetadata getEntityMetadata(String className) {
        /* Call getClassMetadata to collect metadata. */
        getClassMetadata(className);
        /* Return the collected entity metadata. */
        EntityInfo info = entityMap.get(className);
        if (info != null) {
            return new EntityMetadata
                (className, info.priKey,
                 Collections.unmodifiableMap(info.secKeys));
        } else {
            return null;
        }
    }

    @Override
    public synchronized ClassMetadata getClassMetadata(String className) {
        ClassMetadata metadata = classMap.get(className);
        if (metadata == null) {
            Class<?> type;
            try {
                type = EntityModel.classForName(className);
            } catch (ClassNotFoundException e) {
                return null;
            }
            /* Get class annotation. */
            Entity entity = type.getAnnotation(Entity.class);
            Persistent persistent = type.getAnnotation(Persistent.class);
            if (entity == null && persistent == null) {
                return null;
            }
            if (type.isEnum() ||
                type.isInterface() ||
                type.isPrimitive()) {
                throw new IllegalArgumentException
                    ("@Entity and @Persistent not allowed for enum, " +
                     "interface, or primitive type: " + type.getName());
            }
            if (entity != null && persistent != null) {
                throw new IllegalArgumentException
                    ("Both @Entity and @Persistent are not allowed: " +
                     type.getName());
            }
            boolean isEntity;
            int version;
            String proxiedClassName;
            if (entity != null) {
                isEntity = true;
                version = entity.version();
                proxiedClassName = null;
            } else {
                isEntity = false;
                version = persistent.version();
                Class proxiedClass = persistent.proxyFor();
                proxiedClassName = (proxiedClass != void.class) ?
                                    proxiedClass.getName() : null;
            }
            /* Get instance fields. */
            List<Field> fields = new ArrayList<Field>();
            boolean nonDefaultRules = getInstanceFields(fields, type);
            Collection<FieldMetadata> nonDefaultFields = null;
            if (nonDefaultRules) {
                nonDefaultFields = new ArrayList<FieldMetadata>(fields.size());
                for (Field field : fields) {
                    nonDefaultFields.add(new FieldMetadata
                        (field.getName(), field.getType().getName(),
                         type.getName()));
                }
                nonDefaultFields =
                    Collections.unmodifiableCollection(nonDefaultFields);
            }
            /* Get the rest of the metadata and save it. */
            metadata = new ClassMetadata
                (className, version, proxiedClassName, isEntity,
                 getPrimaryKey(type, fields),
                 getSecondaryKeys(type, fields),
                 getCompositeKeyFields(type, fields),
                 nonDefaultFields);
            classMap.put(className, metadata);
            /* Add any new information about entities. */
            updateEntityInfo(metadata);
        }
        return metadata;
    }

    /**
     * Fills in the fields array and returns true if the default rules for
     * field persistence were overridden.
     */
    private boolean getInstanceFields(List<Field> fields, Class<?> type) {
        boolean nonDefaultRules = false;
        for (Field field : type.getDeclaredFields()) {
            boolean notPersistent =
                (field.getAnnotation(NotPersistent.class) != null);
            boolean notTransient = 
                (field.getAnnotation(NotTransient.class) != null);
            if (notPersistent && notTransient) {
                throw new IllegalArgumentException
                    ("Both @NotTransient and @NotPersistent not allowed");
            }
            if (notPersistent || notTransient) {
                nonDefaultRules = true;
            }
            int mods = field.getModifiers();

            if (!Modifier.isStatic(mods) &&
                !notPersistent &&
                (!Modifier.isTransient(mods) || notTransient)) {
                /* Field is DPL persistent. */
                fields.add(field);
            } else {
                /* If non-persistent, no other annotations should be used. */
                if (field.getAnnotation(PrimaryKey.class) != null ||
                    field.getAnnotation(SecondaryKey.class) != null ||
                    field.getAnnotation(KeyField.class) != null) {
                    throw new IllegalArgumentException
                        ("@PrimaryKey, @SecondaryKey and @KeyField not " +
                         "allowed on non-persistent field");
                }
            }
        }
        return nonDefaultRules;
    }

    private PrimaryKeyMetadata getPrimaryKey(Class<?> type,
                                             List<Field> fields) {
        Field foundField = null;
        String sequence = null;
        for (Field field : fields) {
            PrimaryKey priKey = field.getAnnotation(PrimaryKey.class);
            if (priKey != null) {
                if (foundField != null) {
                    throw new IllegalArgumentException
                        ("Only one @PrimaryKey allowed: " + type.getName());
                } else {
                    foundField = field;
                    sequence = priKey.sequence();
                    if (sequence.length() == 0) {
                        sequence = null;
                    }
                }
            }
        }
        if (foundField != null) {
            return new PrimaryKeyMetadata
                (foundField.getName(), foundField.getType().getName(),
                 type.getName(), sequence);
        } else {
            return null;
        }
    }

    private Map<String,SecondaryKeyMetadata> getSecondaryKeys(Class<?> type,
                                                         List<Field> fields) {
        Map<String,SecondaryKeyMetadata> map = null;
        for (Field field : fields) {
            SecondaryKey secKey = field.getAnnotation(SecondaryKey.class);
            if (secKey != null) {
                Relationship rel = secKey.relate();
                String elemClassName = null;
                if (rel == Relationship.ONE_TO_MANY ||
                    rel == Relationship.MANY_TO_MANY) {
                    elemClassName = getElementClass(field);
                }
                String keyName = secKey.name();
                if (keyName.length() == 0) {
                    keyName = field.getName();
                }
                Class<?> relatedClass = secKey.relatedEntity();
                String relatedEntity = (relatedClass != void.class) ?
                                        relatedClass.getName() : null;
                DeleteAction deleteAction = (relatedEntity != null) ?
                                        secKey.onRelatedEntityDelete() : null;
                SecondaryKeyMetadata metadata = new SecondaryKeyMetadata
                    (field.getName(), field.getType().getName(),
                     type.getName(), elemClassName, keyName, rel,
                     relatedEntity, deleteAction);
                if (map == null) {
                    map = new HashMap<String,SecondaryKeyMetadata>();
                }
                if (map.put(keyName, metadata) != null) {
                    throw new IllegalArgumentException
                        ("Only one @SecondaryKey with the same name allowed: "
                         + type.getName() + '.' + keyName);
                }
            }
        }
        if (map != null) {
            map = Collections.unmodifiableMap(map);
        }
        return map;
    }

    private String getElementClass(Field field) {
        Class cls = field.getType();
        if (cls.isArray()) {
            return cls.getComponentType().getName();
        }
        if (Collection.class.isAssignableFrom(cls)) {
            Type[] typeArgs = null;
            if (field.getGenericType() instanceof ParameterizedType) {
                typeArgs = ((ParameterizedType) field.getGenericType()).
                    getActualTypeArguments();
            }
            if (typeArgs == null ||
                typeArgs.length != 1 ||
                !(typeArgs[0] instanceof Class)) {
                throw new IllegalArgumentException
                    ("Collection typed secondary key field must have a" +
                     " single generic type argument and a wildcard or" +
                     " type bound is not allowed: " +
                     field.getDeclaringClass().getName() + '.' +
                     field.getName());
            }
            return ((Class) typeArgs[0]).getName();
        }
        throw new IllegalArgumentException
            ("ONE_TO_MANY or MANY_TO_MANY secondary key field must have" +
             " an array or Collection type: " +
             field.getDeclaringClass().getName() + '.' + field.getName());
    }

    private List<FieldMetadata> getCompositeKeyFields(Class<?> type,
                                                      List<Field> fields) {
        List<FieldMetadata> list = null;
        for (Field field : fields) {
            KeyField keyField = field.getAnnotation(KeyField.class);
            if (keyField != null) {
                int value = keyField.value();
                if (value < 1 || value > fields.size()) {
                    throw new IllegalArgumentException
                        ("Unreasonable @KeyField index value " + value +
                         ": " + type.getName());
                }
                if (list == null) {
                    list = new ArrayList<FieldMetadata>(fields.size());
                }
                if (value <= list.size() && list.get(value - 1) != null) {
                    throw new IllegalArgumentException
                        ("@KeyField index value " + value +
                         " is used more than once: " + type.getName());
                }
                while (value > list.size()) {
                    list.add(null);
                }
                FieldMetadata metadata = new FieldMetadata
                    (field.getName(), field.getType().getName(),
                     type.getName());
                list.set(value - 1, metadata);
            }
        }
        if (list != null) {
            if (list.size() < fields.size()) {
                throw new IllegalArgumentException
                    ("@KeyField is missing on one or more instance fields: " +
                     type.getName());
            }
            for (int i = 0; i < list.size(); i += 1) {
                if (list.get(i) == null) {
                    throw new IllegalArgumentException
                        ("@KeyField is missing for index value " + (i + 1) +
                         ": " + type.getName());
                }
            }
        }
        if (list != null) {
            list = Collections.unmodifiableList(list);
        }
        return list;
    }

    /**
     * Add newly discovered metadata to our stash of entity info.  This info
     * is maintained as it is discovered because it would be expensive to
     * create it on demand -- all class metadata would have to be traversed.
     */
    private void updateEntityInfo(ClassMetadata metadata) {

        /*
         * Find out whether this class or its superclass is an entity.  In the
         * process, traverse all superclasses to load their metadata -- this
         * will populate as much entity info as possible.
         */
        String entityClass = null;
        PrimaryKeyMetadata priKey = null;
        Map<String,SecondaryKeyMetadata> secKeys =
            new HashMap<String,SecondaryKeyMetadata>();
        for (ClassMetadata data = metadata; data != null;) {
            if (data.isEntityClass()) {
                if (entityClass != null) {
                    throw new IllegalArgumentException
                        ("An entity class may not be derived from another" +
                         " entity class: " + entityClass +
                         ' ' + data.getClassName());
                }
                entityClass = data.getClassName();
            }
            /* Save first primary key encountered. */
            if (priKey == null) {
                priKey = data.getPrimaryKey();
            }
            /* Save all secondary keys encountered by key name. */
            Map<String,SecondaryKeyMetadata> classSecKeys =
                data.getSecondaryKeys();
            if (classSecKeys != null) {
                for (SecondaryKeyMetadata secKey : classSecKeys.values()) {
                    secKeys.put(secKey.getKeyName(), secKey);
                }
            }
            /* Load superclass metadata. */
            Class cls;
            try {
                cls = EntityModel.classForName(data.getClassName());
            } catch (ClassNotFoundException e) {
                throw new IllegalStateException(e);
            }
            cls = cls.getSuperclass();
            if (cls != Object.class) {
                data = getClassMetadata(cls.getName());
                if (data == null) {
                    throw new IllegalArgumentException
                        ("Persistent class has non-persistent superclass: " +
                         cls.getName());
                }
            } else {
                data = null;
            }
        }

        /* Add primary and secondary key entity info. */
        if (entityClass != null) {
            EntityInfo info = entityMap.get(entityClass);
            if (info == null) {
                info = new EntityInfo();
                entityMap.put(entityClass, info);
            }
            if (priKey == null) {
                throw new IllegalArgumentException
                    ("Entity class has no primary key: " + entityClass);
            }
            info.priKey = priKey;
            info.secKeys.putAll(secKeys);
        }
    }
}
