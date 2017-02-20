/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.tuple.StringBinding;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Cursor;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.ForeignKeyDeleteAction;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.SecondaryConfig;
import com.sleepycat.db.SecondaryDatabase;
import com.sleepycat.db.Sequence;
import com.sleepycat.db.SequenceConfig;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.DatabaseNamer;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.StoreExistsException;
import com.sleepycat.persist.StoreNotFoundException;
import com.sleepycat.persist.evolve.Converter;
import com.sleepycat.persist.evolve.EvolveConfig;
import com.sleepycat.persist.evolve.EvolveEvent;
import com.sleepycat.persist.evolve.EvolveInternal;
import com.sleepycat.persist.evolve.EvolveListener;
import com.sleepycat.persist.evolve.EvolveStats;
import com.sleepycat.persist.evolve.IncompatibleClassException;
import com.sleepycat.persist.evolve.Mutations;
import com.sleepycat.persist.model.DeleteAction;
import com.sleepycat.persist.model.EntityMetadata;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.model.ModelInternal;
import com.sleepycat.persist.model.PrimaryKeyMetadata;
import com.sleepycat.persist.model.Relationship;
import com.sleepycat.persist.model.SecondaryKeyMetadata;
import com.sleepycat.persist.raw.RawObject;
import com.sleepycat.util.keyrange.KeyRange;

/**
 * Base implementation for EntityStore and  RawStore.  The methods here
 * correspond directly to those in EntityStore; see EntityStore documentation
 * for details.
 *
 * @author Mark Hayes
 */
public class Store {

    public static final String NAME_SEPARATOR = "#";
    private static final String NAME_PREFIX = "persist" + NAME_SEPARATOR;
    private static final String DB_NAME_PREFIX = "com.sleepycat.persist.";
    private static final String CATALOG_DB = DB_NAME_PREFIX + "formats";
    private static final String SEQUENCE_DB = DB_NAME_PREFIX + "sequences";

    private static Map<Environment,Map<String,PersistCatalog>> catalogPool =
        new WeakHashMap<Environment,Map<String,PersistCatalog>>();

    /* For unit testing. */
    private static SyncHook syncHook;

    private Environment env;
    private boolean rawAccess;
    private PersistCatalog catalog;
    private EntityModel model;
    private Mutations mutations;
    private StoreConfig storeConfig;
    private String storeName;
    private String storePrefix;
    private Map<String,PrimaryIndex> priIndexMap;
    private Map<String,SecondaryIndex> secIndexMap;
    private Map<String,DatabaseConfig> priConfigMap;
    private Map<String,SecondaryConfig> secConfigMap;
    private Map<String,PersistKeyBinding> keyBindingMap;
    private Map<String,Sequence> sequenceMap;
    private Map<String,SequenceConfig> sequenceConfigMap;
    private Database sequenceDb;
    private IdentityHashMap<Database,Object> deferredWriteDatabases;
    private Map<String,Set<String>> inverseRelatedEntityMap;

    public Store(Environment env,
                 String storeName,
                 StoreConfig config,
                 boolean rawAccess)
        throws StoreExistsException,
               StoreNotFoundException,
               IncompatibleClassException,
               DatabaseException {

        this.env = env;
        this.storeName = storeName;
        this.rawAccess = rawAccess;

        if (env == null || storeName == null) {
            throw new NullPointerException
                ("env and storeName parameters must not be null");
        }
        if (config != null) {
            model = config.getModel();
            mutations = config.getMutations();
        }
        if (config == null) {
            storeConfig = StoreConfig.DEFAULT;
        } else {
            storeConfig = config.cloneConfig();
        }

        storePrefix = NAME_PREFIX + storeName + NAME_SEPARATOR;
        priIndexMap = new HashMap<String,PrimaryIndex>();
        secIndexMap = new HashMap<String,SecondaryIndex>();
        priConfigMap = new HashMap<String,DatabaseConfig>();
        secConfigMap = new HashMap<String,SecondaryConfig>();
        keyBindingMap = new HashMap<String,PersistKeyBinding>();
        sequenceMap = new HashMap<String,Sequence>();
        sequenceConfigMap = new HashMap<String,SequenceConfig>();
        deferredWriteDatabases = new IdentityHashMap<Database,Object>();

        if (rawAccess) {
            /* Open a read-only catalog that uses the stored model. */
            if (model != null) {
                throw new IllegalArgumentException
                    ("A model may not be specified when opening a RawStore");
            }
            DatabaseConfig dbConfig = new DatabaseConfig();
            dbConfig.setReadOnly(true);
            dbConfig.setTransactional
                (storeConfig.getTransactional());
            catalog = new PersistCatalog
                (null, env, storePrefix, storePrefix + CATALOG_DB, dbConfig,
                 model, mutations, rawAccess, this);
        } else {
            /* Open the shared catalog that uses the current model. */
            synchronized (catalogPool) {
                Map<String,PersistCatalog> catalogMap = catalogPool.get(env);
                if (catalogMap == null) {
                    catalogMap = new HashMap<String,PersistCatalog>();
                    catalogPool.put(env, catalogMap);
                }
                catalog = catalogMap.get(storeName);
                if (catalog != null) {
                    catalog.openExisting();
                } else {
                    Transaction txn = null;
                    if (storeConfig.getTransactional() &&
			DbCompat.getThreadTransaction(env) == null) {
                        txn = env.beginTransaction(null, null);
                    }
                    boolean success = false;
                    try {
                        DatabaseConfig dbConfig = new DatabaseConfig();
                        dbConfig.setAllowCreate(storeConfig.getAllowCreate());
                        dbConfig.setExclusiveCreate
                            (storeConfig.getExclusiveCreate());
                        dbConfig.setReadOnly(storeConfig.getReadOnly());
                        dbConfig.setTransactional
                            (storeConfig.getTransactional());
                        DbCompat.setTypeBtree(dbConfig);
                        catalog = new PersistCatalog
                            (txn, env, storePrefix, storePrefix + CATALOG_DB,
                             dbConfig, model, mutations, rawAccess, this);
                        catalogMap.put(storeName, catalog);
                        success = true;
                    } finally {
                        if (txn != null) {
                            if (success) {
                                txn.commit();
                            } else {
                                txn.abort();
                            }
                        }
                    }
                }
            }
        }

        /* Get the merged mutations from the catalog. */
        mutations = catalog.getMutations();

        /*
         * If there is no model parameter, use the default or stored model
         * obtained from the catalog.
         */
        model = catalog.getResolvedModel();

        /*
         * Give the model a reference to the catalog to fully initialize the
         * model.  Only then may we initialize the Converter mutations, which
         * themselves may call model methods and expect the model to be fully
         * initialized.
         */
        ModelInternal.setCatalog(model, catalog);
        for (Converter converter : mutations.getConverters()) {
            converter.getConversion().initialize(model);
        }

        /*
         * For each existing entity with a relatedEntity reference, create an
         * inverse map (back pointer) from the class named in the relatedEntity
         * to the class containing the secondary key.  This is used to open the
         * class containing the secondary key whenever we open the
         * relatedEntity class, to configure foreign key constraints. Note that
         * we do not need to update this map as new primary indexes are
         * created, because opening the new index will setup the foreign key
         * constraints. [#15358]
         */
        inverseRelatedEntityMap = new HashMap<String,Set<String>>();
        List<Format> entityFormats = new ArrayList<Format>();
        catalog.getEntityFormats(entityFormats);
        for (Format entityFormat : entityFormats) {
            EntityMetadata entityMeta = entityFormat.getEntityMetadata();
            for (SecondaryKeyMetadata secKeyMeta :
                 entityMeta.getSecondaryKeys().values()) {
                String relatedClsName = secKeyMeta.getRelatedEntity();
                if (relatedClsName != null) {
                    Set<String> inverseClassNames =
                        inverseRelatedEntityMap.get(relatedClsName);
                    if (inverseClassNames == null) {
                        inverseClassNames = new HashSet<String>();
                        inverseRelatedEntityMap.put
                            (relatedClsName, inverseClassNames);
                    }
                    inverseClassNames.add(entityMeta.getClassName());
                }
            }
        }
    }

