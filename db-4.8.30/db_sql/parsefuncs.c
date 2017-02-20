/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * This file contains unused functions that are referenced by the
 * SQLite parser.  If one of these functions is actually invoked, it
 * indicates that the SQL input contains syntax that is unsupported
 * by this program.  Functions that we expect to be called by the
 * parser are in another compilation unit, buildpt.c.
 */

#include <stdio.h>
#include "db_sql.h"

/*
 * unsupported: The universal "unsupported syntax" error reporter.  In
 * most cases we'll have a parser context in hand when calling this
 * function, and the error message can be passed back to the caller by
 * setting it in the parser context.  Sometimes, however, we don't
 * have the context.  In all such cases, I expect that a subsequent
 * invocation of unsupported() will have the context and report the
 * error through the context.  Probably we could safely ignore these
 * context-less calls, but for now we'll go with a fallback of simply
 * printing the error message directly from here.
 *
 * The advantage of reporting the error through the parser context is
 * that, when the error message eventually gets printed, it will call
 * out a line number where the error was encountered.  We don't have
 * that information here, but it could be made available if this turns
 * out to be a problem.  Also, setting an error in the parser context
 * causes the program to exit after reporting the error.  Again, we
 * could do that here, but it would be ugly.
 */
static void unsupported(pParse, fname)
	Parse *pParse;
	char *fname;
{
	static char *fmt = "Unsupported SQL syntax (%s)\n";

	if (pParse)
		sqlite3ErrorMsg(pParse, fmt, fname);
	else
		fprintf(stderr, fmt, fname);
}

void sqlite3AddCheckConstraint(
  Parse *pParse,    /* Parsing context */
  Expr *pCheckExpr  /* The check expression */
)
{
	COMPQUIET(pCheckExpr, NULL);
	unsupported(pParse, "AddCheckConstraint");
}

void sqlite3AddCollateType(Parse *pParse, Token *pToken)
{
	COMPQUIET(pToken, NULL);
	unsupported(pParse, "AddCollateType");
}


void sqlite3AddDefaultValue(Parse *pParse, Expr *pExpr)
{
	COMPQUIET(pExpr, NULL);
	unsupported(pParse, "AddDefaultValue");
}

void sqlite3AddNotNull(Parse *pParse, int onError)
{
	COMPQUIET(onError, 0);
	unsupported(pParse, "AddNotNull");
}

void sqlite3AlterBeginAddColumn(Parse *pParse, SrcList *pSrc)
{
	COMPQUIET(pSrc, NULL);
	unsupported(pParse, "AlterBeginAddColumn");
}

void sqlite3AlterFinishAddColumn(Parse *pParse, Token *pColDef)
{
	COMPQUIET(pColDef, NULL);
	unsupported(pParse, "AlterFinishAddColumn");
}

void sqlite3AlterRenameTable(
  Parse *pParse,            /* Parser context. */
  SrcList *pSrc,            /* The table to rename. */
  Token *pName              /* The new table name. */
)
{
	COMPQUIET(pSrc, NULL);
	COMPQUIET(pName, NULL);
	unsupported(pParse, "AlterRenameTable");
}

void sqlite3Analyze(Parse *pParse, Token *pName1, Token *pName2)
{
	COMPQUIET(pName1, NULL);
	COMPQUIET(pName2, NULL);
	unsupported(pParse, "Analyze");
}

void sqlite3Attach(Parse *pParse, Expr *p, Expr *pDbname, Expr *pKey)
{
	COMPQUIET(p, NULL);
	COMPQUIET(pDbname, NULL);
	COMPQUIET(pKey, 0);
	unsupported(pParse, "Attach");
}

void sqlite3BeginTransaction(Parse *pParse, int type)
{
	COMPQUIET(type, 0);
	unsupported(pParse, "BeginTransaction");
}

