/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.Map;
import java.util.Set;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.evolve.Converter;
import com.sleepycat.persist.evolve.Deleter;
import com.sleepycat.persist.evolve.Mutation;
import com.sleepycat.persist.evolve.Mutations;
import com.sleepycat.persist.evolve.Renamer;
import com.sleepycat.persist.model.SecondaryKeyMetadata;

/**
 * Evolves each old format that is still relevant if necessary, using Mutations
 * to configure deleters, renamers, and converters.
 *
 * @author Mark Hayes
 */
class Evolver {

    static final int EVOLVE_NONE = 0;
    static final int EVOLVE_NEEDED = 1;
    static final int EVOLVE_FAILURE = 2;

    private PersistCatalog catalog;
    private String storePrefix;
    private Mutations mutations;
    private Map<String,Format> newFormats;
    private boolean forceEvolution;
    private boolean disallowClassChanges;
    private boolean nestedFormatsChanged;
    private Map<Format,Format> changedFormats;
    private StringBuilder errors;
    private Set<String> deleteDbs;
    private Map<String,String> renameDbs;
    private Map<Format,Format> renameFormats;
    private Map<Integer,Boolean> evolvedFormats;
    private List<Format> unprocessedFormats;
    private Map<Format,Set<Format>> subclassMap;

    Evolver(PersistCatalog catalog,
            String storePrefix,
            Mutations mutations,
            Map<String,Format> newFormats,
            boolean forceEvolution,
            boolean disallowClassChanges) {
        this.catalog = catalog;
        this.storePrefix = storePrefix;
        this.mutations = mutations;
        this.newFormats = newFormats;
        this.forceEvolution = forceEvolution;
        this.disallowClassChanges = disallowClassChanges;
        changedFormats = new IdentityHashMap<Format,Format>();
        errors = new StringBuilder();
        deleteDbs = new HashSet<String>();
        renameDbs = new HashMap<String,String>();
        renameFormats = new IdentityHashMap<Format,Format>();
        evolvedFormats = new HashMap<Integer,Boolean>();
        unprocessedFormats = new ArrayList<Format>();
        subclassMap = catalog.getSubclassMap();
    }

    final Mutations getMutations() {
        return mutations;
    }

    /**
     * Returns whether any formats were changed during evolution, and therefore
     * need to be stored in the catalog.
     */
    boolean areFormatsChanged() {
        return !changedFormats.isEmpty();
    }

    /**
     * Returns whether the given format was changed during evolution.
     */
    boolean isFormatChanged(Format format) {
        return changedFormats.containsKey(format);
    }

    private void setFormatsChanged(Format oldFormat) {
        checkClassChangesAllowed(oldFormat);
        changedFormats.put(oldFormat, oldFormat);
        nestedFormatsChanged = true;
        /* PersistCatalog.expectNoClassChanges is true in unit tests only. */
        if (PersistCatalog.expectNoClassChanges) {
            throw new IllegalStateException("expectNoClassChanges");
        }
    }

    private void checkClassChangesAllowed(Format oldFormat) {
        if (disallowClassChanges) {
            throw new IllegalStateException
                ("When performing an upgrade changes are not allowed " +
                 "but were made to: " + oldFormat.getClassName());
        }
    }

    /**
     * Returns the set of formats for a specific superclass format, or null if
     * the superclass is not a complex type or has not subclasses.
     */
    Set<Format> getSubclassFormats(Format superFormat) {
        return subclassMap.get(superFormat);
    }

    /**
     * Returns an error string if any mutations are invalid or missing, or
     * returns null otherwise.  If non-null is returned, the store may not be
     * opened.
     */
    String getErrors() {
        if (errors.length() > 0) {
            return errors.toString();
        } else {
            return null;
        }
    }

    /**
     * Adds a newline and the given error.
     */
    private void addError(String error) {
        if (errors.length() > 0) {
            errors.append("\n---\n");
        }
        errors.append(error);
    }

