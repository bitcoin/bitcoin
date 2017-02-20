
package BerkeleyDB;


#     Copyright (c) 1997-2009 Paul Marquess. All rights reserved.
#     This program is free software; you can redistribute it and/or
#     modify it under the same terms as Perl itself.
#

# The documentation for this module is at the bottom of this file,
# after the line __END__.

BEGIN { require 5.005 }

use strict;
use Carp;
use vars qw($VERSION @ISA @EXPORT $AUTOLOAD
		$use_XSLoader);

$VERSION = '0.39';

require Exporter;
#require DynaLoader;
require AutoLoader;

BEGIN {
    $use_XSLoader = 1 ;
    { local $SIG{__DIE__} ; eval { require XSLoader } ; }
 
    if ($@) {
        $use_XSLoader = 0 ;
        require DynaLoader;
        @ISA = qw(DynaLoader);
    }
}

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# NOTE -- Do not add to @EXPORT directly. It is written by mkconsts
@EXPORT = qw(
	DB_AFTER
	DB_AGGRESSIVE
	DB_ALREADY_ABORTED
	DB_APPEND
	DB_APPLY_LOGREG
	DB_APP_INIT
	DB_ARCH_ABS
	DB_ARCH_DATA
	DB_ARCH_LOG
	DB_ARCH_REMOVE
	DB_ASSOC_IMMUTABLE_KEY
	DB_AUTO_COMMIT
	DB_BEFORE
	DB_BTREE
	DB_BTREEMAGIC
	DB_BTREEOLDVER
	DB_BTREEVERSION
	DB_BUFFER_SMALL
	DB_CACHED_COUNTS
	DB_CDB_ALLDB
	DB_CHECKPOINT
	DB_CHKSUM
	DB_CHKSUM_SHA1
	DB_CKP_INTERNAL
	DB_CLIENT
	DB_CL_WRITER
	DB_COMMIT
	DB_COMPACT_FLAGS
	DB_CONSUME
	DB_CONSUME_WAIT
	DB_CREATE
	DB_CURLSN
	DB_CURRENT
	DB_CURSOR_BULK
	DB_CURSOR_TRANSIENT
	DB_CXX_NO_EXCEPTIONS
	DB_DEGREE_2
	DB_DELETED
	DB_DELIMITER
	DB_DIRECT
	DB_DIRECT_DB
	DB_DIRECT_LOG
	DB_DIRTY_READ
	DB_DONOTINDEX
	DB_DSYNC_DB
	DB_DSYNC_LOG
	DB_DUP
	DB_DUPCURSOR
	DB_DUPSORT
	DB_DURABLE_UNKNOWN
	DB_EID_BROADCAST
	DB_EID_INVALID
	DB_ENCRYPT
	DB_ENCRYPT_AES
	DB_ENV_APPINIT
	DB_ENV_AUTO_COMMIT
	DB_ENV_CDB
	DB_ENV_CDB_ALLDB
	DB_ENV_CREATE
	DB_ENV_DBLOCAL
	DB_ENV_DIRECT_DB
	DB_ENV_DIRECT_LOG
	DB_ENV_DSYNC_DB
	DB_ENV_DSYNC_LOG
	DB_ENV_FAILCHK
	DB_ENV_FATAL
	DB_ENV_LOCKDOWN
	DB_ENV_LOCKING
	DB_ENV_LOGGING
	DB_ENV_LOG_AUTOREMOVE
	DB_ENV_LOG_INMEMORY
	DB_ENV_MULTIVERSION
	DB_ENV_NOLOCKING
	DB_ENV_NOMMAP
	DB_ENV_NOPANIC
	DB_ENV_NO_OUTPUT_SET
	DB_ENV_OPEN_CALLED
	DB_ENV_OVERWRITE
	DB_ENV_PANIC_OK
	DB_ENV_PRIVATE
	DB_ENV_RECOVER_FATAL
	DB_ENV_REF_COUNTED
	DB_ENV_REGION_INIT
	DB_ENV_REP_CLIENT
	DB_ENV_REP_LOGSONLY
	DB_ENV_REP_MASTER
	DB_ENV_RPCCLIENT
	DB_ENV_RPCCLIENT_GIVEN
	DB_ENV_STANDALONE
	DB_ENV_SYSTEM_MEM
	DB_ENV_THREAD
	DB_ENV_TIME_NOTGRANTED
	DB_ENV_TXN
	DB_ENV_TXN_NOSYNC
	DB_ENV_TXN_NOT_DURABLE
	DB_ENV_TXN_NOWAIT
	DB_ENV_TXN_SNAPSHOT
	DB_ENV_TXN_WRITE_NOSYNC
	DB_ENV_USER_ALLOC
	DB_ENV_YIELDCPU
	DB_EVENT_NOT_HANDLED
	DB_EVENT_NO_SUCH_EVENT
	DB_EVENT_PANIC
	DB_EVENT_REG_ALIVE
	DB_EVENT_REG_PANIC
	DB_EVENT_REP_CLIENT
	DB_EVENT_REP_ELECTED
	DB_EVENT_REP_MASTER
	DB_EVENT_REP_NEWMASTER
	DB_EVENT_REP_PERM_FAILED
	DB_EVENT_REP_STARTUPDONE
	DB_EVENT_WRITE_FAILED
	DB_EXCL
	DB_EXTENT
	DB_FAILCHK
	DB_FAST_STAT
	DB_FCNTL_LOCKING
	DB_FILEOPEN
	DB_FILE_ID_LEN
	DB_FIRST
	DB_FIXEDLEN
	DB_FLUSH
	DB_FORCE
	DB_FOREIGN_ABORT
	DB_FOREIGN_CASCADE
	DB_FOREIGN_CONFLICT
	DB_FOREIGN_NULLIFY
	DB_FREELIST_ONLY
	DB_FREE_SPACE
	DB_GETREC
	DB_GET_BOTH
	DB_GET_BOTHC
	DB_GET_BOTH_LTE
	DB_GET_BOTH_RANGE
	DB_GET_RECNO
	DB_GID_SIZE
	DB_HANDLE_LOCK
	DB_HASH
	DB_HASHMAGIC
	DB_HASHOLDVER
	DB_HASHVERSION
	DB_IGNORE_LEASE
	DB_IMMUTABLE_KEY
	DB_INCOMPLETE
	DB_INIT_CDB
	DB_INIT_LOCK
	DB_INIT_LOG
	DB_INIT_MPOOL
	DB_INIT_REP
	DB_INIT_TXN
	DB_INORDER
	DB_JAVA_CALLBACK
	DB_JOINENV
	DB_JOIN_ITEM
	DB_JOIN_NOSORT
	DB_KEYEMPTY
	DB_KEYEXIST
	DB_KEYFIRST
	DB_KEYLAST
	DB_LAST
	DB_LOCKDOWN
	DB_LOCKMAGIC
	DB_LOCKVERSION
	DB_LOCK_ABORT
	DB_LOCK_CONFLICT
	DB_LOCK_DEADLOCK
	DB_LOCK_DEFAULT
	DB_LOCK_DUMP
	DB_LOCK_EXPIRE
	DB_LOCK_FREE_LOCKER
	DB_LOCK_GET
	DB_LOCK_GET_TIMEOUT
	DB_LOCK_INHERIT
	DB_LOCK_MAXLOCKS
	DB_LOCK_MAXWRITE
	DB_LOCK_MINLOCKS
	DB_LOCK_MINWRITE
	DB_LOCK_NORUN
	DB_LOCK_NOTEXIST
	DB_LOCK_NOTGRANTED
	DB_LOCK_NOTHELD
	DB_LOCK_NOWAIT
	DB_LOCK_OLDEST
	DB_LOCK_PUT
	DB_LOCK_PUT_ALL
	DB_LOCK_PUT_OBJ
	DB_LOCK_PUT_READ
	DB_LOCK_RANDOM
	DB_LOCK_RECORD
	DB_LOCK_REMOVE
	DB_LOCK_RIW_N
	DB_LOCK_RW_N
	DB_LOCK_SET_TIMEOUT
	DB_LOCK_SWITCH
	DB_LOCK_TIMEOUT
	DB_LOCK_TRADE
	DB_LOCK_UPGRADE
	DB_LOCK_UPGRADE_WRITE
	DB_LOCK_YOUNGEST
	DB_LOGCHKSUM
	DB_LOGC_BUF_SIZE
	DB_LOGFILEID_INVALID
	DB_LOGMAGIC
	DB_LOGOLDVER
	DB_LOGVERSION
	DB_LOGVERSION_LATCHING
	DB_LOG_AUTOREMOVE
	DB_LOG_AUTO_REMOVE
	DB_LOG_BUFFER_FULL
	DB_LOG_CHKPNT
	DB_LOG_COMMIT
	DB_LOG_DIRECT
	DB_LOG_DISK
	DB_LOG_DSYNC
	DB_LOG_INMEMORY
	DB_LOG_IN_MEMORY
	DB_LOG_LOCKED
	DB_LOG_NOCOPY
	DB_LOG_NOT_DURABLE
	DB_LOG_PERM
	DB_LOG_RESEND
	DB_LOG_SILENT_ERR
	DB_LOG_WRNOSYNC
	DB_LOG_ZERO
	DB_MAX_PAGES
	DB_MAX_RECORDS
	DB_MPOOL_CLEAN
	DB_MPOOL_CREATE
	DB_MPOOL_DIRTY
	DB_MPOOL_DISCARD
	DB_MPOOL_EDIT
	DB_MPOOL_EXTENT
	DB_MPOOL_FREE
	DB_MPOOL_LAST
	DB_MPOOL_NEW
	DB_MPOOL_NEW_GROUP
	DB_MPOOL_NOFILE
	DB_MPOOL_NOLOCK
	DB_MPOOL_PRIVATE
	DB_MPOOL_TRY
	DB_MPOOL_UNLINK
	DB_MULTIPLE
	DB_MULTIPLE_KEY
	DB_MULTIVERSION
	DB_MUTEXDEBUG
	DB_MUTEXLOCKS
	DB_MUTEX_ALLOCATED
	DB_MUTEX_LOCKED
	DB_MUTEX_LOGICAL_LOCK
	DB_MUTEX_PROCESS_ONLY
	DB_MUTEX_SELF_BLOCK
	DB_MUTEX_SHARED
	DB_MUTEX_THREAD
	DB_NEEDSPLIT
	DB_NEXT
	DB_NEXT_DUP
	DB_NEXT_NODUP
	DB_NOCOPY
	DB_NODUPDATA
	DB_NOLOCKING
	DB_NOMMAP
	DB_NOORDERCHK
	DB_NOOVERWRITE
	DB_NOPANIC
	DB_NORECURSE
	DB_NOSERVER
	DB_NOSERVER_HOME
	DB_NOSERVER_ID
	DB_NOSYNC
	DB_NOTFOUND
	DB_NO_AUTO_COMMIT
	DB_ODDFILESIZE
	DB_OK_BTREE
	DB_OK_HASH
	DB_OK_QUEUE
	DB_OK_RECNO
	DB_OLD_VERSION
	DB_OPEN_CALLED
	DB_OPFLAGS_MASK
	DB_ORDERCHKONLY
	DB_OVERWRITE
	DB_OVERWRITE_DUP
	DB_PAD
	DB_PAGEYIELD
	DB_PAGE_LOCK
	DB_PAGE_NOTFOUND
	DB_PANIC_ENVIRONMENT
	DB_PERMANENT
	DB_POSITION
	DB_POSITIONI
	DB_PREV
	DB_PREV_DUP
	DB_PREV_NODUP
	DB_PRINTABLE
	DB_PRIORITY_DEFAULT
	DB_PRIORITY_HIGH
	DB_PRIORITY_LOW
	DB_PRIORITY_UNCHANGED
	DB_PRIORITY_VERY_HIGH
	DB_PRIORITY_VERY_LOW
	DB_PRIVATE
	DB_PR_HEADERS
	DB_PR_PAGE
	DB_PR_RECOVERYTEST
	DB_QAMMAGIC
	DB_QAMOLDVER
	DB_QAMVERSION
	DB_QUEUE
	DB_RDONLY
	DB_RDWRMASTER
	DB_READ_COMMITTED
	DB_READ_UNCOMMITTED
	DB_RECNO
	DB_RECNUM
	DB_RECORDCOUNT
	DB_RECORD_LOCK
	DB_RECOVER
	DB_RECOVER_FATAL
	DB_REGION_ANON
	DB_REGION_INIT
	DB_REGION_MAGIC
	DB_REGION_NAME
	DB_REGISTER
	DB_REGISTERED
	DB_RENAMEMAGIC
	DB_RENUMBER
	DB_REPFLAGS_MASK
	DB_REPMGR_ACKS_ALL
	DB_REPMGR_ACKS_ALL_PEERS
	DB_REPMGR_ACKS_NONE
	DB_REPMGR_ACKS_ONE
	DB_REPMGR_ACKS_ONE_PEER
	DB_REPMGR_ACKS_QUORUM
	DB_REPMGR_CONF_2SITE_STRICT
	DB_REPMGR_CONNECTED
	DB_REPMGR_DISCONNECTED
	DB_REPMGR_PEER
	DB_REP_ACK_TIMEOUT
	DB_REP_ANYWHERE
	DB_REP_BULKOVF
	DB_REP_CHECKPOINT_DELAY
	DB_REP_CLIENT
	DB_REP_CONF_BULK
	DB_REP_CONF_DELAYCLIENT
	DB_REP_CONF_INMEM
	DB_REP_CONF_LEASE
	DB_REP_CONF_NOAUTOINIT
	DB_REP_CONF_NOWAIT
	DB_REP_CONNECTION_RETRY
	DB_REP_CREATE
	DB_REP_DEFAULT_PRIORITY
	DB_REP_DUPMASTER
	DB_REP_EGENCHG
	DB_REP_ELECTION
	DB_REP_ELECTION_RETRY
	DB_REP_ELECTION_TIMEOUT
	DB_REP_FULL_ELECTION
	DB_REP_FULL_ELECTION_TIMEOUT
	DB_REP_HANDLE_DEAD
	DB_REP_HEARTBEAT_MONITOR
	DB_REP_HEARTBEAT_SEND
	DB_REP_HOLDELECTION
	DB_REP_IGNORE
	DB_REP_ISPERM
	DB_REP_JOIN_FAILURE
	DB_REP_LEASE_EXPIRED
	DB_REP_LEASE_TIMEOUT
	DB_REP_LOCKOUT
	DB_REP_LOGREADY
	DB_REP_LOGSONLY
	DB_REP_MASTER
	DB_REP_NEWMASTER
	DB_REP_NEWSITE
	DB_REP_NOBUFFER
	DB_REP_NOTPERM
	DB_REP_OUTDATED
	DB_REP_PAGEDONE
	DB_REP_PAGELOCKED
	DB_REP_PERMANENT
	DB_REP_REREQUEST
	DB_REP_STARTUPDONE
	DB_REP_UNAVAIL
	DB_REVSPLITOFF
	DB_RMW
	DB_RPCCLIENT
	DB_RPC_SERVERPROG
	DB_RPC_SERVERVERS
	DB_RUNRECOVERY
	DB_SALVAGE
	DB_SA_SKIPFIRSTKEY
	DB_SA_UNKNOWNKEY
	DB_SECONDARY_BAD
	DB_SEQUENCE_OLDVER
	DB_SEQUENCE_VERSION
	DB_SEQUENTIAL
	DB_SEQ_DEC
	DB_SEQ_INC
	DB_SEQ_RANGE_SET
	DB_SEQ_WRAP
	DB_SEQ_WRAPPED
	DB_SET
	DB_SET_LOCK_TIMEOUT
	DB_SET_LTE
	DB_SET_RANGE
	DB_SET_RECNO
	DB_SET_REG_TIMEOUT
	DB_SET_TXN_NOW
	DB_SET_TXN_TIMEOUT
	DB_SHALLOW_DUP
	DB_SNAPSHOT
	DB_SPARE_FLAG
	DB_STAT_ALL
	DB_STAT_CLEAR
	DB_STAT_LOCK_CONF
	DB_STAT_LOCK_LOCKERS
	DB_STAT_LOCK_OBJECTS
	DB_STAT_LOCK_PARAMS
	DB_STAT_MEMP_HASH
	DB_STAT_MEMP_NOERROR
	DB_STAT_NOERROR
	DB_STAT_SUBSYSTEM
	DB_ST_DUPOK
	DB_ST_DUPSET
	DB_ST_DUPSORT
	DB_ST_IS_RECNO
	DB_ST_OVFL_LEAF
	DB_ST_RECNUM
	DB_ST_RELEN
	DB_ST_TOPLEVEL
	DB_SURPRISE_KID
	DB_SWAPBYTES
	DB_SYSTEM_MEM
	DB_TEMPORARY
	DB_TEST_ELECTINIT
	DB_TEST_ELECTSEND
	DB_TEST_ELECTVOTE1
	DB_TEST_ELECTVOTE2
	DB_TEST_ELECTWAIT1
	DB_TEST_ELECTWAIT2
	DB_TEST_POSTDESTROY
	DB_TEST_POSTLOG
	DB_TEST_POSTLOGMETA
	DB_TEST_POSTOPEN
	DB_TEST_POSTRENAME
	DB_TEST_POSTSYNC
	DB_TEST_PREDESTROY
	DB_TEST_PREOPEN
	DB_TEST_PRERENAME
	DB_TEST_RECYCLE
	DB_TEST_SUBDB_LOCKS
	DB_THREAD
	DB_THREADID_STRLEN
	DB_TIMEOUT
	DB_TIME_NOTGRANTED
	DB_TRUNCATE
	DB_TXNMAGIC
	DB_TXNVERSION
	DB_TXN_ABORT
	DB_TXN_APPLY
	DB_TXN_BACKWARD_ROLL
	DB_TXN_CKP
	DB_TXN_FORWARD_ROLL
	DB_TXN_LOCK
	DB_TXN_LOCK_2PL
	DB_TXN_LOCK_MASK
	DB_TXN_LOCK_OPTIMIST
	DB_TXN_LOCK_OPTIMISTIC
	DB_TXN_LOG_MASK
	DB_TXN_LOG_REDO
	DB_TXN_LOG_UNDO
	DB_TXN_LOG_UNDOREDO
	DB_TXN_NOSYNC
	DB_TXN_NOT_DURABLE
	DB_TXN_NOWAIT
	DB_TXN_OPENFILES
	DB_TXN_POPENFILES
	DB_TXN_PRINT
	DB_TXN_REDO
	DB_TXN_SNAPSHOT
	DB_TXN_SYNC
	DB_TXN_UNDO
	DB_TXN_WAIT
	DB_TXN_WRITE_NOSYNC
	DB_UNKNOWN
	DB_UNREF
	DB_UPDATE_SECONDARY
	DB_UPGRADE
	DB_USERCOPY_GETDATA
	DB_USERCOPY_SETDATA
	DB_USE_ENVIRON
	DB_USE_ENVIRON_ROOT
	DB_VERB_CHKPOINT
	DB_VERB_DEADLOCK
	DB_VERB_FILEOPS
	DB_VERB_FILEOPS_ALL
	DB_VERB_RECOVERY
	DB_VERB_REGISTER
	DB_VERB_REPLICATION
	DB_VERB_REPMGR_CONNFAIL
	DB_VERB_REPMGR_MISC
	DB_VERB_REP_ELECT
	DB_VERB_REP_LEASE
	DB_VERB_REP_MISC
	DB_VERB_REP_MSGS
	DB_VERB_REP_SYNC
	DB_VERB_REP_TEST
	DB_VERB_WAITSFOR
	DB_VERIFY
	DB_VERIFY_BAD
	DB_VERIFY_FATAL
	DB_VERIFY_PARTITION
	DB_VERSION_MAJOR
	DB_VERSION_MINOR
	DB_VERSION_MISMATCH
	DB_VERSION_PATCH
	DB_VERSION_STRING
	DB_VRFY_FLAGMASK
	DB_WRITECURSOR
	DB_WRITELOCK
	DB_WRITEOPEN
	DB_WRNOSYNC
	DB_XA_CREATE
	DB_XIDDATASIZE
	DB_YIELDCPU
	DB_debug_FLAG
	DB_user_BEGIN
	);

