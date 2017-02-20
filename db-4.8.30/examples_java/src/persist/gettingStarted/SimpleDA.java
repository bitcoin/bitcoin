/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

package persist.gettingStarted;

import java.io.File;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.EntityCursor;

public class SimpleDA {
    // Open the indices
    public SimpleDA(EntityStore store)
        throws DatabaseException {

        // Primary key for SimpleEntityClass classes
        pIdx = store.getPrimaryIndex(
            String.class, SimpleEntityClass.class);

        // Secondary key for SimpleEntityClass classes
        // Last field in the getSecondaryIndex() method must be
        // the name of a class member; in this case, an 
        // SimpleEntityClass.class data member.
        sIdx = store.getSecondaryIndex(
            pIdx, String.class, "sKey");

        sec_pcursor = pIdx.entities();
        sec_scursor = sIdx.subIndex("skeyone").entities();
    }

    public void close() 
        throws DatabaseException {
            sec_pcursor.close();
            sec_scursor.close();
    }

    // Index Accessors
    PrimaryIndex<String,SimpleEntityClass> pIdx;
    SecondaryIndex<String,String,SimpleEntityClass> sIdx;

    EntityCursor<SimpleEntityClass> sec_pcursor;
    EntityCursor<SimpleEntityClass> sec_scursor;
} 
