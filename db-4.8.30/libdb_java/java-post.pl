#!/usr/bin/env perl -p

# Hide some symbols
s!public (class db_java|[^(]* delete|[^(]* [A-Za-z_]*0\()!/* package */ $1!;

# Mark methods that don't throw exceptions
s!public [^(]*get_version_[a-z]*\([^)]*\)!$& /* no exception */!;
s!public [^(]*[ _]err[a-z_]*\([^)]*\)!$& /* no exception */!;
s!public [^(]*[ _]msg[a-z_]*\([^)]*\)!$& /* no exception */!;
s!public [^(]*[ _]message[a-z_]*\([^)]*\)!$& /* no exception */!;
s!public [^(]*[ _]strerror\([^)]*\)!$& /* no exception */!;
s!public [^(]*log_compare\([^)]*\)!$& /* no exception */!;
s!public [^(]* feedback\([^)]*\)!$& /* no exception */!;

# Mark methods that throw special exceptions
m/DbSequence/ || s!(public [^(]*(open|remove|rename)0?\([^)]*\))( {|;)!$1 throws com.sleepycat.db.DatabaseException, java.io.FileNotFoundException$3!;

# Everything else throws a DbException
s!(public [^(]*\([^)]*\))(;| {)!$1 throws com.sleepycat.db.DatabaseException$2!;

# Add initialize methods for Java parts of Db and DbEnv
s!\.new_DbEnv\(.*$!$&\n    initialize();!;
s!\.new_Db\(.*$!$&\n    initialize(dbenv);!;
