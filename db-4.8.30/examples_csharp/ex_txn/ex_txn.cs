/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using System.Threading;

using BerkeleyDB;

namespace ex_txn {
        public class ex_txn {
        private DatabaseEnvironment env;
        private Database db;
        private Random generator = new Random();
        private bool inMem;
        private string dbName, home;
        private const int NUMTHREADS = 5;

        public static void Main(string[] args) {
            /*
             * ex_txn is meant to be run from build_windows\AnyCPU, 
             * in either the Debug or Release directory. The 
             * required core libraries, however, are in either 
             * build_windows\Win32 or build_windows\x64, depending 
             * upon the platform.  That location needs to be added 
             * to the PATH environment variable for the P/Invoke 
             * calls to work.
             */
            try {
                String pwd = Environment.CurrentDirectory;
                pwd = Path.Combine(pwd, "..");
                pwd = Path.Combine(pwd, "..");
                pwd = Path.Combine(pwd, "..");
                pwd = Path.Combine(pwd, "build_windows");
                if (IntPtr.Size == 4)
                    pwd = Path.Combine(pwd, "Win32");
                else
                    pwd = Path.Combine(pwd, "x64");
#if DEBUG
                pwd = Path.Combine(pwd, "Debug");
#else
                pwd = Path.Combine(pwd, "Release");
#endif
                pwd += ";" + Environment.GetEnvironmentVariable("PATH");
                Environment.SetEnvironmentVariable("PATH", pwd);
            } catch (Exception e) {
                Console.WriteLine(
                    "Unable to set the PATH environment variable.");
                Console.WriteLine(e.Message);
                return;
            }

            ex_txn obj = new ex_txn();
            obj.RunExample(args);
        }

        private void RunExample(string[] args) {
            dbName = "ex_txn.db";
            home = "TESTDIR";

            if (!ParseArgs(args)) {
                Usage();
                return;
            }

            try {
                Open();

                // Start the threads.
                Thread[] threadArray = new Thread[NUMTHREADS];
                for (int i = 0; i < NUMTHREADS; i++) {
                    threadArray[i] = new Thread(
                                            new ThreadStart(WriteData));
                    threadArray[i].Name = "Thread " + i;
                    threadArray[i].Start();
                }

                for (int i = 0; i < NUMTHREADS; i++) {
                    threadArray[i].Join();
                    Console.WriteLine("Thread " + i + " finished.");
                }

            } catch (DatabaseException e) {
                Console.WriteLine("Caught exception: {0}", e.Message);
                Console.WriteLine(e.StackTrace);
            } finally {
                Close();
            }
        }

        private void Open() {
            Console.WriteLine("Opening environment and database");

            // Set up the environment.
            DatabaseEnvironmentConfig envCfg = new DatabaseEnvironmentConfig();
            envCfg.Create = true;
            envCfg.UseMPool = true;
            envCfg.UseLocking = true;
            envCfg.UseLogging = true;
            envCfg.UseTxns = true;

            // Allow multiple threads visit to the environment handle.
            envCfg.FreeThreaded = true;

            if (inMem)
                envCfg.Private = true;
            else 
                envCfg.RunRecovery = true;

            /* 
             * Indicate that we want db to internally perform 
             * deadlock detection, aborting the transaction that 
             * has performed the least amount of WriteData activity 
             * in the event of a deadlock.
             */
            envCfg.LockSystemCfg = new LockingConfig();
            envCfg.LockSystemCfg.DeadlockResolution =
                DeadlockPolicy.MIN_WRITE;

            if (inMem) {
                // Specify in-memory logging.
                envCfg.LogSystemCfg = new LogConfig();
                envCfg.LogSystemCfg.InMemory = true;

                /* 
                 * Specify the size of the in-memory log buffer
                 * Must be large enough to handle the log data
                 * created by the largest transaction.
                 */
                envCfg.LogSystemCfg.BufferSize = 10 * 1024 * 1024;

                /* 
                 * Specify the size of the in-memory cache, 
                 * large enough to avoid paging to disk.
                 */
                envCfg.MPoolSystemCfg = new MPoolConfig();
                envCfg.MPoolSystemCfg.CacheSize =
                    new CacheInfo(0, 10 * 1024 * 1024, 1);
            }

            // Set up the database.
            BTreeDatabaseConfig dbCfg = new BTreeDatabaseConfig();
            dbCfg.AutoCommit = true;
            dbCfg.Creation = CreatePolicy.IF_NEEDED;
            dbCfg.Duplicates = DuplicatesPolicy.SORTED;
            dbCfg.FreeThreaded = true;
            dbCfg.ReadUncommitted = true;

            /*
                         * Open the environment. Any errors will be caught
                         * by the caller.
                         */                          
            env = DatabaseEnvironment.Open(home, envCfg);

            /*
             * Open the database. Do not provide a txn handle. This 
             * Open is autocommitted because BTreeDatabaseConfig.AutoCommit
             * is true.
             */
            dbCfg.Env = env;
            db = BTreeDatabase.Open(dbName, dbCfg);

        }

        private void Close() {
            Console.WriteLine("Closing environment and database");
            if (db != null) {
                try {
                    db.Close();
                } catch (DatabaseException e) {
                    Console.WriteLine("Error closing db: " + e.ToString());
                    Console.WriteLine(e.StackTrace);
                }
            }

            if (env != null) {
                try {
                    env.Close();
                } catch (DatabaseException e) {
                    Console.WriteLine("Error closing env: " + e.ToString());
                    Console.WriteLine(e.StackTrace);
                }
            }
        }

