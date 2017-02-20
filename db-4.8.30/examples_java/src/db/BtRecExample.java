/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */


package db;

import com.sleepycat.db.*;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.PrintStream;

public class BtRecExample {
    static final String progname =  "BtRecExample"; // Program name.
    static final String database =  "access.db";
    static final String wordlist =  "../test/wordlist";

    BtRecExample(BufferedReader reader)
        throws DatabaseException, IOException, FileNotFoundException {

        OperationStatus status;

        // Remove the previous database.
        File f = new File(database);
        f.delete();

        DatabaseConfig config = new DatabaseConfig();

        config.setErrorStream(System.err);
        config.setErrorPrefix(progname);
        config.setPageSize(1024);           // 1K page sizes.

        config.setBtreeRecordNumbers(true);
        config.setType(DatabaseType.BTREE);
        config.setAllowCreate(true);
        db = new Database(database, null, config);

        //
        // Insert records into the database, where the key is the word
        // preceded by its record number, and the data is the same, but
        // in reverse order.
        //

        for (int cnt = 1; cnt <= 1000; ++cnt) {
            String numstr = String.valueOf(cnt);
            while (numstr.length() < 4)
                numstr = "0" + numstr;
            String buf = numstr + '_' + reader.readLine();
            StringBuffer rbuf = new StringBuffer(buf).reverse();

            StringEntry key = new StringEntry(buf);
            StringEntry data = new StringEntry(rbuf.toString());

            status = db.putNoOverwrite(null, key, data);
            if (status != OperationStatus.SUCCESS &&
                status!= OperationStatus.KEYEXIST)
                throw new DatabaseException("Database.put failed " + status);
        }
    }

    void run() throws DatabaseException {
        int recno;
        OperationStatus status;

        // Acquire a cursor for the database.
        cursor = db.openCursor(null, null);

        //
        // Prompt the user for a record number, then retrieve and display
        // that record.
        //
        InputStreamReader reader = new InputStreamReader(System.in);

        for (;;) {
            // Get a record number.
            String line = askForLine(reader, System.out, "recno #> ");
            if (line == null)
                break;

            try {
                recno = Integer.parseInt(line);
            } catch (NumberFormatException nfe) {
                System.err.println("Bad record number: " + nfe);
                continue;
            }

            //
            // Start with a fresh key each time, the db.get() routine returns
            // the key and data pair, not just the key!
            //
            RecnoStringEntry key = new RecnoStringEntry(recno, 4);
            RecnoStringEntry data = new RecnoStringEntry(4);

            status = cursor.getSearchRecordNumber(key, data, null);
            if (status != OperationStatus.SUCCESS)
                throw new DatabaseException("Cursor.setRecno failed: " + status);

            // Display the key and data.
            show("k/d\t", key, data);

            // Move the cursor a record forward.
            status = cursor.getNext(key, data, null);
            if (status != OperationStatus.SUCCESS)
                throw new DatabaseException("Cursor.getNext failed: " + status);

            // Display the key and data.
            show("next\t", key, data);

            RecnoStringEntry datano = new RecnoStringEntry(4);

            //
            // Retrieve the record number for the following record into
            // local memory.
            //
            status = cursor.getRecordNumber(datano, null);
            if (status != OperationStatus.SUCCESS &&
                status != OperationStatus.NOTFOUND &&
                status != OperationStatus.KEYEMPTY)
                throw new DatabaseException("Cursor.get failed: " + status);
            else {
                recno = datano.getRecordNumber();
                System.out.println("retrieved recno: " + recno);
            }
        }

        cursor.close();
        cursor = null;
    }

    //
    // Print out the number of records in the database.
    //
    void stats() throws DatabaseException {
        BtreeStats stats;

        stats = (BtreeStats)db.getStats(null, null);
        System.out.println(progname + ": database contains " +
               stats.getNumData() + " records");
    }

    void show(String msg, RecnoStringEntry key, RecnoStringEntry data)
        throws DatabaseException {

        System.out.println(msg + key.getString() + ": " + data.getString());
    }

    public void shutdown() throws DatabaseException {
        if (cursor != null) {
            cursor.close();
            cursor = null;
        }
        if (db != null) {
            db.close();
            db = null;
        }
    }

    public static void main(String[] argv) {
        try {
            // Open the word database.
            FileReader freader = new FileReader(wordlist);

            BtRecExample app = new BtRecExample(new BufferedReader(freader));

            // Close the word database.
            freader.close();
            freader = null;

            app.stats();
            app.run();
        } catch (FileNotFoundException fnfe) {
            System.err.println(progname + ": unexpected open error " + fnfe);
            System.exit (1);
        } catch (IOException ioe) {
            System.err.println(progname + ": open " + wordlist + ": " + ioe);
            System.exit (1);
        } catch (DatabaseException dbe) {
            System.err.println("Exception: " + dbe);
            System.exit(dbe.getErrno());
        }

        System.exit(0);
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

    private Cursor cursor;
    private Database db;

    // Here's an example of how you can extend DatabaseEntry in a
    // straightforward way to allow easy storage/retrieval of strings.
    // We've declared it as a static inner class, but it need not be.
    //
    static class StringEntry extends DatabaseEntry {
        StringEntry() {}

        StringEntry(String value) {
            setString(value);
        }

        void setString(String value) {
            byte[] data = value.getBytes();
            setData(data);
            setSize(data.length);
        }

        String getString() {
            return new String(getData(), 0, getSize());
        }
    }

    // Here's an example of how you can extend DatabaseEntry to store
    // (potentially) both recno's and strings in the same structure.
    //
    static class RecnoStringEntry extends DatabaseEntry {
        RecnoStringEntry(int maxsize) {
            this(0, maxsize);     // let other constructor do most of the work
        }

        RecnoStringEntry(int value, int maxsize) {
            arr = new byte[maxsize];
            setRecordNumber(value);
            setSize(arr.length);
        }

        RecnoStringEntry(String value) {
            byte[] data = value.getBytes();
            setData(data);                // use our local array for data
            setUserBuffer(data.length, true);
        }

        void setString(String value) {
            byte[] data = value.getBytes();
            setData(data);
            setSize(data.length);
        }

        String getString() {
            return new String(getData(), getOffset(), getSize());
        }

        byte[] arr;
    }
}