    private String getClassVersionLabel(Format format, String prefix) {
        if (format != null) {
            return prefix +
                   " class: " + format.getClassName() +
                   " version: " + format.getVersion();
        } else {
            return "";
        }
    }

    /**
     * Adds a specified error when no specific mutation is involved.
     */
    void addEvolveError(Format oldFormat,
                        Format newFormat,
                        String scenario,
                        String error) {
        checkClassChangesAllowed(oldFormat);
        if (scenario == null) {
            scenario = "Error";
        }
        addError(scenario + " when evolving" +
                 getClassVersionLabel(oldFormat, "") +
                 getClassVersionLabel(newFormat, " to") +
                 " Error: " + error);
    }

    /**
     * Adds an error for an invalid mutation.
     */
    void addInvalidMutation(Format oldFormat,
                            Format newFormat,
                            Mutation mutation,
                            String error) {
        checkClassChangesAllowed(oldFormat);
        addError("Invalid mutation: " + mutation +
                 getClassVersionLabel(oldFormat, " For") +
                 getClassVersionLabel(newFormat, " New") +
                 " Error: " + error);
    }

    /**
     * Adds an error for a missing mutation.
     */
    void addMissingMutation(Format oldFormat,
                            Format newFormat,
                            String error) {
        checkClassChangesAllowed(oldFormat);
        addError("Mutation is missing to evolve" +
                 getClassVersionLabel(oldFormat, "") +
                 getClassVersionLabel(newFormat, " to") +
                 " Error: " + error);
    }

    /**
     * Called by PersistCatalog for all non-entity formats.
     */
    void addNonEntityFormat(Format oldFormat) {
        unprocessedFormats.add(oldFormat);
    }

    /**
     * Called by PersistCatalog after calling evolveFormat or
     * addNonEntityFormat for all old formats.
     *
     * We do not require deletion of an unreferenced class for two
     * reasons: 1) built-in proxy classes may not be referenced, 2) the
     * user may wish to declare persistent classes that are not yet used.
     */
    void finishEvolution() {
        /* Process unreferenced classes. */
        for (Format oldFormat : unprocessedFormats) {
            oldFormat.setUnused(true);
            evolveFormat(oldFormat);
        }
    }

    /**
     * Called by PersistCatalog for all entity formats, and by Format.evolve
     * methods for all potentially referenced non-entity formats.
     */
    boolean evolveFormat(Format oldFormat) {
        if (oldFormat.isNew()) {
            return true;
        }
        boolean result;
        Format oldEntityFormat = oldFormat.getEntityFormat();
        boolean trackEntityChanges = oldEntityFormat != null;
        boolean saveNestedFormatsChanged = nestedFormatsChanged;
        if (trackEntityChanges) {
            nestedFormatsChanged = false;
        }
        Integer oldFormatId = oldFormat.getId();
        if (evolvedFormats.containsKey(oldFormatId)) {
            result = evolvedFormats.get(oldFormatId);
        } else {
            evolvedFormats.put(oldFormatId, true);
            result = evolveFormatInternal(oldFormat);
            evolvedFormats.put(oldFormatId, result);
        }
        if (oldFormat.getLatestVersion().isNew()) {
            nestedFormatsChanged = true;
        }
        if (trackEntityChanges) {
            if (nestedFormatsChanged) {
                Format latest = oldEntityFormat.getLatestVersion();
                if (latest != null) {
                    latest.setEvolveNeeded(true);
                }
            }
            nestedFormatsChanged = saveNestedFormatsChanged;
        }
        return result;
    }

