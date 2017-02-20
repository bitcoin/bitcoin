/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.io.Serializable;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.sleepycat.persist.evolve.Converter;
import com.sleepycat.persist.model.ClassMetadata;
import com.sleepycat.persist.model.EntityMetadata;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.model.FieldMetadata;
import com.sleepycat.persist.model.PrimaryKeyMetadata;
import com.sleepycat.persist.model.SecondaryKeyMetadata;
import com.sleepycat.persist.raw.RawField;
import com.sleepycat.persist.raw.RawObject;
import com.sleepycat.persist.raw.RawType;

/**
 * The base class for all object formats.  Formats are used to define the
 * stored layout for all persistent classes, including simple types.
 *
 * The design documentation below describes the storage format for entities and
 * its relationship to information stored per format in the catalog.
 *
 * Requirements
 * ------------
 * + Provides EntityBinding for objects and EntryBinding for keys.
 * + Provides SecondaryKeyCreator, SecondaryMultiKeyCreator and
 *   SecondaryMultiKeyNullifier (SecondaryKeyNullifier is redundant).
 * + Works with reflection and bytecode enhancement.
 * + For reflection only, works with any entity model not just annotations.
 * + Bindings are usable independently of the persist API.
 * + Performance is almost equivalent to hand coded tuple bindings.
 * + Small performance penalty for compatible class changes (new fields,
 *   widening).
 * + Secondary key create/nullify do not have to deserialize the entire record;
 *   in other words, store secondary keys at the start of the data.
 *
 * Class Format
 * ------------
 * Every distinct class format is given a unique format ID.  Class IDs are not
 * equivalent to class version numbers (as in the version property of @Entity
 * and @Persistent) because the format can change when the version number does
 * not.  Changes that cause a unique format ID to be assigned are:
 *
 * + Add field.
 * + Widen field type.
 * + Change primitive type to primitive wrapper class.
 * + Add or drop secondary key.
 * + Any incompatible class change.
 *
 * The last item, incompatible class changes, also correspond to a class
 * version change.
 *
 * For each distinct class format the following information is conceptually
 * stored in the catalog, keyed by format ID.
 *
 * - Class name
 * - Class version number
 * - Superclass format
 * - Kind: simple, enum, complex, array
 * - For kind == simple:
 *     - Primitive class
 * - For kind == enum:
 *     - Array of constant names, sorted by name.
 * - For kind == complex:
 *     - Primary key fieldInfo, or null if no primary key is declared
 *     - Array of secondary key fieldInfo, sorted by field name
 *     - Array of other fieldInfo, sorted by field name
 * - For kind == array:
 *     - Component class format
 *     - Number of array dimensions
 * - Other metadata for RawType
 *
 * Where fieldInfo is:
 *     - Field name
 *     - Field class
 *     - Other metadata for RawField
 *
 * Data Layout
 * -----------
 * For each entity instance the data layout is as follows:
 *
 *   instanceData: formatId keyFields... nonKeyFields...
 *   keyFields:    fieldValue...
 *   nonKeyFields: fieldValue...
 *
 * The formatId is the (positive non-zero) ID of a class format, defined above.
 * This is ID of the most derived class of the instance.  It is stored as a
 * packed integer.
 *
 * Following the format ID, zero or more sets of secondary key field values
 * appear, followed by zero or more sets of other class field values.
 *
 * The keyFields are the sets of secondary key fields for each class in order
 * of the highest superclass first.  Within a class, fields are ordered by
 * field name.
 *
 * The nonKeyFields are the sets of other non-key fields for each class in
 * order of the highest superclass first.  Within a class, fields are ordered
 * by field name.
 *
 * A field value is:
 *
 *   fieldValue:   primitiveValue
 *               | nullId
 *               | instanceRef
 *               | instanceData
 *               | simpleValue
 *               | enumValue
 *               | arrayValue
 *
 * For a primitive type, a primitive value is used as defined for tuple
 * bindings.  For float and double, sorted float and sorted double tuple values
 * are used.
 *
 * For a non-primitive type with a null value, a nullId is used that has a zero
 * (illegal formatId) value.  This includes String and other simple reference
 * types.  The formatId is stored as a packed integer, meaning that it is
 * stored as a single zero byte.
 *
 * For a non-primitive type, an instanceRef is used for a non-null instance
 * that appears earlier in the data byte array.  An instanceRef is the negation
 * of the byte offset of the instanceData that appears earlier.  It is stored
 * as a packed integer.
 *
 * The remaining rules apply only to reference types with non-null values that
 * do not appear earlier in the data array.
 *
 * For an array type, an array formatId is used that identifies the component
 * type and the number of array dimensions.  This is followed by an array
 * length (stored as a packed integer) and zero or more fieldValue elements.
 * For an array with N+1 dimensions where N is greater than zero, the leftmost
 * dimension is enumerated such that each fieldValue element is itself an array
 * of N dimensions or null.
 *
 *   arrayValue:  formatId length fieldValue...
 *
 * For an enum type, an enumValue is used, consisting of a formatId that
 * identifies the enum class and an enumIndex (stored as a packed integer) that
 * identifies the constant name in the enum constant array of the enum class
 * format:
 *
 *   enumValue:   formatId enumIndex
 *
 * For a simple type, a simpleValue is used.  This consists of the formatId
 * that identifies the class followed by the simple type value.  For a
 * primitive wrapper type the simple type value is the corresponding primitive,
 * for a Date it is the milliseconds as a long primitive, and for BigInteger or
 * BigDecimal it is a byte array as defined for tuple bindings of these types.
 *
 *   simpleValue: formatId value
 *
 * For all other complex types, an instanceData is used, which is defined
 * above.
 *
 * Secondary Keys
 * --------------
 * For secondary key support we must account for writing and nullifying
 * specific keys.  Rather than instantiating the entity and then performing
 * the secondary key operation, we strive to perform the secondary key
 * operation directly on the byte format.
 *
 * To create a secondary key we skip over other fields and then copy the bytes
 * of the embedded key.  This approach is very efficient because a) the entity
 * is not instantiated, and b) the secondary keys are stored at the beginning
 * of the byte format and can be quickly read.
 *
 * To nullify we currently instantiate the raw entity, set the key field to null
 * (or remove it from the array/collection), and convert the raw entity back to
 * bytes.  Although the performance of this approach is not ideal because it
 * requires serialization, it avoids the complexity of modifying the packed
 * serialized format directly, adjusting references to key objects, etc.  Plus,
 * when we nullify a key we are going to write the record, so the serialization
 * overhead may not be significant.  For the record, I tried implementing
 * nullification of the bytes directly and found it was much too complex.
 *
 * Lifecycle
 * ---------
 * Format are managed by a Catalog class.  Simple formats are managed by
 * SimpleCatalog, and are copied from the SimpleCatalog by PersistCatalog.
 * Other formats are managed by PersistCatalog.  The lifecycle of a format
 * instance is:
 *
 * - Constructed by the catalog when a format is requested for a Class
 *   that currently has no associated format.
 *
 * - The catalog calls setId() and adds the format to its format list
 *   (indexed by format id) and map (keyed by class name).
 *
 * - The catalog calls collectRelatedFormats(), where a format can create
 *   additional formats that it needs, or that should also be persistent.
 *
 * - The catalog calls initializeIfNeeded(), which calls the initialize()
 *   method of the format class.
 *
 * - initialize() should initialize any transient fields in the format.
 *   initialize() can assume that all related formats are available in the
 *   catalog.  It may call initializeIfNeeded() for those related formats, if
 *   it needs to interact with an initialized related format; this does not
 *   cause a cycle, because initializeIfNeeded() does nothing for an already
 *   initialized format.
 *
 * - The catalog creates a group of related formats at one time, and then
 *   writes its entire list of formats to the catalog DB as a single record.
 *   This grouping reduces the number of writes.
 *
 * - When a catalog is opened and the list of existing formats is read.  After
 *   a format is deserialized, its initializeIfNeeded() method is called.
 *   setId() and collectRelatedFormats() are not called, since the ID and
 *   related formats are stored in serialized fields.
 *
 * - There are two modes for opening an existing catalog: raw mode and normal
 *   mode.  In raw mode, the old format is used regardless of whether it
 *   matches the current class definition; in fact the class is not accessed
 *   and does not need to be present.
 *
 * - In normal mode, for each existing format that is initialized, a new format
 *   is also created based on the current class and metadata definition.  If
 *   the two formats are equal, the new format is discarded.  If they are
 *   unequal, the new format becomes the current format and the old format's
 *   evolve() method is called.  evolve() is responsible for adjusting the
 *   old format for class evolution.  Any number of non-current formats may
 *   exist for a given class, and are setup to evolve the single current format
 *   for the class.
 *
 * @author Mark Hayes
 */
