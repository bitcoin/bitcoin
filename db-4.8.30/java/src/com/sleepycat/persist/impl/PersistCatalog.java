/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;

import com.sleepycat.bind.tuple.IntegerBinding;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.DatabaseNamer;
import com.sleepycat.persist.StoreExistsException;
import com.sleepycat.persist.StoreNotFoundException;
import com.sleepycat.persist.evolve.DeletedClassException;
import com.sleepycat.persist.evolve.IncompatibleClassException;
import com.sleepycat.persist.evolve.Mutations;
import com.sleepycat.persist.evolve.Renamer;
import com.sleepycat.persist.model.AnnotationModel;
import com.sleepycat.persist.model.ClassMetadata;
import com.sleepycat.persist.model.EntityMetadata;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.raw.RawObject;
import com.sleepycat.persist.raw.RawType;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * The catalog of class formats for a store, along with its associated model
 * and mutations.
 *
 * @author Mark Hayes
 */
public class PersistCatalog implements Catalog {

    /**
     * Key to Data record in the catalog database.  In the JE 3.0.12 beta
     * version the formatList record is stored under this key and is converted
     * to a Data object when it is read.
     */
    private static final byte[] DATA_KEY = getIntBytes(-1);

    /**
     * Key to a JE 3.0.12 beta version mutations record in the catalog
     * database.  This record is no longer used because mutations are stored in
     * the Data record and is deleted when the beta version is detected.
     */
    private static final byte[] BETA_MUTATIONS_KEY = getIntBytes(-2);

    private static byte[] getIntBytes(int val) {
        DatabaseEntry entry = new DatabaseEntry();
        IntegerBinding.intToEntry(val, entry);
        assert entry.getSize() == 4 && entry.getData().length == 4;
        return entry.getData();
    }

    /**
     * Used by unit tests.
     */
    public static boolean expectNoClassChanges;
    public static boolean unevolvedFormatsEncountered;

    /**
     * The object stored under DATA_KEY in the catalog database.
     */
    private static class Data implements Serializable {

        static final long serialVersionUID = 7515058069137413261L;

        List<Format> formatList;
        Mutations mutations;
        int version;
    }

    /**
     * A list of all formats indexed by formatId.  Element zero is unused and
     * null, since IDs start at one; this avoids adjusting the ID to index the
     * list.  Some elements are null to account for predefined IDs that are not
     * used.
     *
     * <p>This field, like formatMap, is volatile because it is reassigned
     * when dynamically adding new formats.  See {@link addNewFormat}.</p>
     */
    private volatile List<Format> formatList;

    /**
     * A map of the current/live formats in formatList, indexed by class name.
     *
     * <p>This field, like formatList, is volatile because it is reassigned
     * when dynamically adding new formats.  See {@link addNewFormat}.</p>
     */
    private volatile Map<String,Format> formatMap;

    /**
     * A map of the latest formats (includes deleted formats) in formatList,
     * indexed by class name.
     *
     * <p>This field, like formatMap, is volatile because it is reassigned
     * when dynamically adding new formats.  See {@link addNewFormat}.</p>
     */
    private volatile Map<String,Format> latestFormatMap;

    /**
     * A temporary map of proxied class name to proxy class name.  Used during
     * catalog creation, and then set to null.  This map is used to force proxy
     * formats to be created prior to proxied formats. [#14665]
     */
    private Map<String,String> proxyClassMap;

    private boolean rawAccess;
    private EntityModel model;
    private Mutations mutations;
    private Database db;
    private int openCount;

    /**
     * The Store is normally present but may be null in unit tests (for
     * example, BindingTest).
     */
    private Store store;

    /**
     * The Evolver and catalog Data are non-null during catalog initialization,
     * and null otherwise.
     */
    private Evolver evolver;
    private Data catalogData;

