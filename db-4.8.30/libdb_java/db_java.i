%module db_java

%include "various.i"
%include "typemaps.i"

%include "java_util.i"
%include "java_except.i"
%include "java_typemaps.i"
%include "java_stat.i"
%include "java_callbacks.i"

/*
 * No finalize methods in general - most classes have "destructor" methods
 * that applications must call explicitly.
 */
%typemap(javafinalize) SWIGTYPE ""

/*
 * These are the exceptions - when there is no "close" method, we need to free
 * the native part at finalization time.  These are exactly the cases where C
 * applications manage the memory for the handles.
 */
%typemap(javafinalize) struct DbLsn, struct DbLock %{
  protected void finalize() {
    try {
      delete();
    } catch(Exception e) {
      System.err.println("Exception during finalization: " + e);
      e.printStackTrace(System.err);
    }
  }
%}

%typemap(javaimports) SWIGTYPE %{
import com.sleepycat.db.*;
import java.util.Comparator;
%}

/* Class names */
%rename(LogSequenceNumber) DbLsn;

/* Destructors */
%rename(close0) close;
%rename(remove0) remove;
%rename(rename0) rename;
%rename(verify0) verify;
%rename(abort0) abort;
%rename(commit0) commit;
%rename(discard0) discard;

/* Special case methods */
%rename(set_tx_timestamp0) set_tx_timestamp;

