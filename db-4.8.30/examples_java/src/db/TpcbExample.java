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
import java.math.BigDecimal;
import java.util.Calendar;
import java.util.Date;
import java.util.Random;
import java.util.GregorianCalendar;

//
// This program implements a basic TPC/B driver program.  To create the
// TPC/B database, run with the -i (init) flag.  The number of records
// with which to populate the account, history, branch, and teller tables
// is specified by the a, s, b, and t flags respectively.  To run a TPC/B
// test, use the n flag to indicate a number of transactions to run in
// each thread and -T to specify the number of threads.
//
class TpcbExample {
    public static final int TELLERS_PER_BRANCH = 10;
    public static final int ACCOUNTS_PER_TELLER = 10000;
    public static final int HISTORY_PER_BRANCH = 2592000;

    //
    // The default configuration that adheres to TPCB scaling rules requires
    // nearly 3 GB of space.  To avoid requiring that much space for testing,
    // we set the parameters much lower.  If you want to run a valid 10 TPS
    // configuration, uncomment the VALID_SCALING configuration
    //

    // VALID_SCALING configuration
    /*
      public static final int ACCOUNTS = 1000000;
      public static final int BRANCHES = 10;
      public static final int TELLERS = 100;
      public static final int HISTORY = 25920000;
    */

    // TINY configuration
    /*
      public static final int ACCOUNTS = 1000;
      public static final int BRANCHES = 10;
      public static final int TELLERS = 100;
      public static final int HISTORY = 10000;
    */

    // Default configuration
    public static final int ACCOUNTS = 100000;
    public static final int BRANCHES = 10;
    public static final int TELLERS = 100;
    public static final int HISTORY = 259200;

    public static final int HISTORY_LEN = 100;
    public static final int RECLEN = 100;
    public static final int BEGID = 1000000;

    // used by random_id()
    public static final int ACCOUNT = 0;
    public static final int BRANCH = 1;
    public static final int TELLER = 2;

    public static boolean verbose = false;
    public static final String progname = "TpcbExample";    // Program name.

    Environment dbenv;
    int accounts, branches, tellers, history;

    public TpcbExample(File home,
                       int accounts, int branches, int tellers, int history,
                       int cachesize, boolean noSync)
        throws DatabaseException, FileNotFoundException {

        this.accounts = accounts;
        this.branches = branches;
        this.tellers = tellers;
        this.history = history;

        EnvironmentConfig config = new EnvironmentConfig();
        config.setErrorStream(System.err);
        config.setErrorPrefix(progname);
        config.setLockDetectMode(LockDetectMode.DEFAULT);
        config.setCacheSize(cachesize == 0 ? 4 * 1024 * 1024 : cachesize);
        config.setTxnNoSync(noSync);
        config.setLockDetectMode(LockDetectMode.DEFAULT);
        config.setAllowCreate(true);

        config.setInitializeCache(true);
        config.setTransactional(true);
        config.setInitializeLocking(true);
        config.setInitializeLogging(true);

        dbenv = new Environment(home, config);
    }

    public void close()
        throws DatabaseException {

        try {
            if (dbenv != null)
                dbenv.close();
        } finally {
            dbenv = null;
        }
    }

