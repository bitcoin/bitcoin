/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#include "db_cxx.h"

DbMultipleIterator::DbMultipleIterator(const Dbt &dbt)
 : data_((u_int8_t*)dbt.get_data()),
   p_((u_int32_t*)(data_ + dbt.get_ulen() - sizeof(u_int32_t)))
{
}

bool DbMultipleDataIterator::next(Dbt &data)
{
    if (*p_ == (u_int32_t)-1) {
        data.set_data(0);
        data.set_size(0);
        p_ = 0;
    } else {
        data.set_data(data_ + *p_--);
        data.set_size(*p_--);
        if (data.get_size() == 0 && data.get_data() == data_)
            data.set_data(0);
    }
    return (p_ != 0);
}

bool DbMultipleKeyDataIterator::next(Dbt &key, Dbt &data)
{
    if (*p_ == (u_int32_t)-1) {
        key.set_data(0);
        key.set_size(0);
        data.set_data(0);
        data.set_size(0);
        p_ = 0;
    } else {
        key.set_data(data_ + *p_--);
        key.set_size(*p_--);
        data.set_data(data_ + *p_--);
        data.set_size(*p_--);
    }
    return (p_ != 0);
}

bool DbMultipleRecnoDataIterator::next(db_recno_t &recno, Dbt &data)
{
    if (*p_ == (u_int32_t)0) {
        recno = 0;
        data.set_data(0);
        data.set_size(0);
        p_ = 0;
    } else {
        recno = *p_--;
        data.set_data(data_ + *p_--);
        data.set_size(*p_--);
    }
    return (p_ != 0);
}


DbMultipleBuilder::DbMultipleBuilder(Dbt &dbt) : dbt_(dbt)
{
    DB_MULTIPLE_WRITE_INIT(p_, dbt_.get_DBT());
}


bool DbMultipleDataBuilder::append(void *dbuf, size_t dlen)
{
    DB_MULTIPLE_WRITE_NEXT(p_, dbt_.get_DBT(), dbuf, dlen);
    return (p_ != 0);
}

bool DbMultipleDataBuilder::reserve(void *&ddest, size_t dlen)
{
    DB_MULTIPLE_RESERVE_NEXT(p_, dbt_.get_DBT(), ddest, dlen);
    return (ddest != 0);
}

bool DbMultipleKeyDataBuilder::append(
    void *kbuf, size_t klen, void *dbuf, size_t dlen)
{
    DB_MULTIPLE_KEY_WRITE_NEXT(p_, dbt_.get_DBT(),
        kbuf, klen, dbuf, dlen);
    return (p_ != 0);
}

bool DbMultipleKeyDataBuilder::reserve(
     void *&kdest, size_t klen, void *&ddest, size_t dlen)
{
    DB_MULTIPLE_KEY_RESERVE_NEXT(p_, dbt_.get_DBT(),
        kdest, klen, ddest, dlen);
    return (kdest != 0 && ddest != 0);
}


DbMultipleRecnoDataBuilder::DbMultipleRecnoDataBuilder(Dbt &dbt) : dbt_(dbt)
{
    DB_MULTIPLE_RECNO_WRITE_INIT(p_, dbt_.get_DBT());
}

bool DbMultipleRecnoDataBuilder::append(
    db_recno_t recno, void *dbuf, size_t dlen)
{
    DB_MULTIPLE_RECNO_WRITE_NEXT(p_, dbt_.get_DBT(),
        recno, dbuf, dlen);
    return (p_ != 0);
}

bool DbMultipleRecnoDataBuilder::reserve(
    db_recno_t recno, void *&ddest, size_t dlen)
{
    DB_MULTIPLE_RECNO_RESERVE_NEXT(p_, dbt_.get_DBT(),
        recno, ddest, dlen);
    return (ddest != 0);
}