sub AUTOLOAD {
    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    my ($error, $val) = constant($constname);
    Carp::croak $error if $error;
    no strict 'refs';
    *{$AUTOLOAD} = sub { $val };
    goto &{$AUTOLOAD};
}         

#bootstrap BerkeleyDB $VERSION;
if ($use_XSLoader)
  { XSLoader::load("BerkeleyDB", $VERSION)}
else
  { bootstrap BerkeleyDB $VERSION }  

# Preloaded methods go here.


sub ParseParameters($@)
{
    my ($default, @rest) = @_ ;
    my (%got) = %$default ;
    my (@Bad) ;
    my ($key, $value) ;
    my $sub = (caller(1))[3] ;
    my %options = () ;
    local ($Carp::CarpLevel) = 1 ;

    # allow the options to be passed as a hash reference or
    # as the complete hash.
    if (@rest == 1) {

        croak "$sub: parameter is not a reference to a hash"
            if ref $rest[0] ne "HASH" ;

        %options = %{ $rest[0] } ;
    }
    elsif (@rest >= 2 && @rest % 2 == 0) {
        %options = @rest ;
    }
    elsif (@rest > 0) {
	    croak "$sub: malformed option list";
    }

    while (($key, $value) = each %options)
    {
	$key =~ s/^-// ;

        if (exists $default->{$key})
          { $got{$key} = $value }
        else
	  { push (@Bad, $key) }
    }
    
    if (@Bad) {
        my ($bad) = join(", ", @Bad) ;
        croak "unknown key value(s) $bad" ;
    }

    return \%got ;
}

