/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class QueueDatabaseTest : DatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "QueueDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestAppendWithoutTxn()
        {
            testName = "TestAppendWithoutTxn";
            testHome = testFixtureHome + "/" + testName;
            string queueDBFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);
            
            QueueDatabaseConfig queueConfig = new QueueDatabaseConfig();
            queueConfig.Creation = CreatePolicy.ALWAYS;
            queueConfig.Length = 1000;
            QueueDatabase queueDB = QueueDatabase.Open(
                queueDBFileName, queueConfig);

            byte[] byteArr = new byte[4];
            byteArr = BitConverter.GetBytes((int)1);
            DatabaseEntry data = new DatabaseEntry(byteArr);
            uint recno = queueDB.Append(data);

            // Confirm that the recno is larger than 0.
            Assert.AreNotEqual(0, recno);

            // Confirm that the record exists in the database.
            byteArr = BitConverter.GetBytes(recno);
            DatabaseEntry key = new DatabaseEntry();
            key.Data = byteArr;
            Assert.IsTrue(queueDB.Exists(key));
            queueDB.Close();
        }

        [Test]
        public void TestAppendWithTxn()
        {
            testName = "TestAppendWithTxn";
            testHome = testFixtureHome + "/" + testName;
            string queueDBFileName = testHome + "/" + testName + ".db";
            string queueDBName =
                Path.GetFileNameWithoutExtension(queueDBFileName);

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;

            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);
            Transaction txn = env.BeginTransaction();

            QueueDatabaseConfig queueConfig = new QueueDatabaseConfig();
            queueConfig.Creation = CreatePolicy.ALWAYS;
            queueConfig.Env = env;
            queueConfig.Length = 1000;

            /* If environmnet home is set, the file name in
             * Open() is the relative path.
             */
            QueueDatabase queueDB = QueueDatabase.Open(
                queueDBName, queueConfig, txn);
            DatabaseEntry data;
            int i = 1000;
            try
            {
                while (i > 0)
                {
                    data = new DatabaseEntry(
                        BitConverter.GetBytes(i));
                    queueDB.Append(data, txn);
                    i--;
                }
                txn.Commit();
            }
            catch
            {
                txn.Abort();
            }
            finally
            {
                queueDB.Close();
                env.Close();
            }

        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestConsumeWithTxn()
        {
            testName = "TestConsumeWithTxn";
            testHome = testFixtureHome + "/" + testName;
            string queueDBFileName = testHome + "/" + testName + ".db";
            string queueDBName = Path.GetFileName(queueDBFileName);

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;

            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);
            Transaction txn = env.BeginTransaction();

            QueueDatabaseConfig queueConfig =
                new QueueDatabaseConfig();
            queueConfig.Creation = CreatePolicy.ALWAYS;
            queueConfig.Env = env;
            queueConfig.Length = 1000;
            QueueDatabase queueDB = QueueDatabase.Open(
                queueDBName, queueConfig, txn);

            int i = 1;
            DatabaseEntry data;
            DatabaseEntry getData = new DatabaseEntry();
            while (i <= 10)
            {
                data = new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes(i.ToString()));
                queueDB.Append(data, txn);
                if (i == 5)
                {
                    getData = data;
                }
                i++;
            }

            KeyValuePair<uint, DatabaseEntry> pair = queueDB.Consume(false, txn);

            queueDB.Close();
            txn.Commit();
            env.Close();

            Database db = Database.Open(queueDBFileName,
                new QueueDatabaseConfig());
            try
            {
                DatabaseEntry key =
                    new DatabaseEntry(BitConverter.GetBytes(pair.Key));
                db.Get(key);
            }
            catch (NotFoundException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                db.Close();
            }
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestConsumeWithoutTxn()
        {
            testName = "TestConsumeWithoutTxn";
            testHome = testFixtureHome + "/" + testName;
            string queueDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            QueueDatabaseConfig queueConfig =
                new QueueDatabaseConfig();
            queueConfig.Creation = CreatePolicy.ALWAYS;
            queueConfig.ErrorPrefix = testName;
            queueConfig.Length = 1000;

            QueueDatabase queueDB = QueueDatabase.Open(
                queueDBFileName, queueConfig);
            DatabaseEntry data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            queueDB.Append(data);

            DatabaseEntry consumeData = new DatabaseEntry();
            KeyValuePair<uint, DatabaseEntry> pair = queueDB.Consume(false);
            try
            {
                DatabaseEntry key =
                    new DatabaseEntry(BitConverter.GetBytes(pair.Key));
                queueDB.Get(key);
            }
            catch (NotFoundException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                queueDB.Close();
            }
        }

        public void TestCursor()
        {
            testName = "TestCursor";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            GetCursur(testHome + "/" + testName + ".db", false);
        }

        public void TestCursorWithConfig()
        {
            testName = "TestCursorWithConfig";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            GetCursur(testHome + "/" + testName + ".db", true);
        }

        public void GetCursur(string dbFileName, bool ifConfig)
        {
            QueueDatabaseConfig dbConfig = new QueueDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Length = 100;
            QueueDatabase db = QueueDatabase.Open(dbFileName, dbConfig);
            Cursor cursor;
            if (ifConfig == false)
                cursor = db.Cursor();
            else
                cursor = db.Cursor(new CursorConfig());
            cursor.Close();
            db.Close();
        }

        //[Test]
        //public void TestDupCompare()
        //{
        //    testName = "TestDupCompare";
        //    testHome = testFixtureHome + "/" + testName;
        //    string dbFileName = testHome + "/" + testName + ".db";

        //    Configuration.ClearDir(testHome);

        //    QueueDatabaseConfig dbConfig = new QueueDatabaseConfig();
        //    dbConfig.Creation = CreatePolicy.IF_NEEDED;
        //    dbConfig.DuplicateCompare = new EntryComparisonDelegate(dbIntCompare);
        //    dbConfig.Length = 10;
        //    dbConfig.PageSize = 40860;
        //    try
        //    {
        //        QueueDatabase db = QueueDatabase.Open(dbFileName, dbConfig);
        //        int ret = db.DupCompare(new DatabaseEntry(BitConverter.GetBytes(255)),
        //            new DatabaseEntry(BitConverter.GetBytes(257)));
        //        Assert.Greater(0, ret);
        //        db.Close();
        //    }
        //    catch (DatabaseException e)
        //    {
        //        Console.WriteLine(e.Message);
        //    }
        //}

        private int dbIntCompare(DatabaseEntry dbt1,
            DatabaseEntry dbt2)
        {
            int a, b;
            a = BitConverter.ToInt16(dbt1.Data, 0);
            b = BitConverter.ToInt16(dbt2.Data, 0);
            return a - b;
        }

        [Test, ExpectedException(typeof(ExpectedTestException))]
        public void TestKeyEmptyException()
        {
            testName = "TestKeyEmptyException";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLocking = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            QueueDatabase db;
            try
            {
                Transaction openTxn = env.BeginTransaction();
                try
                {
                    QueueDatabaseConfig queueConfig =
                        new QueueDatabaseConfig();
                    queueConfig.Creation = CreatePolicy.IF_NEEDED;
                    queueConfig.Length = 10;
                    queueConfig.Env = env;
                    db = QueueDatabase.Open(testName + ".db",
                        queueConfig, openTxn);
                    openTxn.Commit();
                }
                catch (DatabaseException e)
                {
                    openTxn.Abort();
                    throw e;
                }

                Transaction cursorTxn = env.BeginTransaction();
                Cursor cursor;
                try
                {
                    /*
                     * Put a record into queue database with 
                     * cursor and abort the operation.
                     */
                    cursor = db.Cursor(cursorTxn);
                    KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
                    pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                        new DatabaseEntry(BitConverter.GetBytes((int)10)),
                        new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
                    cursor.Add(pair);
                    cursor.Close();
                    cursorTxn.Abort();
                }
                catch (DatabaseException e)
                {
                    cursorTxn.Abort();
                    db.Close();
                    throw e;
                }

                Transaction delTxn = env.BeginTransaction();
                try
                {
                    /*
                     * The put operation is aborted in the queue 
                     * database so querying if the record still exists 
                     * throws KeyEmptyException.
                     */
                    db.Exists(new DatabaseEntry(
                        BitConverter.GetBytes((int)10)), delTxn);
                    delTxn.Commit();
                }
                catch (DatabaseException e)
                {
                    delTxn.Abort();
                    throw e;
                }
                finally
                {
                    db.Close();
                }
            }
            catch (KeyEmptyException)
            {
                throw new ExpectedTestException();
            }
            finally
            {
                env.Close();
            }
        }

        [Test]
        public void TestOpenExistingQueueDB()
        {
            testName = "TestOpenExistingQueueDB";
            testHome = testFixtureHome + "/" + testName;
            string queueDBFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            QueueDatabaseConfig queueConfig = new QueueDatabaseConfig();
            queueConfig.Creation = CreatePolicy.ALWAYS;
            QueueDatabase queueDB = QueueDatabase.Open(
                queueDBFileName, queueConfig);
            queueDB.Close();

            DatabaseConfig dbConfig = new DatabaseConfig();
            Database db = Database.Open(queueDBFileName, dbConfig);
            Assert.AreEqual(db.Type, DatabaseType.QUEUE);
            db.Close();
        }

        [Test]
        public void TestOpenNewQueueDB()
        {
            testName = "TestOpenNewQueueDB";
            testHome = testFixtureHome + "/" + testName;
            string queueDBFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Configure all fields/properties in queue database.
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            QueueDatabaseConfig queueConfig = new QueueDatabaseConfig();
            QueueDatabaseConfigTest.Config(xmlElem, ref queueConfig, true);
            queueConfig.Feedback = new DatabaseFeedbackDelegate(DbFeedback);

            // Open the queue database with above configuration.
            QueueDatabase queueDB = QueueDatabase.Open(
                queueDBFileName, queueConfig);

            // Check the fields/properties in opened queue database.
            Confirm(xmlElem, queueDB, true);

            queueDB.Close();
        }

        private void DbFeedback(DatabaseFeedbackEvent opcode, int percent)
        {
            if (opcode == DatabaseFeedbackEvent.UPGRADE)
                Console.WriteLine("Update for %d%", percent);

            if (opcode == DatabaseFeedbackEvent.VERIFY)
                Console.WriteLine("Vertify for %d", percent);
        }

        [Test, ExpectedException(typeof(NotFoundException))]
        public void TestPutToQueue()
        {
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

            testName = "TestPutQueue";
            testHome = testFixtureHome + "/" + testName;
            string queueDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            QueueDatabaseConfig queueConfig =
                new QueueDatabaseConfig();
            queueConfig.Length = 512;
            queueConfig.Creation = CreatePolicy.ALWAYS;
            using (QueueDatabase queueDB = QueueDatabase.Open(
                queueDBFileName, queueConfig))
            {
                DatabaseEntry key = new DatabaseEntry();
                key.Data = BitConverter.GetBytes((int)100);
                DatabaseEntry data = new DatabaseEntry(
                    BitConverter.GetBytes((int)1));
                queueDB.Put(key, data);
                pair = queueDB.GetBoth(key, data);
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

            QueueDatabaseConfig dbConfig =
                new QueueDatabaseConfig();
            ConfigCase1(dbConfig);
            QueueDatabase db = QueueDatabase.Open(dbFileName, dbConfig);

            QueueStats stats = db.Stats();
            ConfirmStatsPart1Case1(stats);
            db.PrintFastStats(true);

            // Put 500 records into the database.
            PutRecordCase1(db, null);

            stats = db.Stats();
            ConfirmStatsPart2Case1(stats);
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

        public void StatsInTxn(string home, string name, bool ifIsolation)
        {
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            EnvConfigCase1(envConfig);
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, envConfig);

            Transaction openTxn = env.BeginTransaction();
            QueueDatabaseConfig dbConfig =
                new QueueDatabaseConfig();
            ConfigCase1(dbConfig);
            dbConfig.Env = env;
            QueueDatabase db = QueueDatabase.Open(name + ".db",
                dbConfig, openTxn);
            openTxn.Commit();

            Transaction statsTxn = env.BeginTransaction();
            QueueStats stats;
            if (ifIsolation == false)
                stats = db.Stats(statsTxn);
            else
                stats = db.Stats(statsTxn, Isolation.DEGREE_ONE);

            ConfirmStatsPart1Case1(stats);
            db.PrintStats(true);

            // Put 500 records into the database.
            PutRecordCase1(db, statsTxn);

            if (ifIsolation == false)
                stats = db.Stats(statsTxn);
            else
                stats = db.Stats(statsTxn, Isolation.DEGREE_TWO);
            ConfirmStatsPart2Case1(stats);
            db.PrintStats();

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

        public void ConfigCase1(QueueDatabaseConfig dbConfig)
        {
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.PageSize = 4096;
            dbConfig.ExtentSize = 1024;
            dbConfig.Length = 4000;
            dbConfig.PadByte = 32;
        }

        public void PutRecordCase1(QueueDatabase db, Transaction txn)
        {
            byte[] bigArray = new byte[4000];
            for (int i = 1; i <= 100; i++)
            {
                if (txn == null)
                    db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)));
                else
                    db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)), txn);
            }
            DatabaseEntry key = new DatabaseEntry(BitConverter.GetBytes((int)100));
            for (int i = 100; i <= 500; i++)
            {
                if (txn == null)
                    db.Put(key, new DatabaseEntry(bigArray));
                else
                    db.Put(key, new DatabaseEntry(bigArray), txn);
            }
        }

        public void ConfirmStatsPart1Case1(QueueStats stats)
        {
            Assert.AreEqual(1, stats.FirstRecordNumber);
            Assert.AreNotEqual(0, stats.MagicNumber);
            Assert.AreEqual(1, stats.NextRecordNumber);
            Assert.AreEqual(4096, stats.PageSize);
            Assert.AreEqual(1024, stats.PagesPerExtent);
            Assert.AreEqual(4000, stats.RecordLength);
            Assert.AreEqual(32, stats.RecordPadByte);
            Assert.AreEqual(4, stats.Version);  
        }

        public void ConfirmStatsPart2Case1(QueueStats stats)
        {
            Assert.AreNotEqual(0, stats.DataPages);
            Assert.AreEqual(0, stats.DataPagesBytesFree);
            Assert.AreEqual(0, stats.MetadataFlags);
            Assert.AreEqual(100, stats.nData);
            Assert.AreEqual(100, stats.nKeys);
        }

        public static void Confirm(XmlElement xmlElem,
            QueueDatabase queueDB, bool compulsory)
        {
            DatabaseTest.Confirm(xmlElem, queueDB, compulsory);

            // Confirm queue database specific field/property
            Configuration.ConfirmUint(xmlElem, "ExtentSize",
                queueDB.ExtentSize, compulsory);
            Configuration.ConfirmBool(xmlElem, "ConsumeInOrder",
                queueDB.InOrder, compulsory);
            Configuration.ConfirmInt(xmlElem, "PadByte",
                queueDB.PadByte, compulsory);
            Assert.AreEqual(DatabaseType.QUEUE, queueDB.Type);
            string type = queueDB.Type.ToString();
            Assert.IsNotNull(type);
        }
    }
}