void sqlite3BeginTrigger(
  Parse *pParse,      /* The parse context of the CREATE TRIGGER statement */
  Token *pName1,      /* The name of the trigger */
  Token *pName2,      /* The name of the trigger */
  int tr_tm,          /* One of TK_BEFORE, TK_AFTER, TK_INSTEAD */
  int op,             /* One of TK_INSERT, TK_UPDATE, TK_DELETE */
  IdList *pColumns,   /* column list if this is an UPDATE OF trigger */
  SrcList *pTableName,/* The name of the table/view the trigger applies to */
  Expr *pWhen,        /* WHEN clause */
  int isTemp,         /* True if the TEMPORARY keyword is present */
  int noErr           /* Suppress errors if the trigger already exists */
)
{
	COMPQUIET(pName1, NULL);
	COMPQUIET(pName2, NULL);
	COMPQUIET(tr_tm, 0);
	COMPQUIET(op, 0);
	COMPQUIET(pColumns, NULL);
	COMPQUIET(pTableName, NULL);
	COMPQUIET(pWhen, NULL);
	COMPQUIET(isTemp, 0);
	COMPQUIET(noErr, 0);
	unsupported(pParse, "BeginTrigger");
}

void sqlite3CommitTransaction(Parse *pParse)
{
	unsupported(pParse, "CommitTransaction");
}



void sqlite3CreateView(
  Parse *pParse,     /* The parsing context */
  Token *pBegin,     /* The CREATE token that begins the statement */
  Token *pName1,     /* The token that holds the name of the view */
  Token *pName2,     /* The token that holds the name of the view */
  Select *pSelect,   /* A SELECT statement that will become the new view */
  int isTemp,        /* TRUE for a TEMPORARY view */
  int noErr          /* Suppress error messages if VIEW already exists */
)
{
	COMPQUIET(pBegin, NULL);
	COMPQUIET(pName1, NULL);
	COMPQUIET(pName2, NULL);
	COMPQUIET(pSelect, NULL);
	COMPQUIET(isTemp, 0);
	COMPQUIET(noErr, 0);
	unsupported(pParse, "CreateView");
}

void sqlite3DeleteFrom(
  Parse *pParse,         /* The parser context */
  SrcList *pTabList,     /* The table from which we should delete things */
  Expr *pWhere           /* The WHERE clause.  May be null */
)
{
	COMPQUIET(pTabList, NULL);
	COMPQUIET(pWhere, NULL);
	unsupported(pParse, "DeleteFrom");
}

void sqlite3DeleteTriggerStep(TriggerStep *pTriggerStep)
{
	COMPQUIET(pTriggerStep, NULL);
	unsupported(0, "DeleteTriggerStep");
}

void sqlite3Detach(Parse *pParse, Expr *pDbname)
{
	COMPQUIET(pDbname, NULL);
	unsupported(pParse, "Detach");
}

void sqlite3DropIndex(Parse *pParse, SrcList *pName, int ifExists)
{
	COMPQUIET(pName, NULL);
	COMPQUIET(ifExists, 0);
	unsupported(pParse, "DropIndex");
}

void sqlite3DropTable(Parse *pParse, SrcList *pName, int isView, int noErr)
{
	COMPQUIET(pName, NULL);
	COMPQUIET(isView, 0);
	COMPQUIET(noErr, 0);
	unsupported(pParse, "DropTable");
}

void sqlite3DropTrigger(Parse *pParse, SrcList *pName, int noErr)
{
	COMPQUIET(pName, NULL);
	COMPQUIET(noErr, 0);
	unsupported(pParse, "DropTrigger");
}

void sqlite3ExprAssignVarNumber(Parse *pParse, Expr *pExpr)
{
	COMPQUIET(pExpr, NULL);
	unsupported(pParse, "ExprAssignVarNumber");
}

void sqlite3ExprDelete(Expr *p)
{
	COMPQUIET(p, NULL);
	unsupported(0, "ExprDelete");
}

Expr *sqlite3ExprFunction(Parse *pParse, ExprList *pList, Token *pToken)
{
	COMPQUIET(pList, NULL);
	COMPQUIET(pToken, NULL);
	unsupported(pParse, "ExprFunction");
	return NULL;
}

