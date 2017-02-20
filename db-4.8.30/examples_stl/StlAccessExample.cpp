/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
#include <sys/types.h>

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

#include "dbstl_map.h"
#include "dbstl_vector.h"

using namespace std;
using namespace dbstl;

#ifdef _WIN32
extern "C" {
  extern int getopt(int, char * const *, const char *);
  extern int optind;
}
#else
#include <unistd.h>
#endif

#include <db_cxx.h>

using std::cin;
using std::cout;
using std::cerr;

class AccessExample
{
public:
    AccessExample();
    void run();

private:
    // no need for copy and assignment
    AccessExample(const AccessExample &);
    void operator = (const AccessExample &);
};

int
usage()
{
    (void)fprintf(stderr, "usage: AccessExample");
    return (EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    // Use a try block just to report any errors.
    // An alternate approach to using exceptions is to
    // use error models (see DbEnv::set_error_model()) so
    // that error codes are returned for all Berkeley DB methods.
    //
    try {
        AccessExample app;
        app.run();
        return (EXIT_SUCCESS);
    }
    catch (DbException &dbe) {
        cerr << "AccessExample: " << dbe.what() << "\n";
        return (EXIT_FAILURE);
    }
}

AccessExample::AccessExample()
{
}

void AccessExample::run()
{
    typedef db_map<char *, char *, ElementHolder<char *> > strmap_t;
    // Create a map container with inmemory anonymous database.
    strmap_t dbmap;

    // Insert records into dbmap, where the key is the user
    // input and the data is the user input in reverse order.
    //
    char buf[1024], rbuf[1024];
    char *p, *t;
    u_int32_t len;

    for (;;) {
        // Acquire user input string as key.
        cout << "input> ";
        cout.flush();

        cin.getline(buf, sizeof(buf));
        if (cin.eof())
            break;

        if ((len = (u_int32_t)strlen(buf)) <= 0)
            continue;
        if (strcmp(buf, "quit") == 0)
            break;

        // Reverse input string as data.
        for (t = rbuf, p = buf + (len - 1); p >= buf;)
            *t++ = *p--;
        *t++ = '\0';

        
        // Insert key/data pair.
        try {
            dbmap.insert(make_pair(buf, rbuf));
        } catch (DbException ex) {
            if (ex.get_errno() == DB_KEYEXIST) {
                cout << "Key " << buf << " already exists.\n";
            } else
                throw;
        } catch (...) {
            throw;
        }
    }
    cout << "\n";

    // We put a try block around this section of code
    // to ensure that our database is properly closed
    // in the event of an error.
    //
    try {
        strmap_t::iterator itr;
        for (itr = dbmap.begin(); 
            itr != dbmap.end(); ++itr) 
            cout<<itr->first<<" : "<<itr->second<<endl;     
    }
    catch (DbException ex) {
        cerr<<"AccessExample "<<ex.what()<<endl;
    }

    dbstl_exit();

}
