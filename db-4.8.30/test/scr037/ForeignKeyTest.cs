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

namespace CsharpAPITest {
    [TestFixture]
    public class ForeignKeyTest {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests() {
            testFixtureName = "ForeignKeyTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

        }

        [Test]
        public void TestAbortBTree() {
            testName = "TestAbortBTree";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.BTREE, ForeignKeyDeleteAction.ABORT);
        }
        [Test]
        public void TestAbortHash() {
            testName = "TestAbortHash";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.HASH, ForeignKeyDeleteAction.ABORT);
        }
        [Test]
        public void TestAbortQueue() {
            testName = "TestAbortQueue";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.QUEUE, ForeignKeyDeleteAction.ABORT);
        }
        [Test]
        public void TestAbortRecno() {
            testName = "TestAbortRecno";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.RECNO, ForeignKeyDeleteAction.ABORT);
        }

        [Test]
        public void TestCascadeBTree() {
            testName = "TestCascadeBTree";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.BTREE, ForeignKeyDeleteAction.CASCADE);
        }

        [Test]
        public void TestCascadeHash() {
            testName = "TestCascadeHash";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.HASH, ForeignKeyDeleteAction.CASCADE);
        }

        [Test]
        public void TestCascadeQueue() {
            testName = "TestCascadeQueue";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.QUEUE, ForeignKeyDeleteAction.CASCADE);
        }

        [Test]
        public void TestCascadeRecno() {
            testName = "TestCascadeRecno";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.RECNO, ForeignKeyDeleteAction.CASCADE);
        }

        [Test]
        public void TestNullifyBTree() {
            testName = "TestNullifyBTree";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.BTREE, ForeignKeyDeleteAction.NULLIFY);
        }

        [Test]
        public void TestNullifyHash() {
            testName = "TestNullifyHash";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.HASH, ForeignKeyDeleteAction.NULLIFY);
        }

        [Test]
        public void TestNullifyQueue() {
            testName = "TestNullifyQueue";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.QUEUE, ForeignKeyDeleteAction.NULLIFY);
        }

        [Test]
        public void TestNullifyRecno() {
            testName = "TestNullifyRecno";
            testHome = testFixtureHome + "/" + testName;
            TestForeignKeyDelete(DatabaseType.RECNO, ForeignKeyDeleteAction.NULLIFY);
        }

        public void TestForeignKeyDelete(DatabaseType dbtype, ForeignKeyDeleteAction action) {
            string dbFileName = testHome + "/" + testName + ".db";
            string fdbFileName = testHome + "/" + testName + "foreign.db";
            string sdbFileName = testHome + "/" + testName + "sec.db";
            Configuration.ClearDir(testHome);

            Database primaryDB, fdb;
            SecondaryDatabase secDB;

            // Open primary database.
            if (dbtype == DatabaseType.BTREE) {
                BTreeDatabaseConfig btConfig = new BTreeDatabaseConfig();
                btConfig.Creation = CreatePolicy.ALWAYS;
                primaryDB = BTreeDatabase.Open(dbFileName, btConfig);
                fdb = BTreeDatabase.Open(fdbFileName, btConfig);
            } else if (dbtype == DatabaseType.HASH) {
                HashDatabaseConfig hConfig = new HashDatabaseConfig();
                hConfig.Creation = CreatePolicy.ALWAYS;
                primaryDB = HashDatabase.Open(dbFileName, hConfig);
                fdb = HashDatabase.Open(fdbFileName, hConfig);
            } else if (dbtype == DatabaseType.QUEUE) {
                QueueDatabaseConfig qConfig = new QueueDatabaseConfig();
                qConfig.Creation = CreatePolicy.ALWAYS;
                qConfig.Length = 4;
                primaryDB = QueueDatabase.Open(dbFileName, qConfig);
                fdb = QueueDatabase.Open(fdbFileName, qConfig);
            } else if (dbtype == DatabaseType.RECNO) {
                RecnoDatabaseConfig rConfig = new RecnoDatabaseConfig();
                rConfig.Creation = CreatePolicy.ALWAYS;
                primaryDB = RecnoDatabase.Open(dbFileName, rConfig);
                fdb = RecnoDatabase.Open(fdbFileName, rConfig);
            } else {
                throw new ArgumentException("Invalid DatabaseType");
            }

            // Open secondary database.
            if (dbtype == DatabaseType.BTREE) {
                SecondaryBTreeDatabaseConfig secbtConfig =
                    new SecondaryBTreeDatabaseConfig(primaryDB,
                    new SecondaryKeyGenDelegate(SecondaryKeyGen));
                secbtConfig.Creation = CreatePolicy.ALWAYS;
                secbtConfig.Duplicates = DuplicatesPolicy.SORTED;
                if (action == ForeignKeyDeleteAction.NULLIFY)
                    secbtConfig.SetForeignKeyConstraint(fdb, action, new ForeignKeyNullifyDelegate(Nullify));
                else
                    secbtConfig.SetForeignKeyConstraint(fdb, action);
                secDB = SecondaryBTreeDatabase.Open(sdbFileName, secbtConfig);
            } else if (dbtype == DatabaseType.HASH) {
                SecondaryHashDatabaseConfig sechConfig =
                    new SecondaryHashDatabaseConfig(primaryDB,
                    new SecondaryKeyGenDelegate(SecondaryKeyGen));
                sechConfig.Creation = CreatePolicy.ALWAYS;
                sechConfig.Duplicates = DuplicatesPolicy.SORTED;
                if (action == ForeignKeyDeleteAction.NULLIFY)
                    sechConfig.SetForeignKeyConstraint(fdb, action, new ForeignKeyNullifyDelegate(Nullify));
                else
                    sechConfig.SetForeignKeyConstraint(fdb, action);
                secDB = SecondaryHashDatabase.Open(sdbFileName, sechConfig);
            } else if (dbtype == DatabaseType.QUEUE) {
                SecondaryQueueDatabaseConfig secqConfig =
                    new SecondaryQueueDatabaseConfig(primaryDB,
                    new SecondaryKeyGenDelegate(SecondaryKeyGen));
                secqConfig.Creation = CreatePolicy.ALWAYS;
                secqConfig.Length = 4;
                if (action == ForeignKeyDeleteAction.NULLIFY)
                    secqConfig.SetForeignKeyConstraint(fdb, action, new ForeignKeyNullifyDelegate(Nullify));
                else
                    secqConfig.SetForeignKeyConstraint(fdb, action);
                secDB = SecondaryQueueDatabase.Open(sdbFileName, secqConfig);
            } else if (dbtype == DatabaseType.RECNO) {
                SecondaryRecnoDatabaseConfig secrConfig =
                    new SecondaryRecnoDatabaseConfig(primaryDB,
                    new SecondaryKeyGenDelegate(SecondaryKeyGen));
                secrConfig.Creation = CreatePolicy.ALWAYS;
                if (action == ForeignKeyDeleteAction.NULLIFY)
                    secrConfig.SetForeignKeyConstraint(fdb, action, new ForeignKeyNullifyDelegate(Nullify));
                else
                    secrConfig.SetForeignKeyConstraint(fdb, action);
                secDB = SecondaryRecnoDatabase.Open(sdbFileName, secrConfig);
            } else {
                throw new ArgumentException("Invalid DatabaseType");
            }

            /* Use integer keys for Queue/Recno support. */
            fdb.Put(new DatabaseEntry(BitConverter.GetBytes(100)),
                new DatabaseEntry(BitConverter.GetBytes(1001)));
            fdb.Put(new DatabaseEntry(BitConverter.GetBytes(200)),
                new DatabaseEntry(BitConverter.GetBytes(2002)));
            fdb.Put(new DatabaseEntry(BitConverter.GetBytes(300)),
                new DatabaseEntry(BitConverter.GetBytes(3003)));

            primaryDB.Put(new DatabaseEntry(BitConverter.GetBytes(1)),
                new DatabaseEntry(BitConverter.GetBytes(100)));
            primaryDB.Put(new DatabaseEntry(BitConverter.GetBytes(2)),
                new DatabaseEntry(BitConverter.GetBytes(200)));
            if (dbtype == DatabaseType.BTREE || dbtype == DatabaseType.HASH)
                primaryDB.Put(new DatabaseEntry(BitConverter.GetBytes(3)),
                    new DatabaseEntry(BitConverter.GetBytes(100)));

            try {
                fdb.Delete(new DatabaseEntry(BitConverter.GetBytes(100)));
            } catch (ForeignConflictException) {
                Assert.AreEqual(action, ForeignKeyDeleteAction.ABORT);
            }
            if (action == ForeignKeyDeleteAction.ABORT) {
                Assert.IsTrue(secDB.Exists(new DatabaseEntry(BitConverter.GetBytes(100))));
                Assert.IsTrue(primaryDB.Exists(new DatabaseEntry(BitConverter.GetBytes(1))));
                Assert.IsTrue(fdb.Exists(new DatabaseEntry(BitConverter.GetBytes(100))));
            } else if (action == ForeignKeyDeleteAction.CASCADE) {
                try {
                    Assert.IsFalse(secDB.Exists(new DatabaseEntry(BitConverter.GetBytes(100))));
                } catch (KeyEmptyException) {
                    Assert.IsTrue(dbtype == DatabaseType.QUEUE || dbtype == DatabaseType.RECNO);
                }
                try {
                    Assert.IsFalse(primaryDB.Exists(new DatabaseEntry(BitConverter.GetBytes(1))));
                } catch (KeyEmptyException) {
                    Assert.IsTrue(dbtype == DatabaseType.QUEUE || dbtype == DatabaseType.RECNO);
                }
                try {
                    Assert.IsFalse(fdb.Exists(new DatabaseEntry(BitConverter.GetBytes(100))));
                } catch (KeyEmptyException) {
                    Assert.IsTrue(dbtype == DatabaseType.QUEUE || dbtype == DatabaseType.RECNO);
                }
            } else if (action == ForeignKeyDeleteAction.NULLIFY) {
                try {
                    Assert.IsFalse(secDB.Exists(new DatabaseEntry(BitConverter.GetBytes(100))));
                } catch (KeyEmptyException) {
                    Assert.IsTrue(dbtype == DatabaseType.QUEUE || dbtype == DatabaseType.RECNO);
                }
                Assert.IsTrue(primaryDB.Exists(new DatabaseEntry(BitConverter.GetBytes(1))));
                try {
                    Assert.IsFalse(fdb.Exists(new DatabaseEntry(BitConverter.GetBytes(100))));
                } catch (KeyEmptyException) {
                    Assert.IsTrue(dbtype == DatabaseType.QUEUE || dbtype == DatabaseType.RECNO);
                }
            }

            // Close secondary database.
            secDB.Close();

            // Close primary database.
            primaryDB.Close();

            // Close foreign database
            fdb.Close();
        }

        public DatabaseEntry SecondaryKeyGen(
            DatabaseEntry key, DatabaseEntry data) {
            DatabaseEntry dbtGen;

            int skey = BitConverter.ToInt32(data.Data, 0);
            // don't index secondary key of 0
            if (skey == 0)
                return null;

            dbtGen = new DatabaseEntry(data.Data);
            return dbtGen;
        }

        public DatabaseEntry Nullify(DatabaseEntry key, DatabaseEntry data, DatabaseEntry fkey) {
            DatabaseEntry ret = new DatabaseEntry(BitConverter.GetBytes(0));
            return ret;
        }

    }
}