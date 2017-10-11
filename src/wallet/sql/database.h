#ifndef ENTERPRISE_DATABASE_H
#define ENTERPRISE_DATABASE_H

#include <odb/database.hxx>
#include <odb/pgsql/database.hxx>

inline std::auto_ptr<odb::database>
create_database ()
{
    using namespace std;
    using namespace odb::core;

    int argc(5);
    char *argv[] = {"./driver", "--user", "odb_test", "--database", "odb_test"};

    auto_ptr<database> db (new odb::pgsql::database (argc, argv));

    return db;
}

#endif