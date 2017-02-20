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
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.PrintStream;

class AccessExample {
    private static final int EXIT_SUCCESS = 0;
    private static final int EXIT_FAILURE = 1;

    public AccessExample() {
    }

    public static void usage() {
        System.out.println("usage: java " +
               "db.AccessExample [-r] [database]\n");
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
            AccessExample app = new AccessExample();
            app.run(removeExistingDatabase, databaseName);
        } catch (DatabaseException dbe) {
            System.err.println("AccessExample: " + dbe.toString());
            System.exit(EXIT_FAILURE);
        } catch (FileNotFoundException fnfe) {
            System.err.println("AccessExample: " + fnfe.toString());
            System.exit(EXIT_FAILURE);
        }
        System.exit(EXIT_SUCCESS);
    }

    // Prompts for a line, and keeps prompting until a non blank
    // line is returned.  Returns null on error.
    //
    public static String askForLine(InputStreamReader reader,
                                    PrintStream out, String prompt) {
        String result = "";
        while (result != null && result.length() == 0) {
            out.print(prompt);
            out.flush();
            result = getLine(reader);
        }
        return result;
    }

    // Not terribly efficient, but does the job.
    // Works for reading a line from stdin or a file.
    // Returns null on EOF.  If EOF appears in the middle
    // of a line, returns that line, then null on next call.
    //
    public static String getLine(InputStreamReader reader) {
        StringBuffer b = new StringBuffer();
        int c;
        try {
            while ((c = reader.read()) != -1 && c != '\n') {
                if (c != '\r')
                    b.append((char)c);
            }
        } catch (IOException ioe) {
            c = -1;
        }

        if (c == -1 && b.length() == 0)
            return null;
        else
            return b.toString();
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
        dbConfig.setErrorPrefix("AccessExample");
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);
        Database table = new Database(databaseName, null, dbConfig);

        //
        // Insert records into the database, where the key is the user
        // input and the data is the user input in reverse order.
        //
        InputStreamReader reader = new InputStreamReader(System.in);

        for (;;) {
            String line = askForLine(reader, System.out, "input> ");
            if (line == null)
                break;

            String reversed = (new StringBuffer(line)).reverse().toString();

            // See definition of StringDbt below
            //
            StringEntry key = new StringEntry(line);
            StringEntry data = new StringEntry(reversed);

            try {
                if (table.putNoOverwrite(null, key, data) == OperationStatus.KEYEXIST)
                    System.out.println("Key " + line + " already exists.");
            } catch (DatabaseException dbe) {
                System.out.println(dbe.toString());
            }
        }

        // Acquire an iterator for the table.
        Cursor cursor;
        cursor = table.openCursor(null, null);

        // Walk through the table, printing the key/data pairs.
        // See class StringDbt defined below.
        //
        StringEntry key = new StringEntry();
        StringEntry data = new StringEntry();
        while (cursor.getNext(key, data, null) == OperationStatus.SUCCESS)
            System.out.println(key.getString() + " : " + data.getString());
        cursor.close();
        table.close();
    }

    // Here's an example of how you can extend DatabaseEntry in a
    // straightforward way to allow easy storage/retrieval of strings,
    // or whatever kind of data you wish.  We've declared it as a static
    // inner class, but it need not be.
    //
    static /*inner*/
    class StringEntry extends DatabaseEntry {
        StringEntry() {
        }

        StringEntry(String value) {
            setString(value);
        }

        void setString(String value) {
            byte[] data = value.getBytes();
            setData(data);
            setSize(data.length);
        }

        String getString() {
            return new String(getData(), getOffset(), getSize());
        }
    }
}
