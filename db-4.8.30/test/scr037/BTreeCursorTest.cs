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
using System.Threading;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class BTreeCursorTest
    {
        private string testFixtureName;
        private string testFixtureHome;
        private string testName;
        private string testHome;

        private DatabaseEnvironment paramEnv;
        private BTreeDatabase paramDB;
        private EventWaitHandle signal;

        private delegate void BTCursorMoveFuncDelegate(
                    BTreeCursor cursor, LockingInfo lockingInfo);
        private BTCursorMoveFuncDelegate btCursorFunc;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "BTreeCursorTest";
            testFixtureHome = "./TestOut/" + testFixtureName;
        }

        [Test]
        public void TestAddKeyFirst()
        {
            BTreeDatabase db;
            BTreeCursor cursor;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

            testName = "TestAddKeyFirst";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Add record("key", "data") into database.
            CursorTest.GetCursorInBtreeDBWithoutEnv(
                            testHome, testName, out db, out cursor);
            CursorTest.AddOneByCursor(db, cursor);

            // Add record("key","data1") as the first of the data item of "key".
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1")));
            cursor.Add(pair, Cursor.InsertLocation.FIRST);

            // Confirm the record is added as the first of the data item of "key".
            cursor.Move(new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")), true);
            Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data1"),
                            cursor.Current.Value.Data);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestAddKeyLast()
        {
            BTreeDatabase db;
            BTreeCursor cursor;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

            testName = "TestAddKeyLast";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Add record("key", "data") into database.
            CursorTest.GetCursorInBtreeDBWithoutEnv(testHome, testName,
                            out db, out cursor);
            CursorTest.AddOneByCursor(db, cursor);

            // Add new record("key","data1") as the last of the data item of "key".
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1")));
            cursor.Add(pair, Cursor.InsertLocation.LAST);

            // Confirm the record is added as the first of the data item of "key".
            cursor.Move(new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")), true);
            Assert.AreNotEqual(ASCIIEncoding.ASCII.GetBytes("data1"), 
                            cursor.Current.Value.Data);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestAddUnique()
        {
            BTreeDatabase db;
            BTreeCursor cursor;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

            testName = "TestAddUnique";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Open a database and cursor.
            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            
            // To put no duplicate data, the database should be set to be sorted.
            dbConfig.Duplicates = DuplicatesPolicy.SORTED;
            db = BTreeDatabase.Open(testHome + "/" + testName + ".db", dbConfig);
            cursor = db.Cursor();

            // Add record("key", "data") into database.
            CursorTest.AddOneByCursor(db, cursor);

            // Fail to add duplicate record("key","data").
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
            try
            {
                cursor.AddUnique(pair);
            }
            catch (KeyExistException)
            {
            }
            finally
            {
                cursor.Close();
                db.Close();
            }
        }

        [Test]
        public void TestDuplicateWithSamePos()
        {
            BTreeDatabase db;
            BTreeDatabaseConfig dbConfig;
            BTreeCursor cursor, dupCursor;
            DatabaseEnvironment env;
            DatabaseEnvironmentConfig envConfig;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            Transaction txn;

            testName = "TestDuplicateWithSamePos";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            envConfig = new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            envConfig.NoMMap = false;
            env = DatabaseEnvironment.Open(testHome, envConfig);

            txn = env.BeginTransaction();
            dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            db = BTreeDatabase.Open(testName + ".db", dbConfig, txn);
            key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data"));
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
            db.Put(key, data, txn);
            txn.Commit();

            txn = env.BeginTransaction();
            cursor = db.Cursor(txn);
            cursor.Move(key, true);

            //Duplicate a new cursor to the same position. 
            dupCursor = cursor.Duplicate(true);

            // Overwrite the record.
            dupCursor.Overwrite(new DatabaseEntry(
                            ASCIIEncoding.ASCII.GetBytes("newdata")));

            // Confirm that the original data doesn't exist.
            Assert.IsFalse(dupCursor.Move(pair, true));

            dupCursor.Close();
            cursor.Close();
            txn.Commit();
            db.Close();
            env.Close();
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestDuplicateToDifferentPos()
        {
            BTreeDatabase db;
            BTreeDatabaseConfig dbConfig;
            BTreeCursor cursor, dupCursor;
            DatabaseEnvironment env;
            DatabaseEnvironmentConfig envConfig;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            Transaction txn;

            testName = "TestDuplicateToDifferentPos";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            envConfig = new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            envConfig.NoMMap = false;
            env = DatabaseEnvironment.Open(testHome, envConfig);

            txn = env.BeginTransaction();
            dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            db = BTreeDatabase.Open(testName + ".db", dbConfig, txn);
            key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data"));
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
            db.Put(key, data, txn);
            txn.Commit();

            txn = env.BeginTransaction();
            cursor = db.Cursor(txn);
            cursor.Move(key, true);

            //Duplicate a new cursor to the same position. 
            dupCursor = cursor.Duplicate(false);

            /*
             * The duplicate cursor points to nothing so overwriting the 
             * record is not allowed.
             */
            try
            {
                dupCursor.Overwrite(new DatabaseEntry(
                                    ASCIIEncoding.ASCII.GetBytes("newdata")));
            }
            catch (DatabaseException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                dupCursor.Close();
                cursor.Close();
                txn.Commit();
                db.Close();
                env.Close();
            }
        }

        [Test]
        public void TestInsertAfter()
        {
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

            testName = "TestInsertAfter";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Add record("key", "data") into database.
            CursorTest.GetCursorInBtreeDBWithoutEnv(testHome, testName, 
                            out db, out cursor);
            CursorTest.AddOneByCursor(db, cursor);

            // Insert the new record("key","data1") after the record("key", "data").
            data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1"));
            cursor.Insert(data, Cursor.InsertLocation.AFTER);

            /*
             * Move the cursor to the record("key", "data") and confirm that 
             * the next record is the one just inserted. 
             */
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
            Assert.IsTrue(cursor.Move(pair, true));
            Assert.IsTrue(cursor.MoveNext());
            Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("key"), cursor.Current.Key.Data);
            Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data1"), cursor.Current.Value.Data);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveFirstMultipleAndMultipleKey()
        {
            testName = "TestMoveFirstMultipleAndMultipleKey";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> pair;
            MultipleKeyDatabaseEntry multiPair;
            int cnt;
            int[] size = new int[2];
            size[0] = 0;
            size[1] = 1024;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            for (int i = 0; i < 2; i++) {
                cnt = 0;
                if (size[i] == 0)
                    cursor.MoveFirstMultiple();
                else
                    cursor.MoveFirstMultiple(size[i]);
                pair = cursor.CurrentMultiple;
                foreach (DatabaseEntry dbt in pair.Value)
                    cnt++;
                Assert.AreEqual(1, cnt);
            }

            for (int i = 0; i < 2; i++) {
                cnt = 0;
                if (size[i] == 0)
                    cursor.MoveFirstMultipleKey();
                else
                    cursor.MoveFirstMultipleKey(size[i]);
                multiPair = cursor.CurrentMultipleKey;
                foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> 
                                    dbt in multiPair)
                    cnt++;
                Assert.Less(1, cnt);
            }

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveMultiple()
        {
            testName = "TestMoveMultiple";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry key;
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> pair;
            int cnt;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            // Move cursor to pairs with exact 99 as its key.
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)99));
            cursor.MoveMultiple(key, true);
            pair = cursor.CurrentMultiple;
            Assert.AreEqual(99, BitConverter.ToInt32(pair.Key.Data, 0));
            foreach (DatabaseEntry dbt in pair.Value) 
                cnt++;
            Assert.AreEqual(2, cnt);

            // Move cursor to pairs with the smallest key larger than 100.
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)100));
            cursor.MoveMultiple(key, false);
            pair = cursor.CurrentMultiple;
            Assert.AreEqual(101, BitConverter.ToInt32(pair.Key.Data, 0));
            foreach (DatabaseEntry dbt in pair.Value)
                cnt++;
            Assert.AreEqual(1, cnt);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveMultipleKey()
        {
            testName = "TestMoveMultipleKey";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry key;
            MultipleKeyDatabaseEntry mulPair; ;
            int cnt;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            /*
                         * Bulk retrieve key/value pair from the pair whose key 
                         * is exact 99.
                         */                          
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)99));
            cursor.MoveMultipleKey(key, true);
            mulPair = cursor.CurrentMultipleKey;
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry>
                            pair in mulPair) {
                Assert.GreaterOrEqual(3, 
                                    BitConverter.ToInt32(pair.Key.Data, 0) - 98);
                cnt++;
            }
            Assert.AreEqual(3, cnt);

            /*
             * Bulk retrieve key/value pair from the pair whose key 
             * is the smallest one larger than 100.
             */
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)100));
            cursor.MoveMultipleKey(key, false);
            mulPair = cursor.CurrentMultipleKey;
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> 
                            pair in mulPair) {
                Assert.AreEqual(101, 
                                    BitConverter.ToInt32(pair.Key.Data, 0));
                cnt++;
            }
            Assert.LessOrEqual(1, cnt);

            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)100));
            cursor.MoveMultipleKey(key, false, 1024);
            mulPair = cursor.CurrentMultipleKey;
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> 
                            pair in mulPair) {
                Assert.AreEqual(101, 
                                    BitConverter.ToInt32(pair.Key.Data, 0));
                Assert.AreEqual(101, 
                                    BitConverter.ToInt32(pair.Value.Data, 0));
                cnt++;
            }
            Assert.LessOrEqual(1, cnt);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveMultipleKeyWithRecno()
        {
            testName = "TestMoveMultipleKeyWithRecno";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            MultipleKeyDatabaseEntry multiDBT;
            int cnt;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.UseRecordNumbers = true;
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            cnt = 0;
            cursor.MoveMultipleKey(98);
            multiDBT = cursor.CurrentMultipleKey;
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> 
                            pair in multiDBT)
                cnt++;
            Assert.AreEqual(3, cnt);

            cnt = 0;
            cursor.MoveMultipleKey(98, 1024);
            multiDBT = cursor.CurrentMultipleKey;
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> 
                            pair in multiDBT)
                cnt++;
            Assert.AreEqual(3, cnt);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveMultiplePairs()
        {
            testName = "TestMoveMultiplePairs";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            MultipleKeyDatabaseEntry multiKeyDBTs1, multiKeyDBTs2;
            int cnt;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.Duplicates = DuplicatesPolicy.SORTED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            /*
                         * Bulk retrieve pairs from the pair whose key/data 
                         * is exact 99/99.
                         */                          
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)99));
            data = new DatabaseEntry(BitConverter.GetBytes((int)99));
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
            cursor.MoveMultipleKey(pair, true);
            multiKeyDBTs1 = cursor.CurrentMultipleKey;
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> 
                            p in multiKeyDBTs1)
                cnt++;
            Assert.AreEqual(3, cnt);

            // Bulk retrieve pairs from the pair whose key is exact 99.
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)99));
            data = new DatabaseEntry(BitConverter.GetBytes((int)98));
            cursor.MoveMultipleKey(pair, true);
            multiKeyDBTs2 = cursor.CurrentMultipleKey;
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> 
                            dbts in multiKeyDBTs2)
                cnt++;
            Assert.AreEqual(3, cnt);

            /*
                         * Bulk retrieve pairs from the pair whose key is 
                         * exact 99 in buffer size of 1024k.
                         */                          
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)99));
            data = new DatabaseEntry(BitConverter.GetBytes((int)102));
            cursor.MoveMultipleKey(pair, true, 1024);
            multiKeyDBTs2 = cursor.CurrentMultipleKey;
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> 
                            dbts in multiKeyDBTs2)
                cnt++;
            Assert.AreEqual(3, cnt);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveMultiplePairWithKey()
        {
            testName = "TestMoveMultiplePairWithKey";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> mulPair;
            int cnt;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            /*
                         * Move the cursor to pairs with exact 99 as its key 
                         * and 99 as its data.
                         */                          
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)99));
            data = new DatabaseEntry(BitConverter.GetBytes((int)99));
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
            cursor.MoveMultiple(pair, true);
            mulPair = cursor.CurrentMultiple;
            Assert.AreEqual(99, BitConverter.ToInt32(mulPair.Key.Data, 0));
            foreach (DatabaseEntry dbt in mulPair.Value) {
                Assert.AreEqual(99, BitConverter.ToInt32(dbt.Data, 0));
                cnt++;
            }
            Assert.AreEqual(1, cnt);

            // Move cursor to pairs with the smallest key larger than 100.
            cnt = 0;
            key = new DatabaseEntry(BitConverter.GetBytes((int)100));
            data = new DatabaseEntry(BitConverter.GetBytes((int)100));
            cursor.MoveMultiple(pair, false);
            mulPair = cursor.CurrentMultiple;
            Assert.AreEqual(99, BitConverter.ToInt32(mulPair.Key.Data, 0));
            foreach (DatabaseEntry dbt in mulPair.Value) {
                Assert.GreaterOrEqual(1, 
                                   BitConverter.ToInt32(dbt.Data, 0) - 99);
                cnt++;
            }
            Assert.AreEqual(1, cnt);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveMultipleWithRecno()
        {
            testName = "TestMoveMultipleWithRecno";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> pair;
            int cnt;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.PageSize = 1024;
            dbConfig.UseRecordNumbers = true;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            // Move cursor to the No.100 record.
            cnt = 0;
            cursor.MoveMultiple(100);
            pair = cursor.CurrentMultiple;
            Assert.AreEqual(100, BitConverter.ToInt32(pair.Key.Data, 0));
            foreach (DatabaseEntry dbt in pair.Value) 
                cnt++;
            Assert.AreEqual(1, cnt);

            // Move cursor to the No.100 record with buffer size of 1024k.
            cnt = 0;
            cursor.MoveMultiple(100, 1024);
            pair = cursor.CurrentMultiple;
            Assert.AreEqual(100, BitConverter.ToInt32(pair.Key.Data, 0));
            foreach (DatabaseEntry dbt in pair.Value)
                cnt++;
            Assert.AreEqual(1, cnt);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveNextDuplicateMultipleAndMultipleKey()
        {
            testName = "TestMoveNextDuplicateMultipleAndMultipleKey";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> pairs;
            MultipleKeyDatabaseEntry multiPair;
            int cnt;
            int[] size = new int[2];
            size[0] = 0;
            size[1] = 1024;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.Duplicates = DuplicatesPolicy.SORTED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            key = new DatabaseEntry(BitConverter.GetBytes(99));
            data = new DatabaseEntry(BitConverter.GetBytes(99));
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);

            for (int j = 0; j < 2; j++) {
                for (int i = 0; i < 2; i++) {
                    cnt = 0;
                    cursor.Move(pair, true);
                    if (j == 0) {
                        if (size[i] == 0)
                            Assert.IsTrue(cursor.MoveNextDuplicateMultiple());
                        else
                            Assert.IsTrue(cursor.MoveNextDuplicateMultiple(size[i]));
                        pairs = cursor.CurrentMultiple;
                        foreach (DatabaseEntry dbt in pairs.Value) {
                            Assert.AreEqual(100, BitConverter.ToInt32(dbt.Data, 0));
                            cnt++;
                        }
                        Assert.AreEqual(1, cnt);
                    } else {
                        if (size[i] == 0)
                            Assert.IsTrue(cursor.MoveNextDuplicateMultipleKey());
                        else
                            Assert.IsTrue(cursor.MoveNextDuplicateMultipleKey(size[i]));
                        multiPair = cursor.CurrentMultipleKey;
                        foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> p in multiPair) {
                            Assert.AreEqual(100, BitConverter.ToInt32(p.Value.Data, 0));
                            cnt++;
                        }
                        Assert.AreEqual(1, cnt);
                    }
                }
            }

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveNextUniqueMultipleAndMultipleKey()
        {
            testName = "TestMoveNextUniqueMultipleAndMultipleKey";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> pairs;
            MultipleKeyDatabaseEntry multiPair;
            int cnt;
            int[] size = new int[2];
            size[0] = 0;
            size[1] = 1024;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            key = new DatabaseEntry(BitConverter.GetBytes(99));
            data = new DatabaseEntry(BitConverter.GetBytes(99));
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);

            for (int j = 0; j < 2; j++) {
                for (int i = 0; i < 2; i++) {
                    cnt = 0;
                    cursor.Move(pair, true);
                    if (j == 0) {
                        if (size[i] == 0)
                            Assert.IsTrue(cursor.MoveNextUniqueMultiple());
                        else
                            Assert.IsTrue(cursor.MoveNextUniqueMultiple(size[i]));
                        pairs = cursor.CurrentMultiple;
                        foreach (DatabaseEntry dbt in pairs.Value) {
                            Assert.AreEqual(101,  BitConverter.ToInt32(dbt.Data, 0));
                            cnt++;
                        }
                        Assert.AreEqual(1, cnt);
                    } else {
                        if (size[i] == 0)
                            Assert.IsTrue(cursor.MoveNextUniqueMultipleKey());
                        else
                            Assert.IsTrue(cursor.MoveNextUniqueMultipleKey(size[i]));
                        multiPair = cursor.CurrentMultipleKey;
                        foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> p in multiPair) {
                            Assert.AreEqual(101, BitConverter.ToInt32(p.Value.Data, 0));
                            cnt++;
                        }
                        Assert.AreEqual(1, cnt);
                    }
                }
            }

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestRefreshMultipleAndMultipleKey()
        {
            testName = "TestRefreshMultipleAndMultipleKey";
            testHome = testFixtureHome + "/" + testName;
            string btreeDBFileName = testHome + "/" +
                testName + ".db";
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry key, data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> pairs;
            MultipleKeyDatabaseEntry multiPair;
            int cnt;
            int[] size = new int[2];
            size[0] = 0;
            size[1] = 1024;

            Configuration.ClearDir(testHome);

            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.ALWAYS;
            dbConfig.Duplicates = DuplicatesPolicy.SORTED;
            dbConfig.PageSize = 1024;
            GetMultipleDB(btreeDBFileName, dbConfig, out db, out cursor);

            key = new DatabaseEntry(BitConverter.GetBytes(99));
            data = new DatabaseEntry(BitConverter.GetBytes(99));
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);

            for (int j = 0; j < 2; j++) {
                for (int i = 0; i < 2; i++) {
                    cnt = 0;
                    cursor.Move(pair, true);
                    if (j == 0) {
                        if (size[i] == 0)
                            Assert.IsTrue(cursor.RefreshMultiple());
                        else
                            Assert.IsTrue(cursor.RefreshMultiple(size[i]));
                        pairs = cursor.CurrentMultiple;
                        foreach (DatabaseEntry dbt in pairs.Value)
                            cnt++;
                        Assert.AreEqual(2, cnt);
                    } else {
                        if (size[i] == 0)
                            Assert.IsTrue(cursor.RefreshMultipleKey());
                        else
                            Assert.IsTrue(cursor.RefreshMultipleKey(size[i]));
                        multiPair = cursor.CurrentMultipleKey;
                        foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> p in multiPair)
                            cnt++;
                        Assert.AreEqual(3, cnt);
                    }
                }
            }

            cursor.Close();
            db.Close();
        }

        private void GetMultipleDB(string dbFileName, BTreeDatabaseConfig dbConfig,
            out BTreeDatabase db, out BTreeCursor cursor)
        {
            db = BTreeDatabase.Open(dbFileName, dbConfig);
            cursor = db.Cursor();

            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            DatabaseEntry key, data;
            for (int i = 1; i < 100; i++) {
                key = new DatabaseEntry(BitConverter.GetBytes(i));
                data = new DatabaseEntry(BitConverter.GetBytes(i));
                pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
                cursor.Add(pair);
            }

            if (dbConfig.UseRecordNumbers == true) {
                byte[] bytes = new byte[512];
                for (int i = 0; i < 512; i++)
                    bytes[i] = (byte)i;
                key = new DatabaseEntry(BitConverter.GetBytes(100));
                data = new DatabaseEntry(bytes);
                pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
                cursor.Add(pair);
            } else {
                if (dbConfig.Duplicates == DuplicatesPolicy.UNSORTED || 
                    dbConfig.Duplicates == DuplicatesPolicy.SORTED) {
                    key = new DatabaseEntry(BitConverter.GetBytes(99));
                    data = new DatabaseEntry(BitConverter.GetBytes(100));
                    pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
                    cursor.Add(pair);
                }

                key = new DatabaseEntry(BitConverter.GetBytes(101));
                data = new DatabaseEntry(BitConverter.GetBytes(101));
                pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
                cursor.Add(pair);
            }
        }

        [Test]
        public void TestInsertBefore()
        {
            BTreeDatabase db;
            BTreeCursor cursor;
            DatabaseEntry data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

            testName = "TestInsertBefore";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            // Add record("key", "data") into database.
            CursorTest.GetCursorInBtreeDBWithoutEnv(
                testHome, testName, out db, out cursor);
            CursorTest.AddOneByCursor(db, cursor);

            // Insert the new record("key","data1") before the record("key", "data").
            data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1"));
            cursor.Insert(data, Cursor.InsertLocation.BEFORE);

            /*
             * Move the cursor to the record("key", "data") and confirm 
             * that the previous record is the one just inserted.
             */
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
            Assert.IsTrue(cursor.Move(pair, true));
            Assert.IsTrue(cursor.MovePrev());
            Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("key"),cursor.Current.Key.Data);
            Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data1"), cursor.Current.Value.Data);

            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveToRecno()
        {
            BTreeDatabase db;
            BTreeCursor cursor;

            testName = "TestMoveToRecno";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            GetCursorInBtreeDBUsingRecno(testHome, testName,
                out db, out cursor);
            for (int i = 0; i < 10; i++)
                db.Put(
                    new DatabaseEntry(BitConverter.GetBytes(i)),
                    new DatabaseEntry(BitConverter.GetBytes(i)));

            MoveCursorToRecno(cursor, null);
            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestMoveToRecnoWithRMW()
        {
            testName = "TestMoveToRecnoWithRMW";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            btCursorFunc = new BTCursorMoveFuncDelegate(
                MoveCursorToRecno);

            // Move to a specified key and a key/data pair.
            MoveWithRMW(testHome, testName);
        }

        [Test]
        public void TestRecno()
        {
            BTreeDatabase db;
            BTreeCursor cursor;

            testName = "TestRecno";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            GetCursorInBtreeDBUsingRecno(testHome, testName,
                out db, out cursor);
            for (int i = 0; i < 10; i++)
                db.Put(
                    new DatabaseEntry(BitConverter.GetBytes(i)),
                    new DatabaseEntry(BitConverter.GetBytes(i)));

            ReturnRecno(cursor, null);
            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestRecnoWithRMW()
        {
            testName = "TestRecnoWithRMW";
            testHome = testFixtureHome + "/" + testName;
            Configuration.ClearDir(testHome);

            // Use MoveCursorToRecno() as its move function.
            btCursorFunc = new BTCursorMoveFuncDelegate(
                ReturnRecno);

            // Move to a specified key and a key/data pair.
            MoveWithRMW(testHome, testName);
        }

        /*
         * Move the cursor according to recno. The recno 
         * starts from 1 and is increased by 1.
         */
        public void MoveCursorToRecno(BTreeCursor cursor,
            LockingInfo lck)
        {
            for (uint i = 1; i <= 5; i++)
                if (lck == null)
                    Assert.IsTrue(cursor.Move(i));
                else
                    Assert.IsTrue(cursor.Move(i, lck));
        }

        /*l
         * Move the cursor according to a given recno and 
         * return the current record's recno. The given recno
         * and the one got from the cursor should be the same.
         */
        public void ReturnRecno(BTreeCursor cursor,
            LockingInfo lck)
        {
            for (uint i = 1; i <= 5; i++)
                if (lck == null)
                {
                    if (cursor.Move(i) == true)
                        Assert.AreEqual(i, cursor.Recno());
                }
                else
                {
                    if (cursor.Move(i, lck) == true)
                        Assert.AreEqual(i, cursor.Recno(lck));
                }
        }

        public void GetCursorInBtreeDBUsingRecno(string home,
            string name, out BTreeDatabase db,
            out BTreeCursor cursor)
        {
            string dbFileName = home + "/" + name + ".db";
            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.UseRecordNumbers = true;
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            db = BTreeDatabase.Open(dbFileName, dbConfig);
            cursor = db.Cursor();
        }

        public void RdMfWt()
        {
            Transaction txn = paramEnv.BeginTransaction();
            BTreeCursor dbc = paramDB.Cursor(txn);

            try
            {
                LockingInfo lck = new LockingInfo();
                lck.ReadModifyWrite = true;

                // Read record.
                btCursorFunc(dbc, lck);

                // Block the current thread until event is set.
                signal.WaitOne();

                // Write new records into database.
                DatabaseEntry key = new DatabaseEntry(BitConverter.GetBytes(55));
                DatabaseEntry data = new DatabaseEntry(BitConverter.GetBytes(55));
                dbc.Add(new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data));

                dbc.Close();
                txn.Commit();
            }
            catch (DeadlockException)
            {
                dbc.Close();
                txn.Abort();
            }
        }


        public void MoveWithRMW(string home, string name)
        {
            paramEnv = null;
            paramDB = null;

            // Open the environment.
            DatabaseEnvironmentConfig envCfg =
                new DatabaseEnvironmentConfig();
            envCfg.Create = true;
            envCfg.FreeThreaded = true;
            envCfg.UseLocking = true;
            envCfg.UseLogging = true;
            envCfg.UseMPool = true;
            envCfg.UseTxns = true;
            paramEnv = DatabaseEnvironment.Open(home, envCfg);

            // Open database in transaction.
            Transaction openTxn = paramEnv.BeginTransaction();
            BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
            cfg.Creation = CreatePolicy.ALWAYS;
            cfg.Env = paramEnv;
            cfg.FreeThreaded = true;
            cfg.PageSize = 4096;
            // Use record number.
            cfg.UseRecordNumbers = true;
            paramDB = BTreeDatabase.Open(name + ".db", cfg, openTxn);
            openTxn.Commit();

            /*
             * Put 10 different, 2 duplicate and another different 
             * records into database.
             */
            Transaction txn = paramEnv.BeginTransaction();
            for (int i = 0; i < 13; i++)
            {
                DatabaseEntry key, data;
                if (i == 10 || i == 11)
                {
                    key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key"));
                    data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data"));
                }
                else
                {
                    key = new DatabaseEntry(BitConverter.GetBytes(i));
                    data = new DatabaseEntry(BitConverter.GetBytes(i));
                }
                paramDB.Put(key, data, txn);
            }

            txn.Commit();

            // Get a event wait handle.
            signal = new EventWaitHandle(false,
                EventResetMode.ManualReset);

            /*
             * Start RdMfWt() in two threads. RdMfWt() reads 
             * and writes data into database.
             */
            Thread t1 = new Thread(new ThreadStart(RdMfWt));
            Thread t2 = new Thread(new ThreadStart(RdMfWt));
            t1.Start();
            t2.Start();

            // Give both threads time to read before signalling them to write. 
            Thread.Sleep(1000);

            // Invoke the write operation in both threads.
            signal.Set();

            // Return the number of deadlocks.
            while (t1.IsAlive || t2.IsAlive)
            {
                /*
                 * Give both threads time to write before 
                 * counting the number of deadlocks.
                 */
                Thread.Sleep(1000);
                uint deadlocks = paramEnv.DetectDeadlocks(DeadlockPolicy.DEFAULT);
                
                // Confirm that there won't be any deadlock. 
                Assert.AreEqual(0, deadlocks);
            }

            t1.Join();
            t2.Join();
            paramDB.Close();
            paramEnv.Close();
        }

    }
}