    public Environment getEnvironment() {
        return env;
    }

    public StoreConfig getConfig() {
        return storeConfig.cloneConfig();
    }

    public String getStoreName() {
        return storeName;
    }


    public EntityModel getModel() {
        return model;
    }

    public Mutations getMutations() {
        return mutations;
    }

    /**
     * A getPrimaryIndex with extra parameters for opening a raw store.
     * primaryKeyClass and entityClass are used for generic typing; for a raw
     * store, these should always be Object.class and RawObject.class.
     * primaryKeyClassName is used for consistency checking and should be null
     * for a raw store only.  entityClassName is used to identify the store and
     * may not be null.
     */
    public synchronized <PK,E> PrimaryIndex<PK,E>
        getPrimaryIndex(Class<PK> primaryKeyClass,
                        String primaryKeyClassName,
                        Class<E> entityClass,
                        String entityClassName)
        throws DatabaseException {

        assert (rawAccess && entityClass == RawObject.class) ||
              (!rawAccess && entityClass != RawObject.class);
        assert (rawAccess && primaryKeyClassName == null) ||
              (!rawAccess && primaryKeyClassName != null);

        checkOpen();

        PrimaryIndex<PK,E> priIndex = priIndexMap.get(entityClassName);
        if (priIndex == null) {

            /* Check metadata. */
            EntityMetadata entityMeta = checkEntityClass(entityClassName);
            PrimaryKeyMetadata priKeyMeta = entityMeta.getPrimaryKey();
            if (primaryKeyClassName == null) {
                primaryKeyClassName = priKeyMeta.getClassName();
            } else {
                String expectClsName =
                    SimpleCatalog.keyClassName(priKeyMeta.getClassName());
                if (!primaryKeyClassName.equals(expectClsName)) {
                    throw new IllegalArgumentException
                        ("Wrong primary key class: " + primaryKeyClassName +
                         " Correct class is: " + expectClsName);
                }
            }

            /* Create bindings. */
            PersistEntityBinding entityBinding =
                new PersistEntityBinding(catalog, entityClassName, rawAccess);
            PersistKeyBinding keyBinding = getKeyBinding(primaryKeyClassName);

            /* If not read-only, get the primary key sequence. */
            String seqName = priKeyMeta.getSequenceName();
            if (!storeConfig.getReadOnly() && seqName != null) {
                entityBinding.keyAssigner = new PersistKeyAssigner
                    (keyBinding, entityBinding, getSequence(seqName));
            }

            /*
             * Use a single transaction for opening the primary DB and its
             * secondaries.  If opening any secondary fails, abort the
             * transaction and undo the changes to the state of the store.
             * Also support undo if the store is non-transactional.
             */
            Transaction txn = null;
            DatabaseConfig dbConfig = getPrimaryConfig(entityMeta);
            if (dbConfig.getTransactional() &&
		DbCompat.getThreadTransaction(env) == null) {
                txn = env.beginTransaction(null, null);
            }
            PrimaryOpenState priOpenState =
                new PrimaryOpenState(entityClassName);
            boolean success = false;
            try {
        
                /* Open the primary database. */
                String[] fileAndDbNames =
                    parseDbName(storePrefix + entityClassName);
                Database db = DbCompat.openDatabase
                    (env, txn, fileAndDbNames[0], fileAndDbNames[1], dbConfig);
                assert db != null;
                priOpenState.addDatabase(db);

                /* Create index object. */
                priIndex = new PrimaryIndex
                    (db, primaryKeyClass, keyBinding, entityClass,
                     entityBinding);

                /* Update index and database maps. */
                priIndexMap.put(entityClassName, priIndex);
                if (DbCompat.getDeferredWrite(dbConfig)) {
                    deferredWriteDatabases.put(db, null);
                }

                /* If not read-only, open all associated secondaries. */
                if (!dbConfig.getReadOnly()) {
                    openSecondaryIndexes(txn, entityMeta, priOpenState);

                    /*
                     * To enable foreign key contratints, also open all primary
                     * indexes referring to this class via a relatedEntity
                     * property in another entity. [#15358]
                     */
                    Set<String> inverseClassNames =
                        inverseRelatedEntityMap.get(entityClassName);
                    if (inverseClassNames != null) {
                        for (String relatedClsName : inverseClassNames) {
                            getRelatedIndex(relatedClsName);
                        }
                    }
                }
                success = true;
            } finally {
                if (success) {
                    if (txn != null) {
                        txn.commit();
                    }
                } else {
                    if (txn != null) {
                        txn.abort();
                    }
                    priOpenState.undoState();
                }
            }
        }
        return priIndex;
    }

