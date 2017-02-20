/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <db_cxx.h>

using std::cout;
using std::cerr;
using std::ios;
using std::ofstream;

#define MPOOL   "mpool"

int init(const char *, int, int);
int run(DB_ENV *, int, int, int);

static int usage();

const char *progname = "MpoolExample";          // Program name.

class MpoolExample : public DbEnv
{
public:
    MpoolExample();
    int initdb(const char *home, int cachesize);
    int run(int hits, int pagesize, int npages);

private:
    static const char FileName[];

    // no need for copy and assignment
    MpoolExample(const MpoolExample &);
    void operator = (const MpoolExample &);
};

int main(int argc, char *argv[])
{
    int ret;
    int cachesize = 20 * 1024;
    int hits = 1000;
    int npages = 50;
    int pagesize = 1024;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            if ((cachesize = atoi(argv[++i])) < 20 * 1024)
                usage();
        }
        else if (strcmp(argv[i], "-h") == 0) {
            if ((hits = atoi(argv[++i])) <= 0)
                usage();
        }
        else if (strcmp(argv[i], "-n") == 0) {
            if ((npages = atoi(argv[++i])) <= 0)
                usage();
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if ((pagesize = atoi(argv[++i])) <= 0)
                usage();
        }
        else {
            usage();
        }
    }

    // Initialize the file.
    if ((ret = init(MPOOL, pagesize, npages)) != 0)
        return (ret);

    try {
        MpoolExample app;

        cout << progname
             << ": cachesize: " << cachesize
             << "; pagesize: " << pagesize
             << "; N pages: " << npages << "\n";

        if ((ret = app.initdb(NULL, cachesize)) != 0)
            return (ret);
        if ((ret = app.run(hits, pagesize, npages)) != 0)
            return (ret);
        cout << "MpoolExample: completed\n";
        return (EXIT_SUCCESS);
    }
    catch (DbException &dbe) {
        cerr << "MpoolExample: " << dbe.what() << "\n";
        return (EXIT_FAILURE);
    }
}

//
// init --
//  Create a backing file.
//
int
init(const char *file, int pagesize, int npages)
{
    // Create a file with the right number of pages, and store a page
    // number on each page.
    ofstream of(file, ios::out | ios::binary);

    if (of.fail()) {
        cerr << "MpoolExample: " << file << ": open failed\n";
        return (EXIT_FAILURE);
    }
    char *p = new char[pagesize];
    memset(p, 0, pagesize);

    // The pages are numbered from 0.
    for (int cnt = 0; cnt <= npages; ++cnt) {
        *(db_pgno_t *)p = cnt;
        of.write(p, pagesize);
        if (of.fail()) {
            cerr << "MpoolExample: " << file << ": write failed\n";
            return (EXIT_FAILURE);
        }
    }
    delete [] p;
    return (EXIT_SUCCESS);
}

static int
usage()
{
    cerr << "usage: MpoolExample [-c cachesize] "
         << "[-h hits] [-n npages] [-p pagesize]\n";
    return (EXIT_FAILURE);
}

// Note: by using DB_CXX_NO_EXCEPTIONS, we get explicit error returns
// from various methods rather than exceptions so we can report more
// information with each error.
//
MpoolExample::MpoolExample()
:   DbEnv(DB_CXX_NO_EXCEPTIONS)
{
}

int MpoolExample::initdb(const char *home, int cachesize)
{
    set_error_stream(&cerr);
    set_errpfx("MpoolExample");
    set_cachesize(0, cachesize, 0);

    open(home, DB_CREATE | DB_INIT_MPOOL, 0);
    return (EXIT_SUCCESS);
}

//
// run --
//  Get a set of pages.
//
int
MpoolExample::run(int hits, int pagesize, int npages)
{
    db_pgno_t pageno;
    int cnt, ret;
    void *p;

    // Open the file in the environment.
    DbMpoolFile *mfp;

    if ((ret = memp_fcreate(&mfp, 0)) != 0) {
        cerr << "MpoolExample: memp_fcreate failed: "
             << strerror(ret) << "\n";
        return (EXIT_FAILURE);
    }
    mfp->open(MPOOL, 0, 0, pagesize);

    cout << "retrieve " << hits << " random pages... ";

    srand((unsigned int)time(NULL));
    for (cnt = 0; cnt < hits; ++cnt) {
        pageno = (rand() % npages) + 1;
        if ((ret = mfp->get(&pageno, NULL, 0, &p)) != 0) {
            cerr << "MpoolExample: unable to retrieve page "
                 << (unsigned long)pageno << ": "
                 << strerror(ret) << "\n";
            return (EXIT_FAILURE);
        }
        if (*(db_pgno_t *)p != pageno) {
            cerr << "MpoolExample: wrong page retrieved ("
                 << (unsigned long)pageno << " != "
                 << *(int *)p << ")\n";
            return (EXIT_FAILURE);
        }
        if ((ret = mfp->put(p, DB_PRIORITY_UNCHANGED, 0)) != 0) {
            cerr << "MpoolExample: unable to return page "
                 << (unsigned long)pageno << ": "
                 << strerror(ret) << "\n";
            return (EXIT_FAILURE);
        }
    }

    cout << "successful.\n";

    // Close the pool.
    if ((ret = close(0)) != 0) {
        cerr << "MpoolExample: " << strerror(ret) << "\n";
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}
