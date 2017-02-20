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
    public class JoinCursorTest
    {
        private string testFixtureHome;
        private string testFixtureName;
        private string testName;
        private string testHome;

        [TestFixtureSetUp]
        public void RunBeforeTests()
        {
            testFixtureName = "JoinCursorTest";
            testFixtureHome = "./TestOut/" + testFixtureName;

            Configuration.ClearDir(testFixtureHome);
        }

        [Test]
        public void TestJoin()
        {
            testName = "TestJoin";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string secFileName1 = testHome + "/" + "sec_" + testName + "1.db";
            string secFileName2 = testHome + "/" + "sec_" + testName + "2.db";

            Configuration.ClearDir(testHome);

            // Open a primary database.
            BTreeDatabaseConfig dbCfg = new BTreeDatabaseConfig();
            dbCfg.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase db = BTreeDatabase.Open(dbFileName, dbCfg);

            /*
             * Open two databases, their secondary databases and 
             * secondary cursors.
             */
            SecondaryBTreeDatabase secDB1, secDB2;
            SecondaryCursor[] cursors = new SecondaryCursor[2];
            GetSecCursor(db, secFileName1,
                new SecondaryKeyGenDelegate(KeyGenOnBigByte),
                out secDB1, out cursors[0], false, null);
            GetSecCursor(db, secFileName2,
                new SecondaryKeyGenDelegate(KeyGenOnLittleByte),
                out secDB2, out cursors[1], true, null);

            // Get join cursor.
            JoinCursor joinCursor = db.Join(cursors, true);

            // Close all.
            joinCursor.Close();
            cursors[0].Close();
            cursors[1].Close();
            secDB1.Close();
            secDB2.Close();
            db.Close();
        }

        [Test]
        public void TestMoveJoinCursor()
        {
            testName = "TestMoveJoinCursor";
            testHome = testFixtureHome + "/" + testName;
            string dbFileName = testHome + "/" + testName + ".db";
            string secFileName1 = testHome + "/" + "sec_" + testName + "1.db";
            string secFileName2 = testHome + "/" + "sec_" + testName + "2.db";

            Configuration.ClearDir(testHome);

            // Open a primary database.
            BTreeDatabaseConfig dbCfg = new BTreeDatabaseConfig();
            dbCfg.Creation = CreatePolicy.IF_NEEDED;
            BTreeDatabase db = BTreeDatabase.Open(dbFileName, dbCfg);

            /*
             * Open a secondary database on high byte of 
             * its data and another secondary database on 
             * little byte of its data.
             */
            SecondaryBTreeDatabase secDB1, secDB2;
            SecondaryCursor[] cursors = new SecondaryCursor[2];
            byte[] byteValue = new byte[1];

            byteValue[0] = 0;
            GetSecCursor(db, secFileName1,
                new SecondaryKeyGenDelegate(KeyGenOnBigByte),
                out secDB1, out cursors[0], false,
                new DatabaseEntry(byteValue));

            byteValue[0] = 1;
            GetSecCursor(db, secFileName2,
                new SecondaryKeyGenDelegate(KeyGenOnLittleByte),
                out secDB2, out cursors[1], true,
                new DatabaseEntry(byteValue));

            // Get join cursor.
            JoinCursor joinCursor = db.Join(cursors, true);

            /*
             * MoveNextJoinItem do not use data value found 
             * in all of the cursor so the data in Current won't be 
             * changed.
             */
            Assert.IsTrue(joinCursor.MoveNextItem());
            Assert.AreEqual(0, joinCursor.Current.Key.Data[
                joinCursor.Current.Key.Data.Length - 1]);
            Assert.AreEqual(1, joinCursor.Current.Key.Data[0]);
            Assert.IsNull(joinCursor.Current.Value.Data);

            // Iterate on join cursor. 
            foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> pair in joinCursor)
            {
                /*
                 * Confirm that the key got by join cursor has 0 at 
                 * its highest byte and 1 at its lowest byte.
                 */
                Assert.AreEqual(0, pair.Key.Data[pair.Key.Data.Length - 1]);
                Assert.AreEqual(1, pair.Key.Data[0]);
            }

            Assert.IsFalse(joinCursor.MoveNext());

            // Close all.
            joinCursor.Close();
            cursors[0].Close();
            cursors[1].Close();
            secDB1.Close();
            secDB2.Close();
            db.Close();
        }

        public void GetSecCursor(BTreeDatabase db,
            string secFileName, SecondaryKeyGenDelegate keyGen,
            out SecondaryBTreeDatabase secDB,
            out SecondaryCursor cursor, bool ifCfg,
            DatabaseEntry data)
        {
            // Open secondary database.
            SecondaryBTreeDatabaseConfig secCfg = 
                new SecondaryBTreeDatabaseConfig(db, keyGen);
            secCfg.Creation = CreatePolicy.IF_NEEDED;
            secCfg.Duplicates = DuplicatesPolicy.SORTED;
            secDB = SecondaryBTreeDatabase.Open(secFileName, secCfg);

            int[] intArray = new int[4];
            intArray[0] = 0;
            intArray[1] = 1;
            intArray[2] = 2049;
            intArray[3] = 65537;
            for (int i = 0; i < 4; i++)
            {
                DatabaseEntry record = new DatabaseEntry(
                    BitConverter.GetBytes(intArray[i]));
                db.Put(record, record);
            }

            // Get secondary cursor on the secondary database.
            if (ifCfg == false)
                cursor = secDB.SecondaryCursor();
            else
                cursor = secDB.SecondaryCursor(new CursorConfig());

            // Position the cursor.
            if (data != null)
                Assert.IsTrue(cursor.Move(data, true));
        }

        public DatabaseEntry KeyGenOnLittleByte(
            DatabaseEntry key, DatabaseEntry data)
        {
            byte[] byteArr = new byte[1];
            byteArr[0] = data.Data[0];
            DatabaseEntry dbtGen = new DatabaseEntry(byteArr);
            return dbtGen;
        }

        public DatabaseEntry KeyGenOnBigByte(
            DatabaseEntry key, DatabaseEntry data)
        {
            byte[] byteArr = new byte[1];
            byteArr[0] = data.Data[data.Data.Length - 1];
            DatabaseEntry dbtGen = new DatabaseEntry(byteArr);
            return dbtGen;
        }
    }
}
