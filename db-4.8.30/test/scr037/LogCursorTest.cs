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
    public class LogCursorTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;
        DatabaseEnvironment env;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "LogCursorTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            /*
             * Delete existing test ouput directory and files specified 
             * for the current test fixture and then create a new one.
             */
            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestClose()
        {
            testName = "TestClose";
            testHome = testFixtureHome + "/" + testName;
            DatabaseEnvironment env;    
            LogCursor logCursor;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);
            
            // Get log cursor to read/write log.
            logCursor = env.GetLogCursor();

            // Close the log cursor and env.
            logCursor.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TesCurrentLSN()
        {
            testName = "TesCurrentLSN";
            testHome = testFixtureHome + "/" + testName;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);

            // Get log cursor to read/write log. 
            LogCursor logCursor = env.GetLogCursor();

            /*
             * Move the cursor to the beginning of the #1 log file.
             * Get the current LSN and confirm that is the position
             * the cursor is moved to.
             */
            LSN lsn = new LSN(1, 0);
            logCursor.Move(lsn);
            Assert.AreEqual(lsn.LogFileNumber,
                logCursor.CurrentLSN.LogFileNumber);
            Assert.AreEqual(lsn.Offset, logCursor.CurrentLSN.Offset);

            // Close all.
            logCursor.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestCurrentRecord()
        {
            testName = "TestCurrentRecord";
            testHome = testFixtureHome + "/" + testName;
            DatabaseEnvironment env;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);

            // Get log cursor to read/write log. 
            LogCursor logCursor = env.GetLogCursor();

            /*
             * Move the cursor to the beginning of the #1 log file.
             * Get the current LSN and confirm that is the position
             * the cursor is moved to.
             */
            LSN lsn = new LSN(1, 0);
            logCursor.Move(lsn);
            Assert.IsNotNull(logCursor.CurrentRecord.Data);

            // Close all.
            logCursor.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMove()
        {
            testName = "TestMove";
            testHome = testFixtureHome + "/" + testName;
            DatabaseEnvironment env;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);

            // Get log cursor to read/write log. 
            LogCursor logCursor = env.GetLogCursor();

            // Move the cursor to specified location in log files.
            LSN lsn = new LSN(1, 0);
            Assert.IsTrue(logCursor.Move(lsn));

            // Close all.
            logCursor.Close();
            db.Close();
            env.Close();
        }

        /*
        [Test]
        public void TestMoveFirst()
        {
            testName = "TestMoveFirst";
            testHome = testFixtureHome + "/" + testName;
            DatabaseEnvironment env;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);

            // Get log cursor to read/write log. 
            LogCursor logCursor = env.GetLogCursor();

            // Move to the first LSN in log file.
            Assert.IsTrue(logCursor.MoveFirst());
            
            // Confirm offset of the fist LSN should be 0.
            Assert.AreEqual(0, logCursor.CurrentLSN.Offset);

            // Close all.
            logCursor.Close();
            db.Close();
            env.Close();
        }
        */

        [Test]
        public void TestMoveLast()
        {
            testName = "TestMoveLast";
            testHome = testFixtureHome + "/" + testName;
            DatabaseEnvironment env;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);

            // Get log cursor to read/write log. 
            LogCursor logCursor = env.GetLogCursor();

            // Move to the last LSN in log file.
            Assert.IsTrue(logCursor.MoveLast());

            // The offset of last LSN shouldn't be 0.
            Assert.AreNotEqual(0, logCursor.CurrentLSN.Offset);

            // Close all.
            logCursor.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestMoveNext()
        {
            testName = "TestMoveNext";
            testHome = testFixtureHome + "/" + testName;
            DatabaseEnvironment env;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);

            // Get log cursor to read/write log. 
            LogCursor logCursor = env.GetLogCursor();

            logCursor.MoveLast();
            DatabaseEntry curRec = logCursor.CurrentRecord;
            for (int i = 0; i < 1000; i++)
                db.Append(new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes("new data")));

            Assert.IsTrue(logCursor.MoveNext());

            logCursor.MoveNext();

            // Close the log cursor.
            logCursor.Close();
            db.Close();
            env.Close();
        }


        [Test]
        public void TestMovePrev()
        {
            testName = "TestMovePrev";
            testHome = testFixtureHome + "/" + testName;
            DatabaseEnvironment env;
            LSN lsn;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);

            // Get log cursor to read/write log. 
            LogCursor logCursor = env.GetLogCursor();

            // Get the last two LSN in log file.
            logCursor.MoveLast();
            Assert.IsTrue(logCursor.MovePrev());

            // Close all.
            logCursor.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestRefresh()
        {
            testName = "TestRefresh";
            testHome = testFixtureHome + "/" + testName;
            DatabaseEnvironment env;
            LSN lsn;
            RecnoDatabase db;

            Logging(testHome, testName, out env, out db);

            // Get log cursor to read/write log. 
            LogCursor logCursor = env.GetLogCursor();

            // Move the cursor to the last record. 
            logCursor.MoveLast();
            DatabaseEntry curRec = logCursor.CurrentRecord;

            // Put some new records into database.
            for (int i = 0; i < 10; i++)
                db.Append(new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes("new data")));

            // Get the current record that cursor points to.
            logCursor.Refresh();

            // It shouldn't be changed.
            Assert.AreEqual(curRec.Data, 
                logCursor.CurrentRecord.Data);

            // Close all.
            logCursor.Close();
            db.Close();
            env.Close();
        }

        /*
         * Open environment, database and write data into database. 
         * Generated log files are put under testHome.
         */
        public void Logging(string home, string dbName,
            out DatabaseEnvironment env, out RecnoDatabase recnoDB)
        {
            string dbFileName = dbName + ".db";

            Configuration.ClearDir(home);

            // Open environment with logging subsystem.
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLogging = true;
            envConfig.LogSystemCfg = new LogConfig();
            envConfig.LogSystemCfg.FileMode = 755;
            envConfig.LogSystemCfg.ZeroOnCreate = true;
            envConfig.UseMPool = true;
            env = DatabaseEnvironment.Open(home, envConfig);

            /*
             * Open recno database, write 100000 records into 
             * the database and close it.
             */
            RecnoDatabaseConfig recnoConfig =
                new RecnoDatabaseConfig();
            recnoConfig.Creation = CreatePolicy.IF_NEEDED;
            recnoConfig.Env = env;
            // The db needs mpool to open.
            recnoConfig.NoMMap = false;
            recnoDB = RecnoDatabase.Open(dbFileName,
                recnoConfig);
            for (int i = 0; i < 1000; i++)
                recnoDB.Append(new DatabaseEntry(
                    ASCIIEncoding.ASCII.GetBytes("key")));
        }
    }
}