/* Extra code in the Java classes */
%typemap(javacode) struct DbEnv %{
	/*
	 * Internally, the JNI layer creates a global reference to each DbEnv,
	 * which can potentially be different to this.  We keep a copy here so
	 * we can clean up after destructors.
	 */
	private long dbenv_ref;
	public Environment wrapper;

	private LogRecordHandler app_dispatch_handler;
	private EventHandler event_notify_handler;
	private FeedbackHandler env_feedback_handler;
	private ErrorHandler error_handler;
	private String errpfx;
	private MessageHandler message_handler;
	private PanicHandler panic_handler;
	private ReplicationTransport rep_transport_handler;
	private java.io.OutputStream error_stream;
	private java.io.OutputStream message_stream;
	private ThreadLocal errBuf;

	public static class RepProcessMessage {
		public int envid;
	}

	/*
	 * Called by the public DbEnv constructor and for private environments
	 * by the Db constructor.
	 */
	void initialize() {
		dbenv_ref = db_java.initDbEnvRef0(this, this);
		errBuf = new ThreadLocal();
		/* Start with System.err as the default error stream. */
		set_error_stream(System.err);
		set_message_stream(System.out);
	}

	void cleanup() {
		swigCPtr = 0;
		db_java.deleteRef0(dbenv_ref);
		dbenv_ref = 0L;
	}

	public synchronized void close(int flags) throws DatabaseException {
		try {
			close0(flags);
		} finally {
			cleanup();
		}
	}

	private final int handle_app_dispatch(DatabaseEntry dbt,
					      LogSequenceNumber lsn,
					      int recops) {
		return app_dispatch_handler.handleLogRecord(wrapper, dbt, lsn,
		    RecoveryOperation.fromFlag(recops));
	}

	public LogRecordHandler get_app_dispatch() {
		return app_dispatch_handler;
	}

	private final void handle_panic_event_notify() {
		event_notify_handler.handlePanicEvent();
	}

	private final void handle_rep_client_event_notify() {
		event_notify_handler.handleRepClientEvent();
	}

	private final void handle_rep_elected_event_notify() {
		event_notify_handler.handleRepElectedEvent();
	}

	private final void handle_rep_master_event_notify() {
		event_notify_handler.handleRepMasterEvent();
	}

	private final void handle_rep_new_master_event_notify(int envid) {
		event_notify_handler.handleRepNewMasterEvent(envid);
	}

	private final void handle_rep_perm_failed_event_notify() {
		event_notify_handler.handleRepPermFailedEvent();
	}

	private final void handle_rep_startup_done_event_notify() {
		event_notify_handler.handleRepStartupDoneEvent();
	}

	private final void handle_write_failed_event_notify(int errno) {
		event_notify_handler.handleWriteFailedEvent(errno);
	}

	public EventHandler get_event_notify() {
		return event_notify_handler;
	}

	private final void handle_env_feedback(int opcode, int percent) {
		if (opcode == DbConstants.DB_RECOVER)
			env_feedback_handler.recoveryFeedback(wrapper, percent);
		/* No other environment feedback type supported. */
	}

	public FeedbackHandler get_feedback() {
		return env_feedback_handler;
	}

	public void set_errpfx(String errpfx) {
		this.errpfx = errpfx;
	}

	public String get_errpfx() {
		return errpfx;
	}

	private final void handle_error(String msg) {
		com.sleepycat.util.ErrorBuffer ebuf = (com.sleepycat.util.ErrorBuffer)errBuf.get();
		if (ebuf == null) {
			/*
			 * Populate the errBuf ThreadLocal on demand, since the
			 * callback can be made from different threads.
			 */
			ebuf = new com.sleepycat.util.ErrorBuffer(3);
			errBuf.set(ebuf);
		}
		ebuf.append(msg);
		error_handler.error(wrapper, this.errpfx, msg);
	}

	private final String get_err_msg(String orig_msg) {
		com.sleepycat.util.ErrorBuffer ebuf = (com.sleepycat.util.ErrorBuffer)errBuf.get();
		String ret = null;
		if (ebuf != null) {
			ret = ebuf.get();
			ebuf.clear();
		}
		if (ret != null && ret.length() > 0)
			return orig_msg + ": " + ret;
		return orig_msg;
	}

	public ErrorHandler get_errcall() {
		return error_handler;
	}

	private final void handle_message(String msg) {
		message_handler.message(wrapper, msg);
	}

	public MessageHandler get_msgcall() {
		return message_handler;
	}

	private final void handle_panic(DatabaseException e) {
		panic_handler.panic(wrapper, e);
	}

	public PanicHandler get_paniccall() {
		return panic_handler;
	}

	private final int handle_rep_transport(DatabaseEntry control,
					       DatabaseEntry rec,
					       LogSequenceNumber lsn,
					       int envid, int flags)
	    throws DatabaseException {
		return rep_transport_handler.send(wrapper,
		    control, rec, lsn, envid,
		    (flags & DbConstants.DB_REP_NOBUFFER) != 0,
		    (flags & DbConstants.DB_REP_PERMANENT) != 0,
		    (flags & DbConstants.DB_REP_ANYWHERE) != 0,
		    (flags & DbConstants.DB_REP_REREQUEST) != 0);
	}

	public void lock_vec(/*u_int32_t*/ int locker, int flags,
			     LockRequest[] list, int offset, int count)
	    throws DatabaseException {
		db_javaJNI.DbEnv_lock_vec(swigCPtr, this, locker, flags, list,
		    offset, count);
	}

	public synchronized void remove(String db_home, int flags)
	    throws DatabaseException, java.io.FileNotFoundException {
		try {
			remove0(db_home, flags);
		} finally {
			cleanup();
		}
	}

	public void set_error_stream(java.io.OutputStream stream) {
		error_stream = stream;
		final java.io.PrintWriter pw = new java.io.PrintWriter(stream);
		set_errcall(new ErrorHandler() {
			public void error(Environment env,
			    String prefix, String buf) /* no exception */ {
				if (prefix != null)
					pw.print(prefix + ": ");
				pw.println(buf);
				pw.flush();
			}
		});
	}

	public java.io.OutputStream get_error_stream() {
		return error_stream;
	}

	public void set_message_stream(java.io.OutputStream stream) {
		message_stream = stream;
		final java.io.PrintWriter pw = new java.io.PrintWriter(stream);
		set_msgcall(new MessageHandler() {
			public void message(Environment env, String msg)
			    /* no exception */ {
				pw.println(msg);
				pw.flush();
			}
		});
	}

	public java.io.OutputStream get_message_stream() {
		return message_stream;
	}

	public void set_tx_timestamp(java.util.Date timestamp) {
		set_tx_timestamp0(timestamp.getTime()/1000);
	}
%}