    /**
     * Holds state information about opening a primary index and its secondary
     * indexes.  Used to undo the state of this object if the transaction
     * opening the primary and secondaries aborts.  Also used to close all
     * databases opened during this process for a non-transactional store.
     */
    private class PrimaryOpenState {

        private String entityClassName;
        private IdentityHashMap<Database,Object> databases;
        private Set<String> secNames;

        PrimaryOpenState(String entityClassName) {
            this.entityClassName = entityClassName;
            databases = new IdentityHashMap<Database,Object>();
            secNames = new HashSet<String>();
        }

        /**
         * Save a database that was opening during this operation.
         */
        void addDatabase(Database db) {
            databases.put(db, null);
        }

        /**
         * Save the name of a secondary index that was opening during this
         * operation.
         */
        void addSecondaryName(String secName) {
            secNames.add(secName);
        }

        /**
         * Reset all state information and closes any databases opened, when
         * this operation fails.  This method should be called for both
         * transactional and non-transsactional operation.
         *
         * For transactional operations on JE, we don't strictly need to close
         * the databases since the transaction abort will do that.  However,
         * closing them is harmless on JE, and required for DB core.
         */
        void undoState() {
            for (Database db : databases.keySet()) {
                try {
                    db.close();
                } catch (Exception ignored) {
                }
            }
            priIndexMap.remove(entityClassName);
            for (String secName : secNames) {
                secIndexMap.remove(secName);
            }
            for (Database db : databases.keySet()) {
                deferredWriteDatabases.remove(db);
            }
        }
    }

    /**
     * Opens a primary index related via a foreign key (relatedEntity).
     * Related indexes are not opened in the same transaction used by the
     * caller to open a primary or secondary.  It is OK to leave the related
     * index open when the caller's transaction aborts.  It is only important
     * to open a primary and its secondaries atomically.
     */
    private PrimaryIndex getRelatedIndex(String relatedClsName)
        throws DatabaseException {

        PrimaryIndex relatedIndex = priIndexMap.get(relatedClsName);
        if (relatedIndex == null) {
            EntityMetadata relatedEntityMeta =
                checkEntityClass(relatedClsName);
            Class relatedKeyCls;
            String relatedKeyClsName;
            Class relatedCls;
            if (rawAccess) {
                relatedCls = RawObject.class;
                relatedKeyCls = Object.class;
                relatedKeyClsName = null;
            } else {
                try {
                    relatedCls = EntityModel.classForName(relatedClsName);
                } catch (ClassNotFoundException e) {
                    throw new IllegalArgumentException
                        ("Related entity class not found: " +
                         relatedClsName);
                }
                relatedKeyClsName = SimpleCatalog.keyClassName
                    (relatedEntityMeta.getPrimaryKey().getClassName());
                relatedKeyCls =
                    SimpleCatalog.keyClassForName(relatedKeyClsName);
            }

            /*
             * Cycles are prevented here by adding primary indexes to the
             * priIndexMap as soon as they are created, before opening related
             * indexes.
             */
            relatedIndex = getPrimaryIndex
                (relatedKeyCls, relatedKeyClsName,
                 relatedCls, relatedClsName);
        }
        return relatedIndex;
    }

    /**
     * A getSecondaryIndex with extra parameters for opening a raw store.
     * keyClassName is used for consistency checking and should be null for a
     * raw store only.
     */
    public synchronized <SK,PK,E1,E2 extends E1> SecondaryIndex<SK,PK,E2>
        getSecondaryIndex(PrimaryIndex<PK,E1> primaryIndex,
                          Class<E2> entityClass,
                          String entityClassName,
                          Class<SK> keyClass,
                          String keyClassName,
                          String keyName)
        throws DatabaseException {

        assert (rawAccess && keyClassName == null) ||
              (!rawAccess && keyClassName != null);

        checkOpen();

        EntityMetadata entityMeta = null;
        SecondaryKeyMetadata secKeyMeta = null;

        /* Validate the subclass for a subclass index. */
        if (entityClass != primaryIndex.getEntityClass()) {
            entityMeta = model.getEntityMetadata(entityClassName);
            assert entityMeta != null;
            secKeyMeta = checkSecKey(entityMeta, keyName);
            String subclassName = entityClass.getName();
            String declaringClassName = secKeyMeta.getDeclaringClassName();
            if (!subclassName.equals(declaringClassName)) {
                throw new IllegalArgumentException
                    ("Key for subclass " + subclassName +
                     " is declared in a different class: " +
                     makeSecName(declaringClassName, keyName));
            }

            /*
             * Get/create the subclass format to ensure it is stored in the
             * catalog, even if no instances of the subclass are stored.
             * [#16399]
             */
            catalog.getFormat(entityClass,
                              false /*checkEntitySubclassIndexes*/);
        }

        /*
         * Even though the primary is already open, we can't assume the
         * secondary is open because we don't automatically open all
         * secondaries when the primary is read-only.  Use auto-commit (a null
         * transaction) since we're opening only one database.
         */
        String secName = makeSecName(entityClassName, keyName);
        SecondaryIndex<SK,PK,E2> secIndex = secIndexMap.get(secName);
        if (secIndex == null) {
            if (entityMeta == null) {
                entityMeta = model.getEntityMetadata(entityClassName);
                assert entityMeta != null;
            }
            if (secKeyMeta == null) {
                secKeyMeta = checkSecKey(entityMeta, keyName);
            }

            /* Check metadata. */
            if (keyClassName == null) {
                keyClassName = getSecKeyClass(secKeyMeta);
            } else {
                String expectClsName = getSecKeyClass(secKeyMeta);
                if (!keyClassName.equals(expectClsName)) {
                    throw new IllegalArgumentException
                        ("Wrong secondary key class: " + keyClassName +
                         " Correct class is: " + expectClsName);
                }
            }

            secIndex = openSecondaryIndex
                (null, primaryIndex, entityClass, entityMeta,
                 keyClass, keyClassName, secKeyMeta, secName,
                 false /*doNotCreate*/, null /*priOpenState*/);
        }
        return secIndex;
    }