    //
    // Initialize the database to the number of accounts, branches,
    // history records, and tellers given to the constructor.
    //
    public void populate() {
        Database dbp = null;

        int err;
        int balance, idnum;
        int end_anum, end_bnum, end_tnum;
        int start_anum, start_bnum, start_tnum;
        int h_nelem;

        idnum = BEGID;
        balance = 500000;

        h_nelem = accounts;

        try {
            DatabaseConfig config = new DatabaseConfig();
            config.setType(DatabaseType.HASH);
            config.setHashNumElements(h_nelem);
            config.setAllowCreate(true);
            dbp = dbenv.openDatabase(null, "account", null, config);
        } catch (Exception e1) {
            // can be DatabaseException or FileNotFoundException
            errExit(e1, "Open of account file failed");
        }

        start_anum = idnum;
        populateTable(dbp, idnum, balance, h_nelem, "account");
        idnum += h_nelem;
        end_anum = idnum - 1;
        try {
            dbp.close();
        } catch (DatabaseException e2) {
            errExit(e2, "Account file close failed");
        }

        if (verbose)
            System.out.println("Populated accounts: " +
                               String.valueOf(start_anum) + " - " +
                               String.valueOf(end_anum));

        //
        // Since the number of branches is very small, we want to use very
        // small pages and only 1 key per page.  This is the poor-man's way
        // of getting key locking instead of page locking.
        //
        h_nelem = (int)branches;

        try {
            DatabaseConfig config = new DatabaseConfig();
            config.setType(DatabaseType.HASH);
            config.setHashNumElements(h_nelem);
            config.setHashFillFactor(1);
            config.setPageSize(512);
            config.setAllowCreate(true);
            dbp = dbenv.openDatabase(null, "branch", null, config);
        } catch (Exception e3) {
            // can be DatabaseException or FileNotFoundException
            errExit(e3, "Branch file create failed");
        }

        start_bnum = idnum;
        populateTable(dbp, idnum, balance, h_nelem, "branch");
        idnum += h_nelem;
        end_bnum = idnum - 1;

        try {
            dbp.close();
        } catch (DatabaseException dbe4) {
            errExit(dbe4, "Close of branch file failed");
        }

        if (verbose)
            System.out.println("Populated branches: " +
                               String.valueOf(start_bnum) + " - " +
                               String.valueOf(end_bnum));

        //
        // In the case of tellers, we also want small pages, but we'll let
        // the fill factor dynamically adjust itself.
        //
        h_nelem = (int)tellers;

        try {
            DatabaseConfig config = new DatabaseConfig();
            config.setType(DatabaseType.HASH);
            config.setHashNumElements(h_nelem);
            config.setHashFillFactor(0);
            config.setPageSize(512);
            config.setAllowCreate(true);
            dbp = dbenv.openDatabase(null, "teller", null, config);
        } catch (Exception e5) {
            // can be DatabaseException or FileNotFoundException
            errExit(e5, "Teller file create failed");
        }

        start_tnum = idnum;
        populateTable(dbp, idnum, balance, h_nelem, "teller");
        idnum += h_nelem;
        end_tnum = idnum - 1;

        try {
            dbp.close();
        } catch (DatabaseException e6) {
            errExit(e6, "Close of teller file failed");
        }

        if (verbose)
            System.out.println("Populated tellers: " +
                               String.valueOf(start_tnum) + " - " +
                               String.valueOf(end_tnum));

        try {
            DatabaseConfig config = new DatabaseConfig();
            config.setType(DatabaseType.RECNO);
            config.setRecordLength(HISTORY_LEN);
            config.setAllowCreate(true);
            dbp = dbenv.openDatabase(null, "history", null, config);
        } catch (Exception e7) {
            // can be DatabaseException or FileNotFoundException
            errExit(e7, "Create of history file failed");
        }

        populateHistory(dbp);

        try {
            dbp.close();
        } catch (DatabaseException e8) {
            errExit(e8, "Close of history file failed");
        }
    }

    public void populateTable(Database dbp,
                              int start_id, int balance, int nrecs, String msg) {
        Defrec drec = new Defrec();

        DatabaseEntry kdbt = new DatabaseEntry(drec.data);
        kdbt.setSize(4);                  // sizeof(int)
        DatabaseEntry ddbt = new DatabaseEntry(drec.data);
        ddbt.setSize(drec.data.length);   // uses whole array

        try {
            for (int i = 0; i < nrecs; i++) {
                kdbt.setRecordNumber(start_id + (int)i);
                drec.set_balance(balance);
                dbp.putNoOverwrite(null, kdbt, ddbt);
            }
        } catch (DatabaseException dbe) {
            System.err.println("Failure initializing " + msg + " file: " +
                               dbe.toString());
            System.exit(1);
        }
    }