public abstract class Format implements Reader, RawType, Serializable {

    private static final long serialVersionUID = 545633644568489850L;

    /** Null reference. */
    static final int ID_NULL     = 0;
    /** Object */
    static final int ID_OBJECT   = 1;
    /** Boolean */
    static final int ID_BOOL     = 2;
    static final int ID_BOOL_W   = 3;
    /** Byte */
    static final int ID_BYTE     = 4;
    static final int ID_BYTE_W   = 5;
    /** Short */
    static final int ID_SHORT    = 6;
    static final int ID_SHORT_W  = 7;
    /** Integer */
    static final int ID_INT      = 8;
    static final int ID_INT_W    = 9;
    /** Long */
    static final int ID_LONG     = 10;
    static final int ID_LONG_W   = 11;
    /** Float */
    static final int ID_FLOAT    = 12;
    static final int ID_FLOAT_W  = 13;
    /** Double */
    static final int ID_DOUBLE   = 14;
    static final int ID_DOUBLE_W = 15;
    /** Character */
    static final int ID_CHAR     = 16;
    static final int ID_CHAR_W   = 17;
    /** String */
    static final int ID_STRING   = 18;
    /** BigInteger */
    static final int ID_BIGINT   = 19;
    /** BigDecimal */
    static final int ID_BIGDEC   = 20;
    /** Date */
    static final int ID_DATE     = 21;
    /** Number */
    static final int ID_NUMBER   = 22;

