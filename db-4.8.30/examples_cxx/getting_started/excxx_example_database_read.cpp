/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

// File: excxx_example_database_read.cpp

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
int show_item(MyDb &itemnameSDB, MyDb &vendorDB, std::string &itemName);
int show_all_records(MyDb &inventoryDB, MyDb &vendorDB);
int show_vendor(MyDb &vendorDB, const char *vendor);

int
usage()
{
    std::cout << "example_database_read [-i <path to data files>]"
              << " [-h <database home directory>]" << std::endl;

    std::cout << "Note: Any path specified to the -h parameter must end"
              << " with your system's path delimiter (/ or \\)"
              << std::endl;
    return (-1);
}

int
main (int argc, char *argv[])
{

   char ch, lastChar;

   // Initialize the path to the database files
   std::string databaseHome("./");
   std::string itemName;

   // Database names
   std::string vDbName("vendordb.db");
   std::string iDbName("inventorydb.db");
   std::string itemSDbName("itemname.sdb");

    // Parse the command line arguments
    while ((ch = getopt(argc, argv, "h:i:")) != EOF)
        switch (ch) {
        case 'h':
            databaseHome = optarg;
            lastChar = databaseHome[databaseHome.size() -1];
            if (lastChar != '/' && lastChar != '\\')
                return (usage());
            break;
        case 'i':
            itemName = optarg;
            break;
        case '?':
        default:
            return (usage());
            break;
        }

    try
    {
        // Open all databases.
        MyDb inventoryDB(databaseHome, iDbName);
        MyDb vendorDB(databaseHome, vDbName);
        MyDb itemnameSDB(databaseHome, itemSDbName, true);

        // Associate the secondary to the primary
        inventoryDB.getDb().associate(NULL,
                                      &(itemnameSDB.getDb()),
                                      get_item_name,
                                      0);

        if (itemName.empty())
        {
            show_all_records(inventoryDB, vendorDB);
        } else {
            show_item(itemnameSDB, vendorDB, itemName);
        }
    } catch(DbException &e) {
        std::cerr << "Error reading databases. " << std::endl;
        return (e.get_errno());
    } catch(std::exception &e) {
        std::cerr << "Error reading databases. " << std::endl;
        std::cerr << e.what() << std::endl;
        return (-1);
    }

    return (0);
} // End main

// Shows the records in the inventory database that
// have a specific item name. For each inventory record
// shown, the appropriate vendor record is also displayed.
int
show_item(MyDb &itemnameSDB, MyDb &vendorDB, std::string &itemName)
{

    // Get a cursor to the itemname secondary db
    Dbc *cursorp;

    try {
        itemnameSDB.getDb().cursor(NULL, &cursorp, 0);

        // Get the search key. This is the name on the inventory
        // record that we want to examine.
        std::cout << "Looking for " << itemName << std::endl;
        Dbt key((void *)itemName.c_str(), (u_int32_t)itemName.length() + 1);
        Dbt data;

        // Position the cursor to the first record in the secondary
        // database that has the appropriate key.
        int ret = cursorp->get(&key, &data, DB_SET);
        if (!ret) {
            do {
                InventoryData inventoryItem(data.get_data());
                inventoryItem.show();

                show_vendor(vendorDB, inventoryItem.getVendor().c_str());

            } while (cursorp->get(&key, &data, DB_NEXT_DUP) == 0);
        } else {
            std::cerr << "No records found for '" << itemName
                    << "'" << std::endl;
        }
    } catch(DbException &e) {
        itemnameSDB.getDb().err(e.get_errno(), "Error in show_item");
        cursorp->close();
        throw e;
    } catch(std::exception &e) {
        itemnameSDB.getDb().errx("Error in show_item: %s", e.what());
        cursorp->close();
        throw e;
    }

    cursorp->close();
    return (0);
}

// Shows all the records in the inventory database.
// For each inventory record shown, the appropriate
// vendor record is also displayed.
int
show_all_records(MyDb &inventoryDB, MyDb &vendorDB)
{

    // Get a cursor to the inventory db
    Dbc *cursorp;
    try {
        inventoryDB.getDb().cursor(NULL, &cursorp, 0);

        // Iterate over the inventory database, from the first record
        // to the last, displaying each in turn
        Dbt key, data;
        int ret;
        while ((ret = cursorp->get(&key, &data, DB_NEXT)) == 0 )
        {
            InventoryData inventoryItem(data.get_data());
            inventoryItem.show();

            show_vendor(vendorDB, inventoryItem.getVendor().c_str());
        }
    } catch(DbException &e) {
        inventoryDB.getDb().err(e.get_errno(), "Error in show_all_records");
        cursorp->close();
        throw e;
    } catch(std::exception &e) {
        cursorp->close();
        throw e;
    }

    cursorp->close();
    return (0);
}

// Shows a vendor record. Each vendor record is an instance of
// a vendor structure. See loadVendorDB() in
// example_database_load for how this structure was originally
// put into the database.
int
show_vendor(MyDb &vendorDB, const char *vendor)
{
    Dbt data;
    VENDOR my_vendor;

    try {
        // Set the search key to the vendor's name
        // vendor is explicitly cast to char * to stop a compiler
        // complaint.
        Dbt key((char *)vendor, (u_int32_t)strlen(vendor) + 1);

        // Make sure we use the memory we set aside for the VENDOR
        // structure rather than the memory that DB allocates.
        // Some systems may require structures to be aligned in memory
        // in a specific way, and DB may not get it right.

        data.set_data(&my_vendor);
        data.set_ulen(sizeof(VENDOR));
        data.set_flags(DB_DBT_USERMEM);

        // Get the record
        vendorDB.getDb().get(NULL, &key, &data, 0);
        std::cout << "        " << my_vendor.street << "\n"
                  << "        " << my_vendor.city << ", "
                  << my_vendor.state << "\n"
                  << "        " << my_vendor.zipcode << "\n"
                  << "        " << my_vendor.phone_number << "\n"
                  << "        Contact: " << my_vendor.sales_rep << "\n"
                  << "                 " << my_vendor.sales_rep_phone
                  << std::endl;

    } catch(DbException &e) {
        vendorDB.getDb().err(e.get_errno(), "Error in show_vendor");
        throw e;
    } catch(std::exception &e) {
        throw e;
    }
    return (0);
}