    public void populateHistory(Database dbp) {
        Histrec hrec = new Histrec();
        hrec.set_amount(10);

        byte[] arr = new byte[4];                  // sizeof(int)
        int i;
        DatabaseEntry kdbt = new DatabaseEntry(arr);
        kdbt.setSize(arr.length);
        DatabaseEntry ddbt = new DatabaseEntry(hrec.data);
        ddbt.setSize(hrec.data.length);

        try {
            for (i = 1; i <= history; i++) {
                kdbt.setRecordNumber(i);

                hrec.set_aid(random_id(ACCOUNT));
                hrec.set_bid(random_id(BRANCH));
                hrec.set_tid(random_id(TELLER));

                dbp.append(null, kdbt, ddbt);
            }
        } catch (DatabaseException dbe) {
            errExit(dbe, "Failure initializing history file");
        }
    }

    static Random rand = new Random();
    public static int random_int(int lo, int hi) {
        int t = rand.nextInt();
        if (t < 0)
            t = -t;
        int ret = (int)(((double)t / ((double)(Integer.MAX_VALUE) + 1)) *
            (hi - lo + 1));
        ret += lo;
        return (ret);
    }

    public int random_id(int type) {
        int min, max, num;

        max = min = BEGID;
        num = accounts;
        switch(type) {
        case TELLER:
            min += branches;
            num = tellers;
            // fallthrough
        case BRANCH:
            if (type == BRANCH)
                num = branches;
            min += accounts;
            // fallthrough
        case ACCOUNT:
            max = min + num - 1;
        }
        return (random_int(min, max));
    }

    // The byte order is our choice.
    //
    static long get_int_in_array(byte[] array, int offset) {
        return
            ((0xff & array[offset + 0]) << 0)  |
            ((0xff & array[offset + 1]) << 8)  |
            ((0xff & array[offset + 2]) << 16) |
            ((0xff & array[offset + 3]) << 24);
    }

    // Note: Value needs to be long to avoid sign extension
    static void set_int_in_array(byte[] array, int offset, long value) {
        array[offset + 0] = (byte)((value >> 0) & 0xff);
        array[offset + 1] = (byte)((value >> 8) & 0xff);
        array[offset + 2] = (byte)((value >> 16) & 0xff);
        array[offset + 3] = (byte)((value >> 24) & 0xff);
    }

    // round 'd' to 'scale' digits, and return result as string
    static String showRounded(double d, int scale) {
        return new BigDecimal(d).
            setScale(scale, BigDecimal.ROUND_HALF_DOWN).toString();
    }

    public void run(int ntxns, int threads) {
        double gtps;
        int txns, failed;
        long curtime, starttime;
        TxnThread[] txnList = new TxnThread[threads];
        for (int i = 0; i < threads; i++)
            txnList[i] = new TxnThread("Thread " + String.valueOf(i), ntxns);

        starttime = (new Date()).getTime();
        for (int i = 0; i < threads; i++)
            txnList[i].start();
        for (int i = 0; i < threads; i++)
            try {
                txnList[i].join();
            } catch (Exception e1) {
                errExit(e1, "join failed");
            }

        curtime = (new Date()).getTime();
        txns = failed = 0;
        for (int i = 0; i < threads; i++) {
            txns += txnList[i].txns;
            failed += txnList[i].failed;
        }
        gtps = (double)(txns - failed) /
            ((curtime - starttime) / 1000.0);
        System.out.print("\nTotal: " +
                         String.valueOf(txns) + " txns " +
                         String.valueOf(failed) + " failed ");
        System.out.println(showRounded(gtps, 2) + " TPS");
    }

    class TxnThread extends Thread {
        private int ntxns;       /* Number of txns we were asked to run. */
        public int txns, failed; /* Number that succeeded / failed. */
        private Database adb, bdb, hdb, tdb;

        public TxnThread(String name, int ntxns) {
            super(name);
            this.ntxns = ntxns;
        }