void sqlite3ExprListDelete(ExprList *pList)
{
	COMPQUIET(pList, NULL);
	unsupported(0, "ExprListDelete");
}

Expr *sqlite3ExprSetColl(Parse *pParse, Expr *pExpr, Token *pName)
{
	COMPQUIET(pExpr, NULL);
	COMPQUIET(pName, NULL);
	unsupported(pParse, "ExprSetColl");
	return NULL;
}


void sqlite3ExprSetHeight(Expr *p)
{
	COMPQUIET(p, NULL);
	unsupported(0, "ExprSetHeight");
}

void sqlite3ExprSpan(Expr *pExpr, Token *pLeft, Token *pRight)
{
	COMPQUIET(pExpr, NULL);
	COMPQUIET(pLeft, NULL);
	COMPQUIET(pRight, NULL);
	unsupported(0, "ExprSpan");
}

void sqlite3FinishTrigger(
  Parse *pParse,          /* Parser context */
  TriggerStep *pStepList, /* The triggered program */
  Token *pAll             /* Token that describes the complete CREATE TRIGGER */
)
{
	COMPQUIET(pStepList, NULL);
	COMPQUIET(pAll, NULL);
	unsupported(pParse, "FinishTrigger");
}

IdList *sqlite3IdListAppend(sqlite3 *db, IdList *pList, Token *pToken)
{
	COMPQUIET(db, NULL);
	COMPQUIET(pList, NULL);
	COMPQUIET(pToken, NULL);
	unsupported(0, "IdListAppend");
	return NULL;
}

void sqlite3IdListDelete(IdList *pList)
{
	COMPQUIET(pList, NULL);
	unsupported(0, "IdListDelete");
}

void sqlite3Insert(
  Parse *pParse,        /* Parser context */
  SrcList *pTabList,    /* Name of table into which we are inserting */
  ExprList *pList,      /* List of values to be inserted */
  Select *pSelect,      /* A SELECT statement to use as the data source */
  IdList *pColumn,      /* Column names corresponding to IDLIST. */
  int onError           /* How to handle constraint errors */
)
{
	COMPQUIET(pTabList, NULL);
	COMPQUIET(pList, NULL);
	COMPQUIET(pSelect, NULL);
	COMPQUIET(pColumn, NULL);
	COMPQUIET(onError, 0);
	unsupported(pParse, "Insert");
}

int sqlite3JoinType(Parse *pParse, Token *pA, Token *pB, Token *pC)
{
	COMPQUIET(pA, NULL);
	COMPQUIET(pB, NULL);
	COMPQUIET(pC, NULL);
	unsupported(pParse, "JoinType");
	return 0;
}

Expr *sqlite3PExpr(
  Parse *pParse,          /* Parsing context */
  int op,                 /* Expression opcode */
  Expr *pLeft,            /* Left operand */
  Expr *pRight,           /* Right operand */
  const Token *pToken     /* Argument token */
)
{
	COMPQUIET(op, 0);
	COMPQUIET(pLeft, NULL);
	COMPQUIET(pRight, NULL);
	COMPQUIET(pToken, NULL);
	unsupported(pParse, "PExpr");
	return NULL;
}

void sqlite3Pragma(
  Parse *pParse, 
  Token *pId1,        /* First part of [database.]id field */
  Token *pId2,        /* Second part of [database.]id field, or NULL */
  Token *pValue,      /* Token for <value>, or NULL */
  int minusFlag       /* True if a '-' sign preceded <value> */
)
{
	COMPQUIET(pId1, NULL);
	COMPQUIET(pId2, NULL);
	COMPQUIET(pValue, NULL);
	COMPQUIET(minusFlag, 0);
	unsupported(pParse, "Pragma");
}

Expr *sqlite3RegisterExpr(Parse *pParse, Token *pToken)
{
	COMPQUIET(pToken, NULL);
	unsupported(pParse, "RegisterExpr");
	return NULL;
}