sub parseEncrypt
{
    my $got = shift ;


    if (defined $got->{Encrypt}) {
    	croak("Encrypt parameter must be a hash reference")
            if !ref $got->{Encrypt} || ref $got->{Encrypt} ne 'HASH' ;

	my %config = %{ $got->{Encrypt} } ;

        my $p = BerkeleyDB::ParseParameters({
					Password	=> undef,
					Flags		=> undef,
				}, %config);

        croak("Must specify Password and Flags with Encrypt parameter")
	    if ! (defined $p->{Password} && defined $p->{Flags});

        $got->{"Enc_Passwd"} = $p->{Password};
        $got->{"Enc_Flags"} = $p->{Flags};
    }
}

use UNIVERSAL qw( isa ) ;

sub env_remove
{
    # Usage:
    #
    #	$env = BerkeleyDB::env_remove
    #			[ -Home		=> $path, ]
    #			[ -Config	=> { name => value, name => value }
    #			[ -Flags	=> DB_INIT_LOCK| ]
    #			;

    my $got = BerkeleyDB::ParseParameters({
					Home		=> undef,
					Flags     	=> 0,
					Config		=> undef,
					}, @_) ;

    if (defined $got->{Config}) {
    	croak("Config parameter must be a hash reference")
            if ! ref $got->{Config} eq 'HASH' ;

        @BerkeleyDB::a = () ;
	my $k = "" ; my $v = "" ;
	while (($k, $v) = each %{$got->{Config}}) {
	    push @BerkeleyDB::a, "$k\t$v" ;
	}

        $got->{"Config"} = pack("p*", @BerkeleyDB::a, undef) 
	    if @BerkeleyDB::a ;
    }

    return _env_remove($got) ;
}