    /**
     * Opens secondary indexes for a given primary index metadata.
     */
    private void openSecondaryIndexes(Transaction txn,
                                      EntityMetadata entityMeta,
                                      PrimaryOpenState priOpenState)
        throws DatabaseException {

        String entityClassName = entityMeta.getClassName();
        PrimaryIndex<Object,Object> priIndex =
            priIndexMap.get(entityClassName);
        assert priIndex != null;
        Class<Object> entityClass = priIndex.getEntityClass();

        for (SecondaryKeyMetadata secKeyMeta :
             entityMeta.getSecondaryKeys().values()) {
            String keyName = secKeyMeta.getKeyName();
            String secName = makeSecName(entityClassName, keyName);
            SecondaryIndex<Object,Object,Object> secIndex =
                secIndexMap.get(secName);
            if (secIndex == null) {
                String keyClassName = getSecKeyClass(secKeyMeta);
                /* RawMode: should not require class. */
                Class keyClass =
                    SimpleCatalog.keyClassForName(keyClassName);
                openSecondaryIndex
                    (txn, priIndex, entityClass, entityMeta,
                     keyClass, keyClassName, secKeyMeta,
                     makeSecName
                        (entityClassName, secKeyMeta.getKeyName()),
                     storeConfig.getSecondaryBulkLoad() /*doNotCreate*/,
                     priOpenState);
            }
        }
    }

    /**
     * Opens a secondary index with a given transaction and adds it to the
     * secIndexMap.  We assume that the index is not already open.
     */
    private <SK,PK,E1,E2 extends E1> SecondaryIndex<SK,PK,E2>
        openSecondaryIndex(Transaction txn,
                           PrimaryIndex<PK,E1> primaryIndex,
                           Class<E2> entityClass,
                           EntityMetadata entityMeta,
                           Class<SK> keyClass,
                           String keyClassName,
                           SecondaryKeyMetadata secKeyMeta,
                           String secName,
                           boolean doNotCreate,
                           PrimaryOpenState priOpenState)
        throws DatabaseException {

        assert !secIndexMap.containsKey(secName);
        String[] fileAndDbNames = parseDbName(storePrefix + secName);
        SecondaryConfig config =
            getSecondaryConfig(secName, entityMeta, keyClassName, secKeyMeta);
        Database priDb = primaryIndex.getDatabase();
        DatabaseConfig priConfig = priDb.getConfig();

        String relatedClsName = secKeyMeta.getRelatedEntity();
        if (relatedClsName != null) {
            PrimaryIndex relatedIndex = getRelatedIndex(relatedClsName);
            config.setForeignKeyDatabase(relatedIndex.getDatabase());
        }

        if (config.getTransactional() != priConfig.getTransactional() ||
            DbCompat.getDeferredWrite(config) !=
            DbCompat.getDeferredWrite(priConfig) ||
            config.getReadOnly() != priConfig.getReadOnly()) {
            throw new IllegalArgumentException
                ("One of these properties was changed to be inconsistent" +
                 " with the associated primary database: " +
                 " Transactional, DeferredWrite, ReadOnly");
        }

        PersistKeyBinding keyBinding = getKeyBinding(keyClassName);
        
        SecondaryDatabase db = openSecondaryDatabase
            (txn, fileAndDbNames, priDb, config, doNotCreate);
        if (db == null) {
            assert doNotCreate;
            return null;
        }
        SecondaryIndex<SK,PK,E2> secIndex = new SecondaryIndex
            (db, null, primaryIndex, keyClass, keyBinding);

        /* Update index and database maps. */
        secIndexMap.put(secName, secIndex);
        if (DbCompat.getDeferredWrite(config)) {
            deferredWriteDatabases.put(db, null);
        }
        if (priOpenState != null) {
            priOpenState.addDatabase(db);
            priOpenState.addSecondaryName(secName);
        }
        return secIndex;
    }

    /**
     * Open a secondary database, setting AllowCreate, ExclusiveCreate and
     * AllowPopulate appropriately.  We either set all three of these params to
     * true or all to false.  This ensures that we only populate a database
     * when it is created, never if it just happens to be empty.  [#16399]
     *
     * @param doNotCreate is true when StoreConfig.getSecondaryBulkLoad is true
     * and we are opening a secondary as a side effect of opening a primary,
     * i.e., getSecondaryIndex is not being called.  If doNotCreate is true and
     * the database does not exist, we silently ignore the failure to create
     * the DB and return null.  When getSecondaryIndex is subsequently called,
     * the secondary database will be created and populated from the primary --
     * a bulk load.
     */
    private SecondaryDatabase
        openSecondaryDatabase(final Transaction txn,
                              final String[] fileAndDbNames,
                              final Database priDb,
                              final SecondaryConfig config,
                              final boolean doNotCreate)
        throws DatabaseException {

        assert config.getAllowPopulate();
        assert !config.getExclusiveCreate();
        final boolean saveAllowCreate = config.getAllowCreate();
        try {
            if (doNotCreate) {
                config.setAllowCreate(false);
            }
            /* First try creating a new database, populate if needed. */
            if (config.getAllowCreate()) {
                config.setExclusiveCreate(true);
                /* AllowPopulate is already set to true. */
                final SecondaryDatabase db = DbCompat.openSecondaryDatabase
                    (env, txn, fileAndDbNames[0], fileAndDbNames[1], priDb,
                     config);
                if (db != null) {
                    return db;
                }
            }
            /* Next try opening an existing database. */
            config.setAllowCreate(false);
            config.setAllowPopulate(false);
            config.setExclusiveCreate(false);
            final SecondaryDatabase db = DbCompat.openSecondaryDatabase
                (env, txn, fileAndDbNames[0], fileAndDbNames[1], priDb,
                 config);
            return db;
        } finally {
            config.setAllowPopulate(true);
            config.setExclusiveCreate(false);
            config.setAllowCreate(saveAllowCreate);
        }
    }