        /*
         * This simply counts the number of records contained in the
         * database and returns the result. You can use this method
         * in three ways:
         * 
         * First call it with an active txn handle.
         * Secondly, configure the cursor for dirty reads
         * Third, call countRecords AFTER the writer has committed
         * its transaction.
         * 
         * If you do none of these things, the writer thread will
         * self-deadlock.
         * 
         * Note that this method exists only for illustrative purposes.
         * A more straight-forward way to count the number of records in
         * a database is to use the Database.getStats() method.
         */
        private int CountRecords(Transaction txn) {
            int count = 0;
            Cursor cursor = null;

            try {
                // Get the cursor.
                CursorConfig cc = new CursorConfig();
                
                /*
                 * Isolation degree one is ignored if the 
                 * database was not opened for uncommitted 
                 * read support. TxnGuide opens its database
                 * in this way and TxnGuideInMemory does not.
                 */
                cc.IsolationDegree = Isolation.DEGREE_ONE;
                cursor = db.Cursor(cc, txn);
                while (cursor.MoveNext())
                    count++;
            } finally {
                if (cursor != null)
                    cursor.Close();
            }

            return count;
        }

        private bool ParseArgs(string[] args) {
            for (int i = 0; i < args.Length; i++) {
                string s = args[i];
                if (s[0] != '-') {
                    Console.Error.WriteLine(
                        "Unrecognized option: " + args[i]);
                    return false;
                }
                
                switch (s[1]) {
                    case 'h':
                        home = args[++i];
                        break;
                    case 'm':
                        inMem = true;
                        break;
                    default:
                        Console.Error.WriteLine(
                            "Unrecognized option: " + args[i]);
                        return false;
                }
            }
            return true;
        }

        private void Usage() {
            Console.WriteLine("ex_txn [-h <env directory>] [-m]");
            Console.WriteLine("\t -h home (Set environment directory.)");
            Console.WriteLine("\t -m (Run in memory, do not write to disk.)");
        }

        private void WriteData() {
            /*
             * Write a series of records to the database using transaction 
             * protection. Deadlock handling is demonstrated here.
             */
            BinaryFormatter formatter = new BinaryFormatter();
            MemoryStream ms = new MemoryStream();
            Random generator = new Random();
            Transaction txn = null;

            string[] keys = {"key 1", "key 2", "key 3", "key 4",
                "key 5", "key 6", "key 7", "key 8", "key 9", "key 10"};

            // Perform 20 transactions.
            int iters = 0;
            int retry_count = 0;
            int maxRetry = 20;
            while (iters < 50) {
                try {
                    // Get a transaction.
                    txn = env.BeginTransaction();

                    // Write 10 records to the db for each transaction.
                    for (int j = 0; j < 10; j++) {
                        // Get the key.
                        DatabaseEntry key;
                        key = new DatabaseEntry(
                            ASCIIEncoding.ASCII.GetBytes(keys[j]));

                        // Get the data.
                        PayloadData pd = new PayloadData(
                                                    iters + j,
                            Thread.CurrentThread.Name,
                            generator.NextDouble());

                        formatter.Serialize(ms, pd);
                        Byte[] bytes = ms.GetBuffer();
                        DatabaseEntry data = new DatabaseEntry(bytes);

                        // Put key/data pair within the transaction.
                        db.Put(key, data, txn);
                    }

                    // Commit the transaction.
                    Console.WriteLine("{0} committing txn: {1}",
                        Thread.CurrentThread.Name, iters);

                    int recCount = CountRecords(inMem ? txn : null);
                    Console.WriteLine("{0} found {1} records in the database.",
                        Thread.CurrentThread.Name, recCount);

                    try {
                        txn.Commit();
                        txn = null;
                    } catch (DatabaseException e) {
                        Console.WriteLine(
                                                    "Error on txn commit: " + 
                                                    e.ToString());
                    }

                    iters++;
                    retry_count = 0;
                } catch (DeadlockException) {
                    Console.WriteLine(
                        "##### {0} deadlocked.", Thread.CurrentThread.Name);

                    // Retry if necessary.
                    if (retry_count < maxRetry) {
                        Console.WriteLine("{0} retrying.",
                            Thread.CurrentThread.Name);
                        retry_count++;
                    } else {
                        Console.WriteLine("{0} out of retries. Giving up.",
                            Thread.CurrentThread.Name);
                        iters++;
                        retry_count = 0;
                    }
                } catch (DatabaseException e) {
                    // Abort and don't retry.
                    iters++;
                    retry_count = 0;
                    Console.WriteLine(Thread.CurrentThread.Name +
                        " : caught exception: " + e.ToString());
                    Console.WriteLine(Thread.CurrentThread.Name +
                        " : errno: " + e.ErrorCode);
                    Console.WriteLine(e.StackTrace);
                } finally {
                    if (txn != null) {
                        try {
                            txn.Abort();
                        } catch (DatabaseException e) {
                            Console.WriteLine("Error aborting transaction: " +
                                e.ToString());
                            Console.WriteLine(e.StackTrace);
                        }
                    }
                }
            }
        }
    }
}