sub db_remove
{
    my $got = BerkeleyDB::ParseParameters(
		      {
			Filename 	=> undef,
			Subname		=> undef,
			Flags		=> 0,
			Env		=> undef,
			Txn		=> undef,
		      }, @_) ;

    croak("Must specify a filename")
	if ! defined $got->{Filename} ;

    croak("Env not of type BerkeleyDB::Env")
	if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');

    return _db_remove($got);
}

sub db_rename
{
    my $got = BerkeleyDB::ParseParameters(
		      {
			Filename 	=> undef,
			Subname		=> undef,
			Newname		=> undef,
			Flags		=> 0,
			Env		=> undef,
			Txn		=> undef,
		      }, @_) ;

    croak("Env not of type BerkeleyDB::Env")
	if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');

    croak("Must specify a filename")
	if ! defined $got->{Filename} ;

    #croak("Must specify a Subname")
    #if ! defined $got->{Subname} ;

    croak("Must specify a Newname")
	if ! defined $got->{Newname} ;

    return _db_rename($got);
}

sub db_verify
{
    my $got = BerkeleyDB::ParseParameters(
		      {
			Filename 	=> undef,
			Subname		=> undef,
			Outfile		=> undef,
			Flags		=> 0,
			Env		=> undef,
		      }, @_) ;

    croak("Env not of type BerkeleyDB::Env")
	if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');

    croak("Must specify a filename")
	if ! defined $got->{Filename} ;

    return _db_verify($got);
}

package BerkeleyDB::Env ;

use UNIVERSAL qw( isa ) ;
use Carp ;
use IO::File;
use vars qw( %valid_config_keys ) ;

sub isaFilehandle
{
    my $fh = shift ;

    return ((isa($fh,'GLOB') or isa(\$fh,'GLOB')) and defined fileno($fh) )

}

%valid_config_keys = map { $_, 1 } qw( DB_DATA_DIR DB_LOG_DIR DB_TEMP_DIR
DB_TMP_DIR ) ;

sub new
{
    # Usage:
    #
    #	$env = new BerkeleyDB::Env
    #			[ -Home		=> $path, ]
    #			[ -Mode		=> mode, ]
    #			[ -Config	=> { name => value, name => value }
    #			[ -ErrFile   	=> filename, ]
    #			[ -ErrPrefix 	=> "string", ]
    #			[ -Flags	=> DB_INIT_LOCK| ]
    #			[ -Set_Flags	=> $flags,]
    #			[ -Cachesize	=> number ]
    #			[ -LockDetect	=>  ]
    #			[ -Verbose	=> boolean ]
    #			[ -Encrypt	=> { Password => string, Flags => value}
    #
    #			;

    my $pkg = shift ;
    my $got = BerkeleyDB::ParseParameters({
					Home		=> undef,
					Server		=> undef,
					Mode		=> 0666,
					ErrFile  	=> undef,
					MsgFile  	=> undef,
					ErrPrefix 	=> undef,
					Flags     	=> 0,
					SetFlags     	=> 0,
					Cachesize     	=> 0,
					LockDetect     	=> 0,
					Verbose		=> 0,
					Config		=> undef,
					Encrypt		=> undef,
					SharedMemKey	=> undef,
					ThreadCount	=> 0,
					}, @_) ;

    my $errfile  = $got->{ErrFile} ;				
    if (defined $got->{ErrFile}) {
	if (!isaFilehandle($got->{ErrFile})) {
	    my $handle = new IO::File ">$got->{ErrFile}"
		or croak "Cannot open file $got->{ErrFile}: $!\n" ;
	    $errfile = $got->{ErrFile} = $handle ;
	}
    }

    if (defined $got->{MsgFile}) {
        my $msgfile  = $got->{MsgFile} ;				
	if (!isaFilehandle($msgfile)) {
	    my $handle = new IO::File ">$msgfile"
		or croak "Cannot open file $msgfile: $!\n" ;
	    $got->{MsgFile} = $handle ;
	}
    }

    my %config ;
    if (defined $got->{Config}) {
    	croak("Config parameter must be a hash reference")
            if ! ref $got->{Config} eq 'HASH' ;

	%config = %{ $got->{Config} } ;
        @BerkeleyDB::a = () ;
	my $k = "" ; my $v = "" ;
	while (($k, $v) = each %config) {
	    if ($BerkeleyDB::db_version >= 3.1 && ! $valid_config_keys{$k} ){
	        $BerkeleyDB::Error = "illegal name-value pair: $k $v\n" ; 
                croak $BerkeleyDB::Error ;
	    }
	    push @BerkeleyDB::a, "$k\t$v" ;
	    $got->{$k} = $v;
	}

        $got->{"Config"} = pack("p*", @BerkeleyDB::a, undef) 
	    if @BerkeleyDB::a ;
    }

    BerkeleyDB::parseEncrypt($got);

    my ($addr) = _db_appinit($pkg, $got, $errfile);
    my $obj ;
    $obj = bless [$addr] , $pkg if $addr ;
#    if ($obj && $BerkeleyDB::db_version >= 3.1 && keys %config) {
#	my ($k, $v);
#	while (($k, $v) = each %config) {
#	    if ($k eq 'DB_DATA_DIR')
#	      { $obj->set_data_dir($v) }
#	    elsif ($k eq 'DB_LOG_DIR')
#	      { $obj->set_lg_dir($v) }
#	    elsif ($k eq 'DB_TEMP_DIR' || $k eq 'DB_TMP_DIR')
#	      { $obj->set_tmp_dir($v) }
#	    else {
#	      $BerkeleyDB::Error = "illegal name-value pair: $k $v\n" ; 
#              croak $BerkeleyDB::Error 
#            }
#	}
#    }
    return $obj ;
}