    /**
     * Tries to evolve a given existing format to the current version of the
     * class and returns false if an invalid mutation is encountered or the
     * configured mutations are not sufficient.
     */
    private boolean evolveFormatInternal(Format oldFormat) {

        /* Predefined formats and deleted classes never need evolving. */
        if (Format.isPredefined(oldFormat) || oldFormat.isDeleted()) {
            return true;
        }

        /* Get class mutations. */
        String oldName = oldFormat.getClassName();
        int oldVersion = oldFormat.getVersion();
        Renamer renamer = mutations.getRenamer(oldName, oldVersion, null);
        Deleter deleter = mutations.getDeleter(oldName, oldVersion, null);
        Converter converter =
            mutations.getConverter(oldName, oldVersion, null);
        if (deleter != null && (converter != null || renamer != null)) {
            addInvalidMutation
                (oldFormat, null, deleter,
                 "Class Deleter not allowed along with a Renamer or " +
                 "Converter for the same class");
            return false;
        }

        /*
         * For determining the new name, arrays get special treatment.  The
         * component format is evolved in the process, and we disallow muations
         * for arrays.
         */
        String newName;
        if (oldFormat.isArray()) {
            if (deleter != null || converter != null || renamer != null) {
                Mutation mutation = (deleter != null) ? deleter :
                    ((converter != null) ? converter : renamer);
                addInvalidMutation
                    (oldFormat, null, mutation,
                     "Mutations not allowed for an array");
                return false;
            }
            Format compFormat = oldFormat.getComponentType();
            if (!evolveFormat(compFormat)) {
                return false;
            }
            Format latest = compFormat.getLatestVersion();
            if (latest != compFormat) {
                newName = (latest.isArray() ? "[" : "[L") +
                           latest.getClassName() + ';';
            } else {
                newName = oldName;
            }
        } else {
            newName = (renamer != null) ? renamer.getNewName() : oldName;
        }

        /* Try to get the new class format.  Save exception for later. */
        Format newFormat;
        String newFormatException;
        try {
            Class newClass = SimpleCatalog.classForName(newName);
            try {
                newFormat = catalog.createFormat(newClass, newFormats);
                assert newFormat != oldFormat : newFormat.getClassName();
                newFormatException = null;
            } catch (Exception e) {
                newFormat = null;
                newFormatException = e.toString();
            }
        } catch (ClassNotFoundException e) {
            newFormat = null;
            newFormatException = e.toString();
        }

        if (newFormat != null) {

            /*
             * If the old format is not the existing latest version and the new
             * format is not an existing format, then we must evolve the latest
             * old version to the new format first.  We cannot evolve old
             * format to a new format that may be discarded because it is equal
             * to the latest existing format (which will remain the current
             * version).
             */
            if (oldFormat != oldFormat.getLatestVersion() &&
                newFormat.getPreviousVersion() == null) {
                assert newFormats.containsValue(newFormat);
                Format oldLatestFormat = oldFormat.getLatestVersion();
                if (!evolveFormat(oldLatestFormat)) {
                    return false;
                }
                if (oldLatestFormat == oldLatestFormat.getLatestVersion()) {
                    assert !newFormats.containsValue(newFormat) : newFormat;
                    /* newFormat equals oldLatestFormat and was discarded. */
                    newFormat = oldLatestFormat;
                }
            }

            /*
             * If the old format was previously evolved to the new format
             * (which means the new format is actually an existing format),
             * then there is nothing to do.  This is the case where no class
             * changes were made.
             *
             * However, if mutations were specified when opening the catalog
             * that are different than the mutations last used, then we must
             * force the re-evolution of all old formats.
             */
            if (!forceEvolution &&
                newFormat == oldFormat.getLatestVersion()) {
                return true;
            }
        }

        /* Apply class Renamer and continue if successful. */
        if (renamer != null) {
            if (!applyClassRenamer(renamer, oldFormat, newFormat)) {
                return false;
            }
        }

        /* Apply class Converter and return. */
        if (converter != null) {
            if (oldFormat.isEntity()) {
                if (newFormat == null || !newFormat.isEntity()) {
                    addInvalidMutation
                        (oldFormat, newFormat, deleter,
                         "Class converter not allowed for an entity class " +
                         "that is no longer present or not having an " +
                         "@Entity annotation");
                    return false;
                }
                if (!oldFormat.evolveMetadata(newFormat, converter, this)) {
                    return false;
                }
            }
            return applyConverter(converter, oldFormat, newFormat);
        }

        /* Apply class Deleter and return. */
        boolean needDeleter =
            (newFormat == null) ||
            (newFormat.isEntity() != oldFormat.isEntity());
        if (deleter != null) {
            if (!needDeleter) {
                addInvalidMutation
                    (oldFormat, newFormat, deleter,
                     "Class deleter not allowed when the class and its " +
                     "@Entity or @Persistent annotation is still present");
                return false;
            }
            return applyClassDeleter(deleter, oldFormat, newFormat);
        } else {
            if (needDeleter) {
                if (newFormat == null) {
                    assert newFormatException != null;
		    /* FindBugs newFormat known to be null excluded. */
                    addMissingMutation
                        (oldFormat, newFormat, newFormatException);
                } else {
                    addMissingMutation
                        (oldFormat, newFormat,
                         "@Entity switched to/from @Persistent");
                }
                return false;
            }
        }

        /*
         * Class-level mutations have been applied.  Now apply field mutations
         * (for complex classes) or special conversions (enum conversions, for
         * example) by calling the old format's evolve method.
         */
        return oldFormat.evolve(newFormat, this);
    }

