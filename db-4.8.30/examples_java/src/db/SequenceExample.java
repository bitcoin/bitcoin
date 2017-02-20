/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package db;

import com.sleepycat.db.*;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintStream;

class SequenceExample {
    private static final int EXIT_SUCCESS = 0;
    private static final int EXIT_FAILURE = 1;

    public SequenceExample() {
    }

    public static void usage() {
        System.out.println("usage: java " +
               "db.SequenceExample [-r] [database]\n");
        System.exit(EXIT_FAILURE);
    }

    public static void main(String[] argv) {
        boolean removeExistingDatabase = false;
        String databaseName = "access.db";

        for (int i = 0; i < argv.length; i++) {
            if (argv[i].equals("-r"))
                removeExistingDatabase = true;
            else if (argv[i].equals("-?"))
                usage();
            else if (argv[i].startsWith("-"))
                usage();
            else {
                if ((argv.length - i) != 1)
                    usage();
                databaseName = argv[i];
                break;
            }
        }

        try {
            SequenceExample app = new SequenceExample();
            app.run(removeExistingDatabase, databaseName);
        } catch (DatabaseException dbe) {
            System.err.println("SequenceExample: " + dbe.toString());
            System.exit(EXIT_FAILURE);
        } catch (FileNotFoundException fnfe) {
            System.err.println("SequenceExample: " + fnfe.toString());
            System.exit(EXIT_FAILURE);
        }
        System.exit(EXIT_SUCCESS);
    }

    public void run(boolean removeExistingDatabase, String databaseName)
        throws DatabaseException, FileNotFoundException {

        // Remove the previous database.
        if (removeExistingDatabase)
            new File(databaseName).delete();

        // Create the database object.
        // There is no environment for this simple example.
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setErrorStream(System.err);
        dbConfig.setErrorPrefix("SequenceExample");
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);
        Database table = new Database(databaseName, null, dbConfig);

        SequenceConfig config = new SequenceConfig();
        config.setAllowCreate(true);
        DatabaseEntry key =
            new DatabaseEntry("my_sequence".getBytes());
        Sequence sequence = table.openSequence(null, key, config);

        for (int i = 0; i < 10; i++) {
            long seqnum = sequence.get(null, 1);
            System.out.println("Got sequence number: " + seqnum);
        }

        sequence.close();
        table.close();
    }
}
