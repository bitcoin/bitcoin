#!/bin/sh
# $Id: external-ip.sh,v 1.1 2010/08/05 12:57:41 nanard Exp $
# (c) 2010 Reuben Hawkins
upnpc -s | grep ExternalIPAddress | sed 's/[^0-9\.]//g'