    /**
     * Use the old format and discard the new format.  Called by
     * Format.evolve when the old and new formats are identical.
     */
    void useOldFormat(Format oldFormat, Format newFormat) {
        Format renamedFormat = renameFormats.get(oldFormat);
        if (renamedFormat != null) {

            /*
             * The format was renamed but, because this method is called, we
             * know that no other class changes were made.  Use the new/renamed
             * format as the reader.
             */
            assert renamedFormat == newFormat;
            useEvolvedFormat(oldFormat, renamedFormat, renamedFormat);
        } else if (newFormat != null &&
                   (oldFormat.getVersion() != newFormat.getVersion() ||
                    !oldFormat.isCurrentVersion())) {

            /*
             * If the user wants a new version number, but ther are no other
             * changes, we will oblige.  Or, if an attempt is being made to
             * use an old version, then the following events happened and we
             * must evolve the old format:
             * 1) The (previously) latest version of the format was evolved
             * because it is not equal to the live class version.  Note that
             * evolveFormatInternal always evolves the latest version first.
             * 2) We are now attempting to evolve an older version of the same
             * format, and it happens to be equal to the live class version.
             * However, we're already committed to the new format, and we must
             * evolve all versions.
             * [#16467]
             */
            useEvolvedFormat(oldFormat, newFormat, newFormat);
        } else {
            /* The new format is discarded. */
            catalog.useExistingFormat(oldFormat);
            if (newFormat != null) {
                newFormats.remove(newFormat.getClassName());
            }
        }
    }

    /**
     * Install an evolver Reader in the old format.  Called by Format.evolve
     * when the old and new formats are not identical.
     */
    void useEvolvedFormat(Format oldFormat,
                          Reader evolveReader,
                          Format newFormat) {
        oldFormat.setReader(evolveReader);
        if (newFormat != null) {
            oldFormat.setLatestVersion(newFormat);
        }
        setFormatsChanged(oldFormat);
    }

    private boolean applyClassRenamer(Renamer renamer,
                                      Format oldFormat,
                                      Format newFormat) {
        if (!checkUpdatedVersion(renamer, oldFormat, newFormat)) {
            return false;
        }
        if (oldFormat.isEntity() && oldFormat.isCurrentVersion()) {
            String newClassName = newFormat.getClassName();
            String oldClassName = oldFormat.getClassName();
            /* Queue the renaming of the primary and secondary databases. */
            renameDbs.put
                (Store.makePriDbName(storePrefix, oldClassName),
                 Store.makePriDbName(storePrefix, newClassName));
            for (SecondaryKeyMetadata keyMeta :
                 oldFormat.getEntityMetadata().getSecondaryKeys().values()) {
                String keyName = keyMeta.getKeyName();
                renameDbs.put
                    (Store.makeSecDbName(storePrefix, oldClassName, keyName),
                     Store.makeSecDbName(storePrefix, newClassName, keyName));
            }
        }

        /*
         * Link the old format to the renamed format so that we can detect the
         * rename in useOldFormat.
         */
        renameFormats.put(oldFormat, newFormat);

        setFormatsChanged(oldFormat);
        return true;
    }

