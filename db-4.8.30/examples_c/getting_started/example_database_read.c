/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 */

#include "gettingstarted_common.h"

/* Forward declarations */
int usage(void);
char *show_inventory_item(void *);
int show_all_records(STOCK_DBS *);
int show_records(STOCK_DBS *, char *);
int show_vendor_record(char *, DB *);

int
usage()
{
    fprintf(stderr, "example_database_read [-i <item name>]");
    fprintf(stderr, " [-h <database home>]\n");

    fprintf(stderr,
	"\tNote: Any path specified to the -h parameter must end\n");
    fprintf(stderr, " with your system's path delimiter (/ or \\)\n");
    return (-1);
}

/*
 * Searches for a inventory item based on that item's name. The search is
 * performed using the item name secondary database. Displays all
 * inventory items that use the specified name, as well as the vendor
 * associated with that inventory item.
 *
 * If no item name is provided, then all inventory items are displayed.
 */
int
main(int argc, char *argv[])
{
    STOCK_DBS my_stock;
    int ch, ret;
    char *itemname;

    /* Initialize the STOCK_DBS struct */
    initialize_stockdbs(&my_stock);

    /* Parse the command line arguments */
    itemname = NULL;
    while ((ch = getopt(argc, argv, "h:i:?")) != EOF)
	switch (ch) {
	case 'h':
	    if (optarg[strlen(optarg)-1] != '/' &&
		optarg[strlen(optarg)-1] != '\\')
		return (usage());
	    my_stock.db_home_dir = optarg;
	    break;
	case 'i':
	    itemname = optarg;
	    break;
	case '?':
	default:
	    return (usage());
	}

    /* Identify the files that hold our databases */
    set_db_filenames(&my_stock);

    /* Open all databases */
    ret = databases_setup(&my_stock, "example_database_read", stderr);
    if (ret != 0) {
	fprintf(stderr, "Error opening databases\n");
	databases_close(&my_stock);
	return (ret);
    }

    if (itemname == NULL)
	ret = show_all_records(&my_stock);
    else
	ret = show_records(&my_stock, itemname);

    /* close our databases */
    databases_close(&my_stock);
    return (ret);
}

int show_all_records(STOCK_DBS *my_stock)
{
    DBC *inventory_cursorp;
    DBT key, data;
    char *the_vendor;
    int exit_value, ret;

    /* Initialize our DBTs. */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* Get a cursor to the inventory db */
    my_stock->inventory_dbp->cursor(my_stock->inventory_dbp, NULL,
      &inventory_cursorp, 0);

    /*
     * Iterate over the inventory database, from the first record
     * to the last, displaying each in turn.
     */
    exit_value = 0;
    while ((ret =
      inventory_cursorp->get(inventory_cursorp, &key, &data, DB_NEXT)) == 0)
    {
	the_vendor = show_inventory_item(data.data);
	ret = show_vendor_record(the_vendor, my_stock->vendor_dbp);
	if (ret) {
	    exit_value = ret;
	    break;
	}
    }

    /* Close the cursor */
    inventory_cursorp->close(inventory_cursorp);
    return (exit_value);
}

/*
 * Search for an inventory item given its name (using the inventory item
 * secondary database) and display that record and any duplicates that may
 * exist.
 */
int
show_records(STOCK_DBS *my_stock, char *itemname)
{
    DBC *itemname_cursorp;
    DBT key, data;
    char *the_vendor;
    int ret, exit_value;

    /* Initialize our DBTs. */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* Get a cursor to the itemname db */
    my_stock->itemname_sdbp->cursor(my_stock->itemname_sdbp, NULL,
      &itemname_cursorp, 0);

    /*
     * Get the search key. This is the name on the inventory
     * record that we want to examine.
     */
    key.data = itemname;
    key.size = (u_int32_t)strlen(itemname) + 1;

    /*
     * Position our cursor to the first record in the secondary
     * database that has the appropriate key.
     */
    exit_value = 0;
    ret = itemname_cursorp->get(itemname_cursorp, &key, &data, DB_SET);
    if (!ret) {
	do {
	    /*
	     * Show the inventory record and the vendor responsible
	     * for this inventory item.
	     */
	    the_vendor = show_inventory_item(data.data);
	    ret = show_vendor_record(the_vendor, my_stock->vendor_dbp);
	    if (ret) {
		exit_value = ret;
		break;
	    }
	    /*
	     * Our secondary allows duplicates, so we need to loop over
	     * the next duplicate records and show them all. This is done
	     * because an inventory item's name is not a unique value.
	     */
	} while (itemname_cursorp->get(itemname_cursorp, &key, &data,
	    DB_NEXT_DUP) == 0);
    } else {
	printf("No records found for '%s'\n", itemname);
    }

    /* Close the cursor */
    itemname_cursorp->close(itemname_cursorp);

    return (exit_value);
}

/*
 * Shows an inventory item. How we retrieve the inventory
 * item values from the provided buffer is strictly dependent
 * on the order that those items were originally stored in the
 * DBT. See load_inventory_database in example_database_load
 * for how this was done.
 */
char *
show_inventory_item(void *vBuf)
{
    float price;
    int quantity;
    size_t buf_pos;
    char *category, *name, *sku, *vendor_name;
    char *buf = (char *)vBuf;

    price = *((float *)buf);
    buf_pos = sizeof(float);

    quantity = *((int *)(buf + buf_pos));
    buf_pos += sizeof(int);

    name = buf + buf_pos;
    buf_pos += strlen(name) + 1;

    sku = buf + buf_pos;
    buf_pos += strlen(sku) + 1;

    category = buf + buf_pos;
    buf_pos += strlen(category) + 1;

    vendor_name = buf + buf_pos;

    printf("name: %s\n", name);
    printf("\tSKU: %s\n", sku);
    printf("\tCategory: %s\n", category);
    printf("\tPrice: %.2f\n", price);
    printf("\tQuantity: %i\n", quantity);
    printf("\tVendor:\n");

    return (vendor_name);
}

/*
 * Shows a vendor record. Each vendor record is an instance of
 * a vendor structure. See load_vendor_database() in
 * example_database_load for how this structure was originally
 * put into the database.
 */
int
show_vendor_record(char *vendor_name, DB *vendor_dbp)
{
    DBT key, data;
    VENDOR my_vendor;
    int ret;

    /* Zero our DBTs */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* Set the search key to the vendor's name */
    key.data = vendor_name;
    key.size = (u_int32_t)strlen(vendor_name) + 1;

    /*
     * Make sure we use the memory we set aside for the VENDOR
     * structure rather than the memory that DB allocates.
     * Some systems may require structures to be aligned in memory
     * in a specific way, and DB may not get it right.
     */

    data.data = &my_vendor;
    data.ulen = sizeof(VENDOR);
    data.flags = DB_DBT_USERMEM;

    /* Get the record */
    ret = vendor_dbp->get(vendor_dbp, NULL, &key, &data, 0);
    if (ret != 0) {
	vendor_dbp->err(vendor_dbp, ret, "Error searching for vendor: '%s'",
	  vendor_name);
	return (ret);
    } else {
	printf("\t\t%s\n", my_vendor.name);
	printf("\t\t%s\n", my_vendor.street);
	printf("\t\t%s, %s\n", my_vendor.city, my_vendor.state);
	printf("\t\t%s\n\n", my_vendor.zipcode);
	printf("\t\t%s\n\n", my_vendor.phone_number);
	printf("\t\tContact: %s\n", my_vendor.sales_rep);
	printf("\t\t%s\n", my_vendor.sales_rep_phone);
    }
    return (0);
}