    /**
     * Creates a new catalog, opening the database and reading it from a given
     * catalog database if it already exists.  All predefined formats and
     * formats for the given model are added.  For modified classes, old
     * formats are defined based on the rules for compatible class changes and
     * the given mutations.  If any format is changed or added, and the
     * database is not read-only, write the initialized catalog to the
     * database.
     */
    public PersistCatalog(Transaction txn,
                          Environment env,
                          String storePrefix,
                          String dbName,
                          DatabaseConfig dbConfig,
                          EntityModel modelParam,
                          Mutations mutationsParam,
                          boolean rawAccess,
                          Store store)
        throws StoreExistsException,
               StoreNotFoundException,
               IncompatibleClassException,
               DatabaseException {

        this.rawAccess = rawAccess;
        this.store = store;
        /* store may be null for testing. */
        String[] fileAndDbNames = (store != null) ?
            store.parseDbName(dbName) :
            Store.parseDbName(dbName, DatabaseNamer.DEFAULT);
        db = DbCompat.openDatabase
            (env, txn, fileAndDbNames[0], fileAndDbNames[1], dbConfig);
        if (db == null) {
            String dbNameMsg = store.getDbNameMessage(fileAndDbNames);
            if (dbConfig.getExclusiveCreate()) {
                throw new StoreExistsException
                    ("Catalog DB already exists and ExclusiveCreate=true, " +
                     dbNameMsg);
            } else {
                assert !dbConfig.getAllowCreate();
                throw new StoreNotFoundException
                    ("Catalog DB does not exist and AllowCreate=false, " +
                     dbNameMsg);
            }
        }
        openCount = 1;
        boolean success = false;
        try {
            catalogData = readData(txn);
            mutations = catalogData.mutations;
            if (mutations == null) {
                mutations = new Mutations();
            }

            /*
             * When the beta version is detected, force a re-write of the
             * catalog and disallow class changes.  This brings the catalog up
             * to date so that evolution can proceed correctly from then on.
             */
            boolean betaVersion = (catalogData.version == BETA_VERSION);
            boolean forceWriteData = betaVersion;
            boolean disallowClassChanges = betaVersion;

            /*
             * Store the given mutations if they are different from the stored
             * mutations, and force evolution to apply the new mutations.
             */
            boolean forceEvolution = false;
            if (mutationsParam != null &&
                !mutations.equals(mutationsParam)) {
                mutations = mutationsParam;
                forceWriteData = true;
                forceEvolution = true;
            }

            /* Get the existing format list, or copy it from SimpleCatalog. */
            formatList = catalogData.formatList;
            if (formatList == null) {
                formatList = SimpleCatalog.copyFormatList();

                /*
                 * Special cases: Object and Number are predefined but are not
                 * simple types.
                 */
                Format format = new NonPersistentFormat(Object.class);
                format.setId(Format.ID_OBJECT);
                formatList.set(Format.ID_OBJECT, format);
                format = new NonPersistentFormat(Number.class);
                format.setId(Format.ID_NUMBER);
                formatList.set(Format.ID_NUMBER, format);
            } else {
                if (SimpleCatalog.copyMissingFormats(formatList)) {
                    forceWriteData = true;
                }
            }

            /* Special handling for JE 3.0.12 beta formats. */
            if (betaVersion) {
                Map<String,Format> formatMap = new HashMap<String,Format>();
                for (Format format : formatList) {
                    if (format != null) {
                        formatMap.put(format.getClassName(), format);
                    }
                }
                for (Format format : formatList) {
                    if (format != null) {
                        format.migrateFromBeta(formatMap);
                    }
                }
            }

            /*
             * If we should not use the current model, initialize the stored
             * model and return.
             */
            formatMap = new HashMap<String,Format>(formatList.size());
            latestFormatMap = new HashMap<String,Format>(formatList.size());
            if (rawAccess) {
                for (Format format : formatList) {
                    if (format != null) {
                        String name = format.getClassName();
                        if (format.isCurrentVersion()) {
                            formatMap.put(name, format);
                        }
                        if (format == format.getLatestVersion()) {
                            latestFormatMap.put(name, format);
                        }
                    }
                }
                model = new StoredModel(this);
                for (Format format : formatList) {
                    if (format != null) {
                        format.initializeIfNeeded(this, model);
                    }
                }
                success = true;
                return;
            }

            /*
             * We are opening a store that uses the current model. Default to
             * the AnnotationModel if no model is specified.
             */
            if (modelParam != null) {
                model = modelParam;
            } else {
                model = new AnnotationModel();
            }

            /*
             * Add all predefined (simple) formats to the format map.  The
             * current version of other formats will be added below.
             */
            for (int i = 0; i <= Format.ID_PREDEFINED; i += 1) {
                Format simpleFormat = formatList.get(i);
                if (simpleFormat != null) {
                    formatMap.put(simpleFormat.getClassName(), simpleFormat);
                }
            }

            /*
             * Known classes are those explicitly registered by the user via
             * the model, plus the predefined proxy classes.
             */
            List<String> knownClasses =
                new ArrayList<String>(model.getKnownClasses());
            addPredefinedProxies(knownClasses);

            /*
             * Create a temporary map of proxied class name to proxy class
             * name, using all known formats and classes.  This map is used to
             * force proxy formats to be created prior to proxied formats.
             * [#14665]
             */
            proxyClassMap = new HashMap<String,String>();
            for (Format oldFormat : formatList) {
                if (oldFormat == null || Format.isPredefined(oldFormat)) {
                    continue;
                }
                String oldName = oldFormat.getClassName();
                Renamer renamer = mutations.getRenamer
                    (oldName, oldFormat.getVersion(), null);
                String newName =
                    (renamer != null) ? renamer.getNewName() : oldName;
                addProxiedClass(newName);
            }
            for (String className : knownClasses) {
                addProxiedClass(className);
            }

            /*
             * Add known formats from the model and the predefined proxies.
             * In general, classes will not be present in an AnnotationModel
             * until an instance is stored, in which case an old format exists.
             * However, registered proxy classes are an exception and must be
             * added in advance.  And the user may choose to register new
             * classes in advance.  The more formats we define in advance, the
             * less times we have to write to the catalog database.
             */
            Map<String,Format> newFormats = new HashMap<String,Format>();
            for (String className : knownClasses) {
                createFormat(className, newFormats);
            }

            /*
             * Perform class evolution for all old formats, and throw an
             * exception that contains the messages for all of the errors in
             * mutations or in the definition of new classes.
             */
            evolver = new Evolver
                (this, storePrefix, mutations, newFormats, forceEvolution,
                 disallowClassChanges);
            for (Format oldFormat : formatList) {
                if (oldFormat == null || Format.isPredefined(oldFormat)) {
                    continue;
                }
                if (oldFormat.isEntity()) {
                    evolver.evolveFormat(oldFormat);
                } else {
                    evolver.addNonEntityFormat(oldFormat);
                }
            }
            evolver.finishEvolution();
            String errors = evolver.getErrors();
            if (errors != null) {
                throw new IncompatibleClassException(errors);
            }

            /*
             * Add the new formats remaining.  New formats that are equal to
             * old formats were removed from the newFormats map above.
             */
            for (Format newFormat : newFormats.values()) {
                addFormat(newFormat);
            }

            /* Initialize all formats. */
            for (Format format : formatList) {
                if (format != null) {
                    format.initializeIfNeeded(this, model);
                    if (format == format.getLatestVersion()) {
                        latestFormatMap.put(format.getClassName(), format);
                    }
                }
            }

            boolean needWrite =
                 newFormats.size() > 0 ||
                 evolver.areFormatsChanged();

            /* For unit testing. */
            if (expectNoClassChanges && needWrite) {
                throw new IllegalStateException
                    ("Unexpected changes " +
                     " newFormats.size=" + newFormats.size() +
                     " areFormatsChanged=" + evolver.areFormatsChanged());
            }

            /* Write the catalog if anything changed. */
            if ((needWrite || forceWriteData) &&
                !db.getConfig().getReadOnly()) {

                /*
                 * Only rename/remove databases if we are going to update the
                 * catalog to reflect those class changes.
                 */
                evolver.renameAndRemoveDatabases(store, txn);

                /*
                 * Note that we use the Data object that was read above, and
                 * the beta version determines whether to delete the old
                 * mutations record.
                 */
                catalogData.formatList = formatList;
                catalogData.mutations = mutations;
                writeData(txn, catalogData);
            } else if (forceWriteData) {
                throw new IllegalArgumentException
                    ("When an upgrade is required the store may not be " +
                     "opened read-only");
            }

            success = true;
        } finally {

            /*
             * Fields needed only for the duration of this ctor and which
             * should be null afterwards.
             */
            proxyClassMap = null;
            catalogData = null;
            evolver = null;

            if (!success) {
                close();
            }
        }
    }