    /**
     * Checks that all secondary indexes defined in the given entity metadata
     * are already open.  This method is called when a new entity subclass
     * is encountered when an instance of that class is stored.  [#16399]
     *
     * @throws IllegalArgumentException if a secondary is not open.
     */
    synchronized void
        checkEntitySubclassSecondaries(final EntityMetadata entityMeta,
                                       final String subclassName)
        throws DatabaseException {
        
        if (storeConfig.getSecondaryBulkLoad()) {
            return;
        }

        final String entityClassName = entityMeta.getClassName();

        for (final SecondaryKeyMetadata secKeyMeta :
             entityMeta.getSecondaryKeys().values()) {
            final String keyName = secKeyMeta.getKeyName();
            final String secName = makeSecName(entityClassName, keyName);
            if (!secIndexMap.containsKey(secName)) {
                throw new IllegalArgumentException
                    ("Entity subclasses defining a secondary key must be " +
                     "registered by calling EntityModel.registerClass or " +
                     "EntityStore.getSubclassIndex before storing an " +
                     "instance of the subclass: " + subclassName);
            }
        }
    }


    public void truncateClass(Class entityClass)
        throws DatabaseException {

        truncateClass(null, entityClass);
    }

    public synchronized void truncateClass(Transaction txn, Class entityClass)
        throws DatabaseException {

        checkOpen();

        /* Close primary and secondary databases. */
        closeClass(entityClass);

        String clsName = entityClass.getName();
        EntityMetadata entityMeta = checkEntityClass(clsName);

        /*
         * Truncate the primary first and let any exceptions propogate
         * upwards.  Then remove each secondary, only throwing the first
         * exception.
         */
        boolean primaryExists = truncateIfExists(txn, storePrefix + clsName);
        if (primaryExists) {
            DatabaseException firstException = null;
            for (SecondaryKeyMetadata keyMeta :
                 entityMeta.getSecondaryKeys().values()) {
                /* Ignore secondaries that do not exist. */
                removeIfExists
                    (txn,
                     storePrefix +
                     makeSecName(clsName, keyMeta.getKeyName()));
            }
            if (firstException != null) {
                throw firstException;
            }
        }
    }

    private boolean truncateIfExists(Transaction txn, String dbName)
        throws DatabaseException {

        String[] fileAndDbNames = parseDbName(dbName);
        return DbCompat.truncateDatabase
            (env, txn, fileAndDbNames[0], fileAndDbNames[1]);
    }

    private boolean removeIfExists(Transaction txn, String dbName)
        throws DatabaseException {

        String[] fileAndDbNames = parseDbName(dbName);
        return DbCompat.removeDatabase
            (env, txn, fileAndDbNames[0], fileAndDbNames[1]);
    }

    public synchronized void closeClass(Class entityClass)
        throws DatabaseException {

        checkOpen();
        String clsName = entityClass.getName();
        EntityMetadata entityMeta = checkEntityClass(clsName);

        PrimaryIndex priIndex = priIndexMap.get(clsName);
        if (priIndex != null) {
            /* Close the secondaries first. */
            DatabaseException firstException = null;
            for (SecondaryKeyMetadata keyMeta :
                 entityMeta.getSecondaryKeys().values()) {

                String secName = makeSecName(clsName, keyMeta.getKeyName());
                SecondaryIndex secIndex = secIndexMap.get(secName);
                if (secIndex != null) {
                    Database db = secIndex.getDatabase();
                    firstException = closeDb(db, firstException);
                    firstException =
                        closeDb(secIndex.getKeysDatabase(), firstException);
                    secIndexMap.remove(secName);
                    deferredWriteDatabases.remove(db);
                }
            }
            /* Close the primary last. */
            Database db = priIndex.getDatabase();
            firstException = closeDb(db, firstException);
            priIndexMap.remove(clsName);
            deferredWriteDatabases.remove(db);

            /* Throw the first exception encountered. */
            if (firstException != null) {
                throw firstException;
            }
        }
    }

    public synchronized void close()
        throws DatabaseException {

        if (catalog == null) {
            return;
        }

        DatabaseException firstException = null;
        try {
            if (rawAccess) {
                boolean allClosed = catalog.close();
                assert allClosed;
            } else {
                synchronized (catalogPool) {
                    Map<String,PersistCatalog> catalogMap =
                        catalogPool.get(env);
                    assert catalogMap != null;
                    if (catalog.close()) {
                        /* Remove when the reference count goes to zero. */
                        catalogMap.remove(storeName);
                    }
                }
            }
            catalog = null;
        } catch (DatabaseException e) {
            if (firstException == null) {
                firstException = e;
            }
        }
		for (Sequence seq : sequenceMap.values()) {
			try {
				seq.close();
			} catch (DatabaseException e) {
				if (firstException == null) {
					firstException = e;
				}
			}
		}		
        firstException = closeDb(sequenceDb, firstException);
        for (SecondaryIndex index : secIndexMap.values()) {
            firstException = closeDb(index.getDatabase(), firstException);
            firstException = closeDb(index.getKeysDatabase(), firstException);
        }
        for (PrimaryIndex index : priIndexMap.values()) {
            firstException = closeDb(index.getDatabase(), firstException);
        }
        if (firstException != null) {
            throw firstException;
        }
    }

