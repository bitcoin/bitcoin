/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_STL_CONTAINER_H__
#define _DB_STL_CONTAINER_H__

#include "dbstl_common.h"
#include "dbstl_resource_manager.h"
#include <assert.h>

START_NS(dbstl)

class ResourceManager;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// db_container class definition
//
// This class's begin_txn, commit/abort_txn is used to wrap each DB related
// operation inside dbstl. When auto commit is enabled, each operation will
// be auto committed before returning, and aborted when an exception is thrown.
//
// It also contains members to hold all needed parameters and flags for
// transaction and cursor related calls.
//
// Container classes will inherit from this class. Each container will enclose
// every db related operation with db_container::begin_txn and
// db_container::commit_txn, and if exception is not caught, abort_txn()
// should be called.

/** \defgroup dbstl_containers dbstl container classes
A dbstl container is very much like a C++ STL container. It stores a 
collection of data items, or key/data pairs. 
Each container is backed by a Berkeley DB database created in an explicit
database environment or an internal private environment; And the database
itself can be created explicitly with all kinds of configurations, or
by dbstl internally. For each type of container, some specific type of 
database and/or configurations must be used or specified to the database
and its environment. dbstl will check the database and environment conform 
to the requirement. When users don't have a chance to specify a container's
backing database and environment, like in copy constructors, dbstl will
create proper databases and/or environment for it. There are two helper 
functions to make it easier to create/open an environment or database, they
are dbstl::open_db() and dbstl::open_env();
\sa dbstl::open_db() dbstl::open_env() db_vector db_map db_multimap db_set 
db_multiset
*/

/** \ingroup dbstl_containers
@{
This class is the base class for all db container classes, you don't directly
use this class, but all container classes inherit from this class, so you need
to know the methods that can be accessed via concrete container classes.
This class is also used to support auto commit transactions. Autocommit is
enabled when DB_AUTO_COMMIT is set to the database or database environment
handle and the environment is transactional.

Inside dbstl, there are transactions begun and committed/aborted if the backing
database and/or environment requires auto commit, and there are cursors
opened internally, and you can set the flags used by the transaction and cursor
functions via set functions of this class.

All dbstl containers are fully multi-threaded, you should not need any
synchronization to use them in the correct way, but this class is not thread
safe, access to its members are not proctected by any mutex because the data
members of this class are supposed to be set before they are used, 
and remain read only afterwards. If this is not the case, you must synchronize
the access.
*/
class _exported db_container
{
private:
    // By default these flags are 0, users should configure these values
    // on container initialization.
    //
    u_int32_t txn_begin_flags_, commit_flags_;
    mutable u_int32_t cursor_oflags_;

    // Berkeley DB database handle for each container. Subclasses do not
    // need to define it.
    //
    Db *pdb_;

    // Berkeley DB environment handle in which the db handle is opened.
    DbEnv *dbenv_;

    // db_map_iterator<> needs to know whether the container is a
    // db_(multi)set or not
    //
    bool is_set_;

    // Determined automatically, by inspecting users the Berkeley DB
    // database and environment configuration options.
    //
    bool auto_commit_;

    // If exisiting random temporary database name generation mechanism is
    // still causing name clashes, users can set this global suffix number
    // which will be append to each temporary database file name and by 
    // default it is 0. there is a dbstl::set_global_dbfile_suffix_number
    // to do so.
    static u_int32_t g_db_file_suffix_;
    friend void set_global_dbfile_suffix_number(u_int32_t);
protected:

    // Does not clone or copy data from database parameter.
    // Return the db file name back in the dbfname parameter.
    // We construct a default database name the user can rename it using
    // either DbEnv::dbrename or Db::rename.
    //
    Db* clone_db_config(Db *dbp, std::string &dbfname);
    Db* clone_db_config(Db *dbp);

    // Store the name into name parameter whose length is n. Return -1 if
    // not enough space.
    int construct_db_file_name(std::string &filename) const;

    // Check that this container and cntnr are backed by different databases
    // and if any one of them is using transactions, both should be in the
    // same transactional environment.
    // Called by all deriving classes' methods
    // which have a container parameter.
    void verify_db_handles(const db_container &cntnr) const;

    void open_db_handles(Db *&pdb, DbEnv *&penv, DBTYPE dbtype,
        u_int32_t oflags, u_int32_t sflags);

    inline void set_is_set(bool b)
    {
        is_set_ = b;
    }

    inline bool is_set() const
    {
        return is_set_;
    }