%typemap(javacode) struct Db %{
	/* package */ static final int GIGABYTE = 1 << 30;
	/*
	 * Internally, the JNI layer creates a global reference to each Db,
	 * which can potentially be different to this.  We keep a copy here so
	 * we can clean up after destructors.
	 */
	private long db_ref;
	private DbEnv dbenv;
	private boolean private_dbenv;

	public Database wrapper;
	private RecordNumberAppender append_recno_handler;
	private Comparator bt_compare_handler;
	private BtreeCompressor bt_compress_handler;
	private BtreeCompressor bt_decompress_handler;
	private BtreePrefixCalculator bt_prefix_handler;
	private Comparator dup_compare_handler;
	private FeedbackHandler db_feedback_handler;
	private Comparator h_compare_handler;
	private Hasher h_hash_handler;
	private PartitionHandler partition_handler;
	private SecondaryKeyCreator seckey_create_handler;
	private SecondaryMultiKeyCreator secmultikey_create_handler;
	private ForeignKeyNullifier foreignkey_nullify_handler;
	private ForeignMultiKeyNullifier foreignmultikey_nullify_handler;

	/* Called by the Db constructor */
	private void initialize(DbEnv dbenv) {
		if (dbenv == null) {
			private_dbenv = true;
			dbenv = db_java.getDbEnv0(this);
			dbenv.initialize();
		}
		this.dbenv = dbenv;
		db_ref = db_java.initDbRef0(this, this);
	}

	private void cleanup() {
		swigCPtr = 0;
		db_java.deleteRef0(db_ref);
		db_ref = 0L;
		if (private_dbenv)
			dbenv.cleanup();
		dbenv = null;
	}

	public boolean getPrivateDbEnv() {
		return private_dbenv;
	}

	public synchronized void close(int flags) throws DatabaseException {
		try {
			close0(flags);
		} finally {
			cleanup();
		}
	}

	public DbEnv get_env() throws DatabaseException {
		return dbenv;
	}

	private final void handle_append_recno(DatabaseEntry data, int recno)
	    throws DatabaseException {
		append_recno_handler.appendRecordNumber(wrapper, data, recno);
	}

	public RecordNumberAppender get_append_recno() {
		return append_recno_handler;
	}

	private final int handle_bt_compare(byte[] arr1, byte[] arr2) {
		return bt_compare_handler.compare(arr1, arr2);
	}

	private final int handle_bt_compress(DatabaseEntry dbt1,
	    DatabaseEntry dbt2, DatabaseEntry dbt3, DatabaseEntry dbt4,
	    DatabaseEntry dbt5) {
		return bt_compress_handler.compress(wrapper, dbt1, dbt2,
		    dbt3, dbt4, dbt5) ? 0 : DbConstants.DB_BUFFER_SMALL;
	}

	private final int handle_bt_decompress(DatabaseEntry dbt1,
	    DatabaseEntry dbt2, DatabaseEntry dbt3, DatabaseEntry dbt4,
	    DatabaseEntry dbt5) {
		return bt_compress_handler.decompress(wrapper, dbt1, dbt2,
		    dbt3, dbt4, dbt5) ? 0 : DbConstants.DB_BUFFER_SMALL;
	}

	public Comparator get_bt_compare() {
		return bt_compare_handler;
	}

	public BtreeCompressor get_bt_compress() {
		return bt_compress_handler;
	}

	public BtreeCompressor get_bt_decompress() {
		return bt_decompress_handler;
	}

	private final int handle_bt_prefix(DatabaseEntry dbt1,
					   DatabaseEntry dbt2) {
		return bt_prefix_handler.prefix(wrapper, dbt1, dbt2);
	}

	public BtreePrefixCalculator get_bt_prefix() {
		return bt_prefix_handler;
	}

	private final void handle_db_feedback(int opcode, int percent) {
		if (opcode == DbConstants.DB_UPGRADE)
			db_feedback_handler.upgradeFeedback(wrapper, percent);
		else if (opcode == DbConstants.DB_VERIFY)
			db_feedback_handler.upgradeFeedback(wrapper, percent);
		/* No other database feedback types known. */
	}

	public FeedbackHandler get_feedback() {
		return db_feedback_handler;
	}

	private final int handle_h_compare(byte[] arr1, byte[] arr2) {
		return h_compare_handler.compare(arr1, arr2);
	}

	public Comparator get_h_compare() {
		return h_compare_handler;
	}

	private final int handle_dup_compare(byte[] arr1, byte[] arr2) {
		return dup_compare_handler.compare(arr1, arr2);
	}

	public Comparator get_dup_compare() {
		return dup_compare_handler;
	}

	private final int handle_h_hash(byte[] data, int len) {
		return h_hash_handler.hash(wrapper, data, len);
	}

	public Hasher get_h_hash() {
		return h_hash_handler;
	}

	private final boolean handle_foreignkey_nullify(
					       DatabaseEntry key,	
					       DatabaseEntry data,	
					       DatabaseEntry seckey)
	    throws DatabaseException {
		if (foreignmultikey_nullify_handler != null)
			return foreignmultikey_nullify_handler.nullifyForeignKey(
			    (SecondaryDatabase)wrapper, key, data, seckey);
		else
			return foreignkey_nullify_handler.nullifyForeignKey(
			    (SecondaryDatabase)wrapper, data);
	}

	private final DatabaseEntry[] handle_seckey_create(
					       DatabaseEntry key,
					       DatabaseEntry data)
	    throws DatabaseException {

		if (secmultikey_create_handler != null) {
			java.util.HashSet keySet = new java.util.HashSet();
			secmultikey_create_handler.createSecondaryKeys(
			    (SecondaryDatabase)wrapper, key, data, keySet);
			if (!keySet.isEmpty())
				return (DatabaseEntry[])keySet.toArray(
				    new DatabaseEntry[keySet.size()]);
		} else {
			DatabaseEntry result = new DatabaseEntry();
			if (seckey_create_handler.createSecondaryKey(
			    (SecondaryDatabase)wrapper, key, data, result)) {
				DatabaseEntry[] results = { result };
				return results;
			}
		}

		return null;
	}

	public SecondaryKeyCreator get_seckey_create() {
		return seckey_create_handler;
	}

	public SecondaryMultiKeyCreator get_secmultikey_create() {
		return secmultikey_create_handler;
	}

	public void set_secmultikey_create(
	    SecondaryMultiKeyCreator secmultikey_create_handler) {
		this.secmultikey_create_handler = secmultikey_create_handler;
	}

	public void set_foreignmultikey_nullifier(ForeignMultiKeyNullifier nullify){
		this.foreignmultikey_nullify_handler = nullify;
	}

	private final int handle_partition(DatabaseEntry dbt1) {
		return partition_handler.partition(wrapper, dbt1);
	}

	public PartitionHandler get_partition_callback() {
		return partition_handler;
	}

	public synchronized void remove(String file, String database, int flags)
	    throws DatabaseException, java.io.FileNotFoundException {
		try {
			remove0(file, database, flags);
		} finally {
			cleanup();
		}
	}

	public synchronized void rename(String file, String database,
	    String newname, int flags)
	    throws DatabaseException, java.io.FileNotFoundException {
		try {
			rename0(file, database, newname, flags);
		} finally {
			cleanup();
		}
	}

	public synchronized boolean verify(String file, String database,
	    java.io.OutputStream outfile, int flags)
	    throws DatabaseException, java.io.FileNotFoundException {
		try {
			return verify0(file, database, outfile, flags);
		} finally {
			cleanup();
		}
	}

	public ErrorHandler get_errcall() {
		return dbenv.get_errcall();
	}

	public void set_errcall(ErrorHandler db_errcall_fcn) {
		dbenv.set_errcall(db_errcall_fcn);
	}

	public java.io.OutputStream get_error_stream() {
		return dbenv.get_error_stream();
	}

	public void set_error_stream(java.io.OutputStream stream) {
		dbenv.set_error_stream(stream);
	}

	public void set_errpfx(String errpfx) {
		dbenv.set_errpfx(errpfx);
	}

	public String get_errpfx() {
		return dbenv.get_errpfx();
	}

	public java.io.OutputStream get_message_stream() {
		return dbenv.get_message_stream();
	}

	public void set_message_stream(java.io.OutputStream stream) {
		dbenv.set_message_stream(stream);
	}

	public MessageHandler get_msgcall() {
		return dbenv.get_msgcall();
	}

	public void set_msgcall(MessageHandler db_msgcall_fcn) {
		dbenv.set_msgcall(db_msgcall_fcn);
	}

	public void set_paniccall(PanicHandler db_panic_fcn)
	    throws DatabaseException {
		dbenv.set_paniccall(db_panic_fcn);
	}

	public PanicHandler get_paniccall() {
		return dbenv.get_paniccall();
	}
%}

