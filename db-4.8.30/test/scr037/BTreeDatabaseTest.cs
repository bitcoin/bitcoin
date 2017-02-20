/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class BTreeDatabaseTest : DatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "BTreeDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestCompactWithoutTxn()
        {
            int i, nRecs;
            nRecs = 10000;
            testName = "TestCompactWithoutTxn";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            // The minimum page size
            btreeDBConfig.PageSize = 512;
            btreeDBConfig.BTreeCompare =
                new EntryComparisonDelegate(dbIntCompare);
            using (BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBConfig))
            {
                DatabaseEntry key;
                DatabaseEntry data;

                // Fill the database with entries from 0 to 9999
                for (i = 0; i < nRecs; i++)
                {
                    key = new DatabaseEntry(
                        BitConverter.GetBytes(i));
                    data = new DatabaseEntry(
                        BitConverter.GetBytes(i));
                    btreeDB.Put(key, data);
                }

                /*
                 * Delete entries below 500, between 3000 and
                 * 5000 and above 7000
                 */
                for (i = 0; i < nRecs; i++)
                    if (i < 500 || i > 7000 ||
                        (i < 5000 && i > 3000))
                    {
                        key = new DatabaseEntry(
                            BitConverter.GetBytes(i));
                        btreeDB.Delete(key);
                    }

                btreeDB.Sync();
                long fileSize = new FileInfo(
                   btreeDBFileName).Length;

                // Compact database
                CompactConfig cCfg = new CompactConfig();
                cCfg.FillPercentage = 30;
                cCfg.Pages = 10;
                cCfg.Timeout = 1000;
                cCfg.TruncatePages = true;
                cCfg.start = new DatabaseEntry(
                    BitConverter.GetBytes(1));
                cCfg.stop = new DatabaseEntry(
                    BitConverter.GetBytes(7000));
                CompactData compactData = btreeDB.Compact(cCfg);
                Assert.IsFalse((compactData.Deadlocks == 0) &&
                    (compactData.Levels == 0) &&
                    (compactData.PagesExamined == 0) &&
                    (compactData.PagesFreed == 0) &&
                    (compactData.PagesTruncated == 0));

                btreeDB.Sync();
                long compactedFileSize =
                    new FileInfo(btreeDBFileName).Length;
                Assert.Less(compactedFileSize, fileSize);
            }
        }
        
                [Test]
                public void TestCompression() {
                        testName = "TestCompression";
                        testHome = testFixtureHome + "/" + testName;
                        string btreeDBName = testHome + "/" + testName + ".db";

                        Configuration.ClearDir(testHome);

                        BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
                        cfg.Creation = CreatePolicy.ALWAYS;
                        cfg.SetCompression(compress, decompress);
                        BTreeDatabase db = BTreeDatabase.Open(btreeDBName, cfg);
                        DatabaseEntry key, data;
                        char[] keyData = { 'A', 'A', 'A', 'A' };
                        byte[] dataData = new byte[20];
                        Random generator = new Random();
                        int i;
                        for (i = 0; i < 20000; i++) {
                                // Write random data
                                key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes(keyData));
                                generator.NextBytes(dataData);
                                data = new DatabaseEntry(dataData);
                                db.Put(key, data);

                                // Bump the key. Rollover from Z to A if necessary
                                int j = keyData.Length;
                                do {
                                        j--;
                                        if (keyData[j]++ == 'Z')
                                                keyData[j] = 'A';
                                } while (keyData[j] == 'A');
                       }
                        db.Close();
                }

                bool compress(DatabaseEntry prevKey, DatabaseEntry prevData, 
                    DatabaseEntry key, DatabaseEntry data, ref byte[] dest, out int size) {
                        /* 
                         * Just a dummy function that doesn't do any compression.  It just
                         * writes the 5 byte key and 20 byte data to the buffer.
                         */
                        size = key.Data.Length + data.Data.Length;
                        if (size > dest.Length)
                                return false;
                        key.Data.CopyTo(dest, 0);
                        data.Data.CopyTo(dest, key.Data.Length);
                        return true;
                }

                KeyValuePair<DatabaseEntry, DatabaseEntry> decompress(
                    DatabaseEntry prevKey, DatabaseEntry prevData, byte[] compressed, out uint bytesRead) {
                        byte[] keyData = new byte[4];
                        byte[] dataData = new byte[20];
                        Array.ConstrainedCopy(compressed, 0, keyData, 0, 4);
                        Array.ConstrainedCopy(compressed, 4, dataData, 0, 20);
                        DatabaseEntry key = new DatabaseEntry(keyData);
                        DatabaseEntry data = new DatabaseEntry(dataData);
                        bytesRead = (uint)(key.Data.Length + data.Data.Length);
                        return new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
                }

                [Test]
                public void TestCompressionDefault() {
                        testName = "TestCompressionDefault";
                        testHome = testFixtureHome + "/" + testName;
                        string btreeDBName = testHome + "/" + testName + ".db";

                        Configuration.ClearDir(testHome);

                        BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
                        cfg.Creation = CreatePolicy.ALWAYS;
                        BTreeDatabase db = BTreeDatabase.Open(btreeDBName, cfg);
                        DatabaseEntry key, data;
                        char[] keyData = { 'A', 'A', 'A', 'A' };
                        byte[] dataData = new byte[20];
                        Random generator = new Random();
                        int i;
                        for (i = 0; i < 20000; i++) {
                                // Write random data
                                key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes(keyData));
                                generator.NextBytes(dataData);
                                data = new DatabaseEntry(dataData);
                                db.Put(key, data);

                                // Bump the key. Rollover from Z to A if necessary
                                int j = keyData.Length;
                                do {
                                        j--;
                                        if (keyData[j]++ == 'Z')
                                                keyData[j] = 'A';
                                } while (keyData[j] == 'A');
                        }
                        db.Close();

                        FileInfo dbInfo = new FileInfo(btreeDBName);
                        long uncompressedSize = dbInfo.Length;
                        Configuration.ClearDir(testHome);

                        cfg = new BTreeDatabaseConfig();
                        cfg.Creation = CreatePolicy.ALWAYS;
                        cfg.SetCompression();
                        db = BTreeDatabase.Open(btreeDBName, cfg);
                        keyData = new char[]{ 'A', 'A', 'A', 'A' };
                        for (i = 0; i < 20000; i++) {
                                // Write random data
                                key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes(keyData));
                                generator.NextBytes(dataData);
                                data = new DatabaseEntry(dataData);
                                db.Put(key, data);

                                // Bump the key. Rollover from Z to A if necessary
                                int j = keyData.Length;
                                do {
                                        j--;
                                        if (keyData[j]++ == 'Z')
                                                keyData[j] = 'A';
                                } while (keyData[j] == 'A');
                        }
                        Cursor dbc = db.Cursor();
                        foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> kvp in dbc) 
                                i--;
                        dbc.Close();
                        Assert.AreEqual(i, 0);
                        db.Close();

                        dbInfo = new FileInfo(btreeDBName);
                        Assert.Less(dbInfo.Length, uncompressedSize);
                        Console.WriteLine("Uncompressed: {0}", uncompressedSize);
                        Console.WriteLine("Compressed: {0}", dbInfo.Length);
                        
                        Configuration.ClearDir(testHome);

                        cfg = new BTreeDatabaseConfig();
                        cfg.Creation = CreatePolicy.ALWAYS;
                        cfg.SetCompression();
                        db = BTreeDatabase.Open(btreeDBName, cfg);
                        for (i = 1023; i < 1124; i++){
                                key = new DatabaseEntry(BitConverter.GetBytes(i));
                                data = new DatabaseEntry(BitConverter.GetBytes(i + 3));
                                db.Put(key, data);
                        }
                        dbc = db.Cursor();
                        foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> kvp in dbc){ 
                                int keyInt = BitConverter.ToInt32(kvp.Key.Data, 0);
                                int dataInt = BitConverter.ToInt32(kvp.Value.Data, 0);
                                Assert.AreEqual(3, dataInt - keyInt);
                        }
                        dbc.Close();

                        db.Close();    
                }

        [Test, ExpectedException(typeof(AccessViolationException))]
        public void TestClose()
        {
            testName = "TestClose";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBConfig);
            btreeDB.Close();
            DatabaseEntry key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("hi"));
            DatabaseEntry data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("hi"));
            btreeDB.Put(key, data);
        }

        [Test]
        public void TestCloseWithoutSync()
        {
            testName = "TestCloseWithoutSync";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBName = testName + ".db";

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.ForceFlush = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            envConfig.UseLogging = true;
            envConfig.LogSystemCfg = new LogConfig();
            envConfig.LogSystemCfg.ForceSync = false;
            envConfig.LogSystemCfg.AutoRemove = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            TransactionConfig txnConfig = new TransactionConfig();
            txnConfig.SyncAction =
                TransactionConfig.LogFlush.WRITE_NOSYNC;
            Transaction txn = env.BeginTransaction(txnConfig);

            BTreeDatabaseConfig btreeConfig =
                new BTreeDatabaseConfig();
            btreeConfig.Creation = CreatePolicy.ALWAYS;
            btreeConfig.Env = env;

            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBName, btreeConfig, txn);

            DatabaseEntry key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            DatabaseEntry data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            Assert.IsFalse(btreeDB.Exists(key, txn));
            btreeDB.Put(key, data, txn);
            btreeDB.Close(false);
            txn.Commit();
            env.Close();

            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.NEVER;
            using (BTreeDatabase db = BTreeDatabase.Open(
                testHome + "/" + btreeDBName, dbConfig))
            {
                Assert.IsFalse(db.Exists(key));
            }
        }

        [Test]
        public void TestCursorWithoutEnv()
        {
            BTreeCursor cursor;
            BTreeDatabase db;
            string dbFileName;

            testName = "TestCursorWithoutEnv";
            testHome = testFixtureHome + "/" + testName;
            dbFileName = testHome + "/" + testName + ".db";

            // Open btree database.
            Configuration.ClearDir(testHome);
            OpenBtreeDB(null, null, dbFileName, out db);

            // Get a cursor.
            cursor = db.Cursor();

            /*
             * Add a record to the database with cursor and 
             * confirm that the record exists in the database.
             */
            CursorTest.AddOneByCursor(db, cursor);

            // Close cursor and database.
            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestCursorWithConfigInTxn()
        {
            BTreeCursor cursor;
            BTreeDatabase db;
            DatabaseEnvironment env;
            Transaction txn;
            string dbFileName;

            testName = "TestCursorWithConfigInTxn";
            testHome = testFixtureHome + "/" + testName;
            dbFileName = testName + ".db";

            // Open environment and begin a transaction.
            Configuration.ClearDir(testHome);

            SetUpEnvAndTxn(testHome, out env, out txn);
            OpenBtreeDB(env, txn, dbFileName, out db);

            // Config and get a cursor.
            cursor = db.Cursor(new CursorConfig(), txn);

            /*
             * Add a record to the database with cursor and 
             * confirm that the record exists in the database.
             */
            CursorTest.AddOneByCursor(db, cursor);

            /*
             * Close cursor, database, commit the transaction 
             * and close the environment.
             */
            cursor.Close();
            db.Close();
            txn.Commit();
            env.Close();
        }

        [Test]
        public void TestCursorWithoutConfigInTxn()
        {
            BTreeCursor cursor;
            BTreeDatabase db;
            DatabaseEnvironment env;
            Transaction txn;
            string dbFileName;

            testName = "TestCursorWithoutConfigInTxn";
            testHome = testFixtureHome + "/" + testName;
            dbFileName = testName + ".db";

            // Open environment and begin a transaction.
            Configuration.ClearDir(testHome);
            SetUpEnvAndTxn(testHome, out env, out txn);
            OpenBtreeDB(env, txn, dbFileName, out db);

            // Get a cursor in the transaction.
            cursor = db.Cursor(txn);

            /*
             * Add a record to the database with cursor and 
             * confirm that the record exists in the database.
             */
            CursorTest.AddOneByCursor(db, cursor);

            /*
             * Close cursor, database, commit the transaction 
             * and close the environment.
             */
            cursor.Close();
            db.Close();
            txn.Commit();
            env.Close();
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestDelete()
        {
            testName = "TestDelete";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBConfig);
            DatabaseEntry key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            DatabaseEntry data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            btreeDB.Put(key, data);
            btreeDB.Delete(key);
            try
            {
                btreeDB.Get(key);
            }
            catch (NotFoundException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                btreeDB.Close();
            }
        }

        [Test]
        public void TestExist()
        {
            testName = "TestExist";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase btreeDB;
            using (btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig))
            {
                DatabaseEntry key = new DatabaseEntry(
                     ASCIIEncoding.ASCII.GetBytes("key"));
                DatabaseEntry data = new DatabaseEntry(
                     ASCIIEncoding.ASCII.GetBytes("data"));

                btreeDB.Put(key, data);
                Assert.IsTrue(btreeDB.Exists(
                     new DatabaseEntry(
                     ASCIIEncoding.ASCII.GetBytes("key"))));
                Assert.IsFalse(btreeDB.Exists(
                     new DatabaseEntry(
                     ASCIIEncoding.ASCII.GetBytes("data"))));
            }
        }

        [Test]
        public void TestExistWithTxn()
        {
            BTreeDatabase btreeDB;
            Transaction txn;
            DatabaseEnvironmentConfig envConfig;
            DatabaseEnvironment env;
            DatabaseEntry key, data;

            testName = "TestExistWithTxn";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Open an environment.
            envConfig = new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            env = DatabaseEnvironment.Open(testHome, envConfig);

            // Begin a transaction.
            txn = env.BeginTransaction();

            // Open a database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            btreeDBConfig.Env = env;
            btreeDB = BTreeDatabase.Open(testName + ".db",
                btreeDBConfig, txn);

            // Put key data pair into database.
            key = new DatabaseEntry(
                     ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(
                 ASCIIEncoding.ASCII.GetBytes("data"));
            btreeDB.Put(key, data, txn);

            // Confirm that the pair exists in the database.
            Assert.IsTrue(btreeDB.Exists(new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key")), txn));
            Assert.IsFalse(btreeDB.Exists(new DatabaseEntry(
                 ASCIIEncoding.ASCII.GetBytes("data")), txn));

            // Dispose all.
            btreeDB.Close();
            txn.Commit();
            env.Close();
        }


        [Test]
        public void TestExistWithLockingInfo()
        {
            BTreeDatabase btreeDB;
            DatabaseEnvironment env;
            DatabaseEntry key, data;

            testName = "TestExistWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Open the environment.
            DatabaseEnvironmentConfig envCfg =
                new DatabaseEnvironmentConfig();
            envCfg.Create = true;
            envCfg.FreeThreaded = true;
            envCfg.UseLocking = true;
            envCfg.UseLogging = true;
            envCfg.UseMPool = true;
            envCfg.UseTxns = true;
            env = DatabaseEnvironment.Open(
                testHome, envCfg);

            // Open database in transaction.
            Transaction openTxn = env.BeginTransaction();
            BTreeDatabaseConfig cfg =
                new BTreeDatabaseConfig();
            cfg.Creation = CreatePolicy.ALWAYS;
            cfg.Env = env;
            cfg.FreeThreaded = true;
            cfg.PageSize = 4096;
            cfg.Duplicates = DuplicatesPolicy.UNSORTED;
            btreeDB = BTreeDatabase.Open(testName + ".db",
                cfg, openTxn);
            openTxn.Commit();

            // Put key data pair into database.
            Transaction txn = env.BeginTransaction();
            key = new DatabaseEntry(
                     ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(
                 ASCIIEncoding.ASCII.GetBytes("data"));
            btreeDB.Put(key, data, txn);

            // Confirm that the pair exists in the database with LockingInfo.
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.ReadModifyWrite = true;

            // Confirm that the pair exists in the database.
            Assert.IsTrue(btreeDB.Exists(new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key")), txn, lockingInfo));
            Assert.IsFalse(btreeDB.Exists(new DatabaseEntry(
                 ASCIIEncoding.ASCII.GetBytes("data")), txn, lockingInfo));
            txn.Commit();

            btreeDB.Close();
            env.Close();
        }

        [Test]
        public void TestGetByKey()
        {
            testName = "TestGetByKey";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                     testName + ".db";
            string btreeDBName =
                Path.GetFileNameWithoutExtension(btreeDBFileName);

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBName, btreeDBConfig);

            DatabaseEntry key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            DatabaseEntry data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            btreeDB.Put(key, data);

            KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
                new KeyValuePair<DatabaseEntry, DatabaseEntry>();
            pair = btreeDB.Get(key);
            Assert.AreEqual(pair.Key.Data, key.Data);
            Assert.AreEqual(pair.Value.Data, data.Data);
            btreeDB.Close();
        }

        [Test]
        public void TestGetByRecno()
        {
            testName = "TestGetByRecno";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName =
                Path.GetFileNameWithoutExtension(btreeDBFileName);

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            btreeDBConfig.UseRecordNumbers = true;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBName, btreeDBConfig);
            Assert.IsTrue(btreeDB.RecordNumbers);

            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();
            uint recno, count, value;
            for (recno = 1; recno <= 100; recno++)
            {
                value = 200 - recno;
                Configuration.dbtFromString(key,
                    Convert.ToString(value));
                Configuration.dbtFromString(data,
                    Convert.ToString(value));
                btreeDB.Put(key, data);
            }

            KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
                new KeyValuePair<DatabaseEntry, DatabaseEntry>();

            for (count = 1; ; count++)
            {
                try
                {
                    pair = btreeDB.Get(count);
                }
                catch (NotFoundException)
                {
                    Assert.AreEqual(101, count);
                    break;
                }
                value = 299 - 200 + count;
                Assert.AreEqual(value.ToString(),
                    Configuration.strFromDBT(pair.Key));
            }

            btreeDB.Close();
        }

        [Test, ExpectedException(typeof(NotFoundException))]
        public void TestGetBoth()
        {
            testName = "TestGetBoth";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName =
                Path.GetFileNameWithoutExtension(btreeDBFileName);

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            using (BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBName, btreeDBConfig))
            {
                DatabaseEntry key = new DatabaseEntry();
                DatabaseEntry data = new DatabaseEntry();

                Configuration.dbtFromString(key, "key");
                Configuration.dbtFromString(data, "data");
                btreeDB.Put(key, data);
                KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
                    new KeyValuePair<DatabaseEntry, DatabaseEntry>();
                pair = btreeDB.GetBoth(key, data);
                Assert.AreEqual(key.Data, pair.Key.Data);
                Assert.AreEqual(data.Data, pair.Value.Data);

                Configuration.dbtFromString(key, "key");
                Configuration.dbtFromString(data, "key");
                btreeDB.GetBoth(key, data);
            }
        }

        [Test]
        public void TestGetBothMultiple()
        {
            testName = "TestGetBothMultiple";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName = testName;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp;
            int cnt;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            btreeDBConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            btreeDBConfig.PageSize = 1024;
            using (BTreeDatabase btreeDB = GetMultipleDB(
                btreeDBFileName, btreeDBName, btreeDBConfig)) {
                key = new DatabaseEntry(BitConverter.GetBytes(100));
                data = new DatabaseEntry(BitConverter.GetBytes(100));

                kvp = btreeDB.GetBothMultiple(key, data);
                cnt = 0;
                foreach (DatabaseEntry dbt in kvp.Value)
                    cnt++;
                Assert.AreEqual(cnt, 10);

                kvp = btreeDB.GetBothMultiple(key, data, 1024);
                cnt = 0;
                foreach (DatabaseEntry dbt in kvp.Value)
                    cnt++;
                Assert.AreEqual(cnt, 10);
            }
        }
        
        [Test]
        public void TestGetMultiple() 
        {
            testName = "TestGetMultiple";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName = testName;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            btreeDBConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            btreeDBConfig.PageSize = 512;
            
            using (BTreeDatabase btreeDB = GetMultipleDB(
                btreeDBFileName, btreeDBName, btreeDBConfig)) {
                DatabaseEntry key = new DatabaseEntry(
                                    BitConverter.GetBytes(10));
                KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp = 
                                    btreeDB.GetMultiple(key, 1024);
                int cnt = 0;
                foreach (DatabaseEntry dbt in kvp.Value)
                    cnt++;
                Assert.AreEqual(cnt, 10);

                key = new DatabaseEntry(
                                    BitConverter.GetBytes(102));
                kvp = btreeDB.GetMultiple(key, 1024);
                cnt = 0;
                foreach (DatabaseEntry dbt in kvp.Value)
                    cnt++;
                Assert.AreEqual(cnt, 1);
            }
        }


        [Test]
        public void TestGetMultipleByRecno()
        {
            testName = "TestGetMultipleByRecno";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName =
                Path.GetFileNameWithoutExtension(btreeDBFileName);

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            btreeDBConfig.Duplicates = DuplicatesPolicy.NONE;
            btreeDBConfig.UseRecordNumbers = true;
            using (BTreeDatabase btreeDB = GetMultipleDB(
                btreeDBFileName, btreeDBName, btreeDBConfig)) {
                int recno = 44;
                KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp = 
                                    btreeDB.GetMultiple((uint)recno);
                int cnt = 0;
                int kdata = BitConverter.ToInt32(kvp.Key.Data, 0);
                Assert.AreEqual(kdata, recno);
                foreach (DatabaseEntry dbt in kvp.Value) {
                    cnt++;
                    int ddata = BitConverter.ToInt32(dbt.Data, 0);
                    Assert.AreEqual(ddata, recno);
                }
                Assert.AreEqual(cnt, 1);
            }
        }

        [Test]
        public void TestGetMultipleByRecnoInSize()
        {
            testName = "TestGetMultipleByRecnoInSize";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName =
                Path.GetFileNameWithoutExtension(btreeDBFileName);

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            btreeDBConfig.Duplicates = DuplicatesPolicy.NONE;
            btreeDBConfig.UseRecordNumbers = true;
            btreeDBConfig.PageSize = 512;
            using (BTreeDatabase btreeDB = GetMultipleDB(
                btreeDBFileName, btreeDBName, btreeDBConfig)) {
                int recno = 100;
                int bufferSize = 1024;
                KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp = 
                                    btreeDB.GetMultiple((uint)recno, bufferSize);
                int cnt = 0;
                int kdata = BitConverter.ToInt32(kvp.Key.Data, 0);
                Assert.AreEqual(kdata, recno);
                foreach (DatabaseEntry dbt in kvp.Value) {
                    cnt++;
                    Assert.AreEqual(dbt.Data.Length, 111);
                }
                Assert.AreEqual(1, cnt);
            }
        }

        [Test]
        public void TestGetMultipleInSize()
        {
            testName = "TestGetMultipleInSize";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName =
                Path.GetFileNameWithoutExtension(btreeDBFileName);

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            btreeDBConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            btreeDBConfig.PageSize = 1024;
            using (BTreeDatabase btreeDB = GetMultipleDB(
                btreeDBFileName, btreeDBName, btreeDBConfig)) {

                int num = 101;
                DatabaseEntry key = new DatabaseEntry(
                                    BitConverter.GetBytes(num));
                int bufferSize = 10240;
                KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp = 
                                    btreeDB.GetMultiple(key, bufferSize);
                int cnt = 0;
                foreach (DatabaseEntry dbt in kvp.Value) {
                    cnt++;
                    Assert.AreEqual(BitConverter.ToInt32(
                                            dbt.Data, 0), num);
                    num++;
                }
                Assert.AreEqual(cnt, 923);
            }
        }
        
        [Test]
        public void TestGetWithTxn()
        {
            testName = "TestGetWithTxn";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            try
            {
                Transaction openTxn = env.BeginTransaction();
                BTreeDatabaseConfig dbConfig =
                    new BTreeDatabaseConfig();
                dbConfig.Env = env;
                dbConfig.Creation = CreatePolicy.IF_NEEDED;
                BTreeDatabase db = BTreeDatabase.Open(
                    testName + ".db", dbConfig, openTxn);
                openTxn.Commit();

                Transaction putTxn = env.BeginTransaction();
                try
                {
                    for (int i = 0; i < 20; i++)
                        db.Put(new DatabaseEntry(
                            BitConverter.GetBytes(i)),
                            new DatabaseEntry(
                            BitConverter.GetBytes(i)), putTxn);
                    putTxn.Commit();
                }
                catch (DatabaseException e)
                {
                    putTxn.Abort();
                    db.Close();
                    throw e;
                }

                Transaction getTxn = env.BeginTransaction();
                KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
                try
                {
                    for (int i = 0; i < 20; i++)
                    {
                        pair = db.Get(new DatabaseEntry(
                            BitConverter.GetBytes(i)), getTxn);
                        Assert.AreEqual(BitConverter.GetBytes(i),
                            pair.Key.Data);
                    }

                    getTxn.Commit();
                    db.Close();
                }
                catch (DatabaseException)
                {
                    getTxn.Abort();
                    db.Close();
                    throw new TestException();
                }
            }
            catch (DatabaseException)
            {
            }
            finally
            {
                env.Close();
            }
        }

        [Test]
        public void TestKeyRange()
        {
            testName = "TestKeyRange";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName = Path.GetFileNameWithoutExtension(
                btreeDBFileName);

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBName, btreeDBConfig);

            DatabaseEntry key = new DatabaseEntry();
            DatabaseEntry data = new DatabaseEntry();
            uint recno;
            for (recno = 1; recno <= 10; recno++)
            {
                Configuration.dbtFromString(key,
                    Convert.ToString(recno));
                Configuration.dbtFromString(data,
                    Convert.ToString(recno));
                btreeDB.Put(key, data);
            }

            Configuration.dbtFromString(key, Convert.ToString(5));
            KeyRange keyRange = btreeDB.KeyRange(key);
            Assert.AreEqual(0.5, keyRange.Less);
            Assert.AreEqual(0.1, keyRange.Equal);
            Assert.AreEqual(0.4, keyRange.Greater);

            btreeDB.Close();
        }

        [Test]
        public void TestOpenExistingBtreeDB()
        {
            testName = "TestOpenExistingBtreeDB";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                 testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeConfig =
                new BTreeDatabaseConfig();
            btreeConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeConfig);
            btreeDB.Close();

            DatabaseConfig dbConfig = new DatabaseConfig();
            Database db = Database.Open(btreeDBFileName, 
                dbConfig);
            Assert.AreEqual(db.Type, DatabaseType.BTREE);
            Assert.AreEqual(db.Creation, CreatePolicy.NEVER);
            db.Close();
        }

        [Test]
        public void TestOpenNewBtreeDB()
        {
            testName = "TestOpenNewBtreeDB";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            BTreeDatabaseConfig btreeConfig =
                new BTreeDatabaseConfig();
            BTreeDatabaseConfigTest.Config(xmlElem,
                ref btreeConfig, true);
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeConfig);
            Confirm(xmlElem, btreeDB, true);
            btreeDB.Close();
        }

        [Test]
        public void TestOpenMulDBInSingleFile()
        {
            testName = "TestOpenMulDBInSingleFile";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string[] btreeDBArr = new string[4];

            for (int i = 0; i < 4; i++)
                btreeDBArr[i] = Path.GetFileNameWithoutExtension(
                    btreeDBFileName) + i;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;

            BTreeDatabase btreeDB;
            for (int i = 0; i < 4; i++)
            {
                btreeDB = BTreeDatabase.Open(btreeDBFileName,
                    btreeDBArr[i], btreeDBConfig);
                Assert.AreEqual(CreatePolicy.IF_NEEDED, btreeDB.Creation);
                btreeDB.Close();
            }

            DatabaseConfig dbConfig = new DatabaseConfig();
            Database db;
            for (int i = 0; i < 4; i++)
            {
                using (db = Database.Open(btreeDBFileName,
                    btreeDBArr[i], dbConfig))
                {
                    Assert.AreEqual(btreeDBArr[i],
                        db.DatabaseName);
                    Assert.AreEqual(DatabaseType.BTREE,
                        db.Type);
                }
            }
        }

        [Test]
        public void TestOpenWithTxn()
        {
            testName = "TestOpenWithTxn";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBName = testName + ".db";

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;

            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);
            Transaction txn = env.BeginTransaction(
                new TransactionConfig());

            BTreeDatabaseConfig btreeConfig =
                new BTreeDatabaseConfig();
            btreeConfig.Creation = CreatePolicy.ALWAYS;
            btreeConfig.Env = env;

            /*
             * If environmnet home is set, the file name in Open()
             * is the relative path.
             */
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBName, btreeConfig, txn);
            Assert.IsTrue(btreeDB.Transactional);
            btreeDB.Close();
            txn.Commit();
            env.Close();
        }

        [Test]
        public void TestPrefixCompare()
        {
            testName = "TestPrefixCompare";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                 testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Duplicates = DuplicatesPolicy.SORTED;
            dbConfig.BTreeCompare =
                new EntryComparisonDelegate(dbIntCompare);
            dbConfig.BTreePrefixCompare =
                new EntryComparisonDelegate(dbIntCompare);
            BTreeDatabase db = BTreeDatabase.Open(
                btreeDBFileName, dbConfig);

            Assert.Greater(0, db.PrefixCompare(new DatabaseEntry(
                BitConverter.GetBytes(255)), new DatabaseEntry(
                BitConverter.GetBytes(257))));

            Assert.AreEqual(0, db.PrefixCompare(new DatabaseEntry(
                BitConverter.GetBytes(255)), new DatabaseEntry(
                BitConverter.GetBytes(255))));

            db.Close();
        }

        [Test]
        public void TestPutWithoutTxn()
        {
            testName = "TestPutWithoutTxn";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            using (BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBConfig))
            {
                // Put integer into database
                key = new DatabaseEntry(
                    BitConverter.GetBytes((int)0));
                data = new DatabaseEntry(
                    BitConverter.GetBytes((int)0));
                btreeDB.Put(key, data);
                pair = btreeDB.Get(key);
                Assert.AreEqual(key.Data, pair.Key.Data);
                Assert.AreEqual(data.Data, pair.Value.Data);
            }
        }

        [Test, ExpectedException(typeof(KeyExistException))]
        public void TestPutNoOverWrite()
        {
            testName = "TestPutNoOverWrite";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            DatabaseEntry key, data, newData;
            using (BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBConfig))
            {
                key = new DatabaseEntry(
                    BitConverter.GetBytes((int)0));
                data = new DatabaseEntry(
                    BitConverter.GetBytes((int)0));
                newData = new DatabaseEntry(
                    BitConverter.GetBytes((int)1));

                btreeDB.Put(key, data);
                btreeDB.PutNoOverwrite(key, newData);
            }
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestPutNoDuplicateWithTxn()
        {
            testName = "TestPutNoDuplicateWithTxn";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            Transaction txn = env.BeginTransaction();
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            btreeDBConfig.Env = env;
            btreeDBConfig.Duplicates = DuplicatesPolicy.SORTED;

            DatabaseEntry key, data;
            try
            {
                using (BTreeDatabase btreeDB =
                    BTreeDatabase.Open(
                    testName + ".db", btreeDBConfig, txn))
                {
                    key = new DatabaseEntry(
                        BitConverter.GetBytes((int)0));
                    data = new DatabaseEntry(
                        BitConverter.GetBytes((int)0));

                    btreeDB.Put(key, data, txn);
                    btreeDB.PutNoDuplicate(key, data, txn);
                }
                txn.Commit();
            }
            catch (KeyExistException)
            {
                txn.Abort();
                throw new ExpectedTestException();
            }
            finally
            {
                env.Close();
            }
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestRemoveDBFile()
        {
            testName = "TestRemoveDBFile";
            testHome = testFixtureHome + "/" + testName;
            
            Configuration.ClearDir(testHome);

            RemoveDBWithoutEnv(testHome, testName, false);
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestRemoveOneDBFromDBFile()
        {
            testName = "TestRemoveOneDBFromDBFile";
            testHome = testFixtureHome + "/" + testName;

            RemoveDBWithoutEnv(testHome, testName, true);
        }

        public void RemoveDBWithoutEnv(string home, string dbName, bool ifDBName)
        {
            string dbFileName = home + "/" + dbName + ".db";

            Configuration.ClearDir(home);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;

            BTreeDatabase btreeDB;
            if (ifDBName == false)
            {
                btreeDB = BTreeDatabase.Open(dbFileName, btreeDBConfig);
                btreeDB.Close();
                BTreeDatabase.Remove(dbFileName);
                throw new ExpectedTestException();
            }
            else
            {
                btreeDB = BTreeDatabase.Open(dbFileName, dbName, btreeDBConfig);
                btreeDB.Close();
                BTreeDatabase.Remove(dbFileName, dbName);
                Assert.IsTrue(File.Exists(dbFileName));
                try
                {
                    btreeDB = BTreeDatabase.Open(dbFileName, dbName, new BTreeDatabaseConfig());
                    btreeDB.Close();
                }
                catch (DatabaseException)
                {
                    throw new ExpectedTestException();
                }
            }
        }

        [Test]
        public void TestRemoveDBFromFileInEnv()
        {
            testName = "TestRemoveDBFromFileInEnv";
            testHome = testFixtureHome + "/" + testName;

            RemoveDatabase(testHome, testName, true, true);
        }

        [Test]
        public void TestRemoveDBFromEnv()
        {
            testName = "TestRemoveDBFromEnv";
            testHome = testFixtureHome + "/" + testName;

            RemoveDatabase(testHome, testName, true, false);
        }

        public void RemoveDatabase(string home, string dbName,
            bool ifEnv, bool ifDBName)
        {
            string dbFileName = dbName + ".db";

            Configuration.ClearDir(home);

            DatabaseEnvironmentConfig envConfig = 
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseCDB = true;
            envConfig.UseMPool = true;

            DatabaseEnvironment env;
            env = DatabaseEnvironment.Open(home, envConfig);

            try
            {
                BTreeDatabaseConfig btreeDBConfig =
                    new BTreeDatabaseConfig();
                btreeDBConfig.Creation = CreatePolicy.ALWAYS;
                btreeDBConfig.Env = env;
                BTreeDatabase btreeDB = BTreeDatabase.Open(
                    dbFileName, dbName, btreeDBConfig);
                btreeDB.Close();

                if (ifEnv == true && ifDBName == true)
                    BTreeDatabase.Remove(dbFileName, dbName, env);
                else if (ifEnv == true)
                    BTreeDatabase.Remove(dbFileName, env);
            }
            catch (DatabaseException)
            {
                throw new TestException();
            }
            finally
            {
                try
                {
                    BTreeDatabaseConfig dbConfig =
                        new BTreeDatabaseConfig();
                    dbConfig.Creation = CreatePolicy.NEVER;
                    dbConfig.Env = env;
                    BTreeDatabase db = BTreeDatabase.Open(
                        dbFileName, dbName, dbConfig);
                    Assert.AreEqual(db.Creation, CreatePolicy.NEVER);
                    db.Close();
                    throw new TestException();
                }
                catch (DatabaseException)
                {
                }
                finally
                {
                    env.Close();
                }
            }
        }

        [Test]
        public void TestRenameDB()
        {
            testName = "TestRenameDB";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" + testName + ".db";
            string btreeDBName = testName;
            string newBtreeDBName = btreeDBName + "1" + ".db";

            Configuration.ClearDir(testHome);

            try
            {
                BTreeDatabaseConfig btreeDBConfig =
                    new BTreeDatabaseConfig();
                btreeDBConfig.Creation = CreatePolicy.ALWAYS;
                BTreeDatabase btreeDB = BTreeDatabase.Open(
                    btreeDBFileName, btreeDBName, btreeDBConfig);
                btreeDB.Close();
                BTreeDatabase.Rename(btreeDBFileName,
                    btreeDBName, newBtreeDBName);

                BTreeDatabaseConfig dbConfig =
                    new BTreeDatabaseConfig();
                dbConfig.Creation = CreatePolicy.NEVER;
                BTreeDatabase newDB = BTreeDatabase.Open(
                    btreeDBFileName, newBtreeDBName, dbConfig);
                newDB.Close();
            }
            catch (DatabaseException e)
            {
                throw new TestException(e.Message);
            }
            finally
            {
                try
                {
                    BTreeDatabaseConfig dbConfig =
                        new BTreeDatabaseConfig();
                    dbConfig.Creation = CreatePolicy.NEVER;
                    BTreeDatabase db = BTreeDatabase.Open(
                        btreeDBFileName, btreeDBName,
                        dbConfig);
                    throw new TestException(testName);
                }
                catch (DatabaseException)
                {
                }
            }
        }

        [Test]
        public void TestRenameDBFile()
        {
            testName = "TestRenameDB";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string newBtreeDBFileName = testHome + "/" +
                testName + "1.db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBConfig);
            btreeDB.Close();

            BTreeDatabase.Rename(btreeDBFileName,
                newBtreeDBFileName);
            Assert.IsFalse(File.Exists(btreeDBFileName));
            Assert.IsTrue(File.Exists(newBtreeDBFileName));
        }

        [Test]
        public void TestSync()
        {
            testName = "TestSync";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBName = testName + ".db";

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.ForceFlush = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            envConfig.UseLogging = true;
            envConfig.LogSystemCfg = new LogConfig();
            envConfig.LogSystemCfg.ForceSync = false;
            envConfig.LogSystemCfg.AutoRemove = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            TransactionConfig txnConfig = new TransactionConfig();
            txnConfig.SyncAction =
                TransactionConfig.LogFlush.WRITE_NOSYNC;
            Transaction txn = env.BeginTransaction(txnConfig);

            BTreeDatabaseConfig btreeConfig =
                new BTreeDatabaseConfig();
            btreeConfig.Creation = CreatePolicy.ALWAYS;
            btreeConfig.Env = env;

            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBName, btreeConfig, txn);

            DatabaseEntry key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            DatabaseEntry data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            Assert.IsFalse(btreeDB.Exists(key, txn));
            btreeDB.Put(key, data, txn);
            btreeDB.Sync();
            btreeDB.Close(false);
            txn.Commit();
            env.Close();

            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.NEVER;
            using (BTreeDatabase db = BTreeDatabase.Open(
                testHome + "/" + btreeDBName, dbConfig))
            {
                Assert.IsTrue(db.Exists(key));
            }
        }

        [Test]
        public void TestTruncate()
        {
            testName = "TestTruncate";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            btreeDBConfig.CacheSize =
                new CacheInfo(0, 30 * 1024, 1);
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBConfig);
            DatabaseEntry key;
            DatabaseEntry data;
            for (int i = 0; i < 100; i++)
            {
                key = new DatabaseEntry(
                    BitConverter.GetBytes(i));
                data = new DatabaseEntry(
                    BitConverter.GetBytes(i));
                btreeDB.Put(key, data);
            }
            uint count = btreeDB.Truncate();
            Assert.AreEqual(100, count);
            Assert.IsFalse(btreeDB.Exists(new DatabaseEntry(
                BitConverter.GetBytes((int)50))));
            btreeDB.Close();
        }

        [Test]
        public void TestTruncateInTxn()
        {
            testName = "TestTruncateInTxn";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            try
            {
                Transaction openTxn = env.BeginTransaction();
                BTreeDatabaseConfig dbConfig =
                    new BTreeDatabaseConfig();
                dbConfig.Creation = CreatePolicy.IF_NEEDED;
                dbConfig.Env = env;
                BTreeDatabase db = BTreeDatabase.Open(
                    testName + ".db", dbConfig, openTxn);
                openTxn.Commit();

                Transaction putTxn = env.BeginTransaction();
                try
                {
                    DatabaseEntry key, data;
                    for (int i = 0; i < 10; i++)
                    {
                        key = new DatabaseEntry(
                            BitConverter.GetBytes(i));
                        data = new DatabaseEntry(
                            BitConverter.GetBytes(i));
                        db.Put(key, data, putTxn);
                    }

                    putTxn.Commit();
                }
                catch (DatabaseException e)
                {
                    putTxn.Abort();
                    db.Close();
                    throw e;
                }

                Transaction trunTxn = env.BeginTransaction();
                try
                {
                    uint count = db.Truncate(trunTxn);
                    Assert.AreEqual(10, count);
                    Assert.IsFalse(db.Exists(
                        new DatabaseEntry(
                        BitConverter.GetBytes((int)5)), trunTxn));
                    trunTxn.Commit();
                    db.Close();
                }
                catch (DatabaseException)
                {
                    trunTxn.Abort();
                    db.Close();
                    throw new TestException();
                }
            }
            catch (DatabaseException)
            {
            }
            finally
            {
                env.Close();
            }   
        }

        [Test]
        public void TestTruncateUnusedPages()
        {
            testName = "TestTruncateUnusedPages";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.PageSize = 512;
            BTreeDatabase db = BTreeDatabase.Open(
                dbFileName, dbConfig);
            DatabaseEntry key, data;
            for (int i = 0; i < 100; i++)
            {
                key = new DatabaseEntry(
                    BitConverter.GetBytes(i));
                data = new DatabaseEntry(
                    BitConverter.GetBytes(i));
                db.Put(key, data);
            }

            for (int i = 0; i < 80; i++)
                db.Delete(new DatabaseEntry(
                    BitConverter.GetBytes(i)));

            uint count = db.TruncateUnusedPages();
            Assert.LessOrEqual(0, count);

            db.Close();
        }

        [Test]
        public void TestTruncateUnusedPagesWithTxn()
        {
            testName = "TestTruncateUnusedPagesWithTxn";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            BTreeDatabase db;
            try
            {
                Transaction openTxn = env.BeginTransaction();
                try
                {
                    BTreeDatabaseConfig dbConfig =
                        new BTreeDatabaseConfig();
                    dbConfig.Creation = CreatePolicy.IF_NEEDED;
                    dbConfig.Env = env;
                    dbConfig.PageSize = 512;
                    db = BTreeDatabase.Open(
                        testName + ".db", dbConfig, openTxn);
                    openTxn.Commit();
                    Assert.AreEqual(512, db.Pagesize);
                }
                catch (DatabaseException e)
                {
                    openTxn.Abort();
                    throw e;
                }
                
                Transaction putTxn = env.BeginTransaction();
                try
                {
                    DatabaseEntry key, data;
                    for (int i = 0; i < 100; i++)
                    {
                        key = new DatabaseEntry(
                            BitConverter.GetBytes(i));
                        data = new DatabaseEntry(
                            BitConverter.GetBytes(i));
                        db.Put(key, data, putTxn);
                    }

                    putTxn.Commit();
                }
                catch (DatabaseException e)
                {
                    putTxn.Abort();
                    db.Close();
                    throw e;
                }

                Transaction delTxn = env.BeginTransaction();
                try
                {
                    for (int i = 20; i <= 80; i++)
                        db.Delete(new DatabaseEntry(
                            BitConverter.GetBytes(i)), delTxn);
                    delTxn.Commit();
                }
                catch (DatabaseException e)
                {
                    delTxn.Abort();
                    db.Close();
                    throw e;
                }

                Transaction trunTxn = env.BeginTransaction();
                try
                {
                    uint trunPages = db.TruncateUnusedPages(
                        trunTxn);
                    Assert.LessOrEqual(0, trunPages);
                    trunTxn.Commit();
                    db.Close();
                }
                catch (DatabaseException)
                {
                    trunTxn.Abort();
                    db.Close();
                    throw new Exception();
                }
            }
            catch (DatabaseException)
            {
            }
            finally
            {
                env.Close();
            }
        }

        [Test]
        public void TestSalvage()
        {
            testName = "TestSalvage";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string printableOutPut = testHome + "/" +
                "printableOutPut";
            string inprintableOutPut = testHome + "/" +
                "inprintableOutPut";

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBConfig);

            DatabaseEntry key;
            DatabaseEntry data;

            for (uint i = 0; i < 10; i++)
            {
                key = new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes(i.ToString()));
                data = new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes(i.ToString()));
                btreeDB.Put(key, data);
            }

            btreeDB.Close();

            StreamWriter sw1 = new StreamWriter(printableOutPut);
            StreamWriter sw2 = new StreamWriter(inprintableOutPut);
            BTreeDatabase.Salvage(btreeDBFileName, btreeDBConfig,
                true, true, sw1);
            BTreeDatabase.Salvage(btreeDBFileName, btreeDBConfig,
                false, true, sw2);
            sw1.Close();
            sw2.Close();

            FileStream file1 = new FileStream(printableOutPut,
                FileMode.Open);
            FileStream file2 = new FileStream(inprintableOutPut,
                FileMode.Open);
            if (file1.Length == file2.Length)
            {
                int filebyte1 = 0;
                int filebyte2 = 0;
                do
                {
                    filebyte1 = file1.ReadByte();
                    filebyte2 = file2.ReadByte();
                } while ((filebyte1 == filebyte2) &&
                    (filebyte1 != -1));
                Assert.AreNotEqual(filebyte1, filebyte2);
            }

            file1.Close();
            file2.Close();
        }

        [Test]
        public void TestUpgrade()
        {
            testName = "TestUpgrade";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            string srcDBFileName = "../../bdb4.7.db";
            string testDBFileName = testHome + "/bdb4.7.db";

            FileInfo srcDBFileInfo = new FileInfo(srcDBFileName);

            //Copy the file.
            srcDBFileInfo.CopyTo(testDBFileName);
            Assert.IsTrue(File.Exists(testDBFileName));

            BTreeDatabase.Upgrade(testDBFileName,
                new DatabaseConfig(), true);

            // Open the upgraded database file.
            BTreeDatabase db = BTreeDatabase.Open(
                testDBFileName, new BTreeDatabaseConfig());
            db.Close();
        }

        [Test, ExpectedException(typeof(TestException))]
        public void TestVerify()
        {
            testName = "TestVerify";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            string btreeDBName =
                Path.GetFileNameWithoutExtension(btreeDBFileName);

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                btreeDBFileName, btreeDBName, btreeDBConfig);
            btreeDB.Close();
            btreeDBConfig.Duplicates = DuplicatesPolicy.SORTED;
            BTreeDatabase.Verify(btreeDBFileName, btreeDBConfig,
                Database.VerifyOperation.NO_ORDER_CHECK);
            try
            {
                BTreeDatabase.Verify(btreeDBFileName,
                    btreeDBConfig,
                    Database.VerifyOperation.ORDER_CHECK_ONLY);
            }
            catch (DatabaseException)
            {
                throw new TestException(testName);
            }
            finally
            {
                BTreeDatabase.Verify(btreeDBFileName,
                    btreeDBName, btreeDBConfig,
                    Database.VerifyOperation.ORDER_CHECK_ONLY);
            }

        }

        [Test]
        public void TestStats()
        {
            testName = "TestStats";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" +
                testName + ".db";
            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            ConfigCase1(dbConfig);
            BTreeDatabase db = BTreeDatabase.Open(dbFileName,
                dbConfig);

            BTreeStats stats = db.Stats();
            ConfirmStatsPart1Case1(stats);

            // Put 500 records into the database.
            PutRecordCase1(db, null);

            stats = db.Stats();
            ConfirmStatsPart2Case1(stats);

            // Delete some data to get some free pages.
            byte[] bigArray = new byte[10240];
            db.Delete(new DatabaseEntry(bigArray));

            db.PrintStats();
            db.PrintFastStats();

            db.Close();
        }

        [Test]
        public void TestStatsInTxn()
        {
            testName = "TestStatsInTxn";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            StatsInTxn(testHome, testName, false);
        }

        [Test]
        public void TestStatsWithIsolation()
        {
            testName = "TestStatsWithIsolation";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            StatsInTxn(testHome, testName, true);
        }

        [Test]
        public void TestMultipleDBSingleFile()
        {
            testName = "TestMultipleDBSingleFile";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            string dbName = "test";

            /* Create and initialize database object, open the database. */
            BTreeDatabaseConfig btreeDBconfig = new BTreeDatabaseConfig();
            btreeDBconfig.Creation = CreatePolicy.IF_NEEDED;
            btreeDBconfig.ErrorPrefix = testName;
            btreeDBconfig.UseRecordNumbers = true;

            BTreeDatabase btreeDB = BTreeDatabase.Open(btreeDBName, dbName,
                btreeDBconfig);
            btreeDB.Close();
            btreeDB = BTreeDatabase.Open(btreeDBName, dbName + "2",
                btreeDBconfig);
            btreeDB.Close();

            BTreeDatabaseConfig dbcfg = new BTreeDatabaseConfig();
            dbcfg.ReadOnly = true;
            BTreeDatabase newDb = BTreeDatabase.Open(btreeDBName, dbcfg);
            Boolean val = newDb.HasMultiple;
            Assert.IsTrue(val);
        }

        public void StatsInTxn(string home, string name, bool ifIsolation)
        {
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            EnvConfigCase1(envConfig);
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, envConfig);

            Transaction openTxn = env.BeginTransaction();
            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            ConfigCase1(dbConfig);
            dbConfig.Env = env;
            BTreeDatabase db = BTreeDatabase.Open(name + ".db",
                dbConfig, openTxn);
            openTxn.Commit();

            Transaction statsTxn = env.BeginTransaction();
            BTreeStats stats;
            BTreeStats fastStats;
            if (ifIsolation == false)
            {
                stats = db.Stats(statsTxn);
                fastStats = db.FastStats(statsTxn);
            }
            else
            {
                stats = db.Stats(statsTxn, Isolation.DEGREE_ONE);
                fastStats = db.FastStats(statsTxn, 
                    Isolation.DEGREE_ONE);
            }
            ConfirmStatsPart1Case1(stats);

            // Put 500 records into the database.
            PutRecordCase1(db, statsTxn);

            if (ifIsolation == false)
                stats = db.Stats(statsTxn);
            else
                stats = db.Stats(statsTxn, Isolation.DEGREE_TWO);
            ConfirmStatsPart2Case1(stats);

            // Delete some data to get some free pages.
            byte[] bigArray = new byte[10240];
            db.Delete(new DatabaseEntry(bigArray), statsTxn);
            if (ifIsolation == false)
                stats = db.Stats(statsTxn);
            else
                stats = db.Stats(statsTxn, Isolation.DEGREE_THREE);
            ConfirmStatsPart3Case1(stats);

            db.PrintStats(true);
            Assert.AreEqual(0, stats.EmptyPages);

            statsTxn.Commit();
            db.Close();
            env.Close();
        }

        public void EnvConfigCase1(DatabaseEnvironmentConfig cfg)
        {
            cfg.Create = true;
            cfg.UseTxns = true;
            cfg.UseMPool = true;
            cfg.UseLogging = true;
        }

        public void ConfigCase1(BTreeDatabaseConfig dbConfig)
        {
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            dbConfig.PageSize = 4096;
            dbConfig.MinKeysPerPage = 10;
        }

        public void PutRecordCase1(BTreeDatabase db, Transaction txn)
        {
            byte[] bigArray = new byte[10240];
            for (int i = 0; i < 100; i++)
            {
                if (txn == null)
                    db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)));
                else
                    db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)), txn);
            }
            for (int i = 100; i < 500; i++)
            {
                if (txn == null)
                    db.Put(new DatabaseEntry(bigArray),
                        new DatabaseEntry(bigArray));
                else
                    db.Put(new DatabaseEntry(bigArray),
                        new DatabaseEntry(bigArray), txn);
            }
        }

        public void ConfirmStatsPart1Case1(BTreeStats stats)
        {
            Assert.AreEqual(1, stats.EmptyPages);
            Assert.AreNotEqual(0, stats.LeafPagesFreeBytes);
            Assert.AreEqual(1, stats.Levels);
            Assert.AreNotEqual(0, stats.MagicNumber);
            Assert.AreEqual(1, stats.MetadataFlags);
            Assert.AreEqual(10, stats.MinKey);
            Assert.AreEqual(2, stats.nPages);
            Assert.AreEqual(4096, stats.PageSize);
            Assert.AreEqual(9, stats.Version);
        }

        public void ConfirmStatsPart2Case1(BTreeStats stats)
        {
            Assert.AreNotEqual(0, stats.DuplicatePages);
            Assert.AreNotEqual(0, stats.DuplicatePagesFreeBytes);
            Assert.AreNotEqual(0, stats.InternalPages);
            Assert.AreNotEqual(0, stats.InternalPagesFreeBytes);
            Assert.AreNotEqual(0, stats.LeafPages);
            Assert.AreEqual(500, stats.nData);
            Assert.AreEqual(101, stats.nKeys);
            Assert.AreNotEqual(0, stats.OverflowPages);
            Assert.AreNotEqual(0, stats.OverflowPagesFreeBytes);
        }

        public void ConfirmStatsPart3Case1(BTreeStats stats)
        {
            Assert.AreNotEqual(0, stats.FreePages);
        }

        private int dbIntCompare(DatabaseEntry dbt1,
            DatabaseEntry dbt2)
        {
            int a, b;
            a = BitConverter.ToInt32(dbt1.Data, 0);
            b = BitConverter.ToInt32(dbt2.Data, 0);
            return a - b;
        }

        public void SetUpEnvAndTxn(string home,
            out DatabaseEnvironment env, out Transaction txn)
        {
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            env = DatabaseEnvironment.Open(home, envConfig);
            txn = env.BeginTransaction();
        }

        public void OpenBtreeDB(DatabaseEnvironment env,
            Transaction txn, string dbFileName,
            out BTreeDatabase db)
        {
            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            if (env != null)
            {
                dbConfig.Env = env;
                dbConfig.NoMMap = false;
                db = BTreeDatabase.Open(dbFileName, dbConfig, txn);
            }
            else
            {
                db = BTreeDatabase.Open(dbFileName, dbConfig);
            }
        }

                private BTreeDatabase GetMultipleDB(
                    string filename, string dbname, BTreeDatabaseConfig cfg) {
                        BTreeDatabase ret;
                        DatabaseEntry data, key;
 
                        ret = BTreeDatabase.Open(filename, dbname, cfg);
                        key = null;
                        if (cfg.UseRecordNumbers) {
                                /* 
                                 * Dups aren't allowed with record numbers, so
                                 * we have to put different data.  Also, record
                                 * numbers start at 1, so we do too, which makes 
                                 * checking results easier.
                                 */
                                for (int i = 1; i < 100; i++) {
                                        key = new DatabaseEntry(
                                            BitConverter.GetBytes(i));
                                        data = new DatabaseEntry(
                                            BitConverter.GetBytes(i));
                                        ret.Put(key, data);
                                }

                                key = new DatabaseEntry(
                                    BitConverter.GetBytes(100));
                                data = new DatabaseEntry();
                                data.Data = new byte[111];
                                for (int i = 0; i < 111; i++)
                                        data.Data[i] = (byte)i;
                                        ret.Put(key, data);
                        } else {
                                for (int i = 0; i < 100; i++) {
                                        if (i % 10 == 0)
                                        key = new DatabaseEntry(
                                            BitConverter.GetBytes(i));
                                        data = new DatabaseEntry(
                                            BitConverter.GetBytes(i));
                                        /* Don't put nulls into the db. */
                                        Assert.IsFalse(key == null);
                                        Assert.IsFalse(data == null);
                                        ret.Put(key, data);
                                }
                                
                                if (cfg.Duplicates == DuplicatesPolicy.UNSORTED) {
                                        /* Add in duplicates to check GetBothMultiple */
                                        key = new DatabaseEntry(
                                            BitConverter.GetBytes(100));
                                        data = new DatabaseEntry(
                                            BitConverter.GetBytes(100));
                                        for (int i = 0; i < 10; i++)
                                                ret.Put(key, data);

                                        /*
                                         * Add duplicates to check GetMultiple 
                                         * with given buffer size. 
                                         */
                                        for (int i = 101; i < 1024; i++) {
                                                key = new DatabaseEntry(
                                                    BitConverter.GetBytes(101));
                                                data = new DatabaseEntry(
                                                    BitConverter.GetBytes(i));
                                                ret.Put(key, data);
                                        }

                                        key = new DatabaseEntry(
                                            BitConverter.GetBytes(102));
                                        data = new DatabaseEntry();
                                        data.Data = new byte[112];
                                        for (int i = 0; i < 112; i++)
                                 data.Data[i] = (byte)i;
                                        ret.Put(key, data);
                                }
                        }
                   return ret;  
                }

        public static void Confirm(XmlElement xmlElem,
            BTreeDatabase btreeDB, bool compulsory)
        {
            DatabaseTest.Confirm(xmlElem, btreeDB, compulsory);
            Configuration.ConfirmDuplicatesPolicy(xmlElem,
                "Duplicates", btreeDB.Duplicates, compulsory);
            Configuration.ConfirmUint(xmlElem, "MinKeysPerPage",
                btreeDB.MinKeysPerPage, compulsory);
            /*
             * BTreeDatabase.RecordNumbers is the value of
             * BTreeDatabaseConfig.UseRecordNumbers.
             */
            Configuration.ConfirmBool(xmlElem, "UseRecordNumbers",
                btreeDB.RecordNumbers, compulsory);
            /*
             * BTreeDatabase.ReverseSplit is the value of
             * BTreeDatabaseConfig.NoReverseSplitting.
             */
            Configuration.ConfirmBool(xmlElem, "NoReverseSplitting",
                btreeDB.ReverseSplit, compulsory);
            Assert.AreEqual(DatabaseType.BTREE, btreeDB.Type);
            string type = btreeDB.ToString();
            Assert.IsNotNull(type);
        }
    }
}