    // Database and environment handles and autocommit and is_set_ are 
    // not assigned, because they are the nature of my own db, not 
    // that of dbctnr.
    inline const db_container &operator=(const db_container & dbctnr)
    {
        ASSIGNMENT_PREDCOND(dbctnr)
        txn_begin_flags_ = dbctnr.txn_begin_flags_;
        commit_flags_ = dbctnr.commit_flags_;
        cursor_oflags_ = dbctnr.cursor_oflags_;
        return dbctnr;
    }

public:
    /// Default constructor.
    db_container();

    /// Copy constructor.
    /// The new container will be backed by another database within the
    /// same environment unless dbctnr's backing database is in its own
    /// internal private environment. The name of the database is coined
    /// based on current time and thread id and some random number. If
    /// this is still causing naming clashes, you can set a suffix number
    /// via "set_global_dbfile_suffix_number" function; And following db
    /// file will suffix this number in the file name for additional 
    /// randomness. And the suffix will be incremented after each such use.
    /// You can change the file name via DbEnv::rename.
    /// If dbctnr is using an anonymous database, the newly constructed
    /// container will also use an anonymous one.
    /// \param dbctnr The container to initialize this container.
    db_container(const db_container &dbctnr);

    /**
    This constructor is not directly called by the user, but invoked by
    constructors of concrete container classes. The statement about the
    parameters applies to constructors of all container classes.
    \param dbp Database handle. dbp is supposed to be opened inside envp.
    Each dbstl container is backed by a Berkeley DB database, so dbstl 
    will create an internal anonymous database if dbp is NULL. 
    \param envp Environment handle. And envp can also be NULL, meaning the
    dbp handle may be created in its internal private environment.
    */
    db_container(Db *dbp, DbEnv *envp);

    /// The backing database is not closed in this function. It is closed
    /// when current thread exits and the database is no longer referenced
    /// by any other container instances in this process. 
    /// In order to make the reference counting work alright, you must call
    /// register_db(Db*) and register_db_env(DbEnv*) correctly.
    /// \sa register_db(Db*) register_db_env(DbEnv*) 
    virtual ~db_container(){}

    /// \name Get and set functions for data members.
    /// Note that these functions are not thread safe, because all data 
    /// members of db_container are supposed to be set on container 
    /// construction and initialization, and remain read only afterwards.
    //@{
    /// Get the backing database's open flags.
    /// \return The backing database's open flags.
    inline u_int32_t get_db_open_flags()   const
    {
        u_int32_t oflags;
        pdb_->get_open_flags(&oflags);
        return oflags;
    }

    /// Get the backing database's flags that are set via Db::set_flags()
    /// function.
    /// \return Flags set to this container's database handle.
    inline u_int32_t get_db_set_flags()   const
    {
        u_int32_t oflags;
        pdb_->get_flags(&oflags);
        return oflags;
    }

    /// Get the backing database's handle. 
    /// \return The backing database handle of this container.
    inline Db* get_db_handle() const
    {
        return pdb_;
    }

    /// Get the backing database environment's handle. 
    /// \return The backing database environment handle of this container.
    inline DbEnv* get_db_env_handle()  const
    {
        return dbenv_;
    }

    /** 
    Set the underlying database's handle, and optionally environment 
    handle if the environment has also changed. That is, users can change
    the container object's underlying database while the object is alive.
    dbstl will verify that the handles set conforms to the concrete 
    container's requirement to Berkeley DB database/environment handles.
    \param dbp The database handle to set.
    \param newenv The database environment handle to set.
    */
    void set_db_handle(Db *dbp, DbEnv *newenv = NULL);

    /** Set the flags required by the Berkeley DB functions 
    DbEnv::txn_begin(), DbTxn::commit() and DbEnv::cursor(). These flags
    will be set to this container's auto commit member functions when 
    auto commit transaction is used, except that cursor_oflags is set to 
    the Dbc::cursor when creating an iterator for this container.
    By default the three flags are all zero.
    You can also set the values of the flags individually by using the 
    appropriate set functions in this class. The corresponding get 
    functions return the flags actually used.
    \param txn_begin_flags Flags to be set to DbEnv::txn_begin().
    \param commit_flags Flags to be set to DbTxn::commit().
    \param cursor_open_flags Flags to be set to Db::cursor().
    */
    inline void set_all_flags(u_int32_t txn_begin_flags,
        u_int32_t commit_flags, u_int32_t cursor_open_flags)
    {
        this->txn_begin_flags_ = txn_begin_flags;
        this->commit_flags_ = commit_flags;
        this->cursor_oflags_ = cursor_open_flags;
    }

    /// Set flag of DbEnv::txn_begin() call.
    /// \param flag Flags to be set to DbEnv::txn_begin().
    inline void set_txn_begin_flags(u_int32_t flag )
    {
        txn_begin_flags_ = flag;
    }