    public synchronized Sequence getSequence(String name)
        throws DatabaseException {

        checkOpen();

        if (storeConfig.getReadOnly()) {
            throw new IllegalStateException("Store is read-only");
        }

        Sequence seq = sequenceMap.get(name);
        if (seq == null) {
            if (sequenceDb == null) {
                String[] fileAndDbNames =
                    parseDbName(storePrefix + SEQUENCE_DB);
                DatabaseConfig dbConfig = new DatabaseConfig();
                dbConfig.setTransactional(storeConfig.getTransactional());
                dbConfig.setAllowCreate(true);
                DbCompat.setTypeBtree(dbConfig);
                sequenceDb = DbCompat.openDatabase
                    (env, null/*txn*/, fileAndDbNames[0],
                     fileAndDbNames[1], dbConfig);
                assert sequenceDb != null;
            }
            DatabaseEntry entry = new DatabaseEntry();
            StringBinding.stringToEntry(name, entry);
                seq = sequenceDb.openSequence(null, entry,
                                              getSequenceConfig(name));
            sequenceMap.put(name, seq);
        }
        return seq;
    }

    public synchronized SequenceConfig getSequenceConfig(String name) {
        checkOpen();
        SequenceConfig config = sequenceConfigMap.get(name);
        if (config == null) {
            config = new SequenceConfig();
            config.setInitialValue(1);
            config.setRange(1, Long.MAX_VALUE);
            config.setCacheSize(100);
            config.setAutoCommitNoSync(true);
            config.setAllowCreate(!storeConfig.getReadOnly());
            sequenceConfigMap.put(name, config);
        }
        return config;
    }

    public synchronized void setSequenceConfig(String name,
                                               SequenceConfig config) {
        checkOpen();
        if (config.getExclusiveCreate() ||
            config.getAllowCreate() == storeConfig.getReadOnly()) {
            throw new IllegalArgumentException
                ("One of these properties was illegally changed: " +
                 "AllowCreate, ExclusiveCreate");
        }
        sequenceConfigMap.put(name, config);
    }

    public synchronized DatabaseConfig getPrimaryConfig(Class entityClass) {
        checkOpen();
        String clsName = entityClass.getName();
        EntityMetadata meta = checkEntityClass(clsName);
        return getPrimaryConfig(meta).cloneConfig();
    }

    private synchronized DatabaseConfig getPrimaryConfig(EntityMetadata meta) {
        String clsName = meta.getClassName();
        DatabaseConfig config = priConfigMap.get(clsName);
        if (config == null) {
            config = new DatabaseConfig();
            config.setTransactional(storeConfig.getTransactional());
            config.setAllowCreate(!storeConfig.getReadOnly());
            config.setReadOnly(storeConfig.getReadOnly());
            DbCompat.setTypeBtree(config);
            setBtreeComparator(config, meta.getPrimaryKey().getClassName());
            priConfigMap.put(clsName, config);
        }
        return config;
    }

    public synchronized void setPrimaryConfig(Class entityClass,
                                              DatabaseConfig config) {
        checkOpen();
        String clsName = entityClass.getName();
        if (priIndexMap.containsKey(clsName)) {
            throw new IllegalStateException
                ("Cannot set config after DB is open");
        }
        EntityMetadata meta = checkEntityClass(clsName);
        DatabaseConfig dbConfig = getPrimaryConfig(meta);
        if (config.getExclusiveCreate() ||
            config.getAllowCreate() == config.getReadOnly() ||
            config.getSortedDuplicates() ||
            config.getBtreeComparator() != dbConfig.getBtreeComparator()) {
            throw new IllegalArgumentException
                ("One of these properties was illegally changed: " +
                 "AllowCreate, ExclusiveCreate, SortedDuplicates, Temporary " +
                 "or BtreeComparator, ");
        }
        if (!DbCompat.isTypeBtree(config)) {
            throw new IllegalArgumentException("Only type BTREE allowed");
        }
        priConfigMap.put(clsName, config);
    }

    public synchronized SecondaryConfig getSecondaryConfig(Class entityClass,
                                                           String keyName) {
        checkOpen();
        String entityClsName = entityClass.getName();
        EntityMetadata entityMeta = checkEntityClass(entityClsName);
        SecondaryKeyMetadata secKeyMeta = checkSecKey(entityMeta, keyName);
        String keyClassName = getSecKeyClass(secKeyMeta);
        String secName = makeSecName(entityClass.getName(), keyName);
        return (SecondaryConfig) getSecondaryConfig
            (secName, entityMeta, keyClassName, secKeyMeta).cloneConfig();
    }