    /** First simple type. */
    static final int ID_SIMPLE_MIN  = 2;
    /** Last simple type. */
    static final int ID_SIMPLE_MAX  = 21;
    /** Last predefined ID, after which dynamic IDs are assigned. */
    static final int ID_PREDEFINED  = 30;

    static boolean isPredefined(Format format) {
        return format.getId() <= ID_PREDEFINED;
    }

    private int id;
    private String className;
    private Reader reader;
    private Format superFormat;
    private Format latestFormat;
    private Format previousFormat;
    private Set<String> supertypes;
    private boolean deleted;
    private boolean unused;
    private transient Catalog catalog;
    private transient Class type;
    private transient Format proxiedFormat;
    private transient boolean initialized;

    /**
     * Creates a new format for a given class.
     */
    Format(Class type) {
        this(type.getName());
        this.type = type;
        addSupertypes();
    }

    /**
     * Creates a format for class evolution when no class may be present.
     */
    Format(String className) {
        this.className = className;
        latestFormat = this;
        supertypes = new HashSet<String>();
    }

    /**
     * Special handling for JE 3.0.12 beta formats.
     */
    void migrateFromBeta(Map<String,Format> formatMap) {
        if (latestFormat == null) {
            latestFormat = this;
        }
    }

    final boolean isNew() {
        return id == 0;
    }

    final Catalog getCatalog() {
        return catalog;
    }

    /**
     * Returns the format ID.
     */
    public final int getId() {
        return id;
    }

    /**
     * Called by the Catalog to set the format ID when a new format is added to
     * the format list, before calling initializeIfNeeded().
     */
    final void setId(int id) {
        this.id = id;
    }

    /**
     * Returns the class that this format represents.  This method will return
     * null in rawAccess mode, or for an unevolved format.
     */
    final Class getType() {
        return type;
    }

    /**
     * Called to get the type when it is known to exist for an uninitialized
     * format.
     */
    final Class getExistingType() {
        if (type == null) {
            try {
                type = SimpleCatalog.classForName(className);
            } catch (ClassNotFoundException e) {
                throw new IllegalStateException(e);
            }
        }
        return type;
    }