    /// Get flag of DbEnv::txn_begin() call.
    /// \return Flags to be set to DbEnv::txn_begin().
    inline u_int32_t get_txn_begin_flags()  const
    {
        return txn_begin_flags_;
    }

    /// Set flag of DbTxn::commit() call.
    /// \param flag Flags to be set to DbTxn::commit().
    inline void set_commit_flags(u_int32_t flag)
    {
        commit_flags_ = flag;
    }

    /// Get flag of DbTxn::commit() call.
    /// \return Flags to be set to DbTxn::commit().
    inline u_int32_t get_commit_flags()  const
    {
        return commit_flags_;
    }

    /// Get flag of Db::cursor() call.
    /// \return Flags to be set to Db::cursor().
    inline u_int32_t get_cursor_open_flags()  const
    {
        return cursor_oflags_;
    }

    /// Set flag of Db::cursor() call.
    /// \param flag Flags to be set to Db::cursor().
    inline void set_cursor_open_flags(u_int32_t flag)
    {
        cursor_oflags_ = flag;
    }
    //@} // getter_setters

protected:
    void init_members();
    void init_members(Db *pdb, DbEnv *env);
    void init_members(const db_container&cnt);
    // Called internally by concrete container constructors. Verification
    // is done by the specialized constructors
    void set_db_handle_int(Db *dbp, DbEnv *envp)
    {
        this->pdb_ = dbp;
        this->dbenv_ = envp;
    }

    // Child classes override this function to check the db and environment
    // handles are correctly configured.
    virtual const char* verify_config(Db* pdb, DbEnv* penv) const
    {
        if (pdb != NULL && ((pdb->get_create_flags() & 
            DB_CXX_NO_EXCEPTIONS) == 0))
            return 
"Db and DbEnv object must be constructed with DB_CXX_NO_EXCEPTIONS flag set.";

        if (penv != NULL && ((penv->get_create_flags() & 
            DB_CXX_NO_EXCEPTIONS) == 0))
            return 
"Db and DbEnv object must be constructed with DB_CXX_NO_EXCEPTIONS flag set.";
        return NULL;
    }

    // Use db and dbenv_ to determine whether to enable autocommit. If
    // DB_AUTOCOMMIT is set on dbenv_ or db and dbenv_ is transactional,
    // that db should be autocommit.
    //
    void set_auto_commit(Db* db);

    // Begin a transaction. Used to make a container's db related
    // operations auto commit when the operation completes and abort
    // when the operation fails. If there is already a transaction for this
    // container's environment, then that transaction is used. If the
    // transaction was created by DB STL, its reference count is
    // incremented (user created and external transactions are not
    // reference counted because they can be nested.).
    inline DbTxn* begin_txn() const
    {
        DbTxn *txn = NULL;

        if (this->auto_commit_) {
            txn = ResourceManager::instance()->begin_txn(
                this->txn_begin_flags_, dbenv_, 0);
        }
        return txn;
    }

    inline void commit_txn()  const
    {
        if (this->auto_commit_) {
            ResourceManager::instance()->
                commit_txn(pdb_->get_env(), this->commit_flags_);
        }
    }

    inline void abort_txn() const
    {
        if (this->auto_commit_)
            ResourceManager::instance()->
                abort_txn(pdb_->get_env());
    }

    inline DbTxn *current_txn() const
    {
        return ResourceManager::instance()->
            current_txn(pdb_->get_env());
    }
}; // db_container
/** @} */ // dbstl_containers

/// \addtogroup dbstl_helper_classes
//@{
/// Bulk retrieval configuration helper class. Used by the begin() function of
/// a container.
class _exported BulkRetrievalOption
{
public:
    enum Option {BulkRetrieval, NoBulkRetrieval};

protected:
    Option bulk_retrieve;
    u_int32_t bulk_buf_sz_;

    inline BulkRetrievalOption()
    {
        bulk_retrieve = NoBulkRetrieval;
        bulk_buf_sz_ = DBSTL_BULK_BUF_SIZE;
    }

public:
    inline BulkRetrievalOption(Option bulk_retrieve1, 
        u_int32_t bulk_buf_sz = DBSTL_BULK_BUF_SIZE)
    {
        this->bulk_retrieve = bulk_retrieve1;
        this->bulk_buf_sz_ = bulk_buf_sz;
    }

    // The following two static members should best be const, but this is
    // impossible because in C++ this definition is a function declaration:
    // const static BulkRetrievalOption BulkRetrieval(BulkRetrieval);
    // i.e. const static members can only be inited with default copy
    // constructor.
    //
    // Static data members can't be compiled into a .lib for a dll on
    // Windows, code using the static members will have link error---
    // unresolved symbols, so we have to use a static function here.
    /// This function indicates that you need a bulk retrieval iterator, 
    /// and it can be also used to optionally set the bulk read buffer size.
    inline static BulkRetrievalOption bulk_retrieval(
        u_int32_t bulk_buf_sz = DBSTL_BULK_BUF_SIZE)
    {
        return BulkRetrievalOption(BulkRetrieval, bulk_buf_sz);
    }
  
