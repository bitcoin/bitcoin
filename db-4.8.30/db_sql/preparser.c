/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

#include "db_sql.h"

extern void bdb_create_database(Token *, Parse *pParse);

static void
preparserSyntaxError(t, pParse) 
	Token *t;
	Parse *pParse;
{
	sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", t);
	pParse->parseError = 1;
}

/*
 * The pre-parser is invoked for each token found by the lexical
 * analyzer.  It passes most tokens on to the main SQLite parser
 * unchanged; however it maintains its own state machine so that it
 * can notice certain sequences of tokens.  In particular, it catches
 * CREATE DATABASE, which is a sequence that the SQLite parser does
 * not recognize.
 */
void preparser(pEngine, tokenType, token, pParse) 
	void *pEngine;
	int tokenType; 
	Token token;
	Parse *pParse;
{
	static enum preparserState {
		IDLE = 0, GOT_CREATE = 1, GOT_DATABASE = 2, GOT_NAME = 3
	} state = IDLE;
	
	switch (state) {

	    case IDLE:

		if (tokenType == TK_CREATE)
			state = GOT_CREATE;
		else    /* pass token to sqlite parser -- the common case */
			sqlite3Parser(pEngine, tokenType, 
				      pParse->sLastToken, pParse);
		break;

	    case GOT_CREATE:
		  
		if (tokenType == TK_DATABASE)
			state = GOT_DATABASE;
		else {  /* return to idle, pass the CREATE token, then
			 * the current token to the sqlite parser */
			state = IDLE;
			sqlite3Parser(pEngine, TK_CREATE, 
				      pParse->sLastToken, pParse);
			sqlite3Parser(pEngine, tokenType,
				      pParse->sLastToken, pParse);
		}
		break;
		
	    case GOT_DATABASE:

		if (tokenType == TK_ID) {
			state = GOT_NAME;
			bdb_create_database(&token, pParse);
		} else
			preparserSyntaxError(&token, pParse);

		break;

	    case GOT_NAME:

		if (tokenType == TK_SEMI)
			state = IDLE;
		else
			preparserSyntaxError(&token, pParse);
		break;
	}
}
