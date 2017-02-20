/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.evolve;

/**
 * A class has been changed incompatibly and no mutation has been configured to
 * handle the change or a new class version number has not been assigned.
 *
 * @see com.sleepycat.persist.EntityStore#EntityStore EntityStore.EntityStore
 * @see com.sleepycat.persist.model.Entity#version
 * @see com.sleepycat.persist.model.Persistent#version
 *
 * @see com.sleepycat.persist.evolve Class Evolution
 * @author Mark Hayes
 */
public class IncompatibleClassException extends RuntimeException {

    private static final long serialVersionUID = 2103957824L;

    public IncompatibleClassException(String message) {
        super(message);
    }
}
