/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.util;

/**
 * Unwraps nested exceptions by calling the {@link
 * ExceptionWrapper#getCause()} method for exceptions that implement the
 * {@link ExceptionWrapper} interface.  Does not currently support the Java 1.4
 * <code>Throwable.getCause()</code> method.
 *
 * @author Mark Hayes
 */
public class ExceptionUnwrapper {

    /**
     * Unwraps an Exception and returns the underlying Exception, or throws an
     * Error if the underlying Throwable is an Error.
     *
     * @param e is the Exception to unwrap.
     *
     * @return the underlying Exception.
     *
     * @throws Error if the underlying Throwable is an Error.
     *
     * @throws IllegalArgumentException if the underlying Throwable is not an
     * Exception or an Error.
     */
    public static Exception unwrap(Exception e) {

        Throwable t = unwrapAny(e);
        if (t instanceof Exception) {
            return (Exception) t;
        } else if (t instanceof Error) {
            throw (Error) t;
        } else {
            throw new IllegalArgumentException("Not Exception or Error: " + t);
        }
    }

    /**
     * Unwraps an Exception and returns the underlying Throwable.
     *
     * @param e is the Exception to unwrap.
     *
     * @return the underlying Throwable.
     */
    public static Throwable unwrapAny(Throwable e) {

        while (true) {
            if (e instanceof ExceptionWrapper) {
                Throwable e2 = ((ExceptionWrapper) e).getCause();
                if (e2 == null) {
                    return e;
                } else {
                    e = e2;
                }
            } else {
                return e;
            }
        }
    }
}
