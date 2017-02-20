/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.util.Set;

import com.sleepycat.persist.model.ClassMetadata;
import com.sleepycat.persist.model.EntityMetadata;
import com.sleepycat.persist.model.EntityModel;

/**
 * The EntityModel used when a RawStore is opened.  The metadata and raw type
 * information comes from the catalog directly, without using the current
 * class definitions.
 *
 * @author Mark Hayes
 */
class StoredModel extends EntityModel {

    private PersistCatalog catalog;
    private Set<String> knownClasses;

    StoredModel(PersistCatalog catalog) {
        this.catalog = catalog;
    }

    @Override
    public ClassMetadata getClassMetadata(String className) {
        ClassMetadata metadata = null;
        Format format = catalog.getFormat(className);
        if (format != null && format.isCurrentVersion()) {
            metadata = format.getClassMetadata();
        }
        return metadata;
    }

    @Override
    public EntityMetadata getEntityMetadata(String className) {
        EntityMetadata metadata = null;
        Format format = catalog.getFormat(className);
        if (format != null && format.isCurrentVersion()) {
            metadata = format.getEntityMetadata();
        }
        return metadata;
    }

    @Override
    public Set<String> getKnownClasses() {
        if (knownClasses == null) {
            knownClasses = catalog.getModelClasses();
        }
        return knownClasses;
    }
}
