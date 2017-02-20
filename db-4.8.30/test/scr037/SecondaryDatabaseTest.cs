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
    public class SecondaryDatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SecondaryDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestKeyGen()
        {
            testName = "TestKeyGen";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open primary database.
            BTreeDatabaseConfig primaryDBConfig =
                new BTreeDatabaseConfig();
            primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase primaryDB =
                BTreeDatabase.Open(dbFileName, primaryDBConfig);

            // Open secondary database.
            SecondaryBTreeDatabaseConfig secDBConfig =
                new SecondaryBTreeDatabaseConfig(primaryDB,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            SecondaryBTreeDatabase secDB =
                SecondaryBTreeDatabase.Open(dbFileName,
                secDBConfig);

            primaryDB.Put(new DatabaseEntry(
                BitConverter.GetBytes((int)1)),
                new DatabaseEntry(BitConverter.GetBytes((int)11)));

            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            pair = secDB.Get(new DatabaseEntry(
                BitConverter.GetBytes((int)11)));
            Assert.IsNotNull(pair.Value);

            // Close secondary database.
            secDB.Close();

            // Close primary database.
            primaryDB.Close();
        }

        [Test]
        public void TestSecondaryCursor()
        {
            testName = "TestSecondaryCursor";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open primary database.
            BTreeDatabaseConfig primaryDBConfig =
                new BTreeDatabaseConfig();
            primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase primaryDB =
                BTreeDatabase.Open(dbFileName, primaryDBConfig);

            // Open secondary database.
            SecondaryBTreeDatabaseConfig secDBConfig =
                new SecondaryBTreeDatabaseConfig(primaryDB,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            SecondaryBTreeDatabase secDB =
                SecondaryBTreeDatabase.Open(dbFileName,
                secDBConfig);

            primaryDB.Put(new DatabaseEntry(
                BitConverter.GetBytes((int)1)),
                new DatabaseEntry(BitConverter.GetBytes((int)11)));


            SecondaryCursor cursor = secDB.SecondaryCursor();
            cursor.Move(new DatabaseEntry(
                BitConverter.GetBytes((int)11)), true);
            Assert.AreEqual(BitConverter.GetBytes((int)11),
                cursor.Current.Key.Data);

            // Close the cursor.
            cursor.Close();

            // Close secondary database.
            secDB.Close();

            // Close primary database.
            primaryDB.Close();
        }

        [Test]
        public void TestSecondaryCursorWithConfig()
        {
            testName = "TestSecondaryCursorWithConfig";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            SecondaryBTreeDatabase secDB;
            OpenPrimaryAndSecondaryDB(dbFileName, out db, out secDB);

            for (int i = 0; i < 10; i++)
                db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                    new DatabaseEntry(BitConverter.GetBytes((int)i)));

            CursorConfig cursorConfig = new CursorConfig();
            cursorConfig.WriteCursor = false;
            SecondaryCursor cursor =
                secDB.SecondaryCursor(cursorConfig);

            cursor.Move(new DatabaseEntry(
                BitConverter.GetBytes((int)5)), true);

            Assert.AreEqual(1, cursor.Count());

            // Close the cursor.
            cursor.Close();

            // Close secondary database.
            secDB.Close();

            // Close primary database.
            db.Close();
        }

        [Test]
        public void TestSecondaryCursorWithTxn()
        {
            testName = "TestSecondaryCursorWithTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            GetSecondaryCursurWithTxn(testHome, testName, false);
        }

        [Test]
        public void TestSecondaryCursorWithConfigAndTxn()
        {
            testName = "TestSecondaryCursorWithConfigAndTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);
            GetSecondaryCursurWithTxn(testHome, testName, true);
        }

        public void GetSecondaryCursurWithTxn(string home,
            string name, bool ifCfg)
        {
            string dbFileName = name + ".db";
            SecondaryCursor cursor;

            // Open env.
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(home,
                envConfig);


            // Open primary/secondary database.
            Transaction txn = env.BeginTransaction();
            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            BTreeDatabase db = BTreeDatabase.Open(dbFileName,
                dbConfig, txn);

            SecondaryBTreeDatabaseConfig secDBConfig = new
                SecondaryBTreeDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secDBConfig.Env = env;
            SecondaryBTreeDatabase secDB =
                SecondaryBTreeDatabase.Open(dbFileName,
                secDBConfig, txn);

            for (int i = 0; i < 10; i++)
                db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                    new DatabaseEntry(BitConverter.GetBytes((int)i)), txn);


            // Create secondary cursor.
            if (ifCfg == false)
                secDB.SecondaryCursor(txn);
            else if (ifCfg == true)
            {
                CursorConfig cursorConfig = new CursorConfig();
                cursorConfig.WriteCursor = false;
                cursor = secDB.SecondaryCursor(cursorConfig, txn);
                cursor.Close();
            }

            secDB.Close();
            db.Close();
            txn.Commit();
            env.Close();
        }

        [Test]
        public void TestOpen()
        {
            testName = "TestOpen";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecQueueDB(dbFileName, dbSecFileName, false);
        }

        [Test]
        public void TestOpenWithDBName()
        {
            testName = "TestOpenWithDBName";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string dbSecFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);
            OpenSecQueueDB(dbFileName, dbSecFileName, true);
        }

        public void OpenSecQueueDB(string dbFileName, 
            string dbSecFileName, bool ifDBName)
        {
            // Open a primary btree database.
            BTreeDatabaseConfig primaryDBConfig =
                new BTreeDatabaseConfig();
            primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase primaryDB;

            /*
             * If secondary database name is given, the primary 
             * database is also opened with database name.
             */
            if (ifDBName == false)
                primaryDB = BTreeDatabase.Open(dbFileName,
                    primaryDBConfig);
            else
                primaryDB = BTreeDatabase.Open(dbFileName,
                    "primary", primaryDBConfig);

            try
            {
                // Open a new secondary database.
                SecondaryBTreeDatabaseConfig secBTDBConfig =
                    new SecondaryBTreeDatabaseConfig(
                    primaryDB, null);
                secBTDBConfig.Creation =
                    CreatePolicy.IF_NEEDED;

                SecondaryBTreeDatabase secBTDB;
                if (ifDBName == false)
                    secBTDB = SecondaryBTreeDatabase.Open(
                        dbSecFileName, secBTDBConfig);
                else
                    secBTDB = SecondaryBTreeDatabase.Open(
                        dbSecFileName, "secondary",
                        secBTDBConfig);

                // Close the secondary database.
                secBTDB.Close();

                // Open the existing secondary database.
                SecondaryDatabaseConfig secDBConfig =
                    new SecondaryDatabaseConfig(
                    primaryDB, null);

                SecondaryDatabase secDB;
                if (ifDBName == false)
                    secDB = SecondaryBTreeDatabase.Open(
                        dbSecFileName, secDBConfig);
                else
                    secDB = SecondaryBTreeDatabase.Open(
                        dbSecFileName, "secondary", secDBConfig);

                // Close secondary database.
                secDB.Close();
            }
            catch (DatabaseException)
            {
                throw new TestException();
            }
            finally
            {
                // Close primary database.
                primaryDB.Close();
            }
        }

        [Test]
        public void TestOpenWithinTxn()
        {
            testName = "TestOpenWithinTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecQueueDBWithinTxn(testHome, dbFileName,
                dbSecFileName, false);
        }

        [Test]
        public void TestOpenDBNameWithinTxn()
        {
            testName = "TestOpenDBNameWithinTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";
            string dbSecFileName = testName + "_sec.db";

            Configuration.ClearDir(testHome);

            OpenSecQueueDBWithinTxn(testHome, dbFileName,
                dbSecFileName, true);
        }

        public void OpenSecQueueDBWithinTxn(string home,
            string dbFileName, string dbSecFileName, bool ifDbName)
        {
            // Open an environment.
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            envConfig.UseLogging = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, envConfig);

            // Open a primary btree database.
            Transaction openDBTxn = env.BeginTransaction();
            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            BTreeDatabase db = BTreeDatabase.Open(
                dbFileName, dbConfig, openDBTxn);
            openDBTxn.Commit();

            // Open a secondary btree database.
            Transaction openSecTxn = env.BeginTransaction();
            SecondaryBTreeDatabaseConfig secDBConfig =
                new SecondaryBTreeDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secDBConfig.Env = env;
            secDBConfig.Creation = CreatePolicy.IF_NEEDED;

            SecondaryBTreeDatabase secDB;
            if (ifDbName == false)
                secDB = SecondaryBTreeDatabase.Open(
                    dbSecFileName, secDBConfig, openSecTxn);
            else
                secDB = SecondaryBTreeDatabase.Open(
                    dbSecFileName, "secondary", secDBConfig,
                    openSecTxn);
            openSecTxn.Commit();
            secDB.Close();

            // Open the existing secondary database.
            Transaction secTxn = env.BeginTransaction();
            SecondaryDatabaseConfig secConfig =
                new SecondaryDatabaseConfig(db,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secConfig.Env = env;

            SecondaryDatabase secExDB;
            if (ifDbName == false)
                secExDB = SecondaryBTreeDatabase.Open(
                    dbSecFileName, secConfig, secTxn);
            else
                secExDB = SecondaryBTreeDatabase.Open(
                    dbSecFileName, "secondary", secConfig,
                    secTxn);
            secExDB.Close();
            secTxn.Commit();

            db.Close();
            env.Close();
        }

        public void OpenPrimaryAndSecondaryDB(string dbFileName,
            out BTreeDatabase primaryDB,
            out SecondaryBTreeDatabase secDB)
        {
            // Open primary database.
            BTreeDatabaseConfig primaryDBConfig =
                new BTreeDatabaseConfig();
            primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
            primaryDB =
                BTreeDatabase.Open(dbFileName, primaryDBConfig);

            // Open secondary database.
            SecondaryBTreeDatabaseConfig secDBConfig =
                new SecondaryBTreeDatabaseConfig(primaryDB,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secDB = SecondaryBTreeDatabase.Open(dbFileName,
                secDBConfig);
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestBadSecondaryException()
        {
            testName = "TestBadSecondaryException";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string secDBFileName = testHome + "/" +
                testName + "_sec.db";

            Configuration.ClearDir(testHome);

            // Open primary database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase btreeDB =
                BTreeDatabase.Open(dbFileName, btreeDBConfig);

            // Open secondary database.
            SecondaryBTreeDatabaseConfig secBtDbConfig =
                new SecondaryBTreeDatabaseConfig(btreeDB,
                new SecondaryKeyGenDelegate(SecondaryKeyGen));
            secBtDbConfig.Creation = CreatePolicy.IF_NEEDED;
            SecondaryBTreeDatabase secBtDb =
                SecondaryBTreeDatabase.Open(secDBFileName,
                secBtDbConfig);

            // Put some data into primary database.
            for (int i = 0; i < 10; i++)
                btreeDB.Put(new DatabaseEntry(
                    BitConverter.GetBytes(i)),
                    new DatabaseEntry(BitConverter.GetBytes(i)));

            // Close the secondary database.
            secBtDb.Close();

            // Delete record(5, 5) in primary database.
            btreeDB.Delete(new DatabaseEntry(
                BitConverter.GetBytes((int)5)));

            // Reopen the secondary database.
            SecondaryDatabase secDB = SecondaryDatabase.Open(
                secDBFileName,
                new SecondaryDatabaseConfig(btreeDB,
                new SecondaryKeyGenDelegate(SecondaryKeyGen)));

            /*
             * Getting record(5, 5) by secondary database should 
             * throw BadSecondaryException since it has been
             * deleted in the primary database.
             */
            try
            {
                secDB.Exists(new DatabaseEntry(
                    BitConverter.GetBytes((int)5)));
            }
            catch (BadSecondaryException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                secDB.Close();
                btreeDB.Close();
            }
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