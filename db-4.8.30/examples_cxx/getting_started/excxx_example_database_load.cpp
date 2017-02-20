/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

// File: excxx_example_database_load.cpp
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "MyDb.hpp"
#include "gettingStartedCommon.hpp"

#ifdef _WIN32
extern "C" {
  extern int getopt(int, char * const *, const char *);
  extern char *optarg;
}
#else
#include <unistd.h>
#endif

// Forward declarations
void loadInventoryDB(MyDb &, std::string &);
void loadVendorDB(MyDb &, std::string &);

using namespace std;

int
usage()
{
    std::cout << "example_database_load [-b <path to data files>]"
              << " [-h <database home directory>]" << std::endl;

    std::cout << "Note: If -b -h is specified, then the path must end"
              << " with your system's path delimiter (/ or \\)"
              << std::endl;
    return (-1);
}

// Loads the contents of vendors.txt and inventory.txt into
// Berkeley DB databases.
int
main (int argc, char *argv[])
{

   char ch, lastChar;

   // Initialize the path to the database files
   std::string basename("./");
   std::string databaseHome("./");

   // Database names
   std::string vDbName("vendordb.db");
   std::string iDbName("inventorydb.db");
   std::string itemSDbName("itemname.sdb");

    // Parse the command line arguments
    while ((ch = getopt(argc, argv, "b:h:")) != EOF)
        switch (ch) {
        case 'h':
            databaseHome = optarg;
            lastChar = databaseHome[databaseHome.size() -1];
            if (lastChar != '/' && lastChar != '\\')
                return (usage());
            break;
        case 'b':
            basename = optarg;
            lastChar = basename[basename.size() -1];
            if (lastChar != '/' && lastChar != '\\')
                return (usage());
            break;
        case '?':
        default:
            return (usage());
            break;
        }

    // Identify the full name for our input files, which should
    // also include some path information.
    std::string inventoryFile = basename + "inventory.txt";
    std::string vendorFile = basename + "vendors.txt";

    try
    {
        // Open all databases.
        MyDb inventoryDB(databaseHome, iDbName);
        MyDb vendorDB(databaseHome, vDbName);
        MyDb itemnameSDB(databaseHome, itemSDbName, true);

        // Associate the primary and the secondary
        inventoryDB.getDb().associate(NULL,
                                      &(itemnameSDB.getDb()),
                                      get_item_name,
                                      0);

        // Load the inventory database
        loadInventoryDB(inventoryDB, inventoryFile);

        // Load the vendor database
        loadVendorDB(vendorDB, vendorFile);
    } catch(DbException &e) {
        std::cerr << "Error loading databases. " << std::endl;
        std::cerr << e.what() << std::endl;
        return (e.get_errno());
    } catch(std::exception &e) {
        std::cerr << "Error loading databases. " << std::endl;
        std::cerr << e.what() << std::endl;
        return (-1);
    }

    // MyDb class constructors will close the databases when they
    // go out of scope.
    return (0);
} // End main

// Used to locate the first pound sign (a field delimiter)
// in the input string.
size_t
getNextPound(std::string &theString, std::string &substring)
{
    size_t pos = theString.find("#");
    substring.assign(theString, 0, pos);
    theString.assign(theString, pos + 1, theString.size());
    return (pos);
}

// Loads the contents of the inventory.txt file into a database
void
loadInventoryDB(MyDb &inventoryDB, std::string &inventoryFile)
{
    InventoryData inventoryData;
    std::string substring;
    size_t nextPound;

    std::ifstream inFile(inventoryFile.c_str(), std::ios::in);
    if ( !inFile )
    {
        std::cerr << "Could not open file '" << inventoryFile
                  << "'. Giving up." << std::endl;
        throw std::exception();
    }

    while (!inFile.eof())
    {
        inventoryData.clear();
        std::string stringBuf;
        std::getline(inFile, stringBuf);

        // Now parse the line
        if (!stringBuf.empty())
        {
            nextPound = getNextPound(stringBuf, substring);
            inventoryData.setName(substring);

            nextPound = getNextPound(stringBuf, substring);
            inventoryData.setSKU(substring);

            nextPound = getNextPound(stringBuf, substring);
            inventoryData.setPrice(strtod(substring.c_str(), 0));

            nextPound = getNextPound(stringBuf, substring);
            inventoryData.setQuantity(strtol(substring.c_str(), 0, 10));

            nextPound = getNextPound(stringBuf, substring);
            inventoryData.setCategory(substring);

            nextPound = getNextPound(stringBuf, substring);
            inventoryData.setVendor(substring);

            void *buff = (void *)inventoryData.getSKU().c_str();
            size_t size = inventoryData.getSKU().size()+1;
            Dbt key(buff, (u_int32_t)size);

            buff = inventoryData.getBuffer();
            size = inventoryData.getBufferSize();
            Dbt data(buff, (u_int32_t)size);

            inventoryDB.getDb().put(NULL, &key, &data, 0);
        }

    }

    inFile.close();

}

// Loads the contents of the vendors.txt file into a database
void
loadVendorDB(MyDb &vendorDB, std::string &vendorFile)
{
    std::ifstream inFile(vendorFile.c_str(), std::ios::in);
    if ( !inFile )
    {
        std::cerr << "Could not open file '" << vendorFile
                  << "'. Giving up." << std::endl;
        throw std::exception();
    }

    VENDOR my_vendor;
    while (!inFile.eof())
    {
        std::string stringBuf;
        std::getline(inFile, stringBuf);
        memset(&my_vendor, 0, sizeof(VENDOR));

         // Scan the line into the structure.
         // Convenient, but not particularly safe.
         // In a real program, there would be a lot more
         // defensive code here.
        sscanf(stringBuf.c_str(),
          "%20[^#]#%20[^#]#%20[^#]#%3[^#]#%6[^#]#%13[^#]#%20[^#]#%20[^\n]",
          my_vendor.name, my_vendor.street,
          my_vendor.city, my_vendor.state,
          my_vendor.zipcode, my_vendor.phone_number,
          my_vendor.sales_rep, my_vendor.sales_rep_phone);

        Dbt key(my_vendor.name, (u_int32_t)strlen(my_vendor.name) + 1);
        Dbt data(&my_vendor, sizeof(VENDOR));

        vendorDB.getDb().put(NULL, &key, &data, 0);
    }

    inFile.close();
}