    private SecondaryConfig getSecondaryConfig(String secName,
                                               EntityMetadata entityMeta,
                                               String keyClassName,
                                               SecondaryKeyMetadata
                                               secKeyMeta) {
        SecondaryConfig config = secConfigMap.get(secName);
        if (config == null) {
            /* Set common properties to match the primary DB. */
            DatabaseConfig priConfig = getPrimaryConfig(entityMeta);
            config = new SecondaryConfig();
            config.setTransactional(priConfig.getTransactional());
            config.setAllowCreate(!priConfig.getReadOnly());
            config.setReadOnly(priConfig.getReadOnly());
            DbCompat.setTypeBtree(config);
            /* Set secondary properties based on metadata. */
            config.setAllowPopulate(true);
            Relationship rel = secKeyMeta.getRelationship();
            config.setSortedDuplicates(rel == Relationship.MANY_TO_ONE ||
                                       rel == Relationship.MANY_TO_MANY);
            setBtreeComparator(config, keyClassName);
            PersistKeyCreator keyCreator = new PersistKeyCreator
                (catalog, entityMeta, keyClassName, secKeyMeta, rawAccess);
            if (rel == Relationship.ONE_TO_MANY ||
                rel == Relationship.MANY_TO_MANY) {
                config.setMultiKeyCreator(keyCreator);
            } else {
                config.setKeyCreator(keyCreator);
            }
            DeleteAction deleteAction = secKeyMeta.getDeleteAction();
            if (deleteAction != null) {
                ForeignKeyDeleteAction baseDeleteAction;
                switch (deleteAction) {
                case ABORT:
                    baseDeleteAction = ForeignKeyDeleteAction.ABORT;
                    break;
                case CASCADE:
                    baseDeleteAction = ForeignKeyDeleteAction.CASCADE;
                    break;
                case NULLIFY:
                    baseDeleteAction = ForeignKeyDeleteAction.NULLIFY;
                    break;
                default:
                    throw new IllegalStateException(deleteAction.toString());
                }
                config.setForeignKeyDeleteAction(baseDeleteAction);
                if (deleteAction == DeleteAction.NULLIFY) {
                    config.setForeignMultiKeyNullifier(keyCreator);
                }
            }
            secConfigMap.put(secName, config);
        }
        return config;
    }

    public synchronized void setSecondaryConfig(Class entityClass,
                                                String keyName,
                                                SecondaryConfig config) {
        checkOpen();
        String entityClsName = entityClass.getName();
        EntityMetadata entityMeta = checkEntityClass(entityClsName);
        SecondaryKeyMetadata secKeyMeta = checkSecKey(entityMeta, keyName);
        String keyClassName = getSecKeyClass(secKeyMeta);
        String secName = makeSecName(entityClass.getName(), keyName);
        if (secIndexMap.containsKey(secName)) {
            throw new IllegalStateException
                ("Cannot set config after DB is open");
        }
        SecondaryConfig dbConfig =
            getSecondaryConfig(secName, entityMeta, keyClassName, secKeyMeta);
        if (config.getExclusiveCreate() ||
            config.getAllowCreate() == config.getReadOnly() ||
            config.getSortedDuplicates() != dbConfig.getSortedDuplicates() ||
            config.getBtreeComparator() != dbConfig.getBtreeComparator() ||
            config.getDuplicateComparator() != null ||
            config.getAllowPopulate() != dbConfig.getAllowPopulate() ||
            config.getKeyCreator() != dbConfig.getKeyCreator() ||
            config.getMultiKeyCreator() != dbConfig.getMultiKeyCreator() ||
            config.getForeignKeyNullifier() !=
                dbConfig.getForeignKeyNullifier() ||
            config.getForeignMultiKeyNullifier() !=
                dbConfig.getForeignMultiKeyNullifier() ||
            config.getForeignKeyDeleteAction() !=
                dbConfig.getForeignKeyDeleteAction() ||
            config.getForeignKeyDatabase() != null) {
            throw new IllegalArgumentException
                ("One of these properties was illegally changed: " +
                 " AllowCreate, ExclusiveCreate, SortedDuplicates," +
                 " BtreeComparator, DuplicateComparator, Temporary," +
                 " AllowPopulate, KeyCreator, MultiKeyCreator," +
                 " ForeignKeyNullifer, ForeignMultiKeyNullifier," +
                 " ForeignKeyDeleteAction, ForeignKeyDatabase");
        }
        if (!DbCompat.isTypeBtree(config)) {
            throw new IllegalArgumentException("Only type BTREE allowed");
        }
        secConfigMap.put(secName, config);
    }

    private static String makeSecName(String entityClsName, String keyName) {
         return entityClsName + NAME_SEPARATOR + keyName;
    }

    static String makePriDbName(String storePrefix, String entityClsName) {
        return storePrefix + entityClsName;
    }

    static String makeSecDbName(String storePrefix,
                                String entityClsName,
                                String keyName) {
        return storePrefix + makeSecName(entityClsName, keyName);
    }

    /**
     * Parses a whole DB name and returns an array of 2 strings where element 0
     * is the file name (always null for JE, always non-null for DB core) and
     * element 1 is the logical DB name (always non-null for JE, may be null
     * for DB core).
     */
    public String[] parseDbName(String wholeName) {
        return parseDbName(wholeName, storeConfig.getDatabaseNamer());
    }

    /**
     * Allows passing a namer to a static method for testing.
     */
    public static String[] parseDbName(String wholeName, DatabaseNamer namer) {
        String[] result = new String[2];
        if (DbCompat.SEPARATE_DATABASE_FILES) {
            String[] splitName = wholeName.split(NAME_SEPARATOR);
            assert splitName.length == 3 || splitName.length == 4 : wholeName;
            assert splitName[0].equals("persist") : wholeName;
            String storeName = splitName[1];
            String clsName = splitName[2];
            String keyName = (splitName.length > 3) ? splitName[3] : null;
            result[0] = namer.getFileName(storeName, clsName, keyName);
            result[1] = null;
        } else {
            result[0] = null;
            result[1] = wholeName;
        }
        return result;
    }

    /**
     * Creates a message identifying the database from the pair of strings
     * returned by parseDbName.
     */
    String getDbNameMessage(String[] names) {
        if (DbCompat.SEPARATE_DATABASE_FILES) {
            return "file: " + names[0];
        } else {
            return "database: " + names[1];
        }
    }

    private void checkOpen() {
        if (catalog == null) {
            throw new IllegalStateException("Store has been closed");
        }
    }

    private EntityMetadata checkEntityClass(String clsName) {
        EntityMetadata meta = model.getEntityMetadata(clsName);
        if (meta == null) {
            throw new IllegalArgumentException
                ("Class could not be loaded or is not an entity class: " +
                 clsName);
        }
        return meta;
    }

    private SecondaryKeyMetadata checkSecKey(EntityMetadata entityMeta,
                                             String keyName) {
        SecondaryKeyMetadata secKeyMeta =
            entityMeta.getSecondaryKeys().get(keyName);
        if (secKeyMeta == null) {
            throw new IllegalArgumentException
                ("Not a secondary key: " +
                 makeSecName(entityMeta.getClassName(), keyName));
        }
        return secKeyMeta;
    }