%typemap(javacode) struct Dbc %{
	public synchronized void close() throws DatabaseException {
		try {
			close0();
		} finally {
			swigCPtr = 0;
		}
	}
%}

%typemap(javacode) struct DbLock %{
	public Lock wrapper;
%}

%typemap(javacode) struct DbLogc %{
	public synchronized void close(int flags) throws DatabaseException {
		try {
			close0(flags);
		} finally {
			swigCPtr = 0;
		}
	}
%}

%typemap(javacode) struct DbSequence %{
	public Sequence wrapper;

	public synchronized void close(int flags) throws DatabaseException {
		try {
			close0(flags);
		} finally {
			swigCPtr = 0;
		}
	}

	public synchronized void remove(DbTxn txn, int flags)
	    throws DatabaseException {
		try {
			remove0(txn, flags);
		} finally {
			swigCPtr = 0;
		}
	}
%}

%typemap(javacode) struct DbTxn %{
	public void abort() throws DatabaseException {
		try {
			abort0();
		} finally {
			swigCPtr = 0;
		}
	}

	public void commit(int flags) throws DatabaseException {
		try {
			commit0(flags);
		} finally {
			swigCPtr = 0;
		}
	}

	public void discard(int flags) throws DatabaseException {
		try {
			discard0(flags);
		} finally {
			swigCPtr = 0;
		}
	}

	/*
	 * We override Object.equals because it is possible for the Java API to
	 * create multiple DbTxns that reference the same underlying object.
	 * This can happen for example during DbEnv.txn_recover().
	 */
	public boolean equals(Object obj)
	{
		if (this == obj)
			return true;

		if (obj != null && (obj instanceof DbTxn)) {
			DbTxn that = (DbTxn)obj;
			return (this.swigCPtr == that.swigCPtr);
		}
		return false;
	}

	/*
	 * We must override Object.hashCode whenever we override
	 * Object.equals() to enforce the maxim that equal objects have the
	 * same hashcode.
	 */
	public int hashCode()
	{
		return ((int)swigCPtr ^ (int)(swigCPtr >> 32));
	}
%}

