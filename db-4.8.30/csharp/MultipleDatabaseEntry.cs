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
    /// A class providing access to multiple <see cref="DatabaseEntry"/>
    /// objects.
    /// </summary>
    public class MultipleDatabaseEntry : IEnumerable<DatabaseEntry> {
        private byte[] data;
        private uint ulen;

        internal MultipleDatabaseEntry(DatabaseEntry dbt) {
            data = dbt.UserData;
            ulen = dbt.ulen;
        }

        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }

        /// <summary>
        /// Return an enumerator which iterates over all
        /// <see cref="DatabaseEntry"/> objects represented by the 
        /// <see cref="MultipleDatabaseEntry"/>.
        /// </summary>
        /// <returns>
        /// An enumerator for the <see cref="MultipleDatabaseEntry"/>
        /// </returns>
        public IEnumerator<DatabaseEntry> GetEnumerator() {
            uint pos = ulen - 4;
            int off = BitConverter.ToInt32(data, (int)pos);
            for (int i = 0;
                off >= 0; off = BitConverter.ToInt32(data, (int)pos), i++) {
                pos -= 4;
                int sz = BitConverter.ToInt32(data, (int)pos);
                byte[] arr = new byte[sz];
                Array.Copy(data, off, arr, 0, sz);
                pos -= 4;
                yield return new DatabaseEntry(arr);
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