    private String getSecKeyClass(SecondaryKeyMetadata secKeyMeta) {
        String clsName = secKeyMeta.getElementClassName();
        if (clsName == null) {
            clsName = secKeyMeta.getClassName();
        }
        return SimpleCatalog.keyClassName(clsName);
    }

    private PersistKeyBinding getKeyBinding(String keyClassName) {
        PersistKeyBinding binding = keyBindingMap.get(keyClassName);
        if (binding == null) {
            binding = new PersistKeyBinding(catalog, keyClassName, rawAccess);
            keyBindingMap.put(keyClassName, binding);
        }
        return binding;
    }

    private void setBtreeComparator(DatabaseConfig config, String clsName) {
        if (!rawAccess) {
            PersistKeyBinding binding = getKeyBinding(clsName);
            Format format = binding.keyFormat;
            if (format instanceof CompositeKeyFormat) {
                Class keyClass = format.getType();
                if (Comparable.class.isAssignableFrom(keyClass)) {
                    config.setBtreeComparator(new PersistComparator(binding));
                }
            }
        }
    }

    private DatabaseException closeDb(Database db,
                                      DatabaseException firstException) {
        if (db != null) {
            try {
                db.close();
            } catch (DatabaseException e) {
                if (firstException == null) {
                    firstException = e;
                }
            }
        }
        return firstException;
    }

    public EvolveStats evolve(EvolveConfig config)
        throws DatabaseException {

        checkOpen();
        List<Format> toEvolve = new ArrayList<Format>();
        Set<String> configToEvolve = config.getClassesToEvolve();
        if (configToEvolve.isEmpty()) {
            catalog.getEntityFormats(toEvolve);
        } else {
            for (String name : configToEvolve) {
                Format format = catalog.getFormat(name);
                if (format == null) {
                    throw new IllegalArgumentException
                        ("Class to evolve is not persistent: " + name);
                }
                if (!format.isEntity()) {
                    throw new IllegalArgumentException
                        ("Class to evolve is not an entity class: " + name);
                }
                toEvolve.add(format);
            }
        }

        EvolveEvent event = EvolveInternal.newEvent();
        for (Format format : toEvolve) {
            if (format.getEvolveNeeded()) {
                evolveIndex(format, event, config.getEvolveListener());
                format.setEvolveNeeded(false);
                catalog.flush();
            }
        }

        return event.getStats();
    }

    private void evolveIndex(Format format,
                             EvolveEvent event,
                             EvolveListener listener)
        throws DatabaseException {

        /* We may make this configurable later. */
        final int WRITES_PER_TXN = 1;

        Class entityClass = format.getType();
        String entityClassName = format.getClassName();
        EntityMetadata meta = model.getEntityMetadata(entityClassName);
        String keyClassName = meta.getPrimaryKey().getClassName();
        keyClassName = SimpleCatalog.keyClassName(keyClassName);
        DatabaseConfig dbConfig = getPrimaryConfig(meta);

        PrimaryIndex<Object,Object> index = getPrimaryIndex
            (Object.class, keyClassName, entityClass, entityClassName);
        Database db = index.getDatabase();

        EntityBinding binding = index.getEntityBinding();
        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry data = new DatabaseEntry();

        CursorConfig cursorConfig = null;
        Transaction txn = null;
        if (dbConfig.getTransactional()) {
            txn = env.beginTransaction(null, null);
            cursorConfig = CursorConfig.READ_COMMITTED;
        }

        Cursor cursor = null;
        int nWritten = 0;
        try {
            cursor = db.openCursor(txn, cursorConfig);
            OperationStatus status = cursor.getFirst(key, data, null);
            while (status == OperationStatus.SUCCESS) {
                boolean oneWritten = false;
                if (evolveNeeded(key, data, binding)) {
                    cursor.putCurrent(data);
                    oneWritten = true;
                    nWritten += 1;
                }
                /* Update event stats, even if no listener. [#17024] */
                EvolveInternal.updateEvent
                    (event, entityClassName, 1, oneWritten ? 1 : 0);
                if (listener != null) {
                    if (!listener.evolveProgress(event)) {
                        break;
                    }
                }
                if (txn != null && nWritten >= WRITES_PER_TXN) {
                    cursor.close();
                    cursor = null;
                    txn.commit();
                    txn = null;
                    txn = env.beginTransaction(null, null);
                    cursor = db.openCursor(txn, cursorConfig);
                    DatabaseEntry saveKey = KeyRange.copy(key);
                    status = cursor.getSearchKeyRange(key, data, null);
                    if (status == OperationStatus.SUCCESS &&
                        KeyRange.equalBytes(key, saveKey)) {
                        status = cursor.getNext(key, data, null);
                    }
                } else {
                    status = cursor.getNext(key, data, null);
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            if (txn != null) {
                if (nWritten > 0) {
                    txn.commit();
                } else {
                    txn.abort();
                }
            }
        }
    }

    /**
     * Checks whether the given data is in the current format by translating it
     * to/from an object.  If true is returned, data is updated.
     */
    private boolean evolveNeeded(DatabaseEntry key,
                                 DatabaseEntry data,
                                 EntityBinding binding) {
        Object entity = binding.entryToObject(key, data);
        DatabaseEntry newData = new DatabaseEntry();
        binding.objectToData(entity, newData);
        if (data.equals(newData)) {
            return false;
        } else {
            byte[] bytes = newData.getData();
            int off = newData.getOffset();
            int size = newData.getSize();
            data.setData(bytes, off, size);
            return true;
        }
    }

    /**
     * For unit testing.
     */
    public static void setSyncHook(SyncHook hook) {
        syncHook = hook;
    }

    /**
     * For unit testing.
     */
    public interface SyncHook {
        void onSync(Database db, boolean flushLog);
    }
}