void sqlite3Reindex(Parse *pParse, Token *pName1, Token *pName2)
{
	COMPQUIET(pName1, NULL);
	COMPQUIET(pName2, NULL);
	unsupported(pParse, "Reindex");
}

void sqlite3RollbackTransaction(Parse *pParse)
{
	unsupported(pParse, "RollbackTransaction");
}

int sqlite3Select(
  Parse *pParse,         /* The parser context */
  Select *p,             /* The SELECT statement being coded. */
  SelectDest *pDest,     /* What to do with the query results */
  Select *pParent,       /* Another SELECT for which this is a sub-query */
  int parentTab,         /* Index in pParent->pSrc of this query */
  int *pParentAgg,       /* True if pParent uses aggregate functions */
  char *aff              /* If eDest is SRT_Union, the affinity string */
)
{
	COMPQUIET(p, NULL);
	COMPQUIET(pDest, NULL);
	COMPQUIET(pParent, NULL);
	COMPQUIET(parentTab, 0);
	COMPQUIET(pParentAgg, NULL);
	COMPQUIET(aff, NULL);
	unsupported(pParse, "Select");
	return 0;
}

void sqlite3SelectDelete(Select *p)
{
	COMPQUIET(p, NULL);
	unsupported(0, "SelectDelete");
}

Select *sqlite3SelectNew(
  Parse *pParse,        /* Parsing context */
  ExprList *pEList,     /* which columns to include in the result */
  SrcList *pSrc,        /* the FROM clause -- which tables to scan */
  Expr *pWhere,         /* the WHERE clause */
  ExprList *pGroupBy,   /* the GROUP BY clause */
  Expr *pHaving,        /* the HAVING clause */
  ExprList *pOrderBy,   /* the ORDER BY clause */
  int isDistinct,       /* true if the DISTINCT keyword is present */
  Expr *pLimit,         /* LIMIT value.  NULL means not used */
  Expr *pOffset         /* OFFSET value.  NULL means no offset */
)
{
	COMPQUIET(pEList, NULL);
	COMPQUIET(pSrc, NULL);
	COMPQUIET(pWhere, NULL);
	COMPQUIET(pGroupBy, NULL);
	COMPQUIET(pHaving, NULL);
	COMPQUIET(pOrderBy, NULL);
	COMPQUIET(isDistinct, 0);
	COMPQUIET(pLimit, NULL);
	COMPQUIET(pOffset, NULL);
	unsupported(pParse, NULL);
	return NULL;
}

SrcList *sqlite3SrcListAppendFromTerm(
  Parse *pParse,          /* Parsing context */
  SrcList *p,             /* The left part of the FROM clause already seen */
  Token *pTable,          /* Name of the table to add to the FROM clause */
  Token *pDatabase,       /* Name of the database containing pTable */
  Token *pAlias,          /* The right-hand side of the AS subexpression */
  Select *pSubquery,      /* A subquery used in place of a table name */
  Expr *pOn,              /* The ON clause of a join */
  IdList *pUsing          /* The USING clause of a join */
)
{
	COMPQUIET(p, NULL);
	COMPQUIET(pTable, NULL);
	COMPQUIET(pDatabase, NULL);
	COMPQUIET(pAlias, NULL);
	COMPQUIET(pSubquery, NULL);
	COMPQUIET(pOn, NULL);
	COMPQUIET(pUsing, NULL);
	unsupported(pParse, "SrcListAppendFromTerm");
	return NULL;
}

void sqlite3SrcListDelete(SrcList *pList)
{
	COMPQUIET(pList, NULL);
	unsupported(0, "SrcListDelete");
}

void sqlite3SrcListShiftJoinType(SrcList *p)
{
	COMPQUIET(p, NULL);
	unsupported(0, "SrcListShiftJoinType");
}