    /**
     * Returns the object for reading objects of the latest format.  For the
     * latest version format, 'this' is returned.  For prior version formats, a
     * reader that converts this version to the latest version is returned.
     */
    final Reader getReader() {

        /*
         * For unit testing, record whether any un-evolved formats are
         * encountered.
         */
        if (this != reader) {
            PersistCatalog.unevolvedFormatsEncountered = true;
        }

        return reader;
    }

    /**
     * Changes the reader during format evolution.
     */
    final void setReader(Reader reader) {
        this.reader = reader;
    }

    /**
     * Returns the format of the superclass.
     */
    final Format getSuperFormat() {
        return superFormat;
    }

    /**
     * Called to set the format of the superclass during initialize().
     */
    final void setSuperFormat(Format superFormat) {
        this.superFormat = superFormat;
    }

    /**
     * Returns the format that is proxied by this format.  If non-null is
     * returned, then this format is a PersistentProxy.
     */
    final Format getProxiedFormat() {
        return proxiedFormat;
    }

    /**
     * Called by ProxiedFormat to set the proxied format.
     */
    final void setProxiedFormat(Format proxiedFormat) {
        this.proxiedFormat = proxiedFormat;
    }

    /**
     * If this is the latest/evolved format, returns this; otherwise, returns
     * the current version of this format.  Note that this WILL return a
     * format for a deleted class if the latest format happens to be deleted.
     */
    final Format getLatestVersion() {
        return latestFormat;
    }

    /**
     * Returns the previous version of this format in the linked list of
     * versions, or null if this is the only version.
     */
    public final Format getPreviousVersion() {
        return previousFormat;
    }

    /**
     * Called by Evolver to set the latest format when this old format is
     * evolved.
     */
    final void setLatestVersion(Format newFormat) {

        /*
         * If this old format is the former latest version, link it to the new
         * latest version.  This creates a singly linked list of versions
         * starting with the latest.
         */
        if (latestFormat == this) {
            newFormat.previousFormat = this;
        }

        latestFormat = newFormat;
    }

    /**
     * Returns whether the class for this format was deleted.
     */
    final boolean isDeleted() {
        return deleted;
    }

    /**
     * Called by the Evolver when applying a Deleter mutation.
     */
    final void setDeleted(boolean deleted) {
        this.deleted = deleted;
    }

    /**
     * Called by the Evolver for a format that is never referenced.
     */
    final void setUnused(boolean unused) {
        this.unused = unused;
    }

    /**
     * Called by the Evolver with true when an entity format or any of its
     * nested format were changed.  Called by Store.evolve when an entity has
     * been fully converted.  Overridden by ComplexFormat.
     */
    void setEvolveNeeded(boolean needed) {
        throw new UnsupportedOperationException();
    }

    /**
     * Overridden by ComplexFormat.
     */
    boolean getEvolveNeeded() {
        throw new UnsupportedOperationException();
    }

    final boolean isInitialized() {
        return initialized;
    }

    /**
     * Called by the Catalog to initialize a format, and may also be called
     * during initialize() for a related format to ensure that the related
     * format is initialized.  This latter case is allowed to support
     * bidirectional dependencies.  This method will do nothing if the format
     * is already intialized.
     */
    final void initializeIfNeeded(Catalog catalog, EntityModel model) {
        if (!initialized) {
            initialized = true;
            this.catalog = catalog;

            /* Initialize objects serialized by an older Format class. */
            if (latestFormat == null) {
                latestFormat = this;
            }
            if (reader == null) {
                reader = this;
            }

            /*
             * The class is only guaranteed to be available in live (not raw)
             * mode, for the current version of the format.
             */
            if (type == null &&
                isCurrentVersion() &&
                (isSimple() || !catalog.isRawAccess())) {
                getExistingType();
            }

            /* Perform subclass-specific initialization. */
            initialize(catalog, model,
                       catalog.getInitVersion(this, false /*forReader*/));
            reader.initializeReader
                (catalog, model,
                 catalog.getInitVersion(this, true /*forReader*/),
                 this);
        }
    }

    /**
     * Called to initialize a separate Reader implementation.  This method is
     * called when no separate Reader exists, and does nothing.
     */
    public void initializeReader(Catalog catalog,
                                 EntityModel model,
                                 int initVersion,
                                 Format oldFormat) {
    }

