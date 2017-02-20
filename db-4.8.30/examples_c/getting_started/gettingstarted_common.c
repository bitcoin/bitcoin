/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 */

#include "gettingstarted_common.h"

int get_item_name(DB *, const DBT *, const DBT *, DBT *);

/*
 * Used to extract an inventory item's name from an
 * inventory database record. This function is used to create
 * keys for secondary database records.
 */
int
get_item_name(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey)
{
    u_int offset;

    dbp = NULL;                         /* Not needed, unused. */
    pkey = NULL;

    /*
     * First, obtain the buffer location where we placed the
     * item's name. In this example, the item's name is located
     * in the primary data. It is the first string in the
     * buffer after the price (a float) and the quantity (an int).
     *
     * See load_inventory_database() in example_database_load.c
     * for how we packed the inventory information into the
     * data DBT.
     */
    offset = sizeof(float) + sizeof(int);

    /* Check to make sure there's data */
    if (pdata->size < offset)
	return (-1); /* Returning non-zero means that the
		      * secondary record is not created/updated.
		      */

    /* Now set the secondary key's data to be the item name */
    memset(skey, 0, sizeof(DBT));
    skey->data = (u_int8_t *)pdata->data + offset;
    skey->size = (u_int32_t)strlen(skey->data) + 1;

    return (0);
}

/* Opens a database */
int
open_database(DB **dbpp, const char *file_name,
  const char *program_name, FILE *error_file_pointer,
  int is_secondary)
{
    DB *dbp;    /* For convenience */
    u_int32_t open_flags;
    int ret;

    /* Initialize the DB handle */
    ret = db_create(&dbp, NULL, 0);
    if (ret != 0) {
	fprintf(error_file_pointer, "%s: %s\n", program_name,
		db_strerror(ret));
	return (ret);
    }
    /* Point to the memory malloc'd by db_create() */
    *dbpp = dbp;

    /* Set up error handling for this database */
    dbp->set_errfile(dbp, error_file_pointer);
    dbp->set_errpfx(dbp, program_name);

    /*
     * If this is a secondary database, then we want to allow
     * sorted duplicates.
     */
    if (is_secondary) {
	ret = dbp->set_flags(dbp, DB_DUPSORT);
	if (ret != 0) {
	    dbp->err(dbp, ret, "Attempt to set DUPSORT flags failed.",
	      file_name);
	    return (ret);
	}
    }

    /* Set the open flags */
    open_flags = DB_CREATE;    /* Allow database creation */

    /* Now open the database */
    ret = dbp->open(dbp,        /* Pointer to the database */
		    NULL,       /* Txn pointer */
		    file_name,  /* File name */
		    NULL,       /* Logical db name */
		    DB_BTREE,   /* Database type (using btree) */
		    open_flags, /* Open flags */
		    0);         /* File mode. Using defaults */
    if (ret != 0) {
	dbp->err(dbp, ret, "Database '%s' open failed.", file_name);
	return (ret);
    }

    return (0);
}

/* opens all databases */
int
databases_setup(STOCK_DBS *my_stock, const char *program_name,
  FILE *error_file_pointer)
{
    int ret;

    /* Open the vendor database */
    ret = open_database(&(my_stock->vendor_dbp),
      my_stock->vendor_db_name,
      program_name, error_file_pointer,
      PRIMARY_DB);
    if (ret != 0)
	/*
	 * Error reporting is handled in open_database() so just return
	 * the return code.
	 */
	return (ret);

    /* Open the inventory database */
    ret = open_database(&(my_stock->inventory_dbp),
      my_stock->inventory_db_name,
      program_name, error_file_pointer,
      PRIMARY_DB);
    if (ret != 0)
	/*
	 * Error reporting is handled in open_database() so just return
	 * the return code.
	 */
	return (ret);

    /*
     * Open the itemname secondary database. This is used to
     * index the product names found in the inventory
     * database.
     */
    ret = open_database(&(my_stock->itemname_sdbp),
      my_stock->itemname_db_name,
      program_name, error_file_pointer,
      SECONDARY_DB);
    if (ret != 0)
	/*
	 * Error reporting is handled in open_database() so just return
	 * the return code.
	 */
	return (ret);

    /*
     * Associate the itemname db with its primary db
     * (inventory db).
     */
     my_stock->inventory_dbp->associate(
       my_stock->inventory_dbp,    /* Primary db */
       NULL,                       /* txn id */
       my_stock->itemname_sdbp,    /* Secondary db */
       get_item_name,              /* Secondary key creator */
       0);                         /* Flags */

    printf("databases opened successfully\n");
    return (0);
}

/* Initializes the STOCK_DBS struct.*/
void
initialize_stockdbs(STOCK_DBS *my_stock)
{
    my_stock->db_home_dir = DEFAULT_HOMEDIR;
    my_stock->inventory_dbp = NULL;
    my_stock->vendor_dbp = NULL;
    my_stock->itemname_sdbp = NULL;
    my_stock->vendor_db_name = NULL;
    my_stock->inventory_db_name = NULL;
    my_stock->itemname_db_name = NULL;
}

/* Identify all the files that will hold our databases. */
void
set_db_filenames(STOCK_DBS *my_stock)
{
    size_t size;

    /* Create the Inventory DB file name */
    size = strlen(my_stock->db_home_dir) + strlen(INVENTORYDB) + 1;
    my_stock->inventory_db_name = malloc(size);
    snprintf(my_stock->inventory_db_name, size, "%s%s",
      my_stock->db_home_dir, INVENTORYDB);

    /* Create the Vendor DB file name */
    size = strlen(my_stock->db_home_dir) + strlen(VENDORDB) + 1;
    my_stock->vendor_db_name = malloc(size);
    snprintf(my_stock->vendor_db_name, size, "%s%s",
      my_stock->db_home_dir, VENDORDB);

    /* Create the itemname DB file name */
    size = strlen(my_stock->db_home_dir) + strlen(ITEMNAMEDB) + 1;
    my_stock->itemname_db_name = malloc(size);
    snprintf(my_stock->itemname_db_name, size, "%s%s",
      my_stock->db_home_dir, ITEMNAMEDB);

}

/* Closes all the databases and secondary databases. */
int
databases_close(STOCK_DBS *my_stock)
{
    int ret;
    /*
     * Note that closing a database automatically flushes its cached data
     * to disk, so no sync is required here.
     */
    if (my_stock->itemname_sdbp != NULL) {
	ret = my_stock->itemname_sdbp->close(my_stock->itemname_sdbp, 0);
	if (ret != 0)
	    fprintf(stderr, "Itemname database close failed: %s\n",
	      db_strerror(ret));
    }

    if (my_stock->inventory_dbp != NULL) {
	ret = my_stock->inventory_dbp->close(my_stock->inventory_dbp, 0);
	if (ret != 0)
	    fprintf(stderr, "Inventory database close failed: %s\n",
	      db_strerror(ret));
    }

    if (my_stock->vendor_dbp != NULL) {
	ret = my_stock->vendor_dbp->close(my_stock->vendor_dbp, 0);
	if (ret != 0)
	    fprintf(stderr, "Vendor database close failed: %s\n",
	      db_strerror(ret));
    }

    printf("databases closed.\n");
    return (0);
}