sub TxnMgr
{
    my $env = shift ;
    my ($addr) = $env->_TxnMgr() ;
    my $obj ;
    $obj = bless [$addr, $env] , "BerkeleyDB::TxnMgr" if $addr ;
    return $obj ;
}

sub txn_begin
{
    my $env = shift ;
    my ($addr) = $env->_txn_begin(@_) ;
    my $obj ;
    $obj = bless [$addr, $env] , "BerkeleyDB::Txn" if $addr ;
    return $obj ;
}

sub DESTROY
{
    my $self = shift ;
    $self->_DESTROY() ;
}

package BerkeleyDB::Hash ;

use vars qw(@ISA) ;
@ISA = qw( BerkeleyDB::Common BerkeleyDB::_tiedHash ) ;
use UNIVERSAL qw( isa ) ;
use Carp ;

sub new
{
    my $self = shift ;
    my $got = BerkeleyDB::ParseParameters(
		      {
			# Generic Stuff
			Filename 	=> undef,
			Subname		=> undef,
			#Flags		=> BerkeleyDB::DB_CREATE(),
			Flags		=> 0,
			Property	=> 0,
			Mode		=> 0666,
			Cachesize 	=> 0,
			Lorder 		=> 0,
			Pagesize 	=> 0,
			Env		=> undef,
			#Tie 		=> undef,
			Txn		=> undef,
			Encrypt		=> undef,

			# Hash specific
			Ffactor		=> 0,
			Nelem 		=> 0,
			Hash 		=> undef,
			DupCompare	=> undef,

			# BerkeleyDB specific
			ReadKey		=> undef,
			WriteKey	=> undef,
			ReadValue	=> undef,
			WriteValue	=> undef,
		      }, @_) ;

    croak("Env not of type BerkeleyDB::Env")
	if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');

    croak("Txn not of type BerkeleyDB::Txn")
	if defined $got->{Txn} and ! isa($got->{Txn},'BerkeleyDB::Txn');

    croak("-Tie needs a reference to a hash")
	if defined $got->{Tie} and $got->{Tie} !~ /HASH/ ;

    BerkeleyDB::parseEncrypt($got);

    my ($addr) = _db_open_hash($self, $got);
    my $obj ;
    if ($addr) {
        $obj = bless [$addr] , $self ;
	push @{ $obj }, $got->{Env} if $got->{Env} ;
        $obj->Txn($got->{Txn}) 
            if $got->{Txn} ;
    }
    return $obj ;
}

*TIEHASH = \&new ;

 
package BerkeleyDB::Btree ;

use vars qw(@ISA) ;
@ISA = qw( BerkeleyDB::Common BerkeleyDB::_tiedHash ) ;
use UNIVERSAL qw( isa ) ;
use Carp ;

sub new
{
    my $self = shift ;
    my $got = BerkeleyDB::ParseParameters(
		      {
			# Generic Stuff
			Filename 	=> undef,
			Subname		=> undef,
			#Flags		=> BerkeleyDB::DB_CREATE(),
			Flags		=> 0,
			Property	=> 0,
			Mode		=> 0666,
			Cachesize 	=> 0,
			Lorder 		=> 0,
			Pagesize 	=> 0,
			Env		=> undef,
			#Tie 		=> undef,
			Txn		=> undef,
			Encrypt		=> undef,

			# Btree specific
			Minkey		=> 0,
			Compare		=> undef,
			DupCompare	=> undef,
			Prefix 		=> undef,
			set_bt_compress	=> undef,
		      }, @_) ;

    croak("Env not of type BerkeleyDB::Env")
        if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');

    croak("Txn not of type BerkeleyDB::Txn")
        if defined $got->{Txn} and ! isa($got->{Txn},'BerkeleyDB::Txn');

    croak("-Tie needs a reference to a hash")
        if defined $got->{Tie} and $got->{Tie} !~ /HASH/ ;

#    if (defined $got->{set_bt_compress} )
#    {
#
#        croak("-set_bt_compress needs a reference to a 2-element array")
#            if $got->{set_bt_compress} !~ /ARRAY/ ||
#
#        croak("-set_bt_compress needs a reference to a 2-element array")
#            if $got->{set_bt_compress} !~ /ARRAY/ ||
#               @{ $got->{set_bt_compress} } != 2;
#
#        $got->{"_btcompress1"} =  $got->{set_bt_compress}[0] 
#            if defined $got->{set_bt_compress}[0];
#
#        $got->{"_btcompress2"} =  $got->{set_bt_compress}[1] 
#            if defined $got->{set_bt_compress}[1];
#    }

    BerkeleyDB::parseEncrypt($got);

    my ($addr) = _db_open_btree($self, $got);
    my $obj ;
    if ($addr) {
        $obj = bless [$addr] , $self ;
	push @{ $obj }, $got->{Env} if $got->{Env} ;
        $obj->Txn($got->{Txn}) 
            if $got->{Txn} ;
    }
    return $obj ;
}

*BerkeleyDB::Btree::TIEHASH = \&BerkeleyDB::Btree::new ;


package BerkeleyDB::Recno ;