    public void getEntityFormats(Collection<Format> entityFormats) {
        for (Format format : formatMap.values()) {
            if (format.isEntity()) {
                entityFormats.add(format);
            }
        }
    }

    private void addProxiedClass(String className) {
        ClassMetadata metadata = model.getClassMetadata(className);
        if (metadata != null) {
            String proxiedClassName = metadata.getProxiedClassName();
            if (proxiedClassName != null) {
                proxyClassMap.put(proxiedClassName, className);
            }
        }
    }

    private void addPredefinedProxies(List<String> knownClasses) {
        knownClasses.add(CollectionProxy.ArrayListProxy.class.getName());
        knownClasses.add(CollectionProxy.LinkedListProxy.class.getName());
        knownClasses.add(CollectionProxy.HashSetProxy.class.getName());
        knownClasses.add(CollectionProxy.TreeSetProxy.class.getName());
        knownClasses.add(MapProxy.HashMapProxy.class.getName());
        knownClasses.add(MapProxy.TreeMapProxy.class.getName());
    }

    /**
     * Returns a map from format to a set of its superclass formats.  The
     * format for simple types, enums and class Object are not included.  Only
     * complex types have superclass formats as defined by
     * Format.getSuperFormat.
     */
    Map<Format,Set<Format>> getSubclassMap() {
        Map<Format,Set<Format>> subclassMap =
            new HashMap<Format,Set<Format>>();
        for (Format format : formatList) {
            if (format == null || Format.isPredefined(format)) {
                continue;
            }
            Format superFormat = format.getSuperFormat();
            if (superFormat != null) {
                Set<Format> subclass = subclassMap.get(superFormat);
                if (subclass == null) {
                    subclass = new HashSet<Format>();
                    subclassMap.put(superFormat, subclass);
                }
                subclass.add(format);
            }
        }
        return subclassMap;
    }

