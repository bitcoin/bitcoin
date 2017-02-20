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
    public class RecnoDatabaseTest : DatabaseTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "RecnoDatabaseTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestOpenExistingRecnoDB()
        {
            testName = "TestOpenExistingRecnoDB";
            testHome = testFixtureHome + "/" + testName;
            string recnoDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            RecnoDatabaseConfig recConfig =
                new RecnoDatabaseConfig();
            recConfig.Creation = CreatePolicy.ALWAYS;
            RecnoDatabase recDB = RecnoDatabase.Open(
                recnoDBFileName, recConfig);
            recDB.Close();

            RecnoDatabaseConfig dbConfig = new RecnoDatabaseConfig();
            string backingFile = testHome + "/backingFile";
            File.Copy(recnoDBFileName, backingFile);
            dbConfig.BackingFile = backingFile;
            RecnoDatabase db = RecnoDatabase.Open(recnoDBFileName, dbConfig);
            Assert.AreEqual(db.Type, DatabaseType.RECNO);
            db.Close();
        }

        [Test]
        public void TestOpenNewRecnoDB()
        {
            RecnoDatabase recnoDB;
            RecnoDatabaseConfig recnoConfig;

            testName = "TestOpenNewRecnoDB";
            testHome = testFixtureHome + "/" + testName;
            string recnoDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            recnoConfig = new RecnoDatabaseConfig();
            RecnoDatabaseConfigTest.Config(xmlElem,
                ref recnoConfig, true);
            recnoDB = RecnoDatabase.Open(recnoDBFileName,
                recnoConfig);
            Confirm(xmlElem, recnoDB, true);
            recnoDB.Close();
        }

        [Test]
        public void TestAppendWithoutTxn()
        {
            testName = "TestAppendWithoutTxn";
            testHome = testFixtureHome + "/" + testName;
            string recnoDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            RecnoDatabaseConfig recnoConfig =
                new RecnoDatabaseConfig();
            recnoConfig.Creation = CreatePolicy.ALWAYS;
            RecnoDatabase recnoDB = RecnoDatabase.Open(
                recnoDBFileName, recnoConfig);

            DatabaseEntry data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("data"));
            uint num = recnoDB.Append(data);
            DatabaseEntry key = new DatabaseEntry(
                BitConverter.GetBytes(num));
            Assert.IsTrue(recnoDB.Exists(key));
            KeyValuePair<DatabaseEntry, DatabaseEntry> record =
                recnoDB.Get(key);
            Assert.IsTrue(data.Data.Length ==
                record.Value.Data.Length);
            for (int i = 0; i < data.Data.Length; i++)
                Assert.IsTrue(data.Data[i] ==
                    record.Value.Data[i]);
            recnoDB.Close();
        }

        [Test]
        public void TestCompact()
        {
            testName = "TestCompact";
            testHome = testFixtureHome + "/" + testName;
            string recnoDBFileName = testHome + "/" +
                testName + ".db";

            Configuration.ClearDir(testHome);

            RecnoDatabaseConfig recnoConfig =
                new RecnoDatabaseConfig();
            recnoConfig.Creation = CreatePolicy.ALWAYS;
            recnoConfig.Length = 512;

            DatabaseEntry key, data;
            RecnoDatabase recnoDB;
            using (recnoDB = RecnoDatabase.Open(
                recnoDBFileName, recnoConfig))
            {
                for (int i = 1; i <= 5000; i++)
                {
                    data = new DatabaseEntry(
                        BitConverter.GetBytes(i));
                    recnoDB.Append(data);
                }

                for (int i = 1; i <= 5000; i++)
                {
                    if (i > 500 && (i % 5 != 0))
                    {
                        key = new DatabaseEntry(
                            BitConverter.GetBytes(i));
                        recnoDB.Delete(key);
                    }
                }

                int startInt = 1;
                int stopInt = 2500;
                DatabaseEntry start, stop;

                start = new DatabaseEntry(
                    BitConverter.GetBytes(startInt));
                stop = new DatabaseEntry(
                    BitConverter.GetBytes(stopInt));
                Assert.IsTrue(recnoDB.Exists(start));
                Assert.IsTrue(recnoDB.Exists(stop));

                CompactConfig cCfg = new CompactConfig();
                cCfg.start = start;
                cCfg.stop = stop;
                cCfg.FillPercentage = 30;
                cCfg.Pages = 1;
                cCfg.returnEnd = true;
                cCfg.Timeout = 5000;
                cCfg.TruncatePages = true;
                CompactData compactData = recnoDB.Compact(cCfg);

                Assert.IsNotNull(compactData.End);
                Assert.AreNotEqual(0, compactData.PagesExamined);
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

            RecnoDatabaseConfig dbConfig =
                new RecnoDatabaseConfig();
            ConfigCase1(dbConfig);
            RecnoDatabase db = RecnoDatabase.Open(dbFileName,
                dbConfig);
            RecnoStats stats = db.Stats();
            ConfirmStatsPart1Case1(stats);

            // Put 1000 records into the database.
            PutRecordCase1(db, null);
            stats = db.Stats();
            ConfirmStatsPart2Case1(stats);

            // Delete 500 records.
            for (int i = 250; i <= 750; i++)
                db.Delete(new DatabaseEntry(BitConverter.GetBytes(i)));
            stats = db.Stats();
            ConfirmStatsPart3Case1(stats);

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
            RecnoDatabaseConfig dbConfig =
                new RecnoDatabaseConfig();
            ConfigCase1(dbConfig);
            dbConfig.Env = env;
            RecnoDatabase db = RecnoDatabase.Open(name + ".db",
                dbConfig, openTxn);
            openTxn.Commit();

            Transaction statsTxn = env.BeginTransaction();
            RecnoStats stats;
            RecnoStats fastStats;
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

            // Put 1000 records into the database.
            PutRecordCase1(db, statsTxn);

            if (ifIsolation == false)
            {
                stats = db.Stats(statsTxn);
                fastStats = db.FastStats(statsTxn);
            }
            else
            {
                stats = db.Stats(statsTxn, Isolation.DEGREE_TWO);
                fastStats = db.FastStats(statsTxn, 
                    Isolation.DEGREE_TWO);
            }
            ConfirmStatsPart2Case1(stats);

            // Delete 500 records.
            for (int i = 250; i <= 750; i++)
                db.Delete(new DatabaseEntry(BitConverter.GetBytes(i)),
                    statsTxn);

            if (ifIsolation == false)
            {
                stats = db.Stats(statsTxn);
                fastStats = db.FastStats(statsTxn);
            }
            else
            {
                stats = db.Stats(statsTxn, Isolation.DEGREE_THREE);
                fastStats = db.FastStats(statsTxn,
                    Isolation.DEGREE_THREE);
            }
            ConfirmStatsPart3Case1(stats);

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

        public void ConfigCase1(RecnoDatabaseConfig dbConfig)
        {
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.PageSize = 4096;
            dbConfig.Length = 4000;
            dbConfig.PadByte = 256;
        }

        public void PutRecordCase1(RecnoDatabase db, Transaction txn)
        {
            for (int i = 1; i <= 1000; i++)
            {
                if (txn == null)
                    db.Put(new DatabaseEntry(
                        BitConverter.GetBytes(i)),
                        new DatabaseEntry(BitConverter.GetBytes(i)));
                else
                    db.Put(new DatabaseEntry(
                        BitConverter.GetBytes(i)),
                        new DatabaseEntry(
                        BitConverter.GetBytes(i)), txn);
            }
        }

        public void ConfirmStatsPart1Case1(RecnoStats stats)
        {
            Assert.AreEqual(1, stats.EmptyPages);
            Assert.AreEqual(1, stats.Levels);
            Assert.AreNotEqual(0, stats.MagicNumber);
            Assert.AreEqual(10, stats.MetadataFlags);
            Assert.AreEqual(2, stats.MinKey);
            Assert.AreEqual(2, stats.nPages);
            Assert.AreEqual(4096, stats.PageSize);
            Assert.AreEqual(4000, stats.RecordLength);
            Assert.AreEqual(256, stats.RecordPadByte);
            Assert.AreEqual(9, stats.Version);
        }

        public void ConfirmStatsPart2Case1(RecnoStats stats)
        {
            Assert.AreEqual(0, stats.DuplicatePages);
            Assert.AreEqual(0, stats.DuplicatePagesFreeBytes);
            Assert.AreNotEqual(0, stats.InternalPages);
            Assert.AreNotEqual(0, stats.InternalPagesFreeBytes);
            Assert.AreNotEqual(0, stats.LeafPages);
            Assert.AreNotEqual(0, stats.LeafPagesFreeBytes);
            Assert.AreEqual(1000, stats.nData);
            Assert.AreEqual(1000, stats.nKeys);
            Assert.AreNotEqual(0, stats.OverflowPages);
            Assert.AreNotEqual(0, stats.OverflowPagesFreeBytes);
        }

        public void ConfirmStatsPart3Case1(RecnoStats stats)
        {
            Assert.AreNotEqual(0, stats.FreePages);
        }

        [Test]
        public void TestTruncateUnusedPages()
        {
            testName = "TestTruncateUnusedPages";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            DatabaseEnvironmentConfig envConfig = 
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseCDB = true;
            envConfig.UseMPool = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);
            
            RecnoDatabaseConfig dbConfig = 
                new RecnoDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            dbConfig.PageSize = 512;
            RecnoDatabase db = RecnoDatabase.Open(
                testName + ".db", dbConfig);

            ModifyRecordsInDB(db, null);
            Assert.Less(0, db.TruncateUnusedPages());

            db.Close();
            env.Close();
        }

        [Test]
        public void TestTruncateUnusedPagesInTxn()
        {
            testName = "TestTruncateUnusedPagesInTxn";
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

            Transaction openTxn = env.BeginTransaction();
            RecnoDatabaseConfig dbConfig =
                new RecnoDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            dbConfig.PageSize = 512;
            RecnoDatabase db = RecnoDatabase.Open(
                testName + ".db", dbConfig, openTxn);
            openTxn.Commit();

            Transaction modifyTxn = env.BeginTransaction();
            ModifyRecordsInDB(db, modifyTxn);
            Assert.Less(0, db.TruncateUnusedPages(modifyTxn));
            modifyTxn.Commit();

            db.Close();
            env.Close();
        }

        public void ModifyRecordsInDB(RecnoDatabase db, 
            Transaction txn)
        {
            uint[] recnos = new uint[100];

            if (txn == null)
            {
                // Add a lot of records into database.
                for (int i = 0; i < 100; i++)
                    recnos[i] = db.Append(new DatabaseEntry(
                        new byte[10240]));

                // Remove some records from database.
                for (int i = 30; i < 100; i++)
                    db.Delete(new DatabaseEntry(
                        BitConverter.GetBytes(recnos[i])));
            }
            else
            {
                // Add a lot of records into database in txn.
                for (int i = 0; i < 100; i++)
                    recnos[i] = db.Append(new DatabaseEntry(
                        new byte[10240]), txn);

                // Remove some records from database in txn.
                for (int i = 30; i < 100; i++)
                    db.Delete(new DatabaseEntry(
                        BitConverter.GetBytes(recnos[i])), txn);
            }
        }

        public static void Confirm(XmlElement xmlElem,
            RecnoDatabase recnoDB, bool compulsory)
        {
            DatabaseTest.Confirm(xmlElem, recnoDB, compulsory);

            // Confirm recno database specific field/property
            Configuration.ConfirmInt(xmlElem, "Delimiter",
                recnoDB.RecordDelimiter, compulsory);
            Configuration.ConfirmUint(xmlElem, "Length",
                recnoDB.RecordLength, compulsory);
            Configuration.ConfirmInt(xmlElem, "PadByte",
                recnoDB.RecordPad, compulsory);
            Configuration.ConfirmBool(xmlElem, "Renumber",
                recnoDB.Renumber, compulsory);
            Configuration.ConfirmBool(xmlElem, "Snapshot",
                recnoDB.Snapshot, compulsory);
            Assert.AreEqual(DatabaseType.RECNO, recnoDB.Type);
            string type = recnoDB.Type.ToString();
            Assert.IsNotNull(type);
        }
    }
}

