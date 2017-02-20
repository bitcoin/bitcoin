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
    public class DatabaseExceptionTest
    {
        [Test]
        public void TestDB_REP_DUPMASTER()
        {
            DatabaseException.ThrowException(ErrorCodes.DB_REP_DUPMASTER);
        }

        [Test]
        public void TestDB_REP_HOLDELECTION()
        {
            DatabaseException.ThrowException(ErrorCodes.DB_REP_HOLDELECTION);
        }

        [Test]
        public void TestDB_REP_IGNORE()
        {
            DatabaseException.ThrowException(ErrorCodes.DB_REP_IGNORE);
        }

        [Test]
        public void TestDB_REP_ISPERM()
        {
            DatabaseException.ThrowException(ErrorCodes.DB_REP_ISPERM);
        }

        [Test]
        public void TestDB_REP_JOIN_FAILURE()
        {
            DatabaseException.ThrowException(ErrorCodes.DB_REP_JOIN_FAILURE);
        }

        [Test]
        public void TestDB_REP_NEWSITE()
        {
            DatabaseException.ThrowException(ErrorCodes.DB_REP_NEWSITE);
        }

        [Test]
        public void TestDB_REP_NOTPERM()
        {
            DatabaseException.ThrowException(ErrorCodes.DB_REP_NOTPERM);
        }

        [Test]
        public void TestDeadlockException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_LOCK_DEADLOCK);
            }
            catch (DeadlockException e)
            {
                Assert.AreEqual(ErrorCodes.DB_LOCK_DEADLOCK, e.ErrorCode);
            }
        }

        [Test]
        public void TestForeignConflictException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_FOREIGN_CONFLICT);
            }
            catch (ForeignConflictException e)
            {
                Assert.AreEqual(ErrorCodes.DB_FOREIGN_CONFLICT, e.ErrorCode);
            }
        }

        [Test]
        public void TestKeyEmptyException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_KEYEMPTY);
            }
            catch (KeyEmptyException e)
            {
                Assert.AreEqual(ErrorCodes.DB_KEYEMPTY, e.ErrorCode);
            }
        }

        [Test]
        public void TestKeyExistException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_KEYEXIST);
            }
            catch (KeyExistException e)
            {
                Assert.AreEqual(ErrorCodes.DB_KEYEXIST, e.ErrorCode);
            }
        }

        [Test]
        public void TestLeaseExpiredException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_REP_LEASE_EXPIRED);
            }
            catch (LeaseExpiredException e)
            {
                Assert.AreEqual(ErrorCodes.DB_REP_LEASE_EXPIRED, e.ErrorCode);
            }
        }
        
        [Test]
        public void TestLockNotGrantedException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_LOCK_NOTGRANTED);
            }
            catch (LockNotGrantedException e)
            {
                Assert.AreEqual(ErrorCodes.DB_LOCK_NOTGRANTED, e.ErrorCode);
            }
        }

        [Test]
        public void TestNotFoundException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_NOTFOUND);
            }
            catch (NotFoundException e)
            {
                Assert.AreEqual(ErrorCodes.DB_NOTFOUND, e.ErrorCode);
            }
        }

        [Test]
        public void TestOldVersionException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_OLD_VERSION);
            }
            catch (OldVersionException e)
            {
                Assert.AreEqual(ErrorCodes.DB_OLD_VERSION, e.ErrorCode);
            }
        }

        [Test]
        public void TestPageNotFoundException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_PAGE_NOTFOUND);
            }
            catch (PageNotFoundException e)
            {
                Assert.AreEqual(ErrorCodes.DB_PAGE_NOTFOUND, e.ErrorCode);
            }
        }

        [Test]
        public void TestRunRecoveryException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_RUNRECOVERY);
            }
            catch (RunRecoveryException e)
            {
                Assert.AreEqual(ErrorCodes.DB_RUNRECOVERY, e.ErrorCode);
            }

        }

        [Test]
        public void TestVerificationException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_VERIFY_BAD);
            }
            catch (VerificationException e)
            {
                Assert.AreEqual(ErrorCodes.DB_VERIFY_BAD, e.ErrorCode);
            }
        }

        [Test]
        public void TestVersionMismatchException()
        {
            try
            {
                DatabaseException.ThrowException(ErrorCodes.DB_VERSION_MISMATCH);
            }
            catch (VersionMismatchException e)
            {
                Assert.AreEqual(ErrorCodes.DB_VERSION_MISMATCH, e.ErrorCode);
            }
        }
    }
}
