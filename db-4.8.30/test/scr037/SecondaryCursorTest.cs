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
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class SecondaryCursorTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryCursorTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            /*
             * Delete existing test ouput directory and files specified 
             * for the current test fixture and then create a new one.
             */
            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestDuplicate()
        {
            testName = "TestDuplicate";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName + 
                "_sec.db";

            Configuration.ClearDir(testHome);

            // Open a primary database.
            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase db = BTreeDatabase.Open(
                dbFileName, dbConfig);

            // Open a secondary database.
            SecondaryBTreeDatabaseConfig secConfig = 
                new SecondaryBTreeDatabaseConfig(db, 
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secConfig.Creation = CreatePolicy.IF_NEEDED;
            secConfig.Duplicates = DuplicatesPolicy.UNSORTED;
            SecondaryBTreeDatabase secDB =
                SecondaryBTreeDatabase.Open(dbSecFileName, 
                secConfig);

            // Put a pair of key and data into the database.
            DatabaseEntry key, data;
            key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            db.Put(key, data);

            // Create a cursor.
            SecondaryCursor cursor = secDB.SecondaryCursor();
            cursor.Move(key, true);

            // Duplicate the cursor.
            SecondaryCursor dupCursor;
            dupCursor = cursor.Duplicate(true);

            /*
             * Confirm that the duplicate cursor has the same 
             * position as the original one.
             */
            Assert.AreEqual(cursor.Current.Key, 
                dupCursor.Current.Key);
            Assert.AreEqual(cursor.Current.Value, 
                dupCursor.Current.Value);

            // Close the cursor and the duplicate cursor.
            dupCursor.Close();
            cursor.Close();

            // Close secondary and primary database.
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestDelete()
        {
            testName = "TestDelete";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            // Open a primary database and its secondary database.
            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db, out secDB);

            // Put a pair of key and data into database.
            DatabaseEntry key, data;
            key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            db.Put(key, data);

            // Delete the pair with cursor.
            SecondaryCursor secCursor = secDB.SecondaryCursor();
            Assert.IsTrue(secCursor.MoveFirst());
            secCursor.Delete();

            // Confirm that the pair is deleted.
            Assert.IsFalse(db.Exists(key));

            // Close all databases.
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestCurrent()
        {
            testName = "TestCurrent";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            // Open a primary database and its secondary database.
            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db, out secDB);

            // Put a pair of key and data into database.
            DatabaseEntry key, data;
            key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            db.Put(key, data);

            // Delete the pair with cursor.
            SecondaryCursor secCursor = secDB.SecondaryCursor();
            Assert.IsTrue(secCursor.Move(data, true));
            
            // Confirm that the current is the one we put into database.
            Assert.AreEqual(data.Data, secCursor.Current.Key.Data);

            // Close all databases. 
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestGetEnumerator()
        {
            testName = "TestGetEnumerator";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            // Open a primary database and its secondary database.
            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db, 
                out secDB);

            // Write ten records into the database.
            WriteRecords(db);

            /*
             * Get all records from the secondary database and see
             * if the records with key other than 10 have the same as 
             * their primary key.
             */ 
            SecondaryCursor secCursor = secDB.SecondaryCursor();
            foreach (KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>> secData in secCursor)
            {
                if (BitConverter.ToInt32(secData.Key.Data, 0) != 10)
                    Assert.AreEqual(secData.Value.Key.Data,
                        secData.Value.Value.Data);
            }

            // Close all cursors and databases.
            secCursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestMoveToKey()
        {
            testName = "TestMoveToKey";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            MoveToPos(dbFileName, dbSecFileName, false);
        }

        [Test]
        public void TestMoveToPair()
        {
            testName = "TestMoveToPair";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            MoveToPos(dbFileName, dbSecFileName, true);
        }

        public void MoveToPos(string dbFileName, 
            string dbSecFileName, bool ifPair)
        {
            // Open a primary database and its secondary database.
            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);

            // Write ten records into the database.
            WriteRecords(db);

            SecondaryCursor secCursor = secDB.SecondaryCursor();
            DatabaseEntry key = new DatabaseEntry(
                BitConverter.GetBytes((int)0));
            DatabaseEntry notExistingKey = new DatabaseEntry(
                BitConverter.GetBytes((int)100));
            if (ifPair == false)
            {
                Assert.IsTrue(secCursor.Move(key, true));
                Assert.IsFalse(secCursor.Move(notExistingKey, true));
            }
            else
            {
                KeyValuePair<DatabaseEntry, KeyValuePair<
                    DatabaseEntry, DatabaseEntry>> pair = 
                    new KeyValuePair<DatabaseEntry,
                    KeyValuePair<DatabaseEntry, DatabaseEntry>>(key,
                    new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                    key, key));

                KeyValuePair<DatabaseEntry, KeyValuePair<
                    DatabaseEntry, DatabaseEntry>> notExistingPair;

                notExistingPair = new KeyValuePair<DatabaseEntry,
                    KeyValuePair<DatabaseEntry, DatabaseEntry>>(
                    notExistingKey, new KeyValuePair<
                    DatabaseEntry, DatabaseEntry>(
                    notExistingKey, notExistingKey));
                Assert.IsTrue(secCursor.Move(pair, true));
                Assert.IsFalse(secCursor.Move(notExistingPair, true));
            }

            secCursor.Close();
            secDB.Close();
            db.Close(); 
        }

        [Test]
        public void TestMoveToKeyWithLockingInfo()
        {
            testName = "TestMoveToKeyWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName +  "_sec.db";

            Configuration.ClearDir(testHome);

            MoveToPosWithLockingInfo(testHome, dbFileName, 
                dbSecFileName, false);
        }

        [Test]
        public void TestMoveToPairWithLockingInfo()
        {
            testName = "TestMoveToPairWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            MoveToPosWithLockingInfo(testHome, dbFileName,
                dbSecFileName, true);
        }

        public void MoveToPosWithLockingInfo(string home, 
            string dbFileName, string dbSecFileName, bool ifPair)
        {
            // Open a primary database and its secondary database.
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(home, dbFileName, dbSecFileName, 
                out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            // Create an secondary cursor.
            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor secCursor = secDB.SecondaryCursor(
                cursorTxn);
            DatabaseEntry key = new DatabaseEntry(
                BitConverter.GetBytes((int)0));
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_THREE;
            lockingInfo.ReadModifyWrite = true;
            if (ifPair == false)
            {
                Assert.IsTrue(secCursor.Move(key, true, lockingInfo));
            }
            else
            {
                KeyValuePair<DatabaseEntry, KeyValuePair<
                    DatabaseEntry, DatabaseEntry>> pair;

                pair = new KeyValuePair<DatabaseEntry,
                    KeyValuePair<DatabaseEntry, DatabaseEntry>>(key,
                    new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                    key, key));
                Assert.IsTrue(secCursor.Move(pair, true, lockingInfo));
            }
            secCursor.Close();
            cursorTxn.Commit();

            secDB.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMoveFirst()
        {
            testName = "TestMoveFirst";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            // Open primary and secondary database.
            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db, 
                out secDB);


            // Write ten records into the database.
            WriteRecords(db);

            // Move the cursor to the first record(0,0).
            SecondaryCursor cursor = secDB.SecondaryCursor();
            Assert.IsTrue(cursor.MoveFirst());
            Assert.AreEqual(BitConverter.GetBytes((int)0),
                cursor.Current.Key.Data);

            // Close all.
            cursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestMoveLast()
        {
            testName = "TestMoveLast";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);
            WriteRecords(db);

            SecondaryCursor cursor = secDB.SecondaryCursor();
            Assert.IsTrue(cursor.MoveLast());
            Assert.AreEqual(BitConverter.GetBytes((int)10), 
                cursor.Current.Key.Data);

            cursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestMoveNext()
        {
            testName = "TestMoveNext";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);
            WriteRecords(db);

            SecondaryCursor cursor = secDB.SecondaryCursor();
            cursor.MoveFirst();
            for (int i = 0; i < 5; i++)
                Assert.IsTrue(cursor.MoveNext());

            cursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestMoveNextDuplicate()
        {
            testName = "TestMoveNextDuplicate";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);
            WriteRecords(db);

            // Create a cursor and move the cursor to duplicate record.
            SecondaryCursor cursor = secDB.SecondaryCursor();
            cursor.Move(new DatabaseEntry(
                BitConverter.GetBytes((int)10)), true);
            Assert.IsTrue(cursor.MoveNextDuplicate());
            Assert.AreEqual(BitConverter.GetBytes((int)10), 
                cursor.Current.Key.Data);

            cursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestMoveNextUnique()
        {
            testName = "TestMoveNextUnique";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);
            WriteRecords(db);

            /*
             * Move cursor to duplicate record. Since the duplicate
             * record has the largest key, moving to the next 
             * unique record should fail.
             */
            SecondaryCursor cursor = secDB.SecondaryCursor();
            cursor.Move(new DatabaseEntry(
                BitConverter.GetBytes((int)10)), true);
            Assert.IsFalse(cursor.MoveNextUnique());

            cursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestMovePrev()
        {
            testName = "TestMovePrev";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);
            WriteRecords(db);

            SecondaryCursor cursor = secDB.SecondaryCursor();
            cursor.MoveLast();
            for (int i = 0; i < 5; i++)
                Assert.IsTrue(cursor.MovePrev());

            cursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestMovePrevDuplicate()
        {
            testName = "TestMovePrevDuplicate";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);
            WriteRecords(db);

            SecondaryCursor cursor = secDB.SecondaryCursor();
            KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>> pair;
            DatabaseEntry pKey, pData;
            pKey = new DatabaseEntry(BitConverter.GetBytes((int)6));
            pData = new DatabaseEntry(BitConverter.GetBytes((int)10));
            pair = new KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>>(pData, 
                new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                pKey, pData));
            cursor.Move(pair, true);
            Assert.IsTrue(cursor.MovePrevDuplicate());
            Assert.AreEqual(BitConverter.GetBytes((int)10),
                cursor.Current.Key.Data);

            cursor.Close();
            secDB.Close();
            db.Close();
        }


        [Test]
        public void TestMovePrevUnique()
        {
            testName = "TestMovePrevUnique";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);
            WriteRecords(db);

            SecondaryCursor cursor = secDB.SecondaryCursor();
            KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>> pair;
            DatabaseEntry pKey, pData;
            pKey = new DatabaseEntry(BitConverter.GetBytes((int)6));
            pData = new DatabaseEntry(BitConverter.GetBytes((int)10));
            pair = new KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>>(pData,
                new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                pKey, pData));
            cursor.Move(pair, true);
            Assert.IsTrue(cursor.MovePrevUnique());
            Assert.AreNotEqual(BitConverter.GetBytes((int)10),
                cursor.Current.Key.Data);

            cursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestRefresh()
        {
            testName = "TestRefresh";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" + testName +
                "_sec.db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenSecDB(dbFileName, dbSecFileName, out db,
                out secDB);
            WriteRecords(db);

            SecondaryCursor cursor = secDB.SecondaryCursor();
            KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>> pair;
            DatabaseEntry pKey, pData;
            pKey = new DatabaseEntry(BitConverter.GetBytes((int)6));
            pData = new DatabaseEntry(BitConverter.GetBytes((int)10));
            pair = new KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>>(pData,
                new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                pKey, pData));
            cursor.Move(pair, true);
            Assert.IsTrue(cursor.Refresh());
            Assert.AreEqual(pData.Data, cursor.Current.Key.Data);
            Assert.AreEqual(pKey.Data, cursor.Current.Value.Key.Data);

            cursor.Close();
            secDB.Close();
            db.Close();
        }

        [Test]
        public void TestMoveFirstWithLockingInfo()
        {
            testName = "TestMoveFirstWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */ 
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName, dbSecFileName, 
                out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            // Move the cursor to the first record(0, 0).
            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor = 
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_THREE;
            lockingInfo.ReadModifyWrite = true;
            Assert.IsTrue(cursor.MoveFirst(lockingInfo));
            Assert.AreEqual(BitConverter.GetBytes((int)0),
                cursor.Current.Key.Data);
            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMoveLastWithLockingInfo()
        {
            testName = "TestMoveLastWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName, dbSecFileName,
                out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            /*
             * Move the cursor to the last record(10, 6), that is 
             * record(6, 10) in primary database.
             */ 
            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_THREE;
            lockingInfo.ReadModifyWrite = true;
            Assert.IsTrue(cursor.MoveLast(lockingInfo));
            Assert.AreEqual(BitConverter.GetBytes((int)10),
                cursor.Current.Key.Data);
            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMoveNextWithLockingInfo()
        {
            testName = "TestMoveNextWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName, dbSecFileName,
                out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            /*
             * Move cursor to the first record and move to next 
             * record for five times.
             */
            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_THREE;
            lockingInfo.ReadModifyWrite = true;
            cursor.MoveFirst(lockingInfo);
            for (int i = 0; i < 5; i++)
                Assert.IsTrue(cursor.MoveNext(lockingInfo));
            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMoveNextDuplicateWithLockingInfo()
        {
            testName = "TestMoveNextDuplicateWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName, 
                dbSecFileName, out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            /*
             * Create a cursor and move the cursor to duplicate 
             * record(10, 5), that is record(5,10) in primary database.
             * Then move the cursor to the next duplicate record
             * (10, 6), that is record(6,10) in primary database.
             */ 
            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_THREE;
            lockingInfo.ReadModifyWrite = true;
            cursor.Move(new DatabaseEntry(
                BitConverter.GetBytes((int)10)), true, lockingInfo);
            Assert.IsTrue(cursor.MoveNextDuplicate(lockingInfo));
            Assert.AreEqual(BitConverter.GetBytes((int)10),
                cursor.Current.Key.Data);
            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMoveNextUniqueWithLockingInfo()
        {
            testName = "TestMoveNextUniqueWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName,
                dbSecFileName, out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            /*
             * Move cursor to duplicate record. Since the duplicate
             * record has the largest key, moving to the next 
             * unique record should fail.
             */
            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_THREE;
            lockingInfo.ReadModifyWrite = true;
            cursor.Move(new DatabaseEntry(
                BitConverter.GetBytes((int)10)), true);
            Assert.IsFalse(cursor.MoveNextUnique());
            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMovePrevWithLockingInfo()
        {
            testName = "TestMovePrevWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName,
                dbSecFileName, out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            /*
             * Move the cursor to the last record and move to its 
             * previous record for five times.
             */ 
            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_TWO;
            lockingInfo.ReadModifyWrite = true;
            cursor.MoveLast(lockingInfo);
            for (int i = 0; i < 5; i++)
                Assert.IsTrue(cursor.MovePrev(lockingInfo));
            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMovePrevDuplicateWithLockingInfo()
        {
            testName = "TestMovePrevDuplicateWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName,
                dbSecFileName, out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_TWO;
            lockingInfo.ReadModifyWrite = true;

            /*
             * Move the cursor to the record(10,6), that is the
             * record(6, 10) in the primary database. Move to
             * its previous duplicate record, that is (10,5).
             */
            KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>> pair;
            DatabaseEntry pKey, pData;
            pKey = new DatabaseEntry(BitConverter.GetBytes((int)6));
            pData = new DatabaseEntry(BitConverter.GetBytes((int)10));
            pair = new KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>>(pData,
                new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                pKey, pData));
            cursor.Move(pair, true, lockingInfo);
            Assert.IsTrue(cursor.MovePrevDuplicate(lockingInfo));
            Assert.AreEqual(BitConverter.GetBytes((int)10),
                cursor.Current.Key.Data);

            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }


        [Test]
        public void TestMovePrevUniqueWithLockingInfo()
        {
            testName = "TestMovePrevUniqueWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName,
                dbSecFileName, out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_TWO;
            lockingInfo.ReadModifyWrite = true;

            /*
             * Move the cursor to the record(10, 6) and move to the
             * previous unique record which has different key from 
             * the record(10,6).
             */ 
            KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>> pair;
            DatabaseEntry pKey, pData;
            pKey = new DatabaseEntry(BitConverter.GetBytes((int)6));
            pData = new DatabaseEntry(BitConverter.GetBytes((int)10));
            pair = new KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>>(pData,
                new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                pKey, pData));
            cursor.Move(pair, true, lockingInfo);
            Assert.IsTrue(cursor.MovePrevUnique(lockingInfo));
            Assert.AreNotEqual(BitConverter.GetBytes((int)10),
                cursor.Current.Key.Data);

            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestRefreshWithLockingInfo()
        {
            testName = "TestRefreshWithLockingInfo";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            /*
             * Open environment, primary database and
             * secondary database.
             */
            BTreeDatabase db;
            DatabaseEnvironment env;
            SecondaryBTreeDatabase secDB;
            OpenSecDBInTxn(testHome, dbFileName,
                dbSecFileName, out env, out db, out secDB);

            // Write ten records into the database.
            WriteRecordsInTxn(db, env);

            Transaction cursorTxn = env.BeginTransaction();
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorTxn);
            LockingInfo lockingInfo = new LockingInfo();
            lockingInfo.IsolationDegree = Isolation.DEGREE_TWO;
            lockingInfo.ReadModifyWrite = true;

            // Move cursor to a record and refresh it.
            KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>> pair;
            DatabaseEntry pKey, pData;
            pKey = new DatabaseEntry(BitConverter.GetBytes((int)6));
            pData = new DatabaseEntry(BitConverter.GetBytes((int)10));
            pair = new KeyValuePair<DatabaseEntry, KeyValuePair<
                DatabaseEntry, DatabaseEntry>>(pData,
                new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                pKey, pData));
            cursor.Move(pair, true, lockingInfo);
            Assert.IsTrue(cursor.Refresh(lockingInfo));
            Assert.AreEqual(pData.Data, cursor.Current.Key.Data);
            Assert.AreEqual(pKey.Data, cursor.Current.Value.Key.Data);

            cursor.Close();
            cursorTxn.Commit();

            // Close all.
            secDB.Close();
            db.Close();
            env.Close();
        }

        public void OpenSecDBInTxn(string home, string dbFileName, 
            string dbSecFileName, out DatabaseEnvironment env,
            out BTreeDatabase db, out SecondaryBTreeDatabase secDB)
        { 
            // Open environment.
            DatabaseEnvironmentConfig envCfg =
                new DatabaseEnvironmentConfig();
            envCfg.Create = true;
            envCfg.UseLocking = true;
            envCfg.UseLogging = true;
            envCfg.UseMPool = true;
            envCfg.UseTxns = true;
            env = DatabaseEnvironment.Open(
                home, envCfg);

            // Open primary and secondary database in a transaction.
            Transaction openTxn = env.BeginTransaction();
            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            dbConfig.PageSize = 4096;
            dbConfig.Duplicates = DuplicatesPolicy.NONE;
            dbConfig.ReadUncommitted = true;
            db = BTreeDatabase.Open(dbFileName, dbConfig, 
                openTxn);
            openTxn.Commit();

            openTxn = env.BeginTransaction();
            SecondaryBTreeDatabaseConfig secConfig =
                new SecondaryBTreeDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secConfig.Creation = CreatePolicy.IF_NEEDED;
            secConfig.Duplicates = DuplicatesPolicy.SORTED;
            secConfig.Env = env;
            secConfig.ReadUncommitted = true;
            secDB = SecondaryBTreeDatabase.Open(dbSecFileName,
                secConfig, openTxn);
            openTxn.Commit();
        }

        public void WriteRecords(BTreeDatabase db)
        {
            /*
             * Write ten records into the database. The records 
             * from 1st to 5th and 8th to 10th are unique in the 
             * database. The data in the 6th and 7th records 
             * are the same.
             */ 
            for (int i = 0; i < 10; i++)
            {
                if (i == 5 || i == 6)
                    db.Put(new DatabaseEntry(
                        BitConverter.GetBytes(i)),
                        new DatabaseEntry(
                        BitConverter.GetBytes((int)10)));
                else
                    db.Put(new DatabaseEntry(
                        BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)));
            }
        }

        public void WriteRecordsInTxn(BTreeDatabase db, 
            DatabaseEnvironment env)
        {
            Transaction txn = env.BeginTransaction();
            /*
             * Write ten records into the database. The records 
             * from 1st to 5th and 8th to 10th are unique in the 
             * database. The data in the 6th and 7th records 
             * are the same.
             */
            for (int i = 0; i < 10; i++)
            {
                if (i == 5 || i == 6)
                    db.Put(new DatabaseEntry(
                        BitConverter.GetBytes(i)),
                        new DatabaseEntry(
                        BitConverter.GetBytes((int)10)), txn);
                else
                    db.Put(new DatabaseEntry(
                        BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)), txn);
            }

            txn.Commit();
        }


        public void OpenSecDB(string dbFileName, 
            string dbSecFileName, out BTreeDatabase db,
            out SecondaryBTreeDatabase secDB)
        {
            // Open a primary database.
            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            db = BTreeDatabase.Open(dbFileName, dbConfig);

            // Open a secondary database.
            SecondaryBTreeDatabaseConfig secConfig =
                new SecondaryBTreeDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secConfig.Creation = CreatePolicy.IF_NEEDED;
            secConfig.Duplicates = DuplicatesPolicy.SORTED;
            secDB = SecondaryBTreeDatabase.Open(dbSecFileName,
                secConfig);
        }


        public DatabaseEntry SecondaryKeyGen(
            DatabaseEntry key, DatabaseEntry data)
        {
            DatabaseEntry dbtGen;
            dbtGen = new DatabaseEntry(data.Data);
            return dbtGen;
        }
    }
}