use vars qw(@ISA) ;
@ISA = qw( BerkeleyDB::Common BerkeleyDB::_tiedArray ) ;
use UNIVERSAL qw( isa ) ;
use Carp ;

sub new
{
    my $self = shift ;
    my $got = BerkeleyDB::ParseParameters(
		      {
			# Generic Stuff
			Filename 	=> undef,
			Subname		=> undef,
			#Flags		=> BerkeleyDB::DB_CREATE(),
			Flags		=> 0,
			Property	=> 0,
			Mode		=> 0666,
			Cachesize 	=> 0,
			Lorder 		=> 0,
			Pagesize 	=> 0,
			Env		=> undef,
			#Tie 		=> undef,
			Txn		=> undef,
			Encrypt		=> undef,

			# Recno specific
			Delim		=> undef,
			Len		=> undef,
			Pad		=> undef,
			Source 		=> undef,
			ArrayBase 	=> 1, # lowest index in array
		      }, @_) ;

    croak("Env not of type BerkeleyDB::Env")
	if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');

    croak("Txn not of type BerkeleyDB::Txn")
	if defined $got->{Txn} and ! isa($got->{Txn},'BerkeleyDB::Txn');

    croak("Tie needs a reference to an array")
	if defined $got->{Tie} and $got->{Tie} !~ /ARRAY/ ;

    croak("ArrayBase can only be 0 or 1, parsed $got->{ArrayBase}")
	if $got->{ArrayBase} != 1 and $got->{ArrayBase} != 0 ;


    BerkeleyDB::parseEncrypt($got);

    $got->{Fname} = $got->{Filename} if defined $got->{Filename} ;

    my ($addr) = _db_open_recno($self, $got);
    my $obj ;
    if ($addr) {
        $obj = bless [$addr] , $self ;
	push @{ $obj }, $got->{Env} if $got->{Env} ;
        $obj->Txn($got->{Txn}) 
            if $got->{Txn} ;
    }	
    return $obj ;
}

*BerkeleyDB::Recno::TIEARRAY = \&BerkeleyDB::Recno::new ;
*BerkeleyDB::Recno::db_stat = \&BerkeleyDB::Btree::db_stat ;

package BerkeleyDB::Queue ;

use vars qw(@ISA) ;
@ISA = qw( BerkeleyDB::Common BerkeleyDB::_tiedArray ) ;
use UNIVERSAL qw( isa ) ;
use Carp ;

sub new
{
    my $self = shift ;
    my $got = BerkeleyDB::ParseParameters(
		      {
			# Generic Stuff
			Filename 	=> undef,
			Subname		=> undef,
			#Flags		=> BerkeleyDB::DB_CREATE(),
			Flags		=> 0,
			Property	=> 0,
			Mode		=> 0666,
			Cachesize 	=> 0,
			Lorder 		=> 0,
			Pagesize 	=> 0,
			Env		=> undef,
			#Tie 		=> undef,
			Txn		=> undef,
			Encrypt		=> undef,

			# Queue specific
			Len		=> undef,
			Pad		=> undef,
			ArrayBase 	=> 1, # lowest index in array
			ExtentSize      => undef,
		      }, @_) ;

    croak("Env not of type BerkeleyDB::Env")
	if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');

    croak("Txn not of type BerkeleyDB::Txn")
	if defined $got->{Txn} and ! isa($got->{Txn},'BerkeleyDB::Txn');

    croak("Tie needs a reference to an array")
	if defined $got->{Tie} and $got->{Tie} !~ /ARRAY/ ;

    croak("ArrayBase can only be 0 or 1, parsed $got->{ArrayBase}")
	if $got->{ArrayBase} != 1 and $got->{ArrayBase} != 0 ;

    BerkeleyDB::parseEncrypt($got);

    $got->{Fname} = $got->{Filename} if defined $got->{Filename} ;

    my ($addr) = _db_open_queue($self, $got);
    my $obj ;
    if ($addr) {
        $obj = bless [$addr] , $self ;
	push @{ $obj }, $got->{Env} if $got->{Env} ;
        $obj->Txn($got->{Txn})
            if $got->{Txn} ;
    }	
    return $obj ;
}

*BerkeleyDB::Queue::TIEARRAY = \&BerkeleyDB::Queue::new ;

sub UNSHIFT
{
    my $self = shift;
    croak "unshift is unsupported with Queue databases";
}

## package BerkeleyDB::Text ;
## 
## use vars qw(@ISA) ;
## @ISA = qw( BerkeleyDB::Common BerkeleyDB::_tiedArray ) ;
## use UNIVERSAL qw( isa ) ;
## use Carp ;
## 
## sub new
## {
##     my $self = shift ;
##     my $got = BerkeleyDB::ParseParameters(
## 		      {
## 			# Generic Stuff
## 			Filename 	=> undef,
## 			#Flags		=> BerkeleyDB::DB_CREATE(),
## 			Flags		=> 0,
## 			Property	=> 0,
## 			Mode		=> 0666,
## 			Cachesize 	=> 0,
## 			Lorder 		=> 0,
## 			Pagesize 	=> 0,
## 			Env		=> undef,
## 			#Tie 		=> undef,
## 			Txn		=> undef,
## 
## 			# Recno specific
## 			Delim		=> undef,
## 			Len		=> undef,
## 			Pad		=> undef,
## 			Btree 		=> undef,
## 		      }, @_) ;
## 
##     croak("Env not of type BerkeleyDB::Env")
## 	if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');
## 
##     croak("Txn not of type BerkeleyDB::Txn")
## 	if defined $got->{Txn} and ! isa($got->{Txn},'BerkeleyDB::Txn');
## 
##     croak("-Tie needs a reference to an array")
## 	if defined $got->{Tie} and $got->{Tie} !~ /ARRAY/ ;
## 
##     # rearange for recno
##     $got->{Source} = $got->{Filename} if defined $got->{Filename} ;
##     delete $got->{Filename} ;
##     $got->{Fname} = $got->{Btree} if defined $got->{Btree} ;
##     return BerkeleyDB::Recno::_db_open_recno($self, $got);
## }
## 
## *BerkeleyDB::Text::TIEARRAY = \&BerkeleyDB::Text::new ;
## *BerkeleyDB::Text::db_stat = \&BerkeleyDB::Btree::db_stat ;

package BerkeleyDB::Unknown ;

use vars qw(@ISA) ;
@ISA = qw( BerkeleyDB::Common BerkeleyDB::_tiedArray ) ;
use UNIVERSAL qw( isa ) ;
use Carp ;