    /**
     * Called by ComplexFormat when a secondary key name is changed.
     */
    void renameSecondaryDatabase(String oldEntityClass,
                                 String newEntityClass,
                                 String oldKeyName,
                                 String newKeyName) {
        renameDbs.put
            (Store.makeSecDbName(storePrefix, oldEntityClass, oldKeyName),
             Store.makeSecDbName(storePrefix, newEntityClass, newKeyName));
    }

    private boolean applyClassDeleter(Deleter deleter,
                                      Format oldFormat,
                                      Format newFormat) {
        if (!checkUpdatedVersion(deleter, oldFormat, newFormat)) {
            return false;
        }
        if (oldFormat.isEntity() && oldFormat.isCurrentVersion()) {
            /* Queue the deletion of the primary and secondary databases. */
            String className = oldFormat.getClassName();
            deleteDbs.add(Store.makePriDbName(storePrefix, className));
            for (SecondaryKeyMetadata keyMeta :
                 oldFormat.getEntityMetadata().getSecondaryKeys().values()) {
                deleteDbs.add(Store.makeSecDbName
                    (storePrefix, className, keyMeta.getKeyName()));
            }
        }

        /*
         * Set the format to deleted last, so that the above test using
         * isCurrentVersion works properly.
         */
        oldFormat.setDeleted(true);
        if (newFormat != null) {
            oldFormat.setLatestVersion(newFormat);
        }

        setFormatsChanged(oldFormat);
        return true;
    }

    /**
     * Called by ComplexFormat when a secondary key is dropped.
     */
    void deleteSecondaryDatabase(String oldEntityClass, String keyName) {
        deleteDbs.add(Store.makeSecDbName
            (storePrefix, oldEntityClass, keyName));
    }

    private boolean applyConverter(Converter converter,
                                   Format oldFormat,
                                   Format newFormat) {
        if (!checkUpdatedVersion(converter, oldFormat, newFormat)) {
            return false;
        }
        Reader reader = new ConverterReader(converter);
        useEvolvedFormat(oldFormat, reader, newFormat);
        return true;
    }

    boolean isClassConverted(Format format) {
        return format.getReader() instanceof ConverterReader;
    }

    private boolean checkUpdatedVersion(Mutation mutation,
                                        Format oldFormat,
                                        Format newFormat) {
        if (newFormat != null &&
            !oldFormat.isEnum() &&
            newFormat.getVersion() <= oldFormat.getVersion()) {
            addInvalidMutation
                (oldFormat, newFormat, mutation,
                 "A new higher version number must be assigned");
            return false;
        } else {
            return true;
        }
    }

    boolean checkUpdatedVersion(String scenario,
                                Format oldFormat,
                                Format newFormat) {
        if (newFormat != null &&
            !oldFormat.isEnum() &&
            newFormat.getVersion() <= oldFormat.getVersion()) {
            addEvolveError
                (oldFormat, newFormat, scenario,
                 "A new higher version number must be assigned");
            return false;
        } else {
            return true;
        }
    }

    void renameAndRemoveDatabases(Store store, Transaction txn)
        throws DatabaseException {

        for (String dbName : deleteDbs) {
            String[] fileAndDbNames = store.parseDbName(dbName);
            /* Do nothing if database does not exist. */
            DbCompat.removeDatabase
                (store.getEnvironment(), txn,
                 fileAndDbNames[0], fileAndDbNames[1]);
        }
        for (Map.Entry<String,String> entry : renameDbs.entrySet()) {
            String oldName = entry.getKey();
            String newName = entry.getValue();
            String[] oldFileAndDbNames = store.parseDbName(oldName);
            String[] newFileAndDbNames = store.parseDbName(newName);
            /* Do nothing if database does not exist. */
            DbCompat.renameDatabase
                (store.getEnvironment(), txn,
                 oldFileAndDbNames[0], oldFileAndDbNames[1],
                 newFileAndDbNames[0], newFileAndDbNames[1]);
        }
    }