        public void run() {
            double gtps, itps;
            int n, ret;
            long start_time, end_time;

            //
            // Open the database files.
            //
            int err;
            try {
                DatabaseConfig config = new DatabaseConfig();
                config.setTransactional(true);
                adb = dbenv.openDatabase(null, "account", null, config);
                bdb = dbenv.openDatabase(null, "branch", null, config);
                tdb = dbenv.openDatabase(null, "teller", null, config);
                hdb = dbenv.openDatabase(null, "history", null, config);
            } catch (DatabaseException dbe) {
                TpcbExample.errExit(dbe, "Open of db files failed");
            } catch (FileNotFoundException fnfe) {
                TpcbExample.errExit(fnfe, "Open of db files failed, missing file");
            }

            start_time = (new Date()).getTime();
            for (txns = n = ntxns, failed = 0; n-- > 0;)
                if ((ret = txn()) != 0)
                    failed++;
            end_time = (new Date()).getTime();
            if (end_time == start_time)
                end_time++;

            System.out.println(getName() + ": " + (long)txns + " txns: " +
                failed + " failed, " + TpcbExample.showRounded(
                (txns - failed) / (double)(end_time - start_time), 2) + " TPS");

            try {
                adb.close();
                bdb.close();
                tdb.close();
                hdb.close();
            } catch (DatabaseException dbe2) {
                TpcbExample.errExit(dbe2, "Close of db files failed");
            }
        }

        //
        // XXX Figure out the appropriate way to pick out IDs.
        //
        int txn() {
            Cursor acurs = null;
            Cursor bcurs = null;
            Cursor hcurs = null;
            Cursor tcurs = null;
            Transaction t = null;

            Defrec rec = new Defrec();
            Histrec hrec = new Histrec();
            int account, branch, teller;

            DatabaseEntry d_dbt = new DatabaseEntry();
            DatabaseEntry d_histdbt = new DatabaseEntry();
            DatabaseEntry k_dbt = new DatabaseEntry();
            DatabaseEntry k_histdbt = new DatabaseEntry();

            account = TpcbExample.this.random_id(TpcbExample.ACCOUNT);
            branch = TpcbExample.this.random_id(TpcbExample.BRANCH);
            teller = TpcbExample.this.random_id(TpcbExample.TELLER);

            // The history key will not actually be retrieved,
            // but it does need to be set to something.
            byte[] hist_key = new byte[4];
            k_histdbt.setData(hist_key);
            k_histdbt.setSize(4 /* == sizeof(int)*/);

            byte[] key_bytes = new byte[4];
            k_dbt.setData(key_bytes);
            k_dbt.setSize(4 /* == sizeof(int)*/);

            d_dbt.setData(rec.data);
            d_dbt.setUserBuffer(rec.length(), true);

            hrec.set_aid(account);
            hrec.set_bid(branch);
            hrec.set_tid(teller);
            hrec.set_amount(10);
            // Request 0 bytes since we're just positioning.
            d_histdbt.setPartial(0, 0, true);

            // START PER-TRANSACTION TIMING.
            //
            // Technically, TPCB requires a limit on response time, you only
            // get to count transactions that complete within 2 seconds.
            // That's not an issue for this sample application -- regardless,
            // here's where the transaction begins.
            try {
                t = dbenv.beginTransaction(null, null);

                acurs = adb.openCursor(t, null);
                bcurs = bdb.openCursor(t, null);
                tcurs = tdb.openCursor(t, null);
                hcurs = hdb.openCursor(t, null);

                // Account record
                k_dbt.setRecordNumber(account);
                if (acurs.getSearchKey(k_dbt, d_dbt, null) != OperationStatus.SUCCESS)
                    throw new Exception("acurs get failed");
                rec.set_balance(rec.get_balance() + 10);
                acurs.putCurrent(d_dbt);

                // Branch record
                k_dbt.setRecordNumber(branch);
                if (bcurs.getSearchKey(k_dbt, d_dbt, null) != OperationStatus.SUCCESS)
                    throw new Exception("bcurs get failed");
                rec.set_balance(rec.get_balance() + 10);
                bcurs.putCurrent(d_dbt);

                // Teller record
                k_dbt.setRecordNumber(teller);
                if (tcurs.getSearchKey(k_dbt, d_dbt, null) != OperationStatus.SUCCESS)
                    throw new Exception("ccurs get failed");
                rec.set_balance(rec.get_balance() + 10);
                tcurs.putCurrent(d_dbt);

                // History record
                d_histdbt.setPartial(0, 0, false);
                d_histdbt.setData(hrec.data);
                d_histdbt.setUserBuffer(hrec.length(), true);
                if (hdb.append(t, k_histdbt, d_histdbt) != OperationStatus.SUCCESS)
                    throw new DatabaseException("put failed");

                acurs.close();
                acurs = null;
                bcurs.close();
                bcurs = null;
                tcurs.close();
                tcurs = null;
                hcurs.close();
                hcurs = null;

                // null out t in advance; if the commit fails,
                // we don't want to abort it in the catch clause.
                Transaction tmptxn = t;
                t = null;
                tmptxn.commit();

                // END TIMING
                return (0);
            } catch (Exception e) {
                try {
                    if (acurs != null)
                        acurs.close();
                    if (bcurs != null)
                        bcurs.close();
                    if (tcurs != null)
                        tcurs.close();
                    if (hcurs != null)
                        hcurs.close();
                    if (t != null)
                        t.abort();
                } catch (DatabaseException dbe) {
                    // not much we can do here.
                }

                if (TpcbExample.this.verbose) {
                    System.out.println("Transaction A=" + String.valueOf(account) +
                                       " B=" + String.valueOf(branch) +
                                       " T=" + String.valueOf(teller) +
                                       " failed");
                    System.out.println("Reason: " + e.toString());
                }
                return (-1);
            }
        }
    }

