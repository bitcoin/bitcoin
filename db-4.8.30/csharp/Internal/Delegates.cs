using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace BerkeleyDB.Internal {
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void BDB_AppendRecnoDelegate(IntPtr db, IntPtr data, uint recno);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int BDB_AssociateDelegate(IntPtr db, IntPtr key, IntPtr data, IntPtr result);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int BDB_AssociateForeignDelegate(IntPtr db, IntPtr key, IntPtr data, IntPtr foreign, ref int changed);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int BDB_CompareDelegate(IntPtr db, IntPtr dbt1, IntPtr dbt2);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int BDB_CompressDelegate(IntPtr db, IntPtr prevKey, IntPtr prevData, IntPtr key, IntPtr data, IntPtr dest);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void BDB_DbFeedbackDelegate(IntPtr db, int opcode, int percent);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int BDB_DecompressDelegate(IntPtr db, IntPtr prevKey, IntPtr prevData, IntPtr compressed, IntPtr destKey, IntPtr destData);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void BDB_EnvFeedbackDelegate(IntPtr db, int opcode, int percent);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void BDB_ErrcallDelegate(IntPtr env, string pfx, string msg);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void BDB_EventNotifyDelegate(IntPtr dbenv, uint eventcode, byte[] event_info);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int BDB_FileWriteDelegate(System.IO.TextWriter fs, string buf);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate uint BDB_HashDelegate(IntPtr db, IntPtr data, uint len);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int BDB_IsAliveDelegate(IntPtr dbenv, int pid, uint tid, uint flags);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int BDB_RepTransportDelegate(IntPtr dbenv, IntPtr control, IntPtr rec, IntPtr lsnp, int envid, uint flags);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void BDB_ThreadIDDelegate(IntPtr dbenv, IntPtr pid, IntPtr tid);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate string BDB_ThreadNameDelegate(IntPtr dbenv, int pid, uint tid, ref string buf);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate int DBTCopyDelegate(IntPtr dbt, uint offset, IntPtr buf, uint sz, uint flags);
}