    /**
     * Adds all interfaces and superclasses to the supertypes set.
     */
    private void addSupertypes() {
        addInterfaces(type);
        Class stype = type.getSuperclass();
        while (stype != null && stype != Object.class) {
            supertypes.add(stype.getName());
            addInterfaces(stype);
            stype = stype.getSuperclass();
        }
    }

    /**
     * Recursively adds interfaces to the supertypes set.
     */
    private void addInterfaces(Class cls) {
        Class[] interfaces = cls.getInterfaces();
        for (Class iface : interfaces) {
            if (iface != Enhanced.class) {
                supertypes.add(iface.getName());
                addInterfaces(iface);
            }
        }
    }

    /**
     * Certain formats (ProxiedFormat for example) prohibit nested fields that
     * reference the parent object. [#15815]
     */
    boolean areNestedRefsProhibited() {
        return false;
    }

    /* -- Start of RawType interface methods. -- */

    public String getClassName() {
        return className;
    }

    public int getVersion() {
        ClassMetadata meta = getClassMetadata();
        if (meta != null) {
            return meta.getVersion();
        } else {
            return 0;
        }
    }

    public Format getSuperType() {
        return superFormat;
    }

    /* -- RawType methods that are overridden as needed in subclasses. -- */

    public boolean isSimple() {
        return false;
    }

    public boolean isPrimitive() {
        return false;
    }

    public boolean isEnum() {
        return false;
    }

    public List<String> getEnumConstants() {
        return null;
    }

    public boolean isArray() {
        return false;
    }

    public int getDimensions() {
        return 0;
    }

    public Format getComponentType() {
        return null;
    }

    public Map<String,RawField> getFields() {
        return null;
    }

    public ClassMetadata getClassMetadata() {
        return null;
    }

    public EntityMetadata getEntityMetadata() {
        return null;
    }

    /* -- End of RawType methods. -- */

    /* -- Methods that may optionally be overridden by subclasses. -- */

    /**
     * Called by EntityOutput in rawAccess mode to determine whether an object
     * type is allowed to be assigned to a given field type.
     */
    boolean isAssignableTo(Format format) {
        if (proxiedFormat != null) {
            return proxiedFormat.isAssignableTo(format);
        } else {
            return format == this ||
                   format.id == ID_OBJECT ||
                   supertypes.contains(format.className);
        }
    }

    /**
     * For primitive types only, returns their associated wrapper type.
     */
    Format getWrapperFormat() {
        return null;
    }

    /**
     * Returns whether this format class is an entity class.
     */
    boolean isEntity() {
        return false;
    }

    /**
     * Returns whether this class is present in the EntityModel.  Returns false
     * for a simple type, array type, or enum type.
     */
    boolean isModelClass() {
        return false;
    }

    /**
     * For an entity class or subclass, returns the base entity class; returns
     * null in other cases.
     */
    ComplexFormat getEntityFormat() {
        return null;
    }

    /**
     * Called for an existing format that may not equal the current format for
     * the same class.
     *
     * <p>If this method returns true, then it must have determined one of two
     * things:
     *  - that the old and new formats are equal, and it must have called
     *  Evolver.useOldFormat; or
     *  - that the old format can be evolved to the new format, and it must
     *  have called Evolver.useEvolvedFormat.</p>
     *
     * <p>If this method returns false, then it must have determined that the
     * old format could not be evolved to the new format, and it must have
     * called Evolver.addInvalidMutation, addMissingMutation or
     * addEvolveError.</p>
     */
    abstract boolean evolve(Format newFormat, Evolver evolver);

    /**
     * Called when a Converter handles evolution of a class, but we may still
     * need to evolve the metadata.
     */
    boolean evolveMetadata(Format newFormat,
                           Converter converter,
                           Evolver evolver) {
        return true;
    }

    /**
     * Returns whether this format is the current format for its class.  If
     * false is returned, this format is setup to evolve to the current format.
     */
    final boolean isCurrentVersion() {
        return latestFormat == this && !deleted;
    }