%native(initDbEnvRef0) jlong initDbEnvRef0(DB_ENV *self, void *handle);
%native(initDbRef0) jlong initDbRef0(DB *self, void *handle);
%native(deleteRef0) void deleteRef0(jlong ref);
%native(getDbEnv0) DB_ENV *getDbEnv0(DB *self);

%{
SWIGEXPORT jlong JNICALL
Java_com_sleepycat_db_internal_db_1javaJNI_initDbEnvRef0(
    JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jobject jarg2) {
	DB_ENV *self = *(DB_ENV **)(void *)&jarg1;
	jlong ret;
	COMPQUIET(jcls, NULL);
	COMPQUIET(jarg1_, NULL);

	DB_ENV_INTERNAL(self) = (void *)(*jenv)->NewGlobalRef(jenv, jarg2);
	*(jobject *)(void *)&ret = (jobject)DB_ENV_INTERNAL(self);
	return (ret);
}

SWIGEXPORT jlong JNICALL
Java_com_sleepycat_db_internal_db_1javaJNI_initDbRef0(
    JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jobject jarg2) {
	DB *self = *(DB **)(void *)&jarg1;
	jlong ret;
	COMPQUIET(jcls, NULL);
	COMPQUIET(jarg1_, NULL);

	DB_INTERNAL(self) = (void *)(*jenv)->NewGlobalRef(jenv, jarg2);
	*(jobject *)(void *)&ret = (jobject)DB_INTERNAL(self);
	return (ret);
}

SWIGEXPORT void JNICALL
Java_com_sleepycat_db_internal_db_1javaJNI_deleteRef0(
    JNIEnv *jenv, jclass jcls, jlong jarg1) {
	jobject jref = *(jobject *)(void *)&jarg1;
	COMPQUIET(jcls, NULL);

	if (jref != 0L)
		(*jenv)->DeleteGlobalRef(jenv, jref);
}

SWIGEXPORT jlong JNICALL
Java_com_sleepycat_db_internal_db_1javaJNI_getDbEnv0(
    JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
	DB *self = *(DB **)(void *)&jarg1;
	jlong ret;

	COMPQUIET(jenv, NULL);
	COMPQUIET(jcls, NULL);
	COMPQUIET(jarg1_, NULL);

	*(DB_ENV **)(void *)&ret = self->dbenv;
	return (ret);
}

SWIGEXPORT jboolean JNICALL
Java_com_sleepycat_db_internal_DbUtil_is_1big_1endian(
    JNIEnv *jenv, jclass clazz)
{
	COMPQUIET(jenv, NULL);
	COMPQUIET(clazz, NULL);

	return (__db_isbigendian() ? JNI_TRUE : JNI_FALSE);
}
%}
