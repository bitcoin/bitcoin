/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import static java.lang.annotation.ElementType.FIELD;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

/**
 * Overrides the default rules for field persistence and defines a field as
 * being non-persistent even when it is not declared with the
 * <code>transient</code> keyword.
 *
 * <p>By default, the persistent fields of a class are all declared instance
 * fields that are non-transient (are not declared with the
 * <code>transient</code> keyword).  The default rules may be overridden by
 * specifying the {@link NotPersistent} or {@link NotTransient} annotation.</p>
 *
 * <p>For example, the following field is non-transient (persistent) with
 * respect to Java serialization but is transient with respect to the DPL.</p>
 *
 * <pre style="code">
 *      {@code @NotPersistent}
 *      int myField;
 * }
 * </pre>
 *
 * @see NotTransient
 * @author Mark Hayes
 */
@Documented @Retention(RUNTIME) @Target(FIELD)
public @interface NotPersistent {
}
