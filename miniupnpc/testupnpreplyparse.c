/* $Id: testupnpreplyparse.c,v 1.2 2008/02/21 13:05:27 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2007 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#include <stdio.h>
#include <stdlib.h>
#include "upnpreplyparse.h"

void
test_parsing(const char * buf, int len)
{
	struct NameValueParserData pdata;
	ParseNameValue(buf, len, &pdata);
	ClearNameValueList(&pdata);
}

int main(int argc, char * * argv)
{
	FILE * f;
	char buffer[4096];
	int l;
	if(argc<2)
	{
		fprintf(stderr, "Usage: %s file.xml\n", argv[0]);
		return 1;
	}
	f = fopen(argv[1], "r");
	if(!f)
	{
		fprintf(stderr, "Error : can not open file %s\n", argv[1]);
		return 2;
	}
	l = fread(buffer, 1, sizeof(buffer)-1, f);
	fclose(f);
	buffer[l] = '\0';
#ifdef DEBUG
	DisplayNameValueList(buffer, l);
#endif
	test_parsing(buffer, l);
	return 0;
}

