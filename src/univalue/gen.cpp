// Copyright 2014 BitPay Inc.
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// To re-create univalue_escapes.h:
// $ g++ -o gen gen.cpp
// $ ./gen > univalue_escapes.h
//

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "univalue.h"

using namespace std;

static bool initEscapes;
static const char *escapes[256];

static void initJsonEscape()
{
    escapes[(int)'"'] = "\\\"";
    escapes[(int)'\\'] = "\\\\";
    escapes[(int)'/'] = "\\/";
    escapes[(int)'\b'] = "\\b";
    escapes[(int)'\f'] = "\\f";
    escapes[(int)'\n'] = "\\n";
    escapes[(int)'\r'] = "\\r";
    escapes[(int)'\t'] = "\\t";

    initEscapes = true;
}

static void outputEscape()
{
	printf(	"// Automatically generated file. Do not modify.\n"
		"#ifndef BITCOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n"
		"#define BITCOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n"
		"static const char *escapes[256] = {\n");

	for (unsigned int i = 0; i < 256; i++) {
		if (!escapes[i]) {
			printf("\tNULL,\n");
		} else {
			printf("\t\"");

			unsigned int si;
			for (si = 0; si < strlen(escapes[i]); si++) {
				char ch = escapes[i][si];
				switch (ch) {
				case '"':
					printf("\\\"");
					break;
				case '\\':
					printf("\\\\");
					break;
				default:
					printf("%c", escapes[i][si]);
					break;
				}
			}

			printf("\",\n");
		}
	}

	printf(	"};\n"
		"#endif // BITCOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n");
}

int main (int argc, char *argv[])
{
	initJsonEscape();
	outputEscape();
	return 0;
}

