/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 */

#include <db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
extern char *optarg;
#define snprintf _snprintf
#else
#include <unistd.h>
#endif

#define DEFAULT_HOMEDIR "./"
#define INVENTORY_FILE  "inventory.txt"
#define VENDORS_FILE    "vendors.txt"
#define INVENTORYDB "inventoryDB.db"
#define ITEMNAMEDB  "itemnameDB.db"
#define MAXDATABUF  1024
#define MAXFIELD    20
#define MAXLINE     150
#define PRIMARY_DB  0
#define SECONDARY_DB    1
#define VENDORDB    "vendorDB.db"

typedef struct stock_dbs {
    DB *inventory_dbp; /* Database containing inventory information */
    DB *vendor_dbp;    /* Database containing vendor information */
    DB *itemname_sdbp; /* Index based on the item name index */

    char *db_home_dir;             /* Directory containing the database files */
    char *itemname_db_name;  /* Itemname secondary database */
    char *inventory_db_name; /* Name of the inventory database */
    char *vendor_db_name;    /* Name of the vendor database */
} STOCK_DBS;

typedef struct vendor {
    char name[MAXFIELD];             /* Vendor name */
    char street[MAXFIELD];           /* Street name and number */
    char city[MAXFIELD];             /* City */
    char state[3];                   /* Two-digit US state code */
    char zipcode[6];                 /* US zipcode */
    char phone_number[13];           /* Vendor phone number */
    char sales_rep[MAXFIELD];        /* Name of sales representative */
    char sales_rep_phone[MAXFIELD];  /* Sales rep's phone number */
} VENDOR;

/* Function prototypes */
int databases_close(STOCK_DBS *);
int databases_setup(STOCK_DBS *, const char *, FILE *);
void    initialize_stockdbs(STOCK_DBS *);
int open_database(DB **, const char *, const char *, FILE *, int);
void    set_db_filenames(STOCK_DBS *my_stock);