    /**
     * Returns whether this format has the same class as the given format,
     * irrespective of version changes and renaming.
     */
    final boolean isSameClass(Format other) {
        return latestFormat == other.latestFormat;
    }

    /* -- Abstract methods that must be implemented by subclasses. -- */

    /**
     * Initializes an uninitialized format, initializing its related formats
     * (superclass formats and array component formats) first.
     */
    abstract void initialize(Catalog catalog,
                             EntityModel model,
                             int initVersion);

    /**
     * Calls catalog.createFormat for formats that this format depends on, or
     * that should also be persistent.
     */
    abstract void collectRelatedFormats(Catalog catalog,
                                        Map<String,Format> newFormats);

    /*
     * The remaining methods are used to read objects from data bytes via
     * EntityInput, and to write objects as data bytes via EntityOutput.
     * Ultimately these methods call methods in the Accessor interface to
     * get/set fields in the object.  Most methods have a rawAccess parameter
     * that determines whether the object is a raw object or a real persistent
     * object.
     *
     * The first group of methods are abstract and must be implemented by
     * format classes.  The second group have default implementations that
     * throw UnsupportedOperationException and may optionally be overridden.
     */

    /**
     * Creates an array of the format's class of the given length, as if
     * Array.newInstance(getType(), len) were called.  Formats implement this
     * method for specific classes, or call the accessor, to avoid the
     * reflection overhead of Array.newInstance.
     */
    abstract Object newArray(int len);

    /**
     * Creates a new instance of the target class using its default
     * constructor.  Normally this creates an empty object, and readObject() is
     * called next to fill in the contents.  This is done in two steps to allow
     * the instance to be registered by EntityInput before reading the
     * contents.  This allows the fields in an object or a nested object to
     * refer to the parent object in a graph.
     *
     * Alternatively, this method may read all or the first portion of the
     * data, rather than that being done by readObject().  This is required for
     * simple types and enums, where the object cannot be created without
     * reading the data.  In these cases, there is no possibility that the
     * parent object will be referenced by the child object in the graph.  It
     * should not be done in other cases, or the graph references may not be
     * maintained faithfully.
     *
     * Is public only in order to implement the Reader interface.  Note that
     * this method should only be called directly in raw conversion mode or
     * during conversion of an old format.  Normally it should be called via
     * the getReader method and the Reader interface.
     */
    public abstract Object newInstance(EntityInput input, boolean rawAccess);

    /**
     * Called after newInstance() to read the rest of the data bytes and fill
     * in the object contents.  If the object was read completely by
     * newInstance(), this method does nothing.
     *
     * Is public only in order to implement the Reader interface.  Note that
     * this method should only be called directly in raw conversion mode or
     * during conversion of an old format.  Normally it should be called via
     * the getReader method and the Reader interface.
     */
    public abstract Object readObject(Object o,
                                      EntityInput input,
                                      boolean rawAccess);

    /**
     * Writes a given instance of the target class to the output data bytes.
     * This is the complement of the newInstance()/readObject() pair.
     */
    abstract void writeObject(Object o, EntityOutput output, boolean rawAccess);

    /**
     * Skips over the object's contents, as if readObject() were called, but
     * without returning an object.  Used for extracting secondary key bytes
     * without having to instantiate the object.  For reference types, the
     * format ID is read just before calling this method, so this method is
     * responsible for skipping everything following the format ID.
     */
    abstract void skipContents(RecordInput input);

    /* -- More methods that may optionally be overridden by subclasses. -- */

    /**
     * When extracting a secondary key, called to skip over all fields up to
     * the given secondary key field.  Returns the format of the key field
     * found, or null if the field is not present (nullified) in the object.
     */
    Format skipToSecKey(RecordInput input, String keyName) {
        throw new UnsupportedOperationException(toString());
    }

    /**
     * Called after skipToSecKey() to copy the data bytes of a singular
     * (XXX_TO_ONE) key field.
     */
    void copySecKey(RecordInput input, RecordOutput output) {
        throw new UnsupportedOperationException(toString());
    }

    /**
     * Called after skipToSecKey() to copy the data bytes of an array or
     * collection (XXX_TO_MANY) key field.
     */
    void copySecMultiKey(RecordInput input, Format keyFormat, Set results) {
        throw new UnsupportedOperationException(toString());
    }

