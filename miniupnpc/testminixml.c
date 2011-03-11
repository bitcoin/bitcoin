/* $Id: testminixml.c,v 1.6 2006/11/19 22:32:35 nanard Exp $
 * testminixml.c
 * test program for the "minixml" functions.
 * Author : Thomas Bernard.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "minixml.h"
#include "igd_desc_parse.h"

#ifdef WIN32
#define NO_BZERO
#endif

#ifdef NO_BZERO
#define bzero(p, n) memset(p, 0, n)
#endif

/* ---------------------------------------------------------------------- */
void printeltname1(void * d, const char * name, int l)
{
	int i;
	printf("element ");
	for(i=0;i<l;i++)
		putchar(name[i]);
}
void printeltname2(void * d, const char * name, int l)
{
	int i;
	putchar('/');
	for(i=0;i<l;i++)
		putchar(name[i]);
	putchar('\n');
}
void printdata(void *d, const char * data, int l)
{
	int i;
	printf("data : ");
	for(i=0;i<l;i++)
		putchar(data[i]);
	putchar('\n');
}

void burptest(const char * buffer, int bufsize)
{
	struct IGDdatas data;
	struct xmlparser parser;
	/*objet IGDdatas */
	bzero(&data, sizeof(struct IGDdatas));
	/* objet xmlparser */
	parser.xmlstart = buffer;
	parser.xmlsize = bufsize;
	parser.data = &data;
	/*parser.starteltfunc = printeltname1;
	parser.endeltfunc = printeltname2;
	parser.datafunc = printdata; */
	parser.starteltfunc = IGDstartelt;
	parser.endeltfunc = IGDendelt;
	parser.datafunc = IGDdata; 
	parsexml(&parser);
	printIGD(&data);
}

/* ----- main ---- */
#define XML_MAX_SIZE (8192)
int main(int argc, char * * argv)
{
	FILE * f;
	char buffer[XML_MAX_SIZE];
	int bufsize;
	if(argc<2)
	{
		printf("usage:\t%s file.xml\n", argv[0]);
		return 1;
	}
	f = fopen(argv[1], "r");
	if(!f)
	{
		printf("cannot open file %s\n", argv[1]);
		return 1;
	}
	bufsize = (int)fread(buffer, 1, XML_MAX_SIZE, f);
	fclose(f);
	burptest(buffer, bufsize);
	return 0;
}