sub new
{
    my $self = shift ;
    my $got = BerkeleyDB::ParseParameters(
		      {
			# Generic Stuff
			Filename 	=> undef,
			Subname		=> undef,
			#Flags		=> BerkeleyDB::DB_CREATE(),
			Flags		=> 0,
			Property	=> 0,
			Mode		=> 0666,
			Cachesize 	=> 0,
			Lorder 		=> 0,
			Pagesize 	=> 0,
			Env		=> undef,
			#Tie 		=> undef,
			Txn		=> undef,
			Encrypt		=> undef,

		      }, @_) ;

    croak("Env not of type BerkeleyDB::Env")
	if defined $got->{Env} and ! isa($got->{Env},'BerkeleyDB::Env');

    croak("Txn not of type BerkeleyDB::Txn")
	if defined $got->{Txn} and ! isa($got->{Txn},'BerkeleyDB::Txn');

    croak("-Tie needs a reference to a hash")
	if defined $got->{Tie} and $got->{Tie} !~ /HASH/ ;

    BerkeleyDB::parseEncrypt($got);

    my ($addr, $type) = _db_open_unknown($got);
    my $obj ;
    if ($addr) {
        $obj = bless [$addr], "BerkeleyDB::$type" ;
	push @{ $obj }, $got->{Env} if $got->{Env} ;
        $obj->Txn($got->{Txn})
            if $got->{Txn} ;
    }	
    return $obj ;
}


package BerkeleyDB::_tiedHash ;

use Carp ;

#sub TIEHASH  
#{ 
#    my $self = shift ;
#    my $db_object = shift ;
#
#print "Tiehash REF=[$self] [" . (ref $self) . "]\n" ;
#
#    return bless { Obj => $db_object}, $self ; 
#}

sub Tie
{
    # Usage:
    #
    #   $db->Tie \%hash ;
    #

    my $self = shift ;

    #print "Tie method REF=[$self] [" . (ref $self) . "]\n" ;

    croak("usage \$x->Tie \\%hash\n") unless @_ ;
    my $ref  = shift ; 

    croak("Tie needs a reference to a hash")
	if defined $ref and $ref !~ /HASH/ ;

    #tie %{ $ref }, ref($self), $self ; 
    tie %{ $ref }, "BerkeleyDB::_tiedHash", $self ; 
    return undef ;
}

 
sub TIEHASH  
{ 
    my $self = shift ;
    my $db_object = shift ;
    #return bless $db_object, 'BerkeleyDB::Common' ; 
    return $db_object ;
}

sub STORE
{
    my $self = shift ;
    my $key  = shift ;
    my $value = shift ;

    $self->db_put($key, $value) ;
}

sub FETCH
{
    my $self = shift ;
    my $key  = shift ;
    my $value = undef ;
    $self->db_get($key, $value) ;

    return $value ;
}

sub EXISTS
{
    my $self = shift ;
    my $key  = shift ;
    my $value = undef ;
    $self->db_get($key, $value) == 0 ;
}

sub DELETE
{
    my $self = shift ;
    my $key  = shift ;
    $self->db_del($key) ;
}

sub CLEAR
{
    my $self = shift ;
    my ($key, $value) = (0, 0) ;
    my $cursor = $self->_db_write_cursor() ;
    while ($cursor->c_get($key, $value, BerkeleyDB::DB_PREV()) == 0) 
	{ $cursor->c_del() }
}

#sub DESTROY
#{
#    my $self = shift ;
#    print "BerkeleyDB::_tieHash::DESTROY\n" ;
#    $self->{Cursor}->c_close() if $self->{Cursor} ;
#}

package BerkeleyDB::_tiedArray ;

use Carp ;

sub Tie
{
    # Usage:
    #
    #   $db->Tie \@array ;
    #

    my $self = shift ;

    #print "Tie method REF=[$self] [" . (ref $self) . "]\n" ;

    croak("usage \$x->Tie \\%hash\n") unless @_ ;
    my $ref  = shift ; 

    croak("Tie needs a reference to an array")
	if defined $ref and $ref !~ /ARRAY/ ;

    #tie %{ $ref }, ref($self), $self ; 
    tie @{ $ref }, "BerkeleyDB::_tiedArray", $self ; 
    return undef ;
}

 
#sub TIEARRAY  
#{ 
#    my $self = shift ;
#    my $db_object = shift ;
#
#print "Tiearray REF=[$self] [" . (ref $self) . "]\n" ;
#
#    return bless { Obj => $db_object}, $self ; 
#}

sub TIEARRAY  
{ 
    my $self = shift ;
    my $db_object = shift ;
    #return bless $db_object, 'BerkeleyDB::Common' ; 
    return $db_object ;
}

sub STORE
{
    my $self = shift ;
    my $key  = shift ;
    my $value = shift ;

    $self->db_put($key, $value) ;
}

sub FETCH
{
    my $self = shift ;
    my $key  = shift ;
    my $value = undef ;
    $self->db_get($key, $value) ;

    return $value ;
}

*CLEAR =    \&BerkeleyDB::_tiedHash::CLEAR ;
*FIRSTKEY = \&BerkeleyDB::_tiedHash::FIRSTKEY ;
*NEXTKEY =  \&BerkeleyDB::_tiedHash::NEXTKEY ;

sub EXTEND {} # don't do anything with EXTEND


sub SHIFT
{
    my $self = shift;
    my ($key, $value) = (0, 0) ;
    my $cursor = $self->_db_write_cursor() ;
    return undef if $cursor->c_get($key, $value, BerkeleyDB::DB_FIRST()) != 0 ;
    return undef if $cursor->c_del() != 0 ;

    return $value ;
}


sub UNSHIFT
{
    my $self = shift;
    if (@_)
    {
        my ($key, $value) = (0, 0) ;
        my $cursor = $self->_db_write_cursor() ;
        my $status = $cursor->c_get($key, $value, BerkeleyDB::DB_FIRST()) ;
        if ($status == 0)
        {
            foreach $value (reverse @_)
            {
	        $key = 0 ;
	        $cursor->c_put($key, $value, BerkeleyDB::DB_BEFORE()) ;
            }
        }
        elsif ($status == BerkeleyDB::DB_NOTFOUND())
        {
	    $key = 0 ;
            foreach $value (@_)
            {
	        $self->db_put($key++, $value) ;
            }
        }
    }
}

sub PUSH
{
    my $self = shift;
    if (@_)
    {
        my ($key, $value) = (-1, 0) ;
        my $cursor = $self->_db_write_cursor() ;
        my $status = $cursor->c_get($key, $value, BerkeleyDB::DB_LAST()) ;
        if ($status == 0 || $status == BerkeleyDB::DB_NOTFOUND())
	{
            $key = -1 if $status != 0 and $self->type != BerkeleyDB::DB_RECNO() ;
            foreach $value (@_)
	    {
	        ++ $key ;
	        $status = $self->db_put($key, $value) ;
	    }
	}

# can use this when DB_APPEND is fixed.
#        foreach $value (@_)
#        {
#	    my $status = $cursor->c_put($key, $value, BerkeleyDB::DB_AFTER()) ;
#print "[$status]\n" ;
#        }
    }
}

sub POP
{
    my $self = shift;
    my ($key, $value) = (0, 0) ;
    my $cursor = $self->_db_write_cursor() ;
    return undef if $cursor->c_get($key, $value, BerkeleyDB::DB_LAST()) != 0 ;
    return undef if $cursor->c_del() != 0 ;

    return $value ;
}

