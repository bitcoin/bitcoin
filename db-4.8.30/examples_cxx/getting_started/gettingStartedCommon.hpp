/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

// File: gettingStartedCommon.hpp

#ifndef GETTINGSTARTEDCOMMON_H
#define GETTINGSTARTEDCOMMON_H

#include <cstdlib>
#include <cstring>

class InventoryData
{
public:
    inline void setPrice(double price) {price_ = price;}
    inline void setQuantity(long quantity) {quantity_ = quantity;}
    inline void setCategory(std::string &category) {category_ = category;}
    inline void setName(std::string &name) {name_ = name;}
    inline void setVendor(std::string &vendor) {vendor_ = vendor;}
    inline void setSKU(std::string &sku) {sku_ = sku;}

    inline double& getPrice() {return(price_);}
    inline long& getQuantity() {return(quantity_);}
    inline std::string& getCategory() {return(category_);}
    inline std::string& getName() {return(name_);}
    inline std::string& getVendor() {return(vendor_);}
    inline std::string& getSKU() {return(sku_);}

    /* Initialize our data members */
    void clear()
    {
        price_ = 0.0;
        quantity_ = 0;
        category_ = "";
        name_ = "";
        vendor_ = "";
        sku_ = "";
    }

    // Default constructor
    InventoryData() { clear(); }

    // Constructor from a void *
    // For use with the data returned from a bdb get
    InventoryData(void *buffer)
    {
        char *buf = (char *)buffer;

        price_ = *((double *)buf);
        bufLen_ = sizeof(double);

        quantity_ = *((long *)(buf + bufLen_));
        bufLen_ += sizeof(long);

        name_ = buf + bufLen_;
        bufLen_ += name_.size() + 1;

        sku_ = buf + bufLen_;
        bufLen_ += sku_.size() + 1;

        category_ = buf + bufLen_;
        bufLen_ += category_.size() + 1;

        vendor_ = buf + bufLen_;
        bufLen_ += vendor_.size() + 1;
    }

    /*
     * Marshalls this classes data members into a single
     * contiguous memory location for the purpose of storing
     * the data in a database.
     */
    char *
    getBuffer()
    {
        // Zero out the buffer
        memset(databuf_, 0, 500);
        /*
         * Now pack the data into a single contiguous memory location for
         * storage.
         */
        bufLen_ = 0;
        int dataLen = 0;

        dataLen = sizeof(double);
        memcpy(databuf_, &price_, dataLen);
        bufLen_ += dataLen;

        dataLen = sizeof(long);
        memcpy(databuf_ + bufLen_, &quantity_, dataLen);
        bufLen_ += dataLen;

        packString(databuf_, name_);
        packString(databuf_, sku_);
        packString(databuf_, category_);
        packString(databuf_, vendor_);

        return (databuf_);
    }

    /*
     * Returns the size of the buffer. Used for storing
     * the buffer in a database.
     */
    inline size_t getBufferSize() { return (bufLen_); }

    /* Utility function used to show the contents of this class */
    void
    show() {
        std::cout << "\nName:           " << name_ << std::endl;
        std::cout << "    SKU:        " << sku_ << std::endl;
        std::cout << "    Price:      " << price_ << std::endl;
        std::cout << "    Quantity:   " << quantity_ << std::endl;
        std::cout << "    Category:   " << category_ << std::endl;
        std::cout << "    Vendor:     " << vendor_ << std::endl;
    }

private:

    /*
     * Utility function that appends a char * to the end of
     * the buffer.
     */
    void
    packString(char *buffer, std::string &theString)
    {
        size_t string_size = theString.size() + 1;
        memcpy(buffer+bufLen_, theString.c_str(), string_size);
        bufLen_ += string_size;
    }

    /* Data members */
    std::string category_, name_, vendor_, sku_;
    double price_;
    long quantity_;
    size_t bufLen_;
    char databuf_[500];

};

#define MAXFIELD 20

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

// Forward declarations
class Db;
class Dbt;

// Used to extract an inventory item's name from an
// inventory database record. This function is used to create
// keys for secondary database records.
int
get_item_name(Db *dbp, const Dbt *pkey, const Dbt *pdata, Dbt *skey)
{
    /*
     * First, obtain the buffer location where we placed the item's name. In
     * this example, the item's name is located in the primary data. It is the
     * first string in the buffer after the price (a double) and the quantity
     * (a long).
     */
    u_int32_t offset = sizeof(double) + sizeof(long);
    char *itemname = (char *)pdata->get_data() + offset;

    // unused
    (void)pkey;

    /*
     * If the offset is beyond the end of the data, then there was a problem
     * with the buffer contained in pdata, or there's a programming error in
     * how the buffer is marshalled/unmarshalled.  This should never happen!
     */
    if (offset > pdata->get_size()) {
        dbp->errx("get_item_name: buffer sizes do not match!");
        // When we return non-zero, the index record is not added/updated.
        return (-1);
    }

    /* Now set the secondary key's data to be the item name */
    skey->set_data(itemname);
    skey->set_size((u_int32_t)strlen(itemname) + 1);

    return (0);
};
#endif