TriggerStep *sqlite3TriggerDeleteStep(
  sqlite3 *db,            /* Database connection */
  Token *pTableName,      /* The table from which rows are deleted */
  Expr *pWhere            /* The WHERE clause */
)
{
	COMPQUIET(db, NULL);
	COMPQUIET(pTableName, NULL);
	COMPQUIET(pWhere, NULL);
	unsupported(0, "TriggerDeleteStep");
	return NULL;
}

TriggerStep *sqlite3TriggerInsertStep(
  sqlite3 *db,        /* The database connection */
  Token *pTableName,  /* Name of the table into which we insert */
  IdList *pColumn,    /* List of columns in pTableName to insert into */
  ExprList *pEList,   /* The VALUE clause: a list of values to be inserted */
  Select *pSelect,    /* A SELECT statement that supplies values */
  int orconf          /* The conflict algorithm (OE_Abort, OE_Replace, etc.) */
)
{
	COMPQUIET(db, NULL);
	COMPQUIET(pTableName, NULL);
	COMPQUIET(pColumn, NULL);
	COMPQUIET(pEList, NULL);
	COMPQUIET(pSelect, NULL);
	COMPQUIET(orconf, 0);
	unsupported(0, "TriggerInsertStep");
	return NULL;
}

TriggerStep *sqlite3TriggerSelectStep(sqlite3 *db, Select *pSelect)
{
	COMPQUIET(db, NULL);
	COMPQUIET(pSelect, NULL);
	unsupported(0, "TriggerSelectStep");
	return NULL;
}

TriggerStep *sqlite3TriggerUpdateStep(
  sqlite3 *db,         /* The database connection */
  Token *pTableName,   /* Name of the table to be updated */
  ExprList *pEList,    /* The SET clause: list of column and new values */
  Expr *pWhere,        /* The WHERE clause */
  int orconf           /* The conflict algorithm. (OE_Abort, OE_Ignore, etc) */
)
{
	COMPQUIET(db, NULL);
	COMPQUIET(pTableName, NULL);
	COMPQUIET(pEList, NULL);
	COMPQUIET(pWhere, NULL);
	COMPQUIET(orconf, 0);
	unsupported(0, "TriggerUpdateStep");
	return NULL;
}

void sqlite3Update(
  Parse *pParse,         /* The parser context */
  SrcList *pTabList,     /* The table in which we should change things */
  ExprList *pChanges,    /* Things to be changed */
  Expr *pWhere,          /* The WHERE clause.  May be null */
  int onError            /* How to handle constraint errors */
)
{
	COMPQUIET(pTabList, NULL);
	COMPQUIET(pChanges, NULL);
	COMPQUIET(pWhere, NULL);
	COMPQUIET(onError, 0);
	unsupported(pParse, "Update");
}

void sqlite3Vacuum(Parse *pParse)
{
	unsupported(pParse, "Vacuum");
}

void sqlite3VtabArgExtend(Parse *pParse, Token *p)
{
	COMPQUIET(p, NULL);
	unsupported(pParse, "VtabArgExtend");
}

void sqlite3VtabArgInit(Parse *pParse)
{
	unsupported(pParse, "VtabArgInit");
}

void sqlite3VtabBeginParse(
  Parse *pParse,        /* Parsing context */
  Token *pName1,        /* Name of new table, or database name */
  Token *pName2,        /* Name of new table or NULL */
  Token *pModuleName    /* Name of the module for the virtual table */
)
{
	COMPQUIET(pName1, NULL);
	COMPQUIET(pName2, NULL);
	COMPQUIET(pModuleName, NULL);
	unsupported(pParse, "VtabBeginParse");
}

void sqlite3VtabFinishParse(Parse *pParse, Token *pEnd)
{
	COMPQUIET(pEnd, NULL);
	unsupported(pParse, "VtabFinishParse");
}

void sqlite3DeleteTable(Table *pTable){
	COMPQUIET(pTable, NULL);
	unsupported(0, "DeleteTable");
}

void sqlite3DeleteTrigger(Trigger *pTrigger){
	COMPQUIET(pTrigger, NULL);
	unsupported(0, "DeleteTrigger");
}
