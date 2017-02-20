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
    public class RecnoCursorTest
    {
        private string testFixtureName;
        private string testFixtureHome;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "RecnoCursorTest";
            testFixtureHome = "./TestOut/" + testFixtureName;
        }

        [Test]
        public void TestCursor()
        {
            testName = "TestCursor";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            GetCursorWithImplicitTxn(testHome,
                testName + ".db", false);
        }

        [Test]
        public void TestCursorWithConfig()
        {
            testName = "TestCursorWithConfig";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            GetCursorWithImplicitTxn(testHome, 
                testName + ".db", true);
        }

        public void GetCursorWithImplicitTxn(string home, 
            string dbFile, bool ifConfig)
        {
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseCDB = true;
            envConfig.UseMPool = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, envConfig);

            RecnoDatabaseConfig dbConfig = 
                new RecnoDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            RecnoDatabase db = RecnoDatabase.Open(dbFile,
                dbConfig);

            RecnoCursor cursor;
            if (ifConfig == false)
                cursor = db.Cursor();
            else
                cursor = db.Cursor(new CursorConfig());

            cursor.Close();
            db.Close();
            env.Close();
        }

        [Test]
        public void TestCursorInTxn()
        {
            testName = "TestCursorInTxn";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            GetCursorWithExplicitTxn(testHome,
                testName + ".db", false);
        }

        [Test]
        public void TestConfigedCursorInTxn()
        {
            testName = "TestConfigedCursorInTxn";
            testHome = testFixtureHome + "/" + testName;

            Configuration.ClearDir(testHome);

            GetCursorWithExplicitTxn(testHome,
                testName + ".db", true);
        }

        public void GetCursorWithExplicitTxn(string home,
            string dbFile, bool ifConfig)
        {
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, envConfig);

            Transaction openTxn = env.BeginTransaction();
            RecnoDatabaseConfig dbConfig =
                new RecnoDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
            RecnoDatabase db = RecnoDatabase.Open(dbFile,
                dbConfig, openTxn);
            openTxn.Commit();

            Transaction cursorTxn = env.BeginTransaction();
            RecnoCursor cursor;
            if (ifConfig == false)
                cursor = db.Cursor(cursorTxn);
            else
                cursor = db.Cursor(new CursorConfig(), cursorTxn);
            cursor.Close();
            cursorTxn.Commit(); 
            db.Close();
            env.Close();
        }

        [Test]
        public void TestDuplicate()
        {
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            RecnoDatabase db;
            RecnoDatabaseConfig dbConfig;
            RecnoCursor cursor, dupCursor;
            string dbFileName;

            testName = "TestDuplicate";
            testHome = testFixtureHome + "/" + testName;
            dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            dbConfig = new RecnoDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            db = RecnoDatabase.Open(dbFileName, dbConfig);
            cursor = db.Cursor();

            /*
             * Add a record(1, 1) by cursor and move 
             * the cursor to the current record.
             */
            AddOneByCursor(cursor);
            cursor.Refresh();

            //Duplicate a new cursor to the same position. 
            dupCursor = cursor.Duplicate(true);

            // Overwrite the record.
            dupCursor.Overwrite(new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("newdata")));

            // Confirm that the original data doesn't exist.
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(
                BitConverter.GetBytes((int)1)),
                new DatabaseEntry(
                BitConverter.GetBytes((int)1)));
            Assert.IsFalse(dupCursor.Move(pair, true));

            dupCursor.Close();
            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestInsertToLoc()
        {
            RecnoDatabase db;
            RecnoDatabaseConfig dbConfig;
            RecnoCursor cursor;
            DatabaseEntry data;
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
            string dbFileName;

            testName = "TestInsertToLoc";
            testHome = testFixtureHome + "/" + testName;
            dbFileName = testHome + "/" + testName + ".db";

            Configuration.ClearDir(testHome);

            // Open database and cursor.
            dbConfig = new RecnoDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Renumber = true;
            db = RecnoDatabase.Open(dbFileName, dbConfig);
            cursor = db.Cursor();

            // Add record(1,1) into database.
            /*
             * Add a record(1, 1) by cursor and move 
             * the cursor to the current record.
             */
            AddOneByCursor(cursor);
            cursor.Refresh();

            /*
             * Insert the new record(1,10) after the 
             * record(1,1).
             */
            data = new DatabaseEntry(
                BitConverter.GetBytes((int)10));
            cursor.Insert(data, Cursor.InsertLocation.AFTER);

            /*
             * Move the cursor to the record(1,1) and 
             * confirm that the next record is the one just inserted. 
             */
            pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(
                BitConverter.GetBytes((int)1)),
                new DatabaseEntry(
                BitConverter.GetBytes((int)1)));
            Assert.IsTrue(cursor.Move(pair, true));
            Assert.IsTrue(cursor.MoveNext());
            Assert.AreEqual(BitConverter.GetBytes((int)10),
                cursor.Current.Value.Data);

            cursor.Close();
            db.Close();
        }

        public void AddOneByCursor(RecnoCursor cursor)
        {
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
                new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(
                BitConverter.GetBytes((int)1)),
                new DatabaseEntry(
                  BitConverter.GetBytes((int)1)));
            cursor.Add(pair);
        }
    }
}