sub SPLICE
{
    my $self = shift;
    croak "SPLICE is not implemented yet" ;
}

*shift = \&SHIFT ;
*unshift = \&UNSHIFT ;
*push = \&PUSH ;
*pop = \&POP ;
*clear = \&CLEAR ;
*length = \&FETCHSIZE ;

sub STORESIZE
{
    croak "STORESIZE is not implemented yet" ;
#print "STORESIZE @_\n" ;
#    my $self = shift;
#    my $length = shift ;
#    my $current_length = $self->FETCHSIZE() ;
#print "length is $current_length\n";
#
#    if ($length < $current_length) {
#print "Make smaller $length < $current_length\n" ;
#        my $key ;
#        for ($key = $current_length - 1 ; $key >= $length ; -- $key)
#          { $self->db_del($key) }
#    }
#    elsif ($length > $current_length) {
#print "Make larger $length > $current_length\n" ;
#        $self->db_put($length-1, "") ;
#    }
#    else { print "stay the same\n" }

}



#sub DESTROY
#{
#    my $self = shift ;
#    print "BerkeleyDB::_tieArray::DESTROY\n" ;
#}


package BerkeleyDB::Common ;


use Carp ;

sub DESTROY
{
    my $self = shift ;
    $self->_DESTROY() ;
}

sub Txn
{
    my $self = shift ;
    my $txn  = shift ;
    #print "BerkeleyDB::Common::Txn db [$self] txn [$txn]\n" ;
    if ($txn) {
        $self->_Txn($txn) ;
        push @{ $txn }, $self ;
    }
    else {
        $self->_Txn() ;
    }
    #print "end BerkeleyDB::Common::Txn \n";
}


sub get_dup
{
    croak "Usage: \$db->get_dup(key [,flag])\n"
        unless @_ == 2 or @_ == 3 ;
 
    my $db        = shift ;
    my $key       = shift ;
    my $flag	  = shift ;
    my $value 	  = 0 ;
    my $origkey   = $key ;
    my $wantarray = wantarray ;
    my %values	  = () ;
    my @values    = () ;
    my $counter   = 0 ;
    my $status    = 0 ;
    my $cursor    = $db->db_cursor() ;
 
    # iterate through the database until either EOF ($status == 0)
    # or a different key is encountered ($key ne $origkey).
    for ($status = $cursor->c_get($key, $value, BerkeleyDB::DB_SET()) ;
	 $status == 0 and $key eq $origkey ;
         $status = $cursor->c_get($key, $value, BerkeleyDB::DB_NEXT()) ) {
        # save the value or count number of matches
        if ($wantarray) {
	    if ($flag)
                { ++ $values{$value} }
	    else
                { push (@values, $value) }
	}
        else
            { ++ $counter }
     
    }
 
    return ($wantarray ? ($flag ? %values : @values) : $counter) ;
}

sub db_cursor
{
    my $db = shift ;
    my ($addr) = $db->_db_cursor(@_) ;
    my $obj ;
    $obj = bless [$addr, $db] , "BerkeleyDB::Cursor" if $addr ;
    return $obj ;
}

sub _db_write_cursor
{
    my $db = shift ;
    my ($addr) = $db->__db_write_cursor(@_) ;
    my $obj ;
    $obj = bless [$addr, $db] , "BerkeleyDB::Cursor" if $addr ;
    return $obj ;
}

sub db_join
{
    croak 'Usage: $db->BerkeleyDB::db_join([cursors], flags=0)'
	if @_ < 2 || @_ > 3 ;
    my $db = shift ;
    croak 'db_join: first parameter is not an array reference'
	if ! ref $_[0] || ref $_[0] ne 'ARRAY';
    my ($addr) = $db->_db_join(@_) ;
    my $obj ;
    $obj = bless [$addr, $db, $_[0]] , "BerkeleyDB::Cursor" if $addr ;
    return $obj ;
}

package BerkeleyDB::Cursor ;

sub c_close
{
    my $cursor = shift ;
    $cursor->[1] = "" ;
    return $cursor->_c_close() ;
}

sub c_dup
{
    my $cursor = shift ;
    my ($addr) = $cursor->_c_dup(@_) ;
    my $obj ;
    $obj = bless [$addr, $cursor->[1]] , "BerkeleyDB::Cursor" if $addr ;
    return $obj ;
}

sub DESTROY
{
    my $self = shift ;
    $self->_DESTROY() ;
}

package BerkeleyDB::TxnMgr ;

sub DESTROY
{
    my $self = shift ;
    $self->_DESTROY() ;
}

sub txn_begin
{
    my $txnmgr = shift ;
    my ($addr) = $txnmgr->_txn_begin(@_) ;
    my $obj ;
    $obj = bless [$addr, $txnmgr] , "BerkeleyDB::Txn" if $addr ;
    return $obj ;
}

package BerkeleyDB::Txn ;

sub Txn
{
    my $self = shift ;
    my $db ;
    # keep a reference to each db in the txn object
    foreach $db (@_) {
        $db->_Txn($self) ;
	push @{ $self}, $db ;
    }
}

sub txn_commit
{
    my $self = shift ;
    $self->disassociate() ;
    my $status = $self->_txn_commit() ;
    return $status ;
}

sub txn_abort
{
    my $self = shift ;
    $self->disassociate() ;
    my $status = $self->_txn_abort() ;
    return $status ;
}

sub disassociate
{
    my $self = shift ;
    my $db ;
    while ( @{ $self } > 2) {
        $db = pop @{ $self } ;
        $db->Txn() ;
    }
    #print "end disassociate\n" ;
}


sub DESTROY
{
    my $self = shift ;

    $self->disassociate() ;
    # first close the close the transaction
    $self->_DESTROY() ;
}

package BerkeleyDB::CDS::Lock;

use vars qw(%Object %Count);
use Carp;

sub BerkeleyDB::Common::cds_lock
{
    my $db = shift ;

    # fatal error if database not opened in CDS mode
    croak("CDS not enabled for this database\n") 
        if ! $db->cds_enabled();

    if ( ! defined $Object{"$db"})
    {
        $Object{"$db"} = $db->_db_write_cursor()
         || return undef ;
    }

    ++ $Count{"$db"} ;

    return bless [$db, 1], "BerkeleyDB::CDS::Lock" ;
}

sub cds_unlock
{
    my $self = shift ;
    my $db = $self->[0] ;

    if ($self->[1]) 
    {
        $self->[1] = 0 ;
        -- $Count{"$db"} if $Count{"$db"} > 0 ;

        if ($Count{"$db"} == 0)
        {
            $Object{"$db"}->c_close() ;
            undef $Object{"$db"};
        }

        return 1 ;
    }

    return undef ;
}

sub DESTROY
{
    my $self = shift ;
    $self->cds_unlock() ;	
}

package BerkeleyDB::Term ;

END
{
    close_everything() ;
}


package BerkeleyDB ;



# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__



