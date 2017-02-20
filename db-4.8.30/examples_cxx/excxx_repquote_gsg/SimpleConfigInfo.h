/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <db_cxx.h>

class SimpleConfigInfo {
public:
    SimpleConfigInfo();
    virtual ~SimpleConfigInfo();

public:
    char* home;
};


SimpleConfigInfo::SimpleConfigInfo()
{
    home = NULL;
}

SimpleConfigInfo::~SimpleConfigInfo()
{
}