    /**
     * Returns the model parameter, default model or stored model.
     */
    public EntityModel getResolvedModel() {
        return model;
    }

    /**
     * Increments the reference count for a catalog that is already open.
     */
    public void openExisting() {
        openCount += 1;
    }

    /**
     * Decrements the reference count and closes the catalog DB when it reaches
     * zero.  Returns true if the database was closed or false if the reference
     * count is still non-zero and the database was left open.
     */
    public boolean close()
        throws DatabaseException {

        if (openCount == 0) {
            throw new IllegalStateException("Catalog is not open");
        } else {
            openCount -= 1;
            if (openCount == 0) {
                Database dbToClose = db;
                db = null;
                dbToClose.close();
                return true;
            } else {
                return false;
            }
        }
    }

    /**
     * Returns the current merged mutations.
     */
    public Mutations getMutations() {
        return mutations;
    }

    /**
     * Convenience method that gets the class for the given class name and
     * calls createFormat with the class object.
     */
    public Format createFormat(String clsName, Map<String,Format> newFormats) {
        Class type;
        try {
            type = SimpleCatalog.classForName(clsName);
        } catch (ClassNotFoundException e) {
            throw new IllegalStateException
                ("Class does not exist: " + clsName);
        }
        return createFormat(type, newFormats);
    }

