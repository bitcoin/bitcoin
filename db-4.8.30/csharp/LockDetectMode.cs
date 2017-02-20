/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using CSharp_API.Internal;

namespace CSharp_API
{
    public class LockDetectMode
    {
        public static LockDetectMode DEFAULT = new LockDetectMode(DbConstants.DB_LOCK_DEFAULT);
        public static LockDetectMode EXPIRE = new LockDetectMode(DbConstants.DB_LOCK_EXPIRE);
        public static LockDetectMode MAXLOCKS = new LockDetectMode(DbConstants.DB_LOCK_MAXLOCKS);
        public static LockDetectMode MAXWRITE = new LockDetectMode(DbConstants.DB_LOCK_MAXWRITE);
        public static LockDetectMode MINLOCKS = new LockDetectMode(DbConstants.DB_LOCK_MINLOCKS);
        public static LockDetectMode MINWRITE = new LockDetectMode(DbConstants.DB_LOCK_MINWRITE);
        public static LockDetectMode OLDEST = new LockDetectMode(DbConstants.DB_LOCK_OLDEST);
        public static LockDetectMode RANDOM = new LockDetectMode(DbConstants.DB_LOCK_RANDOM);
        public static LockDetectMode YOUNGEST = new LockDetectMode(DbConstants.DB_LOCK_YOUNGEST);

        private uint mode;

        internal static uint GetMode(LockDetectMode ldm)
        {
            return ldm == null ? 0 : ldm.mode;
        }

        
        private LockDetectMode(uint detectMode)
        {
            mode = detectMode;
        }
    }
}