    private static void usage() {
        System.err.println(
               "usage: TpcbExample [-fiv] [-a accounts] [-b branches]\n" +
               "                   [-c cachesize] [-h home] [-n transactions]\n" +
               "                   [-T threads] [-S seed] [-s history] [-t tellers]");
        System.exit(1);
    }

    private static void invarg(String str) {
        System.err.println("TpcbExample: invalid argument: " + str);
        System.exit(1);
    }

    public static void errExit(Exception err, String s) {
        System.err.print(progname + ": ");
        if (s != null) {
            System.err.print(s + ": ");
        }
        System.err.println(err.toString());
        System.exit(1);
    }

    public static void main(String[] argv) throws java.io.IOException {
        File home = new File("TESTDIR");
        int accounts = ACCOUNTS;
        int branches = BRANCHES;
        int tellers = TELLERS;
        int history = HISTORY;
        int threads = 1;
        int mpool = 0;
        int ntxns = 0;
        boolean iflag = false;
        boolean txn_no_sync = false;
        long seed = (new GregorianCalendar()).get(Calendar.SECOND);

        for (int i = 0; i < argv.length; ++i) {
            if (argv[i].equals("-a")) {
                // Number of account records
                if ((accounts = Integer.parseInt(argv[++i])) <= 0)
                    invarg(argv[i]);
            } else if (argv[i].equals("-b")) {
                // Number of branch records
                if ((branches = Integer.parseInt(argv[++i])) <= 0)
                    invarg(argv[i]);
            } else if (argv[i].equals("-c")) {
                // Cachesize in bytes
                if ((mpool = Integer.parseInt(argv[++i])) <= 0)
                    invarg(argv[i]);
            } else if (argv[i].equals("-f")) {
                // Fast mode: no txn sync.
                txn_no_sync = true;
            } else if (argv[i].equals("-h")) {
                // DB  home.
                home = new File(argv[++i]);
            } else if (argv[i].equals("-i")) {
                // Initialize the test.
                iflag = true;
            } else if (argv[i].equals("-n")) {
                // Number of transactions
                if ((ntxns = Integer.parseInt(argv[++i])) <= 0)
                    invarg(argv[i]);
            } else if (argv[i].equals("-S")) {
                // Random number seed.
                seed = Long.parseLong(argv[++i]);
                if (seed <= 0)
                    invarg(argv[i]);
            } else if (argv[i].equals("-s")) {
                // Number of history records
                if ((history = Integer.parseInt(argv[++i])) <= 0)
                    invarg(argv[i]);
            } else if (argv[i].equals("-T")) {
                // Number of threads
                if ((threads = Integer.parseInt(argv[++i])) <= 0)
                    invarg(argv[i]);
            } else if (argv[i].equals("-t")) {
                // Number of teller records
                if ((tellers = Integer.parseInt(argv[++i])) <= 0)
                    invarg(argv[i]);
            } else if (argv[i].equals("-v")) {
                // Verbose option.
                verbose = true;
            } else {
                usage();
            }
        }

        rand.setSeed((int)seed);

        // Initialize the database environment.
        // Must be done in within a try block.
        //
        TpcbExample app = null;
        try {
            app = new TpcbExample(home, accounts, branches, tellers, history,
                                  mpool, iflag || txn_no_sync);
        } catch (Exception e1) {
            errExit(e1, "initializing environment failed");
        }

        if (verbose)
            System.out.println((long)accounts + " Accounts, " +
                               String.valueOf(branches) + " Branches, " +
                               String.valueOf(tellers) + " Tellers, " +
                               String.valueOf(history) + " History");

        if (iflag) {
            if (ntxns != 0)
                usage();
            app.populate();
        } else {
            if (ntxns == 0)
                usage();
            app.run(ntxns, threads);
        }

        // Shut down the application.

        try {
            app.close();
        } catch (DatabaseException dbe2) {
            errExit(dbe2, "appexit failed");
        }

        System.exit(0);
    }
};