    /**
     * If the given class format is not already present in the given map and
     * a format for this class name does not already exist, creates an
     * uninitialized format, adds it to the map, and also collects related
     * formats in the map.
     */
    public Format createFormat(Class type, Map<String,Format> newFormats) {
        /* Return a new or existing format for this class. */
        String className = type.getName();
        Format format = newFormats.get(className);
        if (format != null) {
            return format;
        }
        format = formatMap.get(className);
        if (format != null) {
            return format;
        }
        /* Simple types are predefined. */
        assert !SimpleCatalog.isSimpleType(type) : className;
        
        /*
         * Although metadata is only needed for a complex type, call
         * getClassMetadata for all types to support checks for illegal
         * metadata on other types.
         */
        ClassMetadata metadata = model.getClassMetadata(className);
        /* Create format of the appropriate type. */
        String proxyClassName = null;
        if (proxyClassMap != null) {
            proxyClassName = proxyClassMap.get(className);
        }
        if (proxyClassName != null) {
            format = new ProxiedFormat(type, proxyClassName);
        } else if (type.isArray()) {
            format = type.getComponentType().isPrimitive() ?
                (new PrimitiveArrayFormat(type)) :
                (new ObjectArrayFormat(type));
        } else if (type.isEnum()) {
            format = new EnumFormat(type);
        } else if (type == Object.class || type.isInterface()) {
            format = new NonPersistentFormat(type);
        } else {
            if (metadata == null) {
                throw new IllegalArgumentException
                    ("Class could not be loaded or is not persistent: " +
                     className);
            }
            if (metadata.getCompositeKeyFields() != null &&
                (metadata.getPrimaryKey() != null ||
                 metadata.getSecondaryKeys() != null)) {
                throw new IllegalArgumentException
                    ("A composite key class may not have primary or" +
                     " secondary key fields: " + type.getName());
            }

            /*
             * Check for inner class before default constructor, to give a
             * specific error message for each.
             */
            if (type.getEnclosingClass() != null &&
                !Modifier.isStatic(type.getModifiers())) {
                throw new IllegalArgumentException
                    ("Inner classes not allowed: " + type.getName());
            }
            try {
                type.getDeclaredConstructor();
            } catch (NoSuchMethodException e) {
                throw new IllegalArgumentException
                    ("No default constructor: " + type.getName(), e);
            }
            if (metadata.getCompositeKeyFields() != null) {
                format = new CompositeKeyFormat
                    (type, metadata, metadata.getCompositeKeyFields());
            } else {
                EntityMetadata entityMetadata =
                    model.getEntityMetadata(className);
                format = new ComplexFormat(type, metadata, entityMetadata);
            }
        }
        /* Collect new format along with any related new formats. */
        newFormats.put(className, format);
        format.collectRelatedFormats(this, newFormats);

        return format;
    }

    /**
     * Adds a format and makes it the current format for the class.
     */
    private void addFormat(Format format) {
        addFormat(format, formatList, formatMap);
    }

    /**
     * Adds a format to the given the format collections, for use when
     * dynamically adding formats.
     */
    private void addFormat(Format format,
                           List<Format> list,
                           Map<String,Format> map) {
        format.setId(list.size());
        list.add(format);
        map.put(format.getClassName(), format);
    }

    /**
     * Installs an existing format when no evolution is needed, i.e, when the
     * new and old formats are identical.
     */
    void useExistingFormat(Format oldFormat) {
        assert oldFormat.isCurrentVersion();
        formatMap.put(oldFormat.getClassName(), oldFormat);
    }

    /**
     * Returns a set of all persistent (non-simple type) class names.
     */
    Set<String> getModelClasses() {
        Set<String> classes = new HashSet<String>();
        for (Format format : formatMap.values()) {
            if (format.isModelClass()) {
                classes.add(format.getClassName());
            }
        }
        return Collections.unmodifiableSet(classes);
    }

    /**
     * Returns all formats as RawTypes.
     */
    public List<RawType> getAllRawTypes() {
        List<RawType> list = new ArrayList<RawType>();
        for (RawType type : formatList) {
            if (type != null) {
                list.add(type);
            }
        }
        return Collections.unmodifiableList(list);
    }

    /**
     * When a format is intialized, this method is called to get the version
     * of the serialized object to be initialized.  See Catalog.
     */
    public int getInitVersion(Format format, boolean forReader) {

        if (catalogData == null || catalogData.formatList == null ||
            format.getId() >= catalogData.formatList.size()) {

            /*
             * For new formats, use the current version.  If catalogData is
             * null, the Catalog ctor is finished and the format must be new.
             * If the ctor is in progress, the format is new if its ID is
             * greater than the ID of all pre-existing formats.
             */
            return Catalog.CURRENT_VERSION;
        } else {

            /*
             * Get the version of a pre-existing format during execution of the
             * Catalog ctor.  The catalogData field is non-null, but evolver
             * may be null if the catalog is opened in raw mode.
             */
            assert catalogData != null;

            if (forReader) {

                /*
                 * Get the version of the evolution reader for a pre-existing
                 * format.  Use the current version if the format changed
                 * during class evolution, otherwise use the stored version.
                 */
                return (evolver != null && evolver.isFormatChanged(format)) ?
                       Catalog.CURRENT_VERSION : catalogData.version;
            } else {
                /* Always used the stored version for a pre-existing format. */
                return catalogData.version;
            }
        }
    }

