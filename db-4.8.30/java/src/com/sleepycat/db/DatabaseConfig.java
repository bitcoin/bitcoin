/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.Db;
import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;
import com.sleepycat.db.internal.DbTxn;
import com.sleepycat.db.internal.DbUtil;

/**
Specify the attributes of a database.
*/
public class DatabaseConfig implements Cloneable {
    /*
     * For internal use, final to allow null as a valid value for
     * the config parameter.
     */
    /**
    An instance created using the default constructor is initialized
    with the system's default settings.
    */
    public static final DatabaseConfig DEFAULT = new DatabaseConfig();

    /* package */
    static DatabaseConfig checkNull(DatabaseConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    /* Parameters */
    private DatabaseType type = DatabaseType.UNKNOWN;
    private int mode = 0644;
    private int btMinKey = 0;
    private int byteOrder = 0;
    private long cacheSize = 0L;
    private java.io.File createDir = null;
    private int cacheCount = 0;
    private java.io.OutputStream errorStream = null;
    private String errorPrefix = null;
    private int hashFillFactor = 0;
    private int hashNumElements = 0;
    private java.io.OutputStream messageStream = null;
    private int pageSize = 0;
    private java.io.File[] partitionDirs = null;
    private DatabaseEntry partitionKeys = null;
    private int partitionParts = 0;
    private String password = null;
    private CacheFilePriority priority = null;
    private int queueExtentSize = 0;
    private int recordDelimiter = 0;
    private int recordLength = 0;
    private int recordPad = -1;       // Zero is a valid, non-default value.
    private java.io.File recordSource = null;

    /* Flags */
    private boolean allowCreate = false;
    private boolean btreeRecordNumbers = false;
    private boolean checksum = false;
    private boolean readUncommitted = false;
    private boolean encrypted = false;
    private boolean exclusiveCreate = false;
    private boolean multiversion = false;
    private boolean noMMap = false;
    private boolean queueInOrder = false;
    private boolean readOnly = false;
    private boolean renumbering = false;
    private boolean reverseSplitOff = false;
    private boolean sortedDuplicates = false;
    private boolean snapshot = false;
    private boolean unsortedDuplicates = false;
    private boolean transactional = false;
    private boolean transactionNotDurable = false;
    private boolean truncate = false;

    /* Callbacks */
    private java.util.Comparator btreeComparator = null;
    private BtreeCompressor btreeCompressor = null;
    private BtreePrefixCalculator btreePrefixCalculator = null;
    private java.util.Comparator duplicateComparator = null;
    private FeedbackHandler feedbackHandler = null;
    private ErrorHandler errorHandler = null;
    private MessageHandler messageHandler = null;
    private PartitionHandler partitionHandler = null;
    private java.util.Comparator hashComparator = null;
    private Hasher hasher = null;
    private RecordNumberAppender recnoAppender = null;
    private PanicHandler panicHandler = null;

    /**
    An instance created using the default constructor is initialized with
    the system's default settings.
    */
    public DatabaseConfig() {
    }

    /**
     * Returns a copy of this configuration object.
     */
    public DatabaseConfig cloneConfig() {
        try {
            return (DatabaseConfig) super.clone();
        } catch (CloneNotSupportedException willNeverOccur) {
            return null;
        }
    }

    /**
    Configure the {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} method to create
    the database if it does not already exist.
    <p>
    @param allowCreate
    If true, configure the {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} method to
    create the database if it does not already exist.
    */
    public void setAllowCreate(final boolean allowCreate) {
        this.allowCreate = allowCreate;
    }

    /**
Return true if the {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} method is configured
    to create the database if it does not already exist.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} method is configured
    to create the database if it does not already exist.
    */
    public boolean getAllowCreate() {
        return allowCreate;
    }

    /**
    By default, a byte by byte lexicographic comparison is used for
    btree keys. To customize the comparison, supply a different
    Comparator.
    <p>
    The <code>compare</code> method is passed the byte arrays representing
    keys that are stored in the database. If you know how your data is
    organized in the byte array, then you can write a comparison routine that
    directly examines the contents of the arrays. Otherwise, you have to
    reconstruct your original objects, and then perform the comparison.
    */
    public void setBtreeComparator(final java.util.Comparator btreeComparator) {
        this.btreeComparator = btreeComparator;
    }

    public java.util.Comparator getBtreeComparator() {
        return btreeComparator;
    }

    /**
    Set the minimum number of key/data pairs intended to be stored on any
    single Btree leaf page.
    <p>
    This value is used to determine if key or data items will be stored
    on overflow pages instead of Btree leaf pages.  The value must be
    at least 2; if the value is not explicitly set, a value of 2 is used.
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    <p>
    @param btMinKey
    The minimum number of key/data pairs intended to be stored on any
    single Btree leaf page.
    */
    public void setBtreeMinKey(final int btMinKey) {
        this.btMinKey = btMinKey;
    }

    /**
Return the minimum number of key/data pairs intended to be stored
    on any single Btree leaf page.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The minimum number of key/data pairs intended to be stored
    on any single Btree leaf page.
    */
    public int getBtreeMinKey() {
        return btMinKey;
    }

    /**
    Set the byte order for integers in the stored database metadata.
    <p>
    The host byte order of the machine where the process is running will
    be used if no byte order is set.
    <p>
    <b>
    The access methods provide no guarantees about the byte ordering of the
    application data stored in the database, and applications are
    responsible for maintaining any necessary ordering.
    </b>
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    If creating additional databases in a single physical file, information
    specified to this method will be ignored and the byte order of the
    existing databases will be used.
    <p>
    @param byteOrder
    The byte order as an integer; for example, big endian order is the
    number 4,321, and little endian order is the number 1,234.
    */
    public void setByteOrder(final int byteOrder) {
        this.byteOrder = byteOrder;
    }

    /**
Return the database byte order; a byte order of 4,321 indicates a
    big endian order, and a byte order of 1,234 indicates a little
    endian order.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The database byte order; a byte order of 4,321 indicates a
    big endian order, and a byte order of 1,234 indicates a little
    endian order.
    */
    public int getByteOrder() {
        return byteOrder;
    }

    /**
    Return if the underlying database files were created on an architecture
    of the same byte order as the current one.
    <p>
    This information may be used to determine whether application data
    needs to be adjusted for this architecture or not.
    <p>
    This method may not be called before the
database has been opened.
    <p>
    @return
    Return false if the underlying database files were created on an
    architecture of the same byte order as the current one, and true if
    they were not (that is, big-endian on a little-endian machine, or
    vice versa).
    */
    public boolean getByteSwapped() {
        return byteOrder != 0 && byteOrder != DbUtil.default_lorder();
    }

    /**
    Set the Btree compression callbacks.
    */
    public void setBtreeCompressor(final BtreeCompressor btreeCompressor) {
    	this.btreeCompressor = btreeCompressor;
    }

    /**
    Get the Btree compression callbacks.
    */
    public BtreeCompressor getBtreeCompressor() {
        return btreeCompressor;
    }

    /**
    Set the Btree prefix callback.  The prefix callback is used to determine
    the amount by which keys stored on the Btree internal pages can be
    safely truncated without losing their uniqueness.  See the
    <a href="{@docRoot}/../programmer_reference/bt_conf.html#am_conf_bt_prefix" target="_top">Btree prefix
    comparison</a> section of the Berkeley DB Reference Guide for more
    details about how this works.  The usefulness of this is data-dependent,
    but can produce significantly reduced tree sizes and search times in
    some data sets.
    <p>
    If no prefix callback or key comparison callback is specified by the
    application, a default lexical comparison function is used to calculate
    prefixes.  If no prefix callback is specified and a key comparison
    callback is specified, no prefix function is used.  It is an error to
    specify a prefix function without also specifying a Btree key comparison
    function.
    */
    public void setBtreePrefixCalculator(
            final BtreePrefixCalculator btreePrefixCalculator) {
        this.btreePrefixCalculator = btreePrefixCalculator;
    }

    /**
Return the Btree prefix callback.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The Btree prefix callback.
    */
    public BtreePrefixCalculator getBtreePrefixCalculator() {
        return btreePrefixCalculator;
    }

    /**
    Set the size of the shared memory buffer pool, that is, the size of the
cache.
<p>
The cache should be the size of the normal working data set of the
application, with some small amount of additional memory for unusual
situations.  (Note: the working set is not the same as the number of
pages accessed simultaneously, and is usually much larger.)
<p>
The default cache size is 256KB, and may not be specified as less than
20KB.  Any cache size less than 500MB is automatically increased by 25%
to account for buffer pool overhead; cache sizes larger than 500MB are
used as specified.  The current maximum size of a single cache is 4GB.
(All sizes are in powers-of-two, that is, 256KB is 2^18 not 256,000.)
<p>
Because databases opened within database environments use the cache
specified to the environment, it is an error to attempt to configure a
cache size in a database created within an environment.
<p>
This method may not be called after the database is opened.
<p>
This method may be called at any time during the life of the application.
<p>
@param cacheSize
The size of the shared memory buffer pool, that is, the size of the
cache.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setCacheSize(final long cacheSize) {
        this.cacheSize = cacheSize;
    }

    /**
Return the size of the shared memory buffer pool, that is, the cache.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The size of the shared memory buffer pool, that is, the cache.
    */
    public long getCacheSize() {
        return cacheSize;
    }

    /**
Specify which directory a database should be created in or looked for.
<p>
@param createDir
The directory will be used to create or locate the database file specified in
the openDatabase method call. The directory must be one of the directories
in the environment list specified by EnvironmentConfig.addDataDirectory.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setCreateDir(final java.io.File createDir) {
        this.createDir = createDir;
    }

    /**
Return the directory a database will/has been created in or looked for.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The directory a database will/has been created in or looked for.
    */
    public java.io.File getCreateDir() {
        return this.createDir;
    }

    /**
    Set the number of shared memory buffer pools, that is, the number of
caches.
<p>
It is possible to specify caches larger than 4GB and/or large enough
they cannot be allocated contiguously on some architectures.  For
example, some releases of Solaris limit the amount of memory that may
be allocated contiguously by a process.  This method allows applications
to break the cache broken up into a number of  equally sized, separate
pieces of memory.
<p>
<p>
Because databases opened within database environments use the cache
specified to the environment, it is an error to attempt to configure
multiple caches in a database created within an environment.
<p>
This method may not be called after the database is opened.
<p>
This method may be called at any time during the life of the application.
<p>
@param cacheCount
The number of shared memory buffer pools, that is, the number of caches.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setCacheCount(final int cacheCount) {
        this.cacheCount = cacheCount;
    }

    /**
Return the number of shared memory buffer pools, that is, the number
    of caches.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The number of shared memory buffer pools, that is, the number
    of caches.
    */
    public int getCacheCount() {
        return cacheCount;
    }

    /**
    Configure the database environment to do checksum verification of
    pages read into the cache from the backing filestore.
    <p>
    Berkeley DB uses the SHA1 Secure Hash Algorithm if encryption is
    also configured for this database, and a general hash algorithm if
    it is not.
    <p>
    Calling this method only affects the specified {@link com.sleepycat.db.Database Database} handle
(and any other library handles opened within the scope of that handle).
    <p>
    If the database already exists when the database is opened, any database
configuration specified by this method
will be ignored.
    If creating additional databases in a file, the checksum behavior
    specified must be consistent with the existing databases in the file or
    an error will be returned.
    <p>
    @param checksum
    If true, configure the database environment to do checksum verification
    of pages read into the cache from the backing filestore.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setChecksum(final boolean checksum) {
        this.checksum = checksum;
    }

    /**
Return true if the database environment is configured to do checksum
    verification of pages read into the cache from the backing
    filestore.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to do checksum
    verification of pages read into the cache from the backing
    filestore.
    */
    public boolean getChecksum() {
        return checksum;
    }

    /**
        Configure the database to support read uncommitted.
    <p>
    Read operations on the database may request the return of modified
    but not yet committed data.  This flag must be specified on all
    {@link com.sleepycat.db.Database Database} handles used to perform read uncommitted or database
    updates, otherwise requests for read uncommitted may not be honored and
    the read may block.
    <p>
    @param readUncommitted
    If true, configure the database to support read uncommitted.
    */
    public void setReadUncommitted(final boolean readUncommitted) {
        this.readUncommitted = readUncommitted;
    }

    /**
Return true if the database is configured to support read uncommitted.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database is configured to support read uncommitted.
    */
    public boolean getReadUncommitted() {
        return readUncommitted;
    }

    /**
        Configure the database to support read uncommitted.
    <p>
    Read operations on the database may request the return of modified
    but not yet committed data.  This flag must be specified on all
    {@link com.sleepycat.db.Database Database} handles used to perform read uncommitted or database
    updates, otherwise requests for read uncommitted may not be honored and
    the read may block.
    <p>
    @param dirtyRead
    If true, configure the database to support read uncommitted.
        <p>
    @deprecated This has been replaced by {@link #setReadUncommitted} to conform to ANSI
    database isolation terminology.
    */
    public void setDirtyRead(final boolean dirtyRead) {
        setReadUncommitted(dirtyRead);
    }

    /**
Return true if the database is configured to support read uncommitted.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database is configured to support read uncommitted.
        <p>
    @deprecated This has been replaced by {@link #getReadUncommitted} to conform to ANSI
    database isolation terminology.
    */
    public boolean getDirtyRead() {
        return getReadUncommitted();
    }

    /**
    Set the duplicate data item comparison callback.  The comparison
    function is called whenever it is necessary to compare a data item
    specified by the application with a data item currently stored in the
    database.  This comparator is only used if
    {@link com.sleepycat.db.DatabaseConfig#setSortedDuplicates DatabaseConfig.setSortedDuplicates} is also configured.
    <p>
    If no comparison function is specified, the data items are compared
    lexically, with shorter data items collating before longer data items.
    <p>
    The <code>compare</code> method is passed the byte arrays representing
    data items in the database. If you know how your data is organized in
    the byte array, then you can write a comparison routine that directly
    examines the contents of the arrays.  Otherwise, you have to
    reconstruct your original objects, and then perform the comparison.
    <p>
    @param duplicateComparator
    the comparison callback for duplicate data items.
    */
    public void setDuplicateComparator(
            final java.util.Comparator duplicateComparator) {
        this.duplicateComparator = duplicateComparator;
    }

    public java.util.Comparator getDuplicateComparator() {
        return duplicateComparator;
    }

    /**
    Set the password used to perform encryption and decryption.
    <p>
    Because databases opened within environments use the password
    specified to the environment, it is an error to attempt to set a
    password in a database created within an environment.
    <p>
    Berkeley DB uses the Rijndael/AES (also known as the Advanced
    Encryption Standard and Federal Information Processing
    Standard (FIPS) 197) algorithm for encryption or decryption.
    */
    public void setEncrypted(final String password) {
        this.password = password;
    }

    /**
Return true if the database has been configured to perform encryption.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database has been configured to perform encryption.
    */
    public boolean getEncrypted() {
        return (password != null);
    }

    /**
    Set the function to be called if an error occurs.
<p>
When an error occurs in the Berkeley DB library, an exception is thrown.
In some cases, however, the error information returned to the
application may be insufficient to completely describe the cause of the
error, especially during initial application debugging.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setErrorHandler EnvironmentConfig.setErrorHandler} and {@link com.sleepycat.db.DatabaseConfig#setErrorHandler DatabaseConfig.setErrorHandler} methods are used to enhance the mechanism for reporting
error messages to the application.  In some cases, when an error occurs,
Berkeley DB will invoke the ErrorHandler's object error method.  It is
up to this method to display the error message in an appropriate manner.
<p>
Alternatively, applications can use {@link com.sleepycat.db.EnvironmentConfig#setErrorStream EnvironmentConfig.setErrorStream} and {@link com.sleepycat.db.DatabaseConfig#setErrorStream DatabaseConfig.setErrorStream} to
display the additional information via an output stream.  Applications
should not mix these approaches.
<p>
This error-logging enhancement does not slow performance or significantly
increase application size, and may be run during normal operation as well
as during application debugging.
<p>
For {@link com.sleepycat.db.Database Database} handles opened inside of database environments,
calling this method affects the entire environment and is equivalent to
calling {@link com.sleepycat.db.EnvironmentConfig#setErrorHandler EnvironmentConfig.setErrorHandler}.
<p>
This method may be called at any time during the life of the application.
<p>
@param errorHandler
The function to be called if an error occurs.
    */
    public void setErrorHandler(final ErrorHandler errorHandler) {
        this.errorHandler = errorHandler;
    }

    /**
Return the function to be called if an error occurs.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The function to be called if an error occurs.
    */
    public ErrorHandler getErrorHandler() {
        return errorHandler;
    }

    /**
    Set the prefix string that appears before error messages.
<p>
For {@link com.sleepycat.db.Database Database} handles opened inside of database environments,
calling this method affects the entire environment and is equivalent to
calling {@link com.sleepycat.db.EnvironmentConfig#setErrorPrefix EnvironmentConfig.setErrorPrefix}.
<p>
This method may be called at any time during the life of the application.
<p>
@param errorPrefix
The prefix string that appears before error messages.
    */
    public void setErrorPrefix(final String errorPrefix) {
        this.errorPrefix = errorPrefix;
    }

    /**
Return the prefix string that appears before error messages.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The prefix string that appears before error messages.
    */
    public String getErrorPrefix() {
        return errorPrefix;
    }

    /**
    Set an OutputStream for displaying error messages.
<p>
When an error occurs in the Berkeley DB library, an exception is thrown.
In some cases, however, the error information returned to the
application may be insufficient to completely describe the cause of the
error, especially during initial application debugging.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setErrorStream EnvironmentConfig.setErrorStream} and
{@link com.sleepycat.db.DatabaseConfig#setErrorStream DatabaseConfig.setErrorStream} methods are used to enhance
the mechanism for reporting error messages to the application by setting
a OutputStream to be used for displaying additional Berkeley DB error
messages.  In some cases, when an error occurs, Berkeley DB will output
an additional error message to the specified stream.
<p>
The error message will consist of the prefix string and a colon
("<b>:</b>") (if a prefix string was previously specified using
{@link com.sleepycat.db.EnvironmentConfig#setErrorPrefix EnvironmentConfig.setErrorPrefix} or {@link com.sleepycat.db.DatabaseConfig#setErrorPrefix DatabaseConfig.setErrorPrefix}), an error string, and a trailing newline character.
<p>
Setting errorStream to null unconfigures the interface.
<p>
Alternatively, applications can use {@link com.sleepycat.db.EnvironmentConfig#setErrorHandler EnvironmentConfig.setErrorHandler} and {@link com.sleepycat.db.DatabaseConfig#setErrorHandler DatabaseConfig.setErrorHandler} to capture
the additional error information in a way that does not use output
streams.  Applications should not mix these approaches.
<p>
This error-logging enhancement does not slow performance or significantly
increase application size, and may be run during normal operation as well
as during application debugging.
<p>
This method may be called at any time during the life of the application.
<p>
@param errorStream
The application-specified OutputStream for error messages.
    */
    public void setErrorStream(final java.io.OutputStream errorStream) {
        this.errorStream = errorStream;
    }

    /**
Return the an OutputStream for displaying error messages.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The an OutputStream for displaying error messages.
    */
    public java.io.OutputStream getErrorStream() {
        return errorStream;
    }

    /**
    Configure the {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} method to fail if
    the database already exists.
    <p>
    The exclusiveCreate mode is only meaningful if specified with the
    allowCreate mode.
    <p>
    @param exclusiveCreate
    If true, configure the {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} method to
    fail if the database already exists.
    */
    public void setExclusiveCreate(final boolean exclusiveCreate) {
        this.exclusiveCreate = exclusiveCreate;
    }

    /**
Return true if the {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} method is configured
    to fail if the database already exists.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} method is configured
    to fail if the database already exists.
    */
    public boolean getExclusiveCreate() {
        return exclusiveCreate;
    }

    /**
    Set an object whose methods are called to provide feedback.
<p>
Some operations performed by the Berkeley DB library can take
non-trivial amounts of time.  This method can be used by applications
to monitor progress within these operations.  When an operation is
likely to take a long time, Berkeley DB will call the object's methods
with progress information.
<p>
It is up to the object's methods to display this information in an
appropriate manner.
<p>
This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle
<p>
This method may be called at any time during the life of the application.
<p>
@param feedbackHandler
An object whose methods are called to provide feedback.
    */
    public void setFeedbackHandler(final FeedbackHandler feedbackHandler) {
        this.feedbackHandler = feedbackHandler;
    }

    /**
Return the object's methods to be called to provide feedback.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The object's methods to be called to provide feedback.
    */
    public FeedbackHandler getFeedbackHandler() {
        return feedbackHandler;
    }

    /**
    Set the desired density within the hash table.
    <p>
    If no value is specified, the fill factor will be selected dynamically
    as pages are filled.
    <p>
    The density is an approximation of the number of keys allowed to
    accumulate in any one bucket, determining when the hash table grows or
    shrinks.  If you know the average sizes of the keys and data in your
    data set, setting the fill factor can enhance performance.  A reasonable
    rule computing fill factor is to set it to the following:
    <blockquote><pre>
        (pagesize - 32) / (average_key_size + average_data_size + 8)
    </pre></blockquote>
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    <p>
    @param hashFillFactor
    The desired density within the hash table.
    */
    public void setHashFillFactor(final int hashFillFactor) {
        this.hashFillFactor = hashFillFactor;
    }

    /**
Return the hash table density.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The hash table density.
    */
    public int getHashFillFactor() {
        return hashFillFactor;
    }

    /**
    Set the Hash key comparison function. The comparison function is called
    whenever it is necessary to compare a key specified by the application with
    a key currently stored in the database.
    <p>
    If no comparison function is specified, a byte-by-byte comparison is
    performed.
    <p>
    The <code>compare</code> method is passed the byte arrays representing
    keys that are stored in the database. If you know how your data is
    organized in the byte array, then you can write a comparison routine that
    directly examines the contents of the arrays. Otherwise, you have to
    reconstruct your original objects, and then perform the comparison.
    */
    public void setHashComparator(final java.util.Comparator hashComparator) {
        this.hashComparator = hashComparator;
    }

    /**
Return the Comparator used to compare keys in a Hash database.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The Comparator used to compare keys in a Hash database.
    */
    public java.util.Comparator getHashComparator() {
        return hashComparator;
    }

    /**
    Set a database-specific hash function.
    <p>
    If no hash function is specified, a default hash function is used.
    Because no hash function performs equally well on all possible data,
    the user may find that the built-in hash function performs poorly
    with a particular data set.
    <p>
    This method configures operations performed using the specified
{@link com.sleepycat.db.Database Database} object, not all operations performed on the underlying
database.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method must be the same as that
historically used to create the database or corruption can occur.
    <p>
    @param hasher
    A database-specific hash function.
    */
    public void setHasher(final Hasher hasher) {
        this.hasher = hasher;
    }

    /**
Return the database-specific hash function.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The database-specific hash function.
    */
    public Hasher getHasher() {
        return hasher;
    }

    /**
    Set an estimate of the final size of the hash table.
    <p>
    In order for the estimate to be used when creating the database, the
    {@link com.sleepycat.db.DatabaseConfig#setHashFillFactor DatabaseConfig.setHashFillFactor} method must also be called.
    If the estimate or fill factor are not set or are set too low, hash
    tables will still expand gracefully as keys are entered, although a
    slight performance degradation may be noticed.
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    <p>
    @param hashNumElements
    An estimate of the final size of the hash table.
    */
    public void setHashNumElements(final int hashNumElements) {
        this.hashNumElements = hashNumElements;
    }

    /**
Return the estimate of the final size of the hash table.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The estimate of the final size of the hash table.
    */
    public int getHashNumElements() {
        return hashNumElements;
    }

    /**
    Set a function to be called with an informational message.
<p>
There are interfaces in the Berkeley DB library which either directly
output informational messages or statistical information, or configure
the library to output such messages when performing other operations,
{@link com.sleepycat.db.EnvironmentConfig#setVerboseDeadlock EnvironmentConfig.setVerboseDeadlock} for example.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setMessageHandler EnvironmentConfig.setMessageHandler} and
{@link com.sleepycat.db.DatabaseConfig#setMessageHandler DatabaseConfig.setMessageHandler} methods are used to display
these messages for the application.
<p>
Setting messageHandler to null unconfigures the interface.
<p>
Alternatively, you can use {@link com.sleepycat.db.EnvironmentConfig#setMessageStream EnvironmentConfig.setMessageStream}
and {@link com.sleepycat.db.DatabaseConfig#setMessageStream DatabaseConfig.setMessageStream} to send the additional
information directly to an output streams.  You should not mix these
approaches.
<p>
For {@link com.sleepycat.db.Database Database} handles opened inside of database environments,
calling this method affects the entire environment and is equivalent to
calling {@link com.sleepycat.db.EnvironmentConfig#setMessageHandler EnvironmentConfig.setMessageHandler}.
<p>
This method may be called at any time during the life of the application.
<p>
@param messageHandler
The application-specified function for informational messages.
    */
    public void setMessageHandler(final MessageHandler messageHandler) {
        this.messageHandler = messageHandler;
    }

    /**
Return the function to be called with an informational message.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The function to be called with an informational message.
    */
    public MessageHandler getMessageHandler() {
        return messageHandler;
    }

    /**
    Set an OutputStream for displaying informational messages.
<p>
There are interfaces in the Berkeley DB library which either directly
output informational messages or statistical information, or configure
the library to output such messages when performing other operations,
{@link com.sleepycat.db.EnvironmentConfig#setVerboseDeadlock EnvironmentConfig.setVerboseDeadlock} for example.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setMessageStream EnvironmentConfig.setMessageStream} and
{@link com.sleepycat.db.DatabaseConfig#setMessageStream DatabaseConfig.setMessageStream} methods are used to display
these messages for the application.  In this case, the message will
include a trailing newline character.
<p>
Setting messageStream to null unconfigures the interface.
<p>
Alternatively, you can use {@link com.sleepycat.db.EnvironmentConfig#setMessageHandler EnvironmentConfig.setMessageHandler}
and {@link com.sleepycat.db.DatabaseConfig#setMessageHandler DatabaseConfig.setMessageHandler} to capture the additional
information in a way that does not use output streams.  You should not
mix these approaches.
<p>
For {@link com.sleepycat.db.Database Database} handles opened inside of database environments,
calling this method affects the entire environment and is equivalent to
calling {@link com.sleepycat.db.EnvironmentConfig#setMessageStream EnvironmentConfig.setMessageStream}.
<p>
This method may be called at any time during the life of the application.
<p>
@param messageStream
The application-specified OutputStream for informational messages.
    */
    public void setMessageStream(final java.io.OutputStream messageStream) {
        this.messageStream = messageStream;
    }

    /**
Return the an OutputStream for displaying informational messages.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The an OutputStream for displaying informational messages.
    */
    public java.io.OutputStream getMessageStream() {
        return messageStream;
    }

    /**
    On UNIX systems or in IEEE/ANSI Std 1003.1 (POSIX) environments, files
    created by the database open are created with mode <code>mode</code>
    (as described in the <code>chmod</code>(2) manual page) and modified
    by the process' umask value at the time of creation (see the
    <code>umask</code>(2) manual page).  Created files are owned by the
    process owner; the group ownership of created files is based on the
    system and directory defaults, and is not further specified by Berkeley
    DB.  System shared memory segments created by the database open are
    created with mode <code>mode</code>, unmodified by the process' umask
    value.  If <code>mode</code> is 0, the database open will use a default
    mode of readable and writable by both owner and group.
    <p>
    On Windows systems, the mode parameter is ignored.
    <p>
    @param mode
    the mode used to create files
    */
    public void setMode(final int mode) {
        this.mode = mode;
    }

    /**
Return the mode used to create files.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The mode used to create files.
    */
    public long getMode() {
        return mode;
    }

    /**
    Configured the database with support for multiversion concurrency control.
    This will cause updates to the database to follow a copy-on-write
    protocol, which is required to support Snapshot Isolation.  See
    {@link TransactionConfig#setSnapshot}) for more information.
    Multiversion access requires that the database be opened
    in a transaction and is not supported for queue databases.
    <p>
    @param multiversion
    If true, configure the database with support for multiversion concurrency
    control.
    */
    public void setMultiversion(final boolean multiversion) {
        this.multiversion = multiversion;
    }

    /**
Return true if the database is configured for multiversion concurrency control.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database is configured for multiversion concurrency control.
    */
    public boolean getMultiversion() {
        return multiversion;
    }

    /**
    Configure the library to not map this database into memory.
    <p>
    @param noMMap
    If true, configure the library to not map this database into memory.
    */
    public void setNoMMap(final boolean noMMap) {
        this.noMMap = noMMap;
    }

    /**
Return true if the library is configured to not map this database into
    memory.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the library is configured to not map this database into
    memory.
    */
    public boolean getNoMMap() {
        return noMMap;
    }

    /**
    Set the size of the pages used to hold items in the database, in bytes.
    <p>
    The minimum page size is 512 bytes, the maximum page size is 64K bytes,
    and the page size must be a power-of-two.  If the page size is not
    explicitly set, one is selected based on the underlying filesystem I/O
    block size.  The automatically selected size has a lower limit of 512
    bytes and an upper limit of 16K bytes.
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    If creating additional databases in a file, the page size specified must
    be consistent with the existing databases in the file or an error will
    be returned.
    <p>
    @param pageSize
    The size of the pages used to hold items in the database, in bytes.
    */
    public void setPageSize(final int pageSize) {
        this.pageSize = pageSize;
    }

    /**
Return the size of the pages used to hold items in the database, in bytes.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The size of the pages used to hold items in the database, in bytes.
    */
    public int getPageSize() {
        return pageSize;
    }

    /**
    Set the function to be called if the database environment panics.
<p>
Errors can occur in the Berkeley DB library where the only solution is
to shut down the application and run recovery (for example, if Berkeley
DB is unable to allocate heap memory).  In such cases, the Berkeley DB
methods will throw a {@link com.sleepycat.db.RunRecoveryException RunRecoveryException}.  It is often easier
to simply exit the application when such errors occur rather than
gracefully return up the stack.  This method specifies a function to be
called when {@link com.sleepycat.db.RunRecoveryException RunRecoveryException} is about to be thrown from a
Berkeley DB method.
<p>
For {@link com.sleepycat.db.Database Database} handles opened inside of database environments,
calling this method affects the entire environment and is equivalent to
calling {@link com.sleepycat.db.EnvironmentConfig#setPanicHandler EnvironmentConfig.setPanicHandler}.
<p>
This method may be called at any time during the life of the application.
<p>
@param panicHandler
The function to be called if the database environment panics.
    */
    public void setPanicHandler(final PanicHandler panicHandler) {
        this.panicHandler = panicHandler;
    }

    /**
Return the function to be called if the database environment panics.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The function to be called if the database environment panics.
    */
    public PanicHandler getPanicHandler() {
        return panicHandler;
    }

    /**
    Enable or disable database partitioning, and set the callback that will
be used for the partitioning.
<p>
This method may only be called before opening a database.
<p>
@param parts
The number of partitions that will be created.
<p>
@param partitionHandler
The function to be called to determine which partition a key resides in.
    */
    public void setPartitionByCallback(int parts, 
        final PartitionHandler partitionHandler) {
        this.partitionParts = parts;
        this.partitionHandler = partitionHandler;
    }

    /**
    Enable or disable database partitioning, and set key ranges that will be
    used for the partitioning.
<p>
This method may only be called before opening a database.
<p>
@param parts
The number of partitions that will be created.
<p>
@param keys
A MultipleDatabaseEntry that contains the boundary keys for partitioning.
    */
    public void setPartitionByRange(int parts, MultipleDataEntry keys) {
        this.partitionParts = parts;
        this.partitionKeys = keys;
    }

    /**
Return the function to be called to determine which partition a key resides in.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The function to be called to determine which partition a key resides in.
    */
    public PartitionHandler getPartitionHandler() {
        return partitionHandler;
    }

    /**
Return the number of partitions the database is configured for.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The number of partitions the database is configured for.
    */
    public int getPartitionParts() {
        return partitionParts;
    }

    /**
Return the array of keys the database is configured to partition with.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The array of keys the database is configured to partition with.
    */
    public DatabaseEntry getPartitionKeys() {
        return partitionKeys;
    }

    /**
Specify the array of directories the database extents should be created in or
looked for. If the number of directories is less than the number of partitions,
the directories will be used in a round robin fasion.
<p>
This method may only be called before the database is opened.
<p>
@param dirs
The array of directories the database extents should be created in or looked
for.
    */
    public void setPartitionDirs(final java.io.File[] dirs) {
    	partitionDirs = dirs;
    }

    /**
Return the array of directories the database extents should be created in or
looked for. If the number of directories is less than the number of partitions,
the directories will be used in a round robin fasion.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The array of directories the database extents should be created in or looked
for.
    */
    public java.io.File[] getPartitionDirs() {
    	return partitionDirs;
    }

    /**
    Set the cache priority for pages referenced by the DB handle.
    <p>
    The priority of a page biases the replacement algorithm to be more or less
    likely to discard a page when space is needed in the buffer pool. The bias
    is temporary, and pages will eventually be discarded if they are not
    referenced again. The priority setting is only advisory, and does not
    guarantee pages will be treated in a specific way.
    <p>
    @param priority
    The desired cache priority.
    */
    public void setPriority(final CacheFilePriority priority) {
        this.priority = priority;
    }

    /**
Return the the cache priority for pages referenced by this handle.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The the cache priority for pages referenced by this handle.
    */
    public CacheFilePriority getPriority() {
        return priority;
    }

    /**
    Set the size of the extents used to hold pages in a Queue database,
    specified as a number of pages.
    <p>
    Each extent is created as a separate physical file.  If no extent
    size is set, the default behavior is to create only a single
    underlying database file.
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    <p>
    @param queueExtentSize
    The number of pages in a Queue database extent.
    */
    public void setQueueExtentSize(final int queueExtentSize) {
        this.queueExtentSize = queueExtentSize;
    }

    /**
Return the size of the extents used to hold pages in a Queue database,
    specified as a number of pages.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The size of the extents used to hold pages in a Queue database,
    specified as a number of pages.
    */
    public int getQueueExtentSize() {
        return queueExtentSize;
    }

    /**
    Configure {@link com.sleepycat.db.Database#consume Database.consume} to return key/data pairs in
    order, always returning the key/data item from the head of the
    queue.
    <p>
    The default behavior of queue databases is optimized for multiple
    readers, and does not guarantee that record will be retrieved in the
    order they are added to the queue.  Specifically, if a writing
    thread adds multiple records to an empty queue, reading threads may
    skip some of the initial records when the next call to retrieve a
    key/data pair returns.
    <p>
    This flag configures the {@link com.sleepycat.db.Database#consume Database.consume} method to verify
    that the record being returned is in fact the head of the queue.
    This will increase contention and reduce concurrency when there are
    many reading threads.
    <p>
    Calling this method only affects the specified {@link com.sleepycat.db.Database Database} handle
(and any other library handles opened within the scope of that handle).
    <p>
    @param queueInOrder
    If true, configure the {@link com.sleepycat.db.Database#consume Database.consume} method to return
    key/data pairs in order, always returning the key/data item from the
    head of the queue.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setQueueInOrder(final boolean queueInOrder) {
        this.queueInOrder = queueInOrder;
    }

    /**
Return true if the {@link com.sleepycat.db.Database#consume Database.consume} method is configured to return
    key/data pairs in order, always returning the key/data item from the
    head of the queue.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the {@link com.sleepycat.db.Database#consume Database.consume} method is configured to return
    key/data pairs in order, always returning the key/data item from the
    head of the queue.
    */
    public boolean getQueueInOrder() {
        return queueInOrder;
    }

    /**
    Configure the database in read-only mode.
    <p>
    Any attempt to modify items in the database will fail, regardless
    of the actual permissions of any underlying files.
    <p>
    @param readOnly
    If true, configure the database in read-only mode.
    */
    public void setReadOnly(final boolean readOnly) {
        this.readOnly = readOnly;
    }

    /**
Return true if the database is configured in read-only mode.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database is configured in read-only mode.
    */
    public boolean getReadOnly() {
        return readOnly;
    }

    /**
    Configure {@link com.sleepycat.db.Database#append Database.append} to call the function after the
    record number has been selected but before the data has been stored
    into the database.
    <p>
    This method configures operations performed using the specified
{@link com.sleepycat.db.Database Database} object, not all operations performed on the underlying
database.
    <p>
    This method may not be called after the database is opened.
    <p>
    @param recnoAppender
    The function to call after the record number has been selected but
    before the data has been stored into the database.
    */
    public void setRecordNumberAppender(
            final RecordNumberAppender recnoAppender) {
        this.recnoAppender = recnoAppender;
    }

    /**
Return the function to call after the record number has been
    selected but before the data has been stored into the database.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The function to call after the record number has been
    selected but before the data has been stored into the database.
    */
    public RecordNumberAppender getRecordNumberAppender() {
        return recnoAppender;
    }

    /**
    Set the delimiting byte used to mark the end of a record in the backing
    source file for the Recno access method.
    <p>
    This byte is used for variable length records if a backing source
    file is specified.  If a backing source file is specified and no
    delimiting byte was specified, newline characters (that is, ASCII
    0x0a) are interpreted as end-of-record markers.
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    <p>
    @param recordDelimiter
    The delimiting byte used to mark the end of a record in the backing
    source file for the Recno access method.
    */
    public void setRecordDelimiter(final int recordDelimiter) {
        this.recordDelimiter = recordDelimiter;
    }

    /**
Return the delimiting byte used to mark the end of a record in the
    backing source file for the Recno access method.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The delimiting byte used to mark the end of a record in the
    backing source file for the Recno access method.
    */
    public int getRecordDelimiter() {
        return recordDelimiter;
    }

    /**
    Specify the database record length, in bytes.
    <p>
    For the Queue access method, specify the record length.  For the
    Queue access method, the record length must be enough smaller than
    the database's page size that at least one record plus the database
    page's metadata information can fit on each database page.
    <p>
    For the Recno access method, specify the records are fixed-length,
    not byte-delimited, and the record length.
    <p>
    Any records added to the database that are less than the specified
    length are automatically padded (see
    {@link com.sleepycat.db.DatabaseConfig#setRecordPad DatabaseConfig.setRecordPad} for more information).
    <p>
    Any attempt to insert records into the database that are greater
    than the specified length will cause the call to fail.
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    <p>
    @param recordLength
    The database record length, in bytes.
    */
    public void setRecordLength(final int recordLength) {
        this.recordLength = recordLength;
    }

    /**
Return the database record length, in bytes.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The database record length, in bytes.
    */
    public int getRecordLength() {
        return recordLength;
    }

    /**
    Configure the Btree to support retrieval by record number.
    <p>
    Logical record numbers in Btree databases are mutable in the face of
    record insertion or deletion.
    <p>
    Maintaining record counts within a Btree introduces a serious point
    of contention, namely the page locations where the record counts are
    stored.  In addition, the entire database must be locked during both
    insertions and deletions, effectively single-threading the database
    for those operations.  Configuring a Btree for retrieval by record
    number can result in serious performance degradation for some
    applications and data sets.
    <p>
    Retrieval by record number may not be configured for a Btree that also
    supports duplicate data items.
    <p>
    Calling this method affects the database, including all threads of
control accessing the database.
    <p>
    If the database already exists when the database is opened, any database
configuration specified by this method
must be the same as the existing database or an error
will be returned.
    <p>
    @param btreeRecordNumbers
    If true, configure the Btree to support retrieval by record number.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setBtreeRecordNumbers(final boolean btreeRecordNumbers) {
        this.btreeRecordNumbers = btreeRecordNumbers;
    }

    /**
Return true if the Btree is configured to support retrieval by record number.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the Btree is configured to support retrieval by record number.
    */
    public boolean getBtreeRecordNumbers() {
        return btreeRecordNumbers;
    }

    /**
    Set the padding character for short, fixed-length records for the Queue
    and Recno access methods.
    <p>
    If no pad character is specified, "space" characters (that is, ASCII
    0x20) are used for padding.
    <p>
    This method configures a database, not only operations performed using
the specified {@link com.sleepycat.db.Database Database} handle.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method will be ignored.
    <p>
    @param recordPad
    The padding character for short, fixed-length records for the Queue
    and Recno access methods.
    */
    public void setRecordPad(final int recordPad) {
        this.recordPad = recordPad;
    }

    /**
Return the padding character for short, fixed-length records for the
    Queue and Recno access methods.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The padding character for short, fixed-length records for the
    Queue and Recno access methods.
    */
    public int getRecordPad() {
        return recordPad;
    }

    /**
    Set the underlying source file for the Recno access method.
    <p>
    The purpose of the source file is to provide fast access and
    modification to databases that are normally stored as flat text
    files.
    <p>
    The recordSource parameter specifies an underlying flat text
    database file that is read to initialize a transient record number
    index.  In the case of variable length records, the records are
    separated, as specified by the
    {@link com.sleepycat.db.DatabaseConfig#setRecordDelimiter DatabaseConfig.setRecordDelimiter} method.  For example,
    standard UNIX byte stream files can be interpreted as a sequence of
    variable length records separated by newline characters (that is, ASCII
    0x0a).
    <p>
    In addition, when cached data would normally be written back to the
    underlying database file (for example, the {@link com.sleepycat.db.Database#close Database.close}
    or {@link com.sleepycat.db.Database#sync Database.sync} methods are called), the in-memory copy
    of the database will be written back to the source file.
    <p>
    By default, the backing source file is read lazily; that is, records
    are not read from the file until they are requested by the application.
    <b>
    If multiple processes (not threads) are accessing a Recno database
    concurrently, and are either inserting or deleting records, the backing
    source file must be read in its entirety before more than a single
    process accesses the database, and only that process should specify
    the backing source file as part of opening the database.  See the
    {@link com.sleepycat.db.DatabaseConfig#setSnapshot DatabaseConfig.setSnapshot} method for more information.
    </b>
    <p>
    <b>
    Reading and writing the backing source file cannot be
    transaction-protected because it involves filesystem operations that
    are not part of the {@link com.sleepycat.db.Database Database} transaction methodology.  For
    this reason, if a temporary database is used to hold the records,
    it is possible to lose the contents of the source file, for example,
    if the system crashes at the right instant.  If a file is used to
    hold the database, normal database recovery on that file can be used
    to prevent information loss, although it is still possible that the
    contents of the source file will be lost if the system crashes.
    </b>
    <p>
    The source file must already exist (but may be zero-length) when
    the database is opened.
    <p>
    It is not an error to specify a read-only source file when creating
    a database, nor is it an error to modify the resulting database.
    However, any attempt to write the changes to the backing source file
    using either the {@link com.sleepycat.db.Database#sync Database.sync} or {@link com.sleepycat.db.Database#close Database.close}
    methods will fail, of course.  Specify the noSync argument to the
    {@link com.sleepycat.db.Database#close Database.close} method to stop it from attempting to write
    the changes to the backing file; instead, they will be silently
    discarded.
    <p>
    For all of the previous reasons, the source file is generally used
    to specify databases that are read-only for Berkeley DB
    applications; and that are either generated on the fly by software
    tools or modified using a different mechanism -- for example, a text
    editor.
    <p>
    This method configures operations performed using the specified
{@link com.sleepycat.db.Database Database} object, not all operations performed on the underlying
database.
    <p>
    This method may not be called after the database is opened.
If the database already exists when it is opened,
the information specified to this method must be the same as that
historically used to create the database or corruption can occur.
    <p>
    @param recordSource
    The name of an underlying flat text database file that is read to
    initialize a transient record number index.  In the case of variable
    length records, the records are separated, as specified by the
    {@link com.sleepycat.db.DatabaseConfig#setRecordDelimiter DatabaseConfig.setRecordDelimiter} method.  For example,
    standard UNIX byte stream files can be interpreted as a sequence of
   variable length records separated by newline characters (that is, ASCII
    0x0a).
    */
    public void setRecordSource(final java.io.File recordSource) {
        this.recordSource = recordSource;
    }

    /**
Return the name of an underlying flat text database file that is
    read to initialize a transient record number index.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The name of an underlying flat text database file that is
    read to initialize a transient record number index.
    */
    public java.io.File getRecordSource() {
        return recordSource;
    }

    /**
    Configure the logical record numbers to be mutable, and change as
    records are added to and deleted from the database.
    <p>
    For example, the deletion of record number 4 causes records numbered
    5 and greater to be renumbered downward by one.  If a cursor was
    positioned to record number 4 before the deletion, it will refer to
    the new record number 4, if any such record exists, after the
    deletion.  If a cursor was positioned after record number 4 before
    the deletion, it will be shifted downward one logical record,
    continuing to refer to the same record as it did before.
    <p>
    Creating new records will cause the creation of multiple records if
    the record number is more than one greater than the largest record
    currently in the database.  For example, creating record 28, when
    record 25 was previously the last record in the database, will
    create records 26 and 27 as well as 28.  Attempts to retrieve
    records that were created in this manner will result in an error
    return of {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY}.
    <p>
    If a created record is not at the end of the database, all records
    following the new record will be automatically renumbered upward by one.
    For example, the creation of a new record numbered 8 causes records
    numbered 8 and greater to be renumbered upward by one.  If a cursor was
    positioned to record number 8 or greater before the insertion, it will
    be shifted upward one logical record, continuing to refer to the same
    record as it did before.
    <p>
    For these reasons, concurrent access to a Recno database configured
    with mutable record numbers may be largely meaningless, although it
    is supported.
    <p>
    Calling this method affects the database, including all threads of
control accessing the database.
    <p>
    If the database already exists when the database is opened, any database
configuration specified by this method
must be the same as the existing database or an error
will be returned.
    <p>
    @param renumbering
    If true, configure the logical record numbers to be mutable, and
    change as records are added to and deleted from the database.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setRenumbering(final boolean renumbering) {
        this.renumbering = renumbering;
    }

    /**
Return true if the logical record numbers are mutable, and change as
    records are added to and deleted from the database.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the logical record numbers are mutable, and change as
    records are added to and deleted from the database.
    */
    public boolean getRenumbering() {
        return renumbering;
    }

    /**
    Configure the Btree to not do reverse splits.
    <p>
    As pages are emptied in a database, the Btree implementation
    attempts to coalesce empty pages into higher-level pages in order
    to keep the database as small as possible and minimize search time.
    This can hurt performance in applications with cyclical data
    demands; that is, applications where the database grows and shrinks
    repeatedly.  For example, because Berkeley DB does page-level locking,
    the maximum level of concurrency in a database of two pages is far
    smaller than that in a database of 100 pages, so a database that has
    shrunk to a minimal size can cause severe deadlocking when a new
    cycle of data insertion begins.
    <p>
    Calling this method only affects the specified {@link com.sleepycat.db.Database Database} handle
(and any other library handles opened within the scope of that handle).
    <p>
    @param reverseSplitOff
    If true, configure the Btree to not do reverse splits.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setReverseSplitOff(final boolean reverseSplitOff) {
        this.reverseSplitOff = reverseSplitOff;
    }

    /**
Return true if the Btree has been configured to not do reverse splits.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the Btree has been configured to not do reverse splits.
    */
    public boolean getReverseSplitOff() {
        return reverseSplitOff;
    }

    /**
    Configure the database to support sorted, duplicate data items.
    <p>
    Insertion when the key of the key/data pair being inserted already
    exists in the database will be successful.  The ordering of
    duplicates in the database is determined by the duplicate comparison
    function.
    <p>
    If the application does not specify a duplicate data item comparison
    function, a default lexical comparison will be used.
    <p>
    If a primary database is to be associated with one or more secondary
    databases, it may not be configured for duplicates.
    <p>
    A Btree that supports duplicate data items cannot also be configured
    for retrieval by record number.
    <p>
    Calling this method affects the database, including all threads of
control accessing the database.
    <p>
    If the database already exists when the database is opened, any database
configuration specified by this method
must be the same as the existing database or an error
will be returned.
    <p>
    @param sortedDuplicates
    If true, configure the database to support duplicate data items.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setSortedDuplicates(final boolean sortedDuplicates) {
        this.sortedDuplicates = sortedDuplicates;
    }

    /**
Return true if the database is configured to support sorted duplicate data
    items.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database is configured to support sorted duplicate data
    items.
    */
    public boolean getSortedDuplicates() {
        return sortedDuplicates;
    }

    /**
    Configure the database to support unsorted duplicate data items.
    <p>
    Insertion when the key of the key/data pair being inserted already
    exists in the database will be successful.  The ordering of
    duplicates in the database is determined by the order of insertion,
    unless the ordering is otherwise specified by use of a database
    cursor operation.
    <p>
    If a primary database is to be associated with one or more secondary
    databases, it may not be configured for duplicates.
    <p>
    Sorted duplicates are preferred to unsorted duplicates for
    performance reasons.  Unsorted duplicates should only be used by
    applications wanting to order duplicate data items manually.
    <p>
    Calling this method affects the database, including all threads of
control accessing the database.
    <p>
    If the database already exists when the database is opened, any database
configuration specified by this method
must be the same as the existing database or an error
will be returned.
    <p>
    @param unsortedDuplicates
    If true, configure the database to support unsorted duplicate data items.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setUnsortedDuplicates(final boolean unsortedDuplicates) {
        this.unsortedDuplicates = unsortedDuplicates;
    }

    /**
Return true if the database is configured to support duplicate data items.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database is configured to support duplicate data items.
    */
    public boolean getUnsortedDuplicates() {
        return unsortedDuplicates;
    }

    /**
    Specify that any specified backing source file be read in its entirety
    when the database is opened.
    <p>
    If this flag is not specified, the backing source file may be read
    lazily.
    <p>
    Calling this method only affects the specified {@link com.sleepycat.db.Database Database} handle
(and any other library handles opened within the scope of that handle).
    <p>
    @param snapshot
    If true, any specified backing source file will be read in its entirety
    when the database is opened.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setSnapshot(final boolean snapshot) {
        this.snapshot = snapshot;
    }

    /**
Return true if the any specified backing source file will be read in its
    entirety when the database is opened.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the any specified backing source file will be read in its
    entirety when the database is opened.
    */
    public boolean getSnapshot() {
        return snapshot;
    }

    /**
Return true if the database open is enclosed within a transaction.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database open is enclosed within a transaction.
    */
    public boolean getTransactional() {
        return transactional;
    }

    /**
    Enclose the database open within a transaction.
    <p>
    If the call succeeds, the open operation will be recoverable.  If
    the call fails, no database will have been created.
    <p>
    All future operations on this database, which are not explicitly
    enclosed in a transaction by the application, will be enclosed in
    in a transaction within the library.
    <p>
    @param transactional
    If true, enclose the database open within a transaction.
    */
    public void setTransactional(final boolean transactional) {
        this.transactional = transactional;
    }

    /**
    Configure the database environment to not write log records for this
    database.
    <p>
    This means that updates of this database exhibit the ACI (atomicity,
    consistency, and isolation) properties, but not D (durability); that
    is, database integrity will be maintained, but if the application
    or system fails, integrity will not persist.  The database file must
    be verified and/or restored from backup after a failure.  In order
    to ensure integrity after application shut down, the database
    must be flushed to disk before the database handles are closed,
    or all
    database changes must be flushed from the database environment cache
    using {@link com.sleepycat.db.Environment#checkpoint Environment.checkpoint}.
    <p>
    All database handles for a single physical file must call this method,
    including database handles for different databases in a physical file.
    <p>
    Calling this method only affects the specified {@link com.sleepycat.db.Database Database} handle
(and any other library handles opened within the scope of that handle).
    <p>
    @param transactionNotDurable
    If true, configure the database environment to not write log records
    for this database.
    A value of false is illegal to this method, that is, once set, the
configuration cannot be cleared.
    */
    public void setTransactionNotDurable(final boolean transactionNotDurable) {
        this.transactionNotDurable = transactionNotDurable;
    }

    /**
Return true if the database environment is configured to not write log
    records for this database.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to not write log
    records for this database.
    */
    public boolean getTransactionNotDurable() {
        return transactionNotDurable;
    }

    /**
    Configure the database to be physically truncated by truncating the
    underlying file, discarding all previous databases it might have
    held.
    <p>
    Underlying filesystem primitives are used to implement this
    configuration.  For this reason, it is applicable only to a physical
    file and cannot be used to discard databases within a file.
    <p>
    This configuration option cannot be lock or transaction-protected, and
    it is an error to specify it in a locking or transaction-protected
    database environment.
    <p>
    @param truncate
    If true, configure the database to be physically truncated by truncating
    the underlying file, discarding all previous databases it might have
    held.
    */
    public void setTruncate(final boolean truncate) {
        this.truncate = truncate;
    }

    /**
Return true if the database has been configured to be physically truncated
    by truncating the underlying file, discarding all previous databases
    it might have held.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database has been configured to be physically truncated
    by truncating the underlying file, discarding all previous databases
    it might have held.
    */
    public boolean getTruncate() {
        return truncate;
    }

    /**
    Configure the type of the database.
    <p>
    If they type is DB_UNKNOWN, the database must already exist.
    <p>
    @param type
    The type of the database.
    */
    public void setType(final DatabaseType type) {
        this.type = type;
    }

    /**
    Return the type of the database.
    <p>
    This method may be used to determine the type of the database after
    opening it.
    <p>
    This method may not be called before the
database has been opened.
    <p>
    @return
    The type of the database.
    */
    public DatabaseType getType() {
        return type;
    }

    /* package */
    Db createDatabase(final DbEnv dbenv)
        throws DatabaseException {

        return new Db(dbenv, 0);
    }

    /* package */
    Db openDatabase(final DbEnv dbenv,
                    final DbTxn txn,
                    final String fileName,
                    final String databaseName)
        throws DatabaseException, java.io.FileNotFoundException {

        final Db db = createDatabase(dbenv);
        // The DB_THREAD flag is inherited from the environment
        // (defaulting to ON if no environment handle is supplied).
        boolean threaded = (dbenv == null ||
            (dbenv.get_open_flags() & DbConstants.DB_THREAD) != 0);

        int openFlags = 0;
        openFlags |= allowCreate ? DbConstants.DB_CREATE : 0;
        openFlags |= readUncommitted ? DbConstants.DB_READ_UNCOMMITTED : 0;
        openFlags |= exclusiveCreate ? DbConstants.DB_EXCL : 0;
        openFlags |= multiversion ? DbConstants.DB_MULTIVERSION : 0;
        openFlags |= noMMap ? DbConstants.DB_NOMMAP : 0;
        openFlags |= readOnly ? DbConstants.DB_RDONLY : 0;
        openFlags |= threaded ? DbConstants.DB_THREAD : 0;
        openFlags |= truncate ? DbConstants.DB_TRUNCATE : 0;

        if (transactional && txn == null)
            openFlags |= DbConstants.DB_AUTO_COMMIT;

        boolean succeeded = false;
        try {
            configureDatabase(db, DEFAULT);
            db.open(txn, fileName, databaseName, type.getId(), openFlags, mode);
            succeeded = true;
            return db;
        } finally {
            if (!succeeded)
                try {
                    db.close(0);
                } catch (Throwable t) {
                    // Ignore it -- an exception is already in flight.
                }
        }
    }

    /* package */
    void configureDatabase(final Db db, final DatabaseConfig oldConfig)
        throws DatabaseException {

        int dbFlags = 0;
        dbFlags |= checksum ? DbConstants.DB_CHKSUM : 0;
        dbFlags |= btreeRecordNumbers ? DbConstants.DB_RECNUM : 0;
        dbFlags |= queueInOrder ? DbConstants.DB_INORDER : 0;
        dbFlags |= renumbering ? DbConstants.DB_RENUMBER : 0;
        dbFlags |= reverseSplitOff ? DbConstants.DB_REVSPLITOFF : 0;
        dbFlags |= sortedDuplicates ? DbConstants.DB_DUPSORT : 0;
        dbFlags |= snapshot ? DbConstants.DB_SNAPSHOT : 0;
        dbFlags |= unsortedDuplicates ? DbConstants.DB_DUP : 0;
        dbFlags |= transactionNotDurable ? DbConstants.DB_TXN_NOT_DURABLE : 0;
        if (!db.getPrivateDbEnv())
                dbFlags |= (password != null) ? DbConstants.DB_ENCRYPT : 0;

        if (dbFlags != 0)
            db.set_flags(dbFlags);

        if (btMinKey != oldConfig.btMinKey)
            db.set_bt_minkey(btMinKey);
        if (byteOrder != oldConfig.byteOrder)
            db.set_lorder(byteOrder);
        if ((cacheSize != oldConfig.cacheSize ||
            cacheCount != oldConfig.cacheCount) && db.getPrivateDbEnv())
            db.set_cachesize(cacheSize, cacheCount);
        if (createDir != oldConfig.createDir && createDir != null &&
            !createDir.equals(oldConfig.createDir))
            db.set_create_dir(createDir.toString());
        if (errorStream != oldConfig.errorStream)
            db.set_error_stream(errorStream);
        if (errorPrefix != oldConfig.errorPrefix)
            db.set_errpfx(errorPrefix);
        if (hashFillFactor != oldConfig.hashFillFactor)
            db.set_h_ffactor(hashFillFactor);
        if (hashNumElements != oldConfig.hashNumElements)
            db.set_h_nelem(hashNumElements);
        if (messageStream != oldConfig.messageStream)
            db.set_message_stream(messageStream);
        if (pageSize != oldConfig.pageSize)
            db.set_pagesize(pageSize);

        if (partitionDirs != null &&
            partitionDirs != oldConfig.partitionDirs) {
            String[] partitionDirArray = new String[partitionDirs.length];
            for (int i = 0; i < partitionDirArray.length; i++)
                partitionDirArray[i] = partitionDirs[i].toString();
            db.set_partition_dirs(partitionDirArray);
        }
        if (password != oldConfig.password && db.getPrivateDbEnv())
            db.set_encrypt(password, DbConstants.DB_ENCRYPT_AES);
        if (priority != oldConfig.priority)
            db.set_priority(priority.getFlag());
        if (queueExtentSize != oldConfig.queueExtentSize)
            db.set_q_extentsize(queueExtentSize);
        if (recordDelimiter != oldConfig.recordDelimiter)
            db.set_re_delim(recordDelimiter);
        if (recordLength != oldConfig.recordLength)
            db.set_re_len(recordLength);
        if (recordPad != oldConfig.recordPad)
            db.set_re_pad(recordPad);
        if (recordSource != oldConfig.recordSource)
            db.set_re_source(
                (recordSource == null) ? null : recordSource.toString());

        if (btreeComparator != oldConfig.btreeComparator)
            db.set_bt_compare(btreeComparator);
        if (btreeCompressor != oldConfig.btreeCompressor)
            db.set_bt_compress(btreeCompressor, btreeCompressor);
        if (btreePrefixCalculator != oldConfig.btreePrefixCalculator)
            db.set_bt_prefix(btreePrefixCalculator);
        if (duplicateComparator != oldConfig.duplicateComparator)
            db.set_dup_compare(duplicateComparator);
        if (feedbackHandler != oldConfig.feedbackHandler)
            db.set_feedback(feedbackHandler);
        if (errorHandler != oldConfig.errorHandler)
            db.set_errcall(errorHandler);
        if (hashComparator != oldConfig.hashComparator)
            db.set_h_compare(hashComparator);
        if (hasher != oldConfig.hasher)
            db.set_h_hash(hasher);
        if (messageHandler != oldConfig.messageHandler)
            db.set_msgcall(messageHandler);
        if (partitionHandler != oldConfig.partitionHandler ||
            partitionKeys != oldConfig.partitionKeys ||
            partitionParts != oldConfig.partitionParts)
            db.set_partition(partitionParts, partitionKeys, partitionHandler);
        if (recnoAppender != oldConfig.recnoAppender)
            db.set_append_recno(recnoAppender);
        if (panicHandler != oldConfig.panicHandler)
            db.set_paniccall(panicHandler);
    }

    /* package */
    DatabaseConfig(final Db db)
        throws DatabaseException {

        type = DatabaseType.fromInt(db.get_type());

        final int openFlags = db.get_open_flags();
        allowCreate = (openFlags & DbConstants.DB_CREATE) != 0;
        readUncommitted = (openFlags & DbConstants.DB_READ_UNCOMMITTED) != 0;
        exclusiveCreate = (openFlags & DbConstants.DB_EXCL) != 0;
        multiversion = (openFlags & DbConstants.DB_MULTIVERSION) != 0;
        noMMap = (openFlags & DbConstants.DB_NOMMAP) != 0;
        readOnly = (openFlags & DbConstants.DB_RDONLY) != 0;
        truncate = (openFlags & DbConstants.DB_TRUNCATE) != 0;

        final int dbFlags = db.get_flags();
        checksum = (dbFlags & DbConstants.DB_CHKSUM) != 0;
        btreeRecordNumbers = (dbFlags & DbConstants.DB_RECNUM) != 0;
        queueInOrder = (dbFlags & DbConstants.DB_INORDER) != 0;
        renumbering = (dbFlags & DbConstants.DB_RENUMBER) != 0;
        reverseSplitOff = (dbFlags & DbConstants.DB_REVSPLITOFF) != 0;
        sortedDuplicates = (dbFlags & DbConstants.DB_DUPSORT) != 0;
        snapshot = (dbFlags & DbConstants.DB_SNAPSHOT) != 0;
        unsortedDuplicates = (dbFlags & DbConstants.DB_DUP) != 0;
        transactionNotDurable = (dbFlags & DbConstants.DB_TXN_NOT_DURABLE) != 0;

        if (type == DatabaseType.BTREE) {
            btMinKey = db.get_bt_minkey();
        }
        byteOrder = db.get_lorder();
        // Call get_cachesize* on the DbEnv to avoid this error:
        //   DB->get_cachesize: method not permitted in shared environment
        cacheSize = db.get_env().get_cachesize();
        cacheCount = db.get_env().get_cachesize_ncache();
        errorStream = db.get_error_stream();
        errorPrefix = db.get_errpfx();
        if (type == DatabaseType.HASH) {
            hashFillFactor = db.get_h_ffactor();
            hashNumElements = db.get_h_nelem();
        }
        messageStream = db.get_message_stream();
        // Not available by design
        password = ((dbFlags & DbConstants.DB_ENCRYPT) != 0) ? "" : null;
        priority = CacheFilePriority.fromFlag(db.get_priority());
        if (type == DatabaseType.QUEUE) {
            queueExtentSize = db.get_q_extentsize();
        }
        if (type == DatabaseType.QUEUE || type == DatabaseType.RECNO) {
            recordLength = db.get_re_len();
            recordPad = db.get_re_pad();
        }
        if (type == DatabaseType.RECNO) {
            recordDelimiter = db.get_re_delim();
            recordSource = (db.get_re_source() == null) ? null :
                new java.io.File(db.get_re_source());
        }
        transactional = db.get_transactional();
        createDir = (db.get_create_dir() == null) ? null:
            new java.io.File(db.get_create_dir());

        String[] partitionDirArray = db.get_partition_dirs();
        if (partitionDirArray == null)
            partitionDirArray = new String[0];
        partitionDirs = new java.io.File[partitionDirArray.length];
        for (int i = 0; i < partitionDirArray.length; i++)
            partitionDirs[i] = new java.io.File(partitionDirArray[i]);

        btreeComparator = db.get_bt_compare();
        btreeCompressor = db.get_bt_compress();
        btreePrefixCalculator = db.get_bt_prefix();
        duplicateComparator = db.get_dup_compare();
        feedbackHandler = db.get_feedback();
        errorHandler = db.get_errcall();
        hashComparator = db.get_h_compare();
        hasher = db.get_h_hash();
        messageHandler = db.get_msgcall();
        partitionParts = db.get_partition_parts();
        partitionKeys = db.get_partition_keys();
        partitionHandler = db.get_partition_callback();
        recnoAppender = db.get_append_recno();
        panicHandler = db.get_paniccall();
    }
}