    /**
     * Nullifies the given key field in the given RawObject --  rawAccess mode
     * is implied.
     */
    boolean nullifySecKey(Catalog catalog,
                          Object entity,
                          String keyName,
                          Object keyElement) {
        throw new UnsupportedOperationException(toString());
    }

    /**
     * Returns whether the entity's primary key field is null or zero, as
     * defined for primary keys that are assigned from a sequence.
     */
    boolean isPriKeyNullOrZero(Object o, boolean rawAccess) {
        throw new UnsupportedOperationException(toString());
    }

    /**
     * Gets the primary key field from the given object and writes it to the
     * given output data bytes.  This is a separate operation because the
     * primary key data bytes are stored separately from the rest of the
     * record.
     */
    void writePriKey(Object o, EntityOutput output, boolean rawAccess) {
        throw new UnsupportedOperationException(toString());
    }

    /**
     * Reads the primary key from the given input bytes and sets the primary
     * key field in the given object.  This is complement of writePriKey().
     *
     * Is public only in order to implement the Reader interface.  Note that
     * this method should only be called directly in raw conversion mode or
     * during conversion of an old format.  Normally it should be called via
     * the getReader method and the Reader interface.
     */
    public void readPriKey(Object o, EntityInput input, boolean rawAccess) {
        throw new UnsupportedOperationException(toString());
    }

    /**
     * Validates and returns the simple integer key format for a sequence key
     * associated with this format.
     *
     * For a composite key type, the format of the one and only field is
     * returned.  For a simple integer type, this format is returned.
     * Otherwise (the default implementation), an IllegalArgumentException is
     * thrown.
     */
    Format getSequenceKeyFormat() {
        throw new IllegalArgumentException
            ("Type not allowed for sequence: " + getClassName());
    }

    /**
     * Converts a RawObject to a current class object and adds the converted
     * pair to the converted map.
     */
    Object convertRawObject(Catalog catalog,
                            boolean rawAccess,
                            RawObject rawObject,
                            IdentityHashMap converted) {
        throw new UnsupportedOperationException(toString());
    }