    public Format getFormat(int formatId) {
        try {
            Format format = formatList.get(formatId);
            if (format == null) {
                throw new DeletedClassException
                    ("Format does not exist: " + formatId);
            }
            return format;
        } catch (NoSuchElementException e) {
            throw new DeletedClassException
                ("Format does not exist: " + formatId);
        }
    }

    /**
     * Get a format for a given class, creating it if it does not exist.
     *
     * <p>This method is called for top level entity instances by
     * PersistEntityBinding.  When a new entity subclass format is added we
     * call Store.checkEntitySubclassSecondaries to ensure that all secondary
     * databases have been opened, before storing the entity.  We do this here
     * while not holding a synchronization mutex, not in addNewFormat, to avoid
     * deadlocks. checkEntitySubclassSecondaries synchronizes on the Store.
     * [#16399]</p>
     *
     * <p>Historical note:  At one time we opened / created the secondary
     * databases rather than requiring the user to open them, see [#15247].
     * Later we found this to be problematic since a user txn may have locked
     * primary records, see [#16399].</p>
     */
    public Format getFormat(Class cls, boolean checkEntitySubclassIndexes) {
        Format format = formatMap.get(cls.getName());
        if (format == null) {
            if (model != null) {
                format = addNewFormat(cls);
                /* Detect and handle new entity subclass. [#15247] */
                if (checkEntitySubclassIndexes && store != null) {
                    Format entityFormat = format.getEntityFormat();
                    if (entityFormat != null && entityFormat != format) {
                        try {
                            store.checkEntitySubclassSecondaries
                                (entityFormat.getEntityMetadata(),
                                 cls.getName());
                        } catch (DatabaseException e) {
                            throw new RuntimeExceptionWrapper(e);
                        }
                    }
                }
            }
            if (format == null) {
                throw new IllegalArgumentException
                    ("Class is not persistent: " + cls.getName());
            }
        }
        return format;
    }

    public Format getFormat(String className) {
        return formatMap.get(className);
    }

    public Format getLatestVersion(String className) {
        return latestFormatMap.get(className);
    }

    /**
     * Adds a format for a new class.  Returns the format added for the given
     * class, or throws an exception if the given class is not persistent.
     *
     * <p>This method uses a copy-on-write technique to add new formats without
     * impacting other threads.</p>
     */
    private synchronized Format addNewFormat(Class cls) {

        /*
         * After synchronizing, check whether another thread has added the
         * format needed.  Note that this is not the double-check technique
         * because the formatMap field is volatile and is not itself checked
         * for null.  (The double-check technique is known to be flawed in
         * Java.)
         */
        Format format = formatMap.get(cls.getName());
        if (format != null) {
            return format;
        }

        /* Copy the read-only format collections. */
        List<Format> newFormatList = new ArrayList<Format>(formatList);
        Map<String,Format> newFormatMap =
            new HashMap<String,Format>(formatMap);
        Map<String,Format> newLatestFormatMap =
            new HashMap<String,Format>(latestFormatMap);

        /* Add the new format and all related new formats. */
        Map<String,Format> newFormats = new HashMap<String,Format>();
        format = createFormat(cls, newFormats);
        for (Format newFormat : newFormats.values()) {
            addFormat(newFormat, newFormatList, newFormatMap);
        }

        /*
         * Initialize new formats using a read-only catalog because we can't
         * update this catalog until after we store it (below).
         */
        Catalog newFormatCatalog =
            new ReadOnlyCatalog(newFormatList, newFormatMap);
        for (Format newFormat : newFormats.values()) {
            newFormat.initializeIfNeeded(newFormatCatalog, model);
            newLatestFormatMap.put(newFormat.getClassName(), newFormat);
        }

        /*
         * Write the updated catalog using auto-commit, then assign the new
         * collections.  The database write must occur before the collections
         * are used, since a format must be persistent before it can be
         * referenced by a data record.
         */
        try {
            Data catalogData = new Data();
            catalogData.formatList = newFormatList;
            catalogData.mutations = mutations;
            writeData(null, catalogData);
        } catch (DatabaseException e) {
            throw new RuntimeExceptionWrapper(e);
        }
        formatList = newFormatList;
        formatMap = newFormatMap;
        latestFormatMap = newLatestFormatMap;

        return format;
    }