    /// This function indicates that you do not need a bulk retrieval 
    /// iterator.
    inline static BulkRetrievalOption no_bulk_retrieval()
    {
        return BulkRetrievalOption(NoBulkRetrieval, 0);
    }
  
    /// Equality comparison.
    inline bool operator==(const BulkRetrievalOption& bro) const
    {
        return bulk_retrieve == bro.bulk_retrieve;
    }
  
    /// Assignment operator.
    inline void operator=(BulkRetrievalOption::Option opt)
    {
        bulk_retrieve = opt;
    }
  
    /// Return the buffer size set to this object.
    inline u_int32_t bulk_buf_size()
    {
        return bulk_buf_sz_;
    }
};
//@}

/// \addtogroup dbstl_helper_classes
//@{
/// Read-modify-write cursor configuration helper class. Used by each begin()
/// function of all containers.
class _exported ReadModifyWriteOption
{
protected:
    enum Option{ReadModifyWrite, NoReadModifyWrite};
    Option rmw;

    inline ReadModifyWriteOption(Option rmw1)
    {
        this->rmw = rmw1;
    }

    inline ReadModifyWriteOption()
    {
        rmw = NoReadModifyWrite;
    }

public:
    /// Assignment operator.
    inline void operator=(ReadModifyWriteOption::Option rmw1)
    {
        this->rmw = rmw1;
    }

    /// Equality comparison.
    inline bool operator==(const ReadModifyWriteOption &rmw1) const
    {
        return this->rmw == rmw1.rmw;
    }

    /// Call this function to tell the container's begin() function that
    /// you need a read-modify-write iterator.
    inline static ReadModifyWriteOption read_modify_write()
    {
        return ReadModifyWriteOption(ReadModifyWrite);
    }

    /// Call this function to tell the container's begin() function that
    /// you do not need a read-modify-write iterator. This is the default
    /// value for the parameter of any container's begin() function.
    inline static ReadModifyWriteOption no_read_modify_write()
    {
        return ReadModifyWriteOption(NoReadModifyWrite);
    }

};
//@} // dbstl_helper_classes

// The classes in the Berkeley DB C++ API do not expose data and p_ member.
// Extend the class to provide this functionality, rather than altering the
// internal implementations.
//
class _exported DbstlMultipleIterator : protected DbMultipleIterator
{
protected:
    DbstlMultipleIterator(const Dbt &dbt) : DbMultipleIterator(dbt) {}
public:
    u_int32_t get_pointer()
    {
        u_int32_t off;
        off = (u_int32_t)((u_int8_t*)p_ - data_);
        return off;
    }

    inline void set_pointer(u_int32_t offset)
    {
        p_ = (u_int32_t*)(data_ + offset);
    }

};

class _exported DbstlMultipleKeyDataIterator : public DbstlMultipleIterator
{
public:
    DbstlMultipleKeyDataIterator(const Dbt &dbt)
        : DbstlMultipleIterator(dbt) {}
    bool next(Dbt &key, Dbt &data);
};

class _exported DbstlMultipleRecnoDataIterator : public DbstlMultipleIterator
{
public:
    DbstlMultipleRecnoDataIterator(const Dbt &dbt)
       : DbstlMultipleIterator(dbt) {}
    bool next(db_recno_t &recno, Dbt &data);
};

class _exported DbstlMultipleDataIterator : public DbstlMultipleIterator
{
public:
    DbstlMultipleDataIterator(const Dbt &dbt)
        : DbstlMultipleIterator(dbt) {}
    bool next(Dbt &data);
};

// These classes are used to give data values meaningful default
// initializations. They are only necessary for types that do not have a
// reasonable default constructor - explicitly char *, wchar_t * and T*.
// The string types don't have a reasonable default initializer since we store
// the underlying content, not a pointer.
// So we fully instantiate types for T* and const T* and set the pointers to
// NULL and leave other types intact.
template <Typename T>
class DbstlInitializeDefault
{
public:
    DbstlInitializeDefault(T&){}
};

template <Typename T>
class DbstlInitializeDefault<T *>
{
public:
    DbstlInitializeDefault(T *& t){t = NULL;}
};

template <Typename T>
class DbstlInitializeDefault<const T *>
{
public:
    DbstlInitializeDefault(const T *& t){t = NULL;}
};

END_NS

#endif //_DB_STL_CONTAINER_H__