    @Override
    public String toString() {
        final String INDENT = "  ";
        final String INDENT2 = INDENT + "  ";
        StringBuffer buf = new StringBuffer(500);
        if (isSimple()) {
            addTypeHeader(buf, "SimpleType");
            buf.append(" primitive=\"");
            buf.append(isPrimitive());
            buf.append("\"/>\n");
        } else if (isEnum()) {
            addTypeHeader(buf, "EnumType");
            buf.append(">\n");
            for (String constant : getEnumConstants()) {
                buf.append(INDENT);
                buf.append("<Constant>");
                buf.append(constant);
                buf.append("</Constant>\n");
            }
            buf.append("</EnumType>\n");
        } else if (isArray()) {
            addTypeHeader(buf, "ArrayType");
            buf.append(" componentId=\"");
            buf.append(getComponentType().getVersion());
            buf.append("\" componentClass=\"");
            buf.append(getComponentType().getClassName());
            buf.append("\" dimensions=\"");
            buf.append(getDimensions());
            buf.append("\"/>\n");
        } else {
            addTypeHeader(buf, "ComplexType");
            Format superType = getSuperType();
            if (superType != null) {
                buf.append(" superTypeId=\"");
                buf.append(superType.getId());
                buf.append("\" superTypeClass=\"");
                buf.append(superType.getClassName());
                buf.append('"');
            }
            Format proxiedFormat = getProxiedFormat();
            if (proxiedFormat != null) {
                buf.append(" proxiedTypeId=\"");
                buf.append(proxiedFormat.getId());
                buf.append("\" proxiedTypeClass=\"");
                buf.append(proxiedFormat.getClassName());
                buf.append('"');
            }
            PrimaryKeyMetadata priMeta = null;
            Map<String,SecondaryKeyMetadata> secondaryKeys = null;
            List<FieldMetadata> compositeKeyFields = null;
            ClassMetadata clsMeta = getClassMetadata();
            if (clsMeta != null) {
                compositeKeyFields = clsMeta.getCompositeKeyFields();
                priMeta = clsMeta.getPrimaryKey();
                secondaryKeys = clsMeta.getSecondaryKeys();
            }
            buf.append(" kind=\"");
            buf.append(isEntity() ? "entity" :
                       ((compositeKeyFields != null) ? "compositeKey" :
                        "persistent"));
            buf.append("\">\n");
            Map<String, RawField> fields = getFields();
            if (fields != null) {
                for (RawField field : fields.values()) {
                    String name = field.getName();
                    RawType type = field.getType();
                    buf.append(INDENT);
                    buf.append("<Field");
                    buf.append(" name=\"");
                    buf.append(name);
                    buf.append("\" typeId=\"");
                    buf.append(type.getId());
                    buf.append("\" typeClass=\"");
                    buf.append(type.getClassName());
                    buf.append('"');
                    if (priMeta != null &&
                        priMeta.getName().equals(name)) {
                        buf.append(" primaryKey=\"true\"");
                        if (priMeta.getSequenceName() != null) {
                            buf.append(" sequence=\"");
                            buf.append(priMeta.getSequenceName());
                            buf.append('"');
                        }
                    }
                    if (secondaryKeys != null) {
                        SecondaryKeyMetadata secMeta =
                            ComplexFormat.getSecondaryKeyMetadataByFieldName
                                (secondaryKeys, name);
                        if (secMeta != null) {
                            buf.append(" secondaryKey=\"true\" keyName=\"");
                            buf.append(secMeta.getKeyName());
                            buf.append("\" relate=\"");
                            buf.append(secMeta.getRelationship());
                            buf.append('"');
                            String related = secMeta.getRelatedEntity();
                            if (related != null) {
                                buf.append("\" relatedEntity=\"");
                                buf.append(related);
                                buf.append("\" onRelatedEntityDelete=\"");
                                buf.append(secMeta.getDeleteAction());
                                buf.append('"');
                            }
                        }
                    }
                    if (compositeKeyFields != null) {
                        int nFields = compositeKeyFields.size();
                        for (int i = 0; i < nFields; i += 1) {
                            FieldMetadata fldMeta = compositeKeyFields.get(i);
                            if (fldMeta.getName().equals(name)) {
                                buf.append(" compositeKeyField=\"");
                                buf.append(i + 1);
                                buf.append('"');
                            }
                        }
                    }
                    buf.append("/>\n");
                }
                EntityMetadata entMeta = getEntityMetadata();
                if (entMeta != null) {
                    buf.append(INDENT);
                    buf.append("<EntityKeys>\n");
                    priMeta = entMeta.getPrimaryKey();
                    if (priMeta != null) {
                        buf.append(INDENT2);
                        buf.append("<Primary class=\"");
                        buf.append(priMeta.getDeclaringClassName());
                        buf.append("\" field=\"");
                        buf.append(priMeta.getName());
                        buf.append("\"/>\n");
                    }
                    secondaryKeys = entMeta.getSecondaryKeys();
                    if (secondaryKeys != null) {
                        for (SecondaryKeyMetadata secMeta :
                             secondaryKeys.values()) {
                            buf.append(INDENT2);
                            buf.append("<Secondary class=\"");
                            buf.append(secMeta.getDeclaringClassName());
                            buf.append("\" field=\"");
                            buf.append(secMeta.getName());
                            buf.append("\"/>\n");
                        }
                    }
                    buf.append("</EntityKeys>\n");
                }
            }
            buf.append("</ComplexType>\n");
        }
        return buf.toString();
    }

    private void addTypeHeader(StringBuffer buf, String elemName) {
        buf.append('<');
        buf.append(elemName);
        buf.append(" id=\"");
        buf.append(getId());
        buf.append("\" class=\"");
        buf.append(getClassName());
        buf.append("\" version=\"");
        buf.append(getVersion());
        buf.append('"');
        Format currVersion = getLatestVersion();
        if (currVersion != null) {
            buf.append(" currentVersionId=\"");
            buf.append(currVersion.getId());
            buf.append('"');
        }
        Format prevVersion = getPreviousVersion();
        if (prevVersion != null) {
            buf.append(" previousVersionId=\"");
            buf.append(prevVersion.getId());
            buf.append('"');
        }
    }
}