    /**
     * Evolves a primary key field or composite key field.
     */
    int evolveRequiredKeyField(Format oldParent,
                               Format newParent,
                               FieldInfo oldField,
                               FieldInfo newField) {
        int result = EVOLVE_NONE;
        String oldName = oldField.getName();
        final String FIELD_KIND =
            "primary key field or composite key class field";
        final String FIELD_LABEL =
            FIELD_KIND + ": " + oldName;

        if (newField == null) {
            addMissingMutation
                (oldParent, newParent,
                 "Field is missing and deletion is not allowed for a " +
                 FIELD_LABEL);
            return EVOLVE_FAILURE;
        }

        /* Check field mutations.  Only a Renamer is allowed. */
        Deleter deleter = mutations.getDeleter
            (oldParent.getClassName(), oldParent.getVersion(), oldName);
        if (deleter != null) {
            addInvalidMutation
                (oldParent, newParent, deleter,
                 "Deleter is not allowed for a " + FIELD_LABEL);
            return EVOLVE_FAILURE;
        }
        Converter converter = mutations.getConverter
            (oldParent.getClassName(), oldParent.getVersion(), oldName);
        if (converter != null) {
            addInvalidMutation
                (oldParent, newParent, converter,
                 "Converter is not allowed for a " + FIELD_LABEL);
            return EVOLVE_FAILURE;
        }
        Renamer renamer = mutations.getRenamer
            (oldParent.getClassName(), oldParent.getVersion(), oldName);
        String newName = newField.getName();
        if (renamer != null) {
            if (!renamer.getNewName().equals(newName)) {
                addInvalidMutation
                    (oldParent, newParent, converter,
                     "Converter is not allowed for a " + FIELD_LABEL);
                return EVOLVE_FAILURE;
            }
            result = EVOLVE_NEEDED;
        } else {
            if (!oldName.equals(newName)) {
                addMissingMutation
                    (oldParent, newParent,
                     "Renamer is required when field name is changed from: " +
                     oldName + " to: " + newName);
                return EVOLVE_FAILURE;
            }
        }

        /*
         * Evolve the declared version of the field format.
         */
        Format oldFieldFormat = oldField.getType();
        if (!evolveFormat(oldFieldFormat)) {
            return EVOLVE_FAILURE;
        }
        Format oldLatestFormat = oldFieldFormat.getLatestVersion();
        if (oldFieldFormat != oldLatestFormat) {
            result = EVOLVE_NEEDED;
        }
        Format newFieldFormat = newField.getType();

        if (oldLatestFormat.getClassName().equals
                (newFieldFormat.getClassName())) {
            /* Formats are identical. */
            return result;
        } else if ((oldLatestFormat.getWrapperFormat() != null &&
                    oldLatestFormat.getWrapperFormat().getId() ==
                    newFieldFormat.getId()) ||
                   (newFieldFormat.getWrapperFormat() != null &&
                    newFieldFormat.getWrapperFormat().getId() ==
                    oldLatestFormat.getId())) {
            /* Primitive <-> primitive wrapper type change. */
            return EVOLVE_NEEDED;
        } else {
            /* Type was changed incompatibly. */
            addEvolveError
                (oldParent, newParent,
                 "Type may not be changed for a " + FIELD_KIND,
                 "Old field type: " + oldLatestFormat.getClassName() +
                 " is not compatible with the new type: " +
                 newFieldFormat.getClassName() +
                 " for field: " + oldName);
            return EVOLVE_FAILURE;
        }
    }
}
