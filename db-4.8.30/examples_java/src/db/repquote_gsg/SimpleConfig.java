/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package db.repquote_gsg;

public class SimpleConfig
{
    // Constant values used in the RepQuote application.
    public static final String progname = "SimpleTxn";
    public static final int CACHESIZE = 10 * 1024 * 1024;

    // Member variables containing configuration information.
    public String home; // String specifying the home directory for 
                        // rep files.

    public SimpleConfig()
    {
        home = "";
    }

    public java.io.File getHome()
    {
        return new java.io.File(home);
    }

}