    /**
     * Used to write the catalog when a format has been changed, for example,
     * when Store.evolve has updated a Format's EvolveNeeded property.  Uses
     * auto-commit.
     */
    public synchronized void flush()
        throws DatabaseException {

        Data catalogData = new Data();
        catalogData.formatList = formatList;
        catalogData.mutations = mutations;
        writeData(null, catalogData);
    }

    /**
     * Reads catalog Data, converting old versions as necessary.  An empty
     * Data object is returned if no catalog data currently exists.  Null is
     * never returned.
     */
    private Data readData(Transaction txn)
        throws DatabaseException {

        Data catalogData;
        DatabaseEntry key = new DatabaseEntry(DATA_KEY);
        DatabaseEntry data = new DatabaseEntry();
        OperationStatus status = db.get(txn, key, data, null);
        if (status == OperationStatus.SUCCESS) {
            ByteArrayInputStream bais = new ByteArrayInputStream
                (data.getData(), data.getOffset(), data.getSize());
            try {
                ObjectInputStream ois = new ObjectInputStream(bais);
                Object object = ois.readObject();
                assert ois.available() == 0;
                if (object instanceof Data) {
                    catalogData = (Data) object;
                } else {
                    if (!(object instanceof List)) {
                        throw new IllegalStateException
                            (object.getClass().getName());
                    }
                    catalogData = new Data();
                    catalogData.formatList = (List) object;
                    catalogData.version = BETA_VERSION;
                }
                return catalogData;
            } catch (ClassNotFoundException e) {
                throw new IllegalStateException(e);
            } catch (IOException e) {
                throw new IllegalStateException(e);
            }
        } else {
            catalogData = new Data();
            catalogData.version = Catalog.CURRENT_VERSION;
        }
        return catalogData;
    }

    /**
     * Writes catalog Data.  If txn is null, auto-commit is used.
     */
    private void writeData(Transaction txn, Data catalogData)
        throws DatabaseException {

        /* Catalog data is written in the current version. */
        boolean wasBetaVersion = (catalogData.version == BETA_VERSION);
        catalogData.version = CURRENT_VERSION;

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try {
            ObjectOutputStream oos = new ObjectOutputStream(baos);
            oos.writeObject(catalogData);
        } catch (IOException e) {
            throw new IllegalStateException(e);
        }
        DatabaseEntry key = new DatabaseEntry(DATA_KEY);
        DatabaseEntry data = new DatabaseEntry(baos.toByteArray());
        db.put(txn, key, data);

        /*
         * Delete the unused beta mutations record if we read the beta version
         * record earlier.
         */
        if (wasBetaVersion) {
            key.setData(BETA_MUTATIONS_KEY);
            db.delete(txn, key);
        }
    }

    public boolean isRawAccess() {
        return rawAccess;
    }

    public Object convertRawObject(RawObject o, IdentityHashMap converted) {
        Format format = (Format) o.getType();
        if (this != format.getCatalog()) {

            /*
             * Use the corresponding format in this catalog when the external
             * raw object was created using a different catalog.  Create the
             * format if it does not already exist, for example, when this
             * store is empty. [#16253].
             */
	    String clsName = format.getClassName();
            Class cls;
            try {
                cls = SimpleCatalog.classForName(clsName);
                format = getFormat(cls, true /*checkEntitySubclassIndexes*/);
            } catch (ClassNotFoundException e) {
                format = null;
            }
            if (format == null) {
                throw new IllegalArgumentException
                    ("External raw type not found: " + clsName);
            }
        }
        Format proxiedFormat = format.getProxiedFormat();
        if (proxiedFormat != null) {
            format = proxiedFormat;
        }
        if (converted == null) {
            converted = new IdentityHashMap();
        }
        return format.convertRawObject(this, false, o, converted);
    }
}
