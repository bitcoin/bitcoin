/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// A class providing access to multiple key/data pairs.
    /// </summary>
    public class MultipleKeyDatabaseEntry
        : IEnumerable<KeyValuePair<DatabaseEntry, DatabaseEntry>> {
        private byte[] data;
        private uint ulen;
        private DatabaseType dbtype;

        internal MultipleKeyDatabaseEntry(DatabaseType type, DatabaseEntry dbt) {
            data = dbt.UserData;
            ulen = dbt.ulen;
            dbtype = type;
        }

        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

        /// <summary>
        /// Return an enumerator which iterates over all
        /// <see cref="DatabaseEntry"/> pairs represented by the 
        /// <see cref="MultipleKeyDatabaseEntry"/>.
        /// </summary>
        /// <returns>
        /// An enumerator for the <see cref="MultipleDatabaseEntry"/>
        /// </returns>
        public IEnumerator<KeyValuePair<DatabaseEntry, DatabaseEntry>> GetEnumerator() {
            byte[] darr, karr;
            int off, sz;
            uint pos, recno;
            
            pos = ulen - 4;
            
            if (dbtype == DatabaseType.BTREE || dbtype == DatabaseType.HASH) {
                off = BitConverter.ToInt32(data, (int)pos);
                for (int i = 0; off >= 0; off = BitConverter.ToInt32(data, (int)pos), i++) {
                    pos -= 4;
                    sz = BitConverter.ToInt32(data, (int)pos);
                    karr = new byte[sz];
                    Array.Copy(data, off, karr, 0, sz);
                    pos -= 4;
                    off = BitConverter.ToInt32(data, (int)pos);
                    pos -= 4;
                    sz = BitConverter.ToInt32(data, (int)pos);
                    darr = new byte[sz];
                    Array.Copy(data, off, darr, 0, sz);
                    pos -= 4;
                    yield return new KeyValuePair<DatabaseEntry, DatabaseEntry>(new DatabaseEntry(karr), new DatabaseEntry(darr));
                }
            } else {
                recno = BitConverter.ToUInt32(data, (int)pos);
                for (int i = 0; recno > 0; recno = BitConverter.ToUInt32(data, (int)pos), i++) {
                    pos -= 4;
                    off = BitConverter.ToInt32(data, (int)pos);
                    pos -= 4;
                    sz = BitConverter.ToInt32(data, (int)pos);
                    darr = new byte[sz];
                    Array.Copy(data, off, darr, 0, sz);
                    pos -= 4;
                    yield return new KeyValuePair<DatabaseEntry, DatabaseEntry>(new DatabaseEntry(BitConverter.GetBytes(recno)), new DatabaseEntry(darr));
                }
            }
        }
        
        // public byte[][] Data;
        /* No Public Constructor */
        //internal MultipleDatabaseEntry(DatabaseEntry dbt) {
        //    byte[] dat = dbt.UserData;
        //    List<byte[]> tmp = new List<byte[]>();
        //    uint pos = dbt.ulen - 4;
        //    int off = BitConverter.ToInt32(dat, (int)pos);
        //    for (int i = 0; off > 0; off = BitConverter.ToInt32(dat, (int)pos), i++) {
        //        pos -= 4;
        //        int sz = BitConverter.ToInt32(dat, (int)pos);
        //        tmp.Add(new byte[sz]);
        //        Array.Copy(dat, off, tmp[i], 0, sz);
        //        pos -= 4;
        //    }
        //    Data = tmp.ToArray();
        //}


    }
}
