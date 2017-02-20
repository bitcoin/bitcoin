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
    public class SequenceTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "SequenceTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestConfig()
        {
            testName = "TestConfig";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);

            Configuration.ClearDir(testHome);

            // Open a database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig);

            // Configure and initialize sequence. 
            SequenceConfig seqConfig = new SequenceConfig();
            seqConfig.BackingDatabase = btreeDB;
            seqConfig.Creation = CreatePolicy.IF_NEEDED;
            seqConfig.key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            seqConfig.SetRange(Int64.MinValue, Int64.MaxValue);
            SequenceConfigTest.Config(xmlElem, ref seqConfig, true);
            Sequence seq = new Sequence(seqConfig);


            /*
             * Confirm that the squence is opened with the 
             * configuration that we set.
             */
            Confirm(xmlElem, seq, true);

            /* Close sequence, database and environment. */
            seq.Close();
            btreeDB.Close();
        }

        [Test]
        public void TestClose()
        {
            testName = "TestClose";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            Sequence seq;
            OpenNewSequence(dbFileName, out db, out seq);
            seq.Close();
            db.Close();
        }

        [Test]
        public void TestDispose()
        {
            testName = "TestDispose";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            BTreeDatabase db;
            Sequence seq;
            OpenNewSequence(dbFileName, out db, out seq);
            seq.Dispose();
            db.Close();
        }

        [Test]
        public void TestGet()
        {
            testName = "TestGet";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a database and an increase sequence.
            BTreeDatabase db;
            Sequence seq;
            OpenNewSequence(dbFileName, out db, out seq);

            /*
             * Check the delta of two sequence number get 
             * from sequence.
             */
            int delta = 100;
            long seqNum1 = seq.Get(delta);
            long seqNum2 = seq.Get(delta);
            Assert.AreEqual(delta, seqNum2 - seqNum1);

            // Close all.
            seq.Close();
            db.Close();
        }

        [Test]
        public void TestGetWithNoSync()
        {
            testName = "TestGetWithNoSync";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a database and an increase sequence.
            BTreeDatabase db;
            DatabaseEnvironment env;
            Sequence seq;
            OpenNewSequenceInEnv(testHome, testName, out env,
                out db, out seq);

            /*
             * Check the delta of two sequence number get 
             * from sequence.
             */
            int delta = 100;
            long seqNum1 = seq.Get(delta, true);
            long seqNum2 = seq.Get(delta, true);
            Assert.AreEqual(delta, seqNum2 - seqNum1);

            // Close all.
            seq.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestGetWithinTxn()
        {
            testName = "TestGetWithinTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a database and an increase sequence.
            BTreeDatabase db;
            DatabaseEnvironment env;
            Sequence seq;
            OpenNewSequenceInEnv(testHome, testName, out env,
                out db, out seq);

            /*
             * Check the delta of two sequence number get 
             * from sequence.
             */
            int delta = 100;
            Transaction txn = env.BeginTransaction();
            long seqNum1 = seq.Get(delta, txn);
            long seqNum2 = seq.Get(delta, txn);
            Assert.AreEqual(delta, seqNum2 - seqNum1);
            txn.Commit();

            // Close all.
            seq.Close();
            db.Close();
            env.Close();
        }


        [Test]
        public void TestRemove()
        {
            testName = "TestRemove";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a database and an increase sequence.
            BTreeDatabase db;
            Sequence seq;
            OpenNewSequence(dbFileName, out db, out seq);

            /*
             * Remove the sequence. The sequence handle can not
             * be accessed again.
             */
            seq.Remove();
            db.Close();
        }
        
        [Test]
        public void TestRemoveWithNoSync()
        {
            testName = "TestRemoveWithNoSync";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a database and an increase sequence.
            DatabaseEnvironmentConfig envConfig = new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(testHome, envConfig);

            /* Configure and open sequence's database. */
            BTreeDatabaseConfig btreeDBConfig = new BTreeDatabaseConfig();
            btreeDBConfig.AutoCommit = true;
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            btreeDBConfig.Env = env;
            BTreeDatabase btreeDB = BTreeDatabase.Open(dbFileName, btreeDBConfig);

            /* Configure and initialize sequence. */
            SequenceConfig seqConfig = new SequenceConfig();
            seqConfig.BackingDatabase = btreeDB;
            seqConfig.Creation = CreatePolicy.IF_NEEDED;
            seqConfig.Increment = true;
            seqConfig.InitialValue = Int64.MaxValue;
            seqConfig.key = new DatabaseEntry();
            seqConfig.SetRange(Int64.MinValue, Int64.MaxValue);
            seqConfig.Wrap = true;
            seqConfig.key = new DatabaseEntry();
            seqConfig.key.Data = ASCIIEncoding.ASCII.GetBytes("ex_csharp_sequence");
            Sequence seq = new Sequence(seqConfig);

            /*
             * Remove the sequence. The sequence handle can not
             * be accessed again.
             */
            seq.Remove(true);
            btreeDB.Close();
            env.Close();
        }

        [Test]
        public void TestRemoveWithInTxn()
        {
            testName = "TestRemoveWithInTxn";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a database and an increase sequence.
            BTreeDatabase db;
            DatabaseEnvironment env;
            Sequence seq;
            OpenNewSequenceInEnv(testHome, testName, out env,
                out db, out seq);

            //Remove the sequence.
            Transaction txn = env.BeginTransaction();
            seq.Remove(txn);
            txn.Commit();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestStats()
        {
            testName = "TestStats";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open a database and an increase sequence.
            BTreeDatabase db;
            Sequence seq;
            OpenNewSequence(dbFileName, out db, out seq);

            // Get a value from sequence.
            seq.Get(100);

            // Get sequence statistics.
            SequenceStats stats = seq.Stats();
            seq.PrintStats(true);
            Assert.AreEqual(200, stats.CachedValue);
            Assert.AreEqual(1000, stats.CacheSize);
            Assert.AreNotEqual(0, stats.Flags);
            Assert.AreEqual(1099, stats.LastCachedValue);
            Assert.AreEqual(Int64.MaxValue, stats.Max);
            Assert.AreEqual(Int64.MinValue, stats.Min);
            Assert.AreEqual(1100, stats.StoredValue);

            stats = seq.Stats(true);
            seq.PrintStats();
            stats = seq.Stats();
            Assert.AreEqual(0, stats.LockNoWait);
            Assert.AreEqual(0, stats.LockWait);

            seq.Close();
            db.Close();
        }

        public void OpenNewSequence(string dbFileName,
            out BTreeDatabase db, out Sequence seq)
        {
            // Open a database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
            db = BTreeDatabase.Open(dbFileName, btreeDBConfig);

            // Configure and initialize sequence. 
            SequenceConfig seqConfig = new SequenceConfig();
            seqConfig.BackingDatabase = db;
            seqConfig.CacheSize = 1000;
            seqConfig.Creation = CreatePolicy.ALWAYS;
            seqConfig.Decrement = false;
            seqConfig.FreeThreaded = true;
            seqConfig.Increment = true;
            seqConfig.InitialValue = 100;
            seqConfig.key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            seqConfig.SetRange(Int64.MinValue, Int64.MaxValue);
            seqConfig.Wrap = true;
            seq = new Sequence(seqConfig);
        }

        public void OpenNewSequenceInEnv(string home, string dbname,
            out DatabaseEnvironment env, out BTreeDatabase db,
            out Sequence seq)
        {
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            envConfig.UseLogging = true;
            env = DatabaseEnvironment.Open(home, envConfig);

            Transaction openTxn = env.BeginTransaction();
            BTreeDatabaseConfig dbConfig =
                new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            db = BTreeDatabase.Open(dbname + ".db", dbConfig,
                openTxn);
            openTxn.Commit();

            Transaction seqTxn = env.BeginTransaction();
            SequenceConfig seqConfig = new SequenceConfig();
            seqConfig.BackingDatabase = db;
            seqConfig.Creation = CreatePolicy.ALWAYS;
            seqConfig.Decrement = false;
            seqConfig.FreeThreaded = true;
            seqConfig.Increment = true;
            seqConfig.InitialValue = 0;
            seqConfig.key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("key"));
            seqConfig.SetRange(Int64.MinValue, Int64.MaxValue);
            seqConfig.Wrap = true;
            seq = new Sequence(seqConfig);
            seqTxn.Commit();
        }

        public static void Confirm(XmlElement xmlElement,
            Sequence seq, bool compulsory)
        {
            Configuration.ConfirmInt(xmlElement, "CacheSize",
                seq.Cachesize, compulsory);
            Configuration.ConfirmBool(xmlElement, "Decrement",
                seq.Decrement, compulsory);
            Configuration.ConfirmBool(xmlElement, "Increment",
                seq.Increment, compulsory);
            Configuration.ConfirmBool(xmlElement, "Wrap",
                seq.Wrap, compulsory);
        }
    }
}
