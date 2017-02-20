/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing a key or data item in a Berkeley DB database
    /// </summary>
    public class DatabaseEntry : IDisposable{
        internal DBT dbt;
        private byte[] _data;

        internal static DBT getDBT(DatabaseEntry ent) {
            return ent == null ? null : ent.dbt;
        }

        internal static DatabaseEntry fromDBT(DBT dbt) {
            if (dbt.app_data != null)
                return dbt.app_data;
            else
                return new DatabaseEntry(dbt);
        }

        private DatabaseEntry(DBT dbtp) {
            dbt = dbtp;
            Data = (byte[])dbtp.data.Clone();
        }

        /// <summary>
        /// Create a new, empty DatabaseEntry object.
        /// </summary>
        public DatabaseEntry() {
            dbt = new DBT();
            if (!this.GetType().IsGenericType)
                dbt.app_data = this;
        }

        /// <summary>
        /// Create a new DatabaseEntry object, with the specified data 
        /// </summary>
        /// <param name="data">The new object's <see cref="Data"/></param>
        public DatabaseEntry(byte[] data)
            : this() {
            Data = data;
        }

        /// <summary>
        /// The byte string stored in or retrieved from a database
        /// </summary>
        public byte[] Data {
            /*
             * Data to be stored in the DB or has been retrieved from the DB.  We
             * keep it in C#'s managed memory here, dbt_usercopy will copy it
             * to/from the library as needed.  Need to set the size of the DBT in
             * the C library so that dbt_usercopy knows there's data to copy.
             */ 
            get { return _data; }
            set {
                _data = value;
                dbt.size = value == null ? 0 : (uint)value.Length;
            }
        }

        internal byte[] UserData {
            get { return dbt.data; }
            set { 
                dbt.data = value;
                ulen = (uint)value.Length;
                flags &= ~DbConstants.DB_DBT_USERCOPY;
                flags |= DbConstants.DB_DBT_USERMEM;
            }
        }

        /// <summary>
        /// Release the resources held by the underlying C library.
        /// </summary>
        public virtual void Dispose() {
            dbt.Dispose();
        }

        /*
         * Copy between the C# byte array and the C library's void * as needed.
         * The library will call this method when it needs data from us or has
         * data to give us.  This prevents us from needing to copy all data in
         * and all data out.  The callback to this method gets set when the 
         * DatabaseEnvironment is created (or the Database if created w/o an 
         * environment.)
         */ 
        internal static int dbt_usercopy(IntPtr dbtp, uint offset, IntPtr buf, uint size, uint flags) {
            DBT dbt = new DBT(dbtp, false);
            DatabaseEntry ent = dbt.app_data;

            if (flags == DbConstants.DB_USERCOPY_GETDATA)
                Marshal.Copy(ent.Data, 0, buf, (int)size);
            else {
                /*
                 * If the offset is zero, we're writing a new buffer and can
                 * simply allocate byte array.  If the offset is not zero,
                 * however, we are appending to the exisiting array.  Since we
                 * can't extend it, we have to allocate a new one and copy.
                 * 
                 * Our caller is setting dbt.size, so set ent._data directly,
                 * since ent.Data would overwrite dbt.size.
                 */
                if (offset != 0) {
                    byte[] t = new byte[ent.Data.Length + (int)size];
                    ent.Data.CopyTo(t, 0);
                    ent._data = t;
                } else
                    ent._data = new byte[(int)size];
                
                Marshal.Copy(buf, ent.Data, (int)offset, (int)size);
            }
            return 0;
        }

        internal uint ulen { 
            get { return dbt.ulen; }
            set { dbt.ulen = value; }
        }
        internal uint flags {
            get { return dbt.flags; }
            set { dbt.flags = value; }
        }
        internal uint size {
            get { return dbt.size; }
            set { dbt.size = value; }
        }
    }
}