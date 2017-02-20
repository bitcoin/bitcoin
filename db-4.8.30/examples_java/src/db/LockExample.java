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
import java.util.Vector;

//
// An example of a program using Lock and related classes.
//
class LockExample {
    private static final String progname = "LockExample";
    private static final File LOCK_HOME = new File("TESTDIR");
    Environment dbenv;

    public LockExample(File home, int maxlocks, boolean do_unlink)
        throws DatabaseException, FileNotFoundException {

        if (do_unlink) {
            Environment.remove(home, true, null);
        }

        EnvironmentConfig config = new EnvironmentConfig();
        config.setErrorStream(System.err);
        config.setErrorPrefix("LockExample");
        config.setMaxLocks(maxlocks);
        config.setAllowCreate(true);
        config.setInitializeLocking(true);
        dbenv = new Environment(home, config);
    }

    public void close() throws DatabaseException {
        dbenv.close();
    }

    // Prompts for a line, returning the default answer if a blank
    // line is entered.
    //
    public static String askForLine(InputStreamReader reader,
                                    PrintStream out, String prompt,
                                    String defaultAnswer) {
        String result;
        do {
            if (defaultAnswer != null)
                out.print(prompt + " [" + defaultAnswer + "] >");
            else
                out.print(prompt);
            out.flush();
            result = getLine(reader);
            if (result == null || result.length() == 0)
                result = defaultAnswer;
        } while (result == null);
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
            while ((c = reader.read()) != -1 && c != '\n')
                if (c != '\r')
                    b.append((char)c);
        } catch (IOException ioe) {
            c = -1;
        }

        if (c == -1 && b.length() == 0)
            return null;
        else
            return b.toString();
    }

    public void run() throws DatabaseException {
        long held;
        int len = 0, locker;
        int ret;
        boolean did_get = false;
        int lockid = 0;
        InputStreamReader in = new InputStreamReader(System.in);
        Vector locks = new Vector();

        //
        // Accept lock requests.
        //
        locker = dbenv.createLockerID();
        for (held = 0;;) {
            String opbuf = askForLine(in, System.out,
                                      "Operation get/release", "get");
            if (opbuf == null)
                break;

            try {
                if (opbuf.equals("get") || opbuf.equals("vget")) {
                    // Acquire a lock.
                    String objbuf = askForLine(in, System.out,
                           "input object (text string) to lock> ", null);
                    if (objbuf == null)
                        break;

                    String lockbuf;
                    do {
                        lockbuf = askForLine(in, System.out,
                                             "lock type read/write", "read");
                        if (lockbuf == null)
                            break;
                        len = lockbuf.length();
                    } while (len >= 1 &&
                             !lockbuf.equals("read") &&
                             !lockbuf.equals("write"));

                    LockRequestMode lock_type;
                    if (len <= 1 || lockbuf.equals("read"))
                        lock_type = LockRequestMode.READ;
                    else
                        lock_type = LockRequestMode.WRITE;

                    DatabaseEntry entry = new DatabaseEntry(objbuf.getBytes());

                    Lock lock;
                    if (opbuf.equals("get")) {
                        did_get = true;
                        lock = dbenv.getLock(locker, true, entry, lock_type);
                    } else {
                        LockRequest req = new LockRequest(
                            LockOperation.GET, lock_type, entry, null);
                        LockRequest[] reqs = { req };
                        dbenv.lockVector(locker, true, reqs);
                        lock = req.getLock();
                        System.out.println("Got lock: " + lock);
                    }
                    lockid = locks.size();
                    locks.addElement(lock);
                } else {
                    // Release a lock.
                    String objbuf;
                    objbuf = askForLine(in, System.out,
                                        "input lock to release> ", null);
                    if (objbuf == null)
                        break;

                    lockid = Integer.parseInt(objbuf, 16);
                    if (lockid < 0 || lockid >= locks.size()) {
                        System.out.println("Lock #" + lockid + " out of range");
                        continue;
                    }
                    did_get = false;
                    Lock lock = (Lock)locks.elementAt(lockid);
                    dbenv.putLock(lock);
                }
                System.out.println("Lock #" + lockid + " " +
                                   (did_get ? "granted" : "released"));
                held += did_get ? 1 : -1;
            } catch (LockNotGrantedException lnge) {
                System.err.println("Lock not granted");
            } catch (DeadlockException de) {
                System.err.println("LockExample: lock_" +
                                   (did_get ? "get" : "put") +
                                   ": returned DEADLOCK");
            } catch (DatabaseException dbe) {
                System.err.println("LockExample: lock_get: " + dbe.toString());
            }
        }
        System.out.println();
        System.out.println("Closing lock region " + String.valueOf(held) +
                           " locks held");
    }

    private static void usage() {
        System.err.println("usage: LockExample [-u] [-h home] [-m maxlocks]");
        System.exit(1);
    }

    public static void main(String[] argv) {
        File home = LOCK_HOME;
        boolean do_unlink = false;
        int maxlocks = 0;

        for (int i = 0; i < argv.length; ++i) {
            if (argv[i].equals("-h")) {
                if (++i >= argv.length)
                    usage();
                home = new File(argv[i]);
            } else if (argv[i].equals("-m")) {
                if (++i >= argv.length)
                    usage();

                try {
                    maxlocks = Integer.parseInt(argv[i]);
                } catch (NumberFormatException nfe) {
                    usage();
                }
            } else if (argv[i].equals("-u")) {
                do_unlink = true;
            } else {
                usage();
            }
        }

        try {
            LockExample app = new LockExample(home, maxlocks, do_unlink);
            app.run();
            app.close();
        } catch (DatabaseException dbe) {
            System.err.println(progname + ": " + dbe.toString());
        } catch (Throwable t) {
            System.err.println(progname + ": " + t.toString());
        }
        System.out.println("LockExample completed");
    }
}