// Simulate the following C struct:
// struct Defrec {
//     u_int32_t   id;
//     u_int32_t   balance;
//     u_int8_t    pad[RECLEN - sizeof(int) - sizeof(int)];
// };

class Defrec {
    public Defrec() {
        data = new byte[TpcbExample.RECLEN];
    }

    public int length() {
        return TpcbExample.RECLEN;
    }

    public long get_id() {
        return TpcbExample.get_int_in_array(data, 0);
    }

    public void set_id(long value) {
        TpcbExample.set_int_in_array(data, 0, value);
    }

    public long get_balance() {
        return TpcbExample.get_int_in_array(data, 4);
    }

    public void set_balance(long value) {
        TpcbExample.set_int_in_array(data, 4, value);
    }

    static {
        Defrec d = new Defrec();
        d.set_balance(500000);
    }

    public byte[] data;
}

// Simulate the following C struct:
// struct Histrec {
//     u_int32_t   aid;
//     u_int32_t   bid;
//     u_int32_t   tid;
//     u_int32_t   amount;
//     u_int8_t    pad[RECLEN - 4 * sizeof(u_int32_t)];
// };

class Histrec {
    public Histrec() {
        data = new byte[TpcbExample.RECLEN];
    }

    public int length() {
        return TpcbExample.RECLEN;
    }

    public long get_aid() {
        return TpcbExample.get_int_in_array(data, 0);
    }

    public void set_aid(long value) {
        TpcbExample.set_int_in_array(data, 0, value);
    }

    public long get_bid() {
        return TpcbExample.get_int_in_array(data, 4);
    }

    public void set_bid(long value) {
        TpcbExample.set_int_in_array(data, 4, value);
    }

    public long get_tid() {
        return TpcbExample.get_int_in_array(data, 8);
    }

    public void set_tid(long value) {
        TpcbExample.set_int_in_array(data, 8, value);
    }

    public long get_amount() {
        return TpcbExample.get_int_in_array(data, 12);
    }

    public void set_amount(long value) {
        TpcbExample.set_int_in_array(data, 12, value);
    }

    public byte[] data;
}
