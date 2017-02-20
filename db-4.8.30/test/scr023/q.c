/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

#include <sys/types.h>
#include <sys/time.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"
#include "shqueue.h"

typedef enum {
	FORWARD_WALK_FAILED = 1,
	FOREACH_WALK_FAILED,
	LIST_END_NOT_MARKED_FAILURE,
	PREV_WALK_FAILED,
	REVERSE_FOREACH_WALK_FAILED,
	EXPECTED_HEAD_FAILED
} FAILURE_REASON;

const char *failure_reason_names[] = {
	"",
	"walking the list using the _NEXT forward failed",
	"walking the list using the _FOREACH macro failed",
	"what was expected to be the last element wasn't marked as such",
	"walking the list using the _PREV macro failed",
	"walking the list using the _REVERSE_FOREACH macro failed",
	"expected to be at the head of the list"
};

SH_LIST_HEAD(sh_lq);
struct sh_le {
	char content;
	SH_LIST_ENTRY sh_les;
};

/* create a string from the content of a list queue */
char *
sh_l_as_string(l)
	struct sh_lq *l;
{
	static char buf[1024];
	struct sh_le *ele = SH_LIST_FIRST(l, sh_le);
	int i = 1;

	buf[0] = '"';
	while (ele != NULL) {
		buf[i] = ele->content;
		ele = SH_LIST_NEXT(ele, sh_les, sh_le);
		if (ele != NULL)
			buf[++i] = ' ';
		i++;
	}
	buf[i++] = '"';
	buf[i] = '\0';
	return buf;
}

/* init a list queue */
struct sh_lq *
sh_l_init(items)
	const char *items;
{
	const char *c = items;
	struct sh_le *ele = NULL, *last_ele = (struct sh_le*)-1;
	struct sh_lq *l = calloc(1, sizeof(struct sh_lq));

	SH_LIST_INIT(l);

	while (*c != '\0') {
		if (c[0] != ' ') {
			last_ele = ele;
			ele = calloc(1, sizeof(struct sh_le));
			ele->content = c[0];
			if (SH_LIST_EMPTY(l))
				SH_LIST_INSERT_HEAD(l, ele, sh_les, sh_le);
			else
				SH_LIST_INSERT_AFTER(
				    last_ele, ele, sh_les, sh_le);
		}
		c++;
	}
	return (l);
}

struct sh_lq *
sh_l_remove_head(l)
	struct sh_lq *l;
{
	struct sh_le *ele = SH_LIST_FIRST(l, sh_le);

	SH_LIST_REMOVE_HEAD(l, sh_les, sh_le);
	if (ele != NULL)
		free(ele);

	return (l);
}

struct sh_lq *
sh_l_remove_tail(l)
	struct sh_lq *l;
{
	struct sh_le *ele = SH_LIST_FIRST(l, sh_le);

	if (SH_LIST_EMPTY(l))
		return (l);

	while (SH_LIST_NEXT(ele, sh_les, sh_le) != NULL)
		ele = SH_LIST_NEXT(ele, sh_les, sh_le);

	if (ele) {
		SH_LIST_REMOVE(ele, sh_les, sh_le);
		free(ele);
	}
	return (l);
}

struct sh_lq *
sh_l_remove_item(l, item)
	struct sh_lq *l;
	const char *item;
{
	struct sh_le *ele = SH_LIST_FIRST(l, sh_le);

	while (ele != NULL) {
		if (ele->content == item[0])
			break;
		ele = SH_LIST_NEXT(ele, sh_les, sh_le);
	}
	if (ele)
		SH_LIST_REMOVE(ele, sh_les, sh_le);
	return (l);
}

struct sh_lq *
sh_l_insert_head(l, item)
	struct sh_lq *l;
	const char *item;
{
	struct sh_le *ele = calloc(1, sizeof(struct sh_le));

	ele->content = item[0];
	SH_LIST_INSERT_HEAD(l, ele, sh_les, sh_le);

	return (l);
}

struct sh_lq *
sh_l_insert_tail(l, item)
	struct sh_lq *l;
	const char *item;
{
	struct sh_le *ele = NULL;
	struct sh_le *last_ele = SH_LIST_FIRST(l, sh_le);

	if (last_ele != NULL)
		while (SH_LIST_NEXT(last_ele, sh_les, sh_le) != NULL)
			last_ele = SH_LIST_NEXT(last_ele, sh_les, sh_le);

	if (last_ele == NULL) {
		ele = calloc(1, sizeof(struct sh_le));
		ele->content = item[0];
		SH_LIST_INSERT_HEAD(l, ele, sh_les, sh_le);
	} else {
		ele = calloc(1, sizeof(struct sh_le));
		ele->content = item[0];
		SH_LIST_INSERT_AFTER(last_ele, ele, sh_les, sh_le);
	}

	return (l);
}

struct sh_lq *
sh_l_insert_before(l, item, before_item)
	struct sh_lq *l;
	const char *item;
	const char *before_item;
{
	struct sh_le *ele = NULL;
	struct sh_le *before_ele = SH_LIST_FIRST(l, sh_le);

	while (before_ele != NULL) {
		if (before_ele->content == before_item[0])
			break;
		before_ele = SH_LIST_NEXT(before_ele, sh_les, sh_le);
	}
	if (before_ele != NULL) {
		ele = calloc(1, sizeof(struct sh_le));
		ele->content = item[0];
		SH_LIST_INSERT_BEFORE(l, before_ele, ele, sh_les, sh_le);
	}
	return (l);
}

struct sh_lq *
sh_l_insert_after(l, item, after_item)
	struct sh_lq *l;
	const char *item;
	const char *after_item;
{
	struct sh_le *ele = NULL;
	struct sh_le *after_ele = SH_LIST_FIRST(l, sh_le);

	while (after_ele != NULL) {
		if (after_ele->content == after_item[0])
			break;
		after_ele = SH_LIST_NEXT(after_ele, sh_les, sh_le);
	}
	if (after_ele != NULL) {
		ele = calloc(1, sizeof(struct sh_le));
		ele->content = item[0];
		SH_LIST_INSERT_AFTER(after_ele, ele, sh_les, sh_le);
	}
	return (l);
}

void
sh_l_discard(l)
	struct sh_lq *l;
{
	struct sh_le *ele = NULL;

	while ((ele = SH_LIST_FIRST(l, sh_le)) != NULL) {
		SH_LIST_REMOVE(ele, sh_les, sh_le);
		free(ele);
	}

	free(l);
}

int
sh_l_verify(l, items)
	struct sh_lq *l;
	const char *items;
{
	const char *c = items;
	struct sh_le *ele = NULL, *lele = NULL;
	int i = 0, nele = 0;

	while (*c != '\0') {
		if (c[0] != ' ')
			nele++;
		c++;
	}

	/* use the FOREACH macro to walk the list */
	c = items;
	i = 0;
	SH_LIST_FOREACH(ele, l, sh_les, sh_le) {
		if (ele->content != c[0])
			return (FOREACH_WALK_FAILED);
		i++;
		c +=2;
	}
	if (i != nele)
		return (FOREACH_WALK_FAILED);
	i = 0;
	if (items[0] != '\0') {
		/* walk the list forward */
		c = items;
		ele = SH_LIST_FIRST(l, sh_le);
		while (*c != '\0') {
			lele = ele;
			if (c[0] != ' ') {
				if (ele->content != c[0])
					  return (FORWARD_WALK_FAILED);
				i++;
				ele = SH_LIST_NEXT(ele, sh_les, sh_le);
			}
			c++;
		}
		ele = lele;

		if (i != nele)
			return (FOREACH_WALK_FAILED);

		/* ele should be the last element in the list... */
		/* ... so sle_next should be -1 */
		if (ele->sh_les.sle_next != -1)
			return (LIST_END_NOT_MARKED_FAILURE);

		/* and NEXT needs to be NULL */
		if (SH_LIST_NEXT(ele, sh_les, sh_le) != NULL)
			return (LIST_END_NOT_MARKED_FAILURE);

		/*
		 * walk the list backwards using PREV macro, first move c
		 * back a bit
		 */
		c--;
		i = 0;
		while (c >= items) {
			if (c[0] != ' ') {
				lele = ele;
				if (ele->content != c[0])
					return (PREV_WALK_FAILED);
				ele = SH_LIST_PREV(ele, sh_les, sh_le);
				i++;
			}
			c--;
		}
		ele = lele;

		if (i != nele)
			return (PREV_WALK_FAILED);

		if (ele != SH_LIST_FIRST(l, sh_le))
			return (EXPECTED_HEAD_FAILED);
	}
	return (0);
}

SH_TAILQ_HEAD(sh_tq);
struct sh_te {
	char content;
	SH_TAILQ_ENTRY sh_tes;
};

/* create a string from the content of a list queue */
char *
sh_t_as_string(l)
	struct sh_tq *l;
{
	static char buf[1024];
	struct sh_te *ele = SH_TAILQ_FIRST(l, sh_te);
	int i = 1;

	buf[0] = '"';
	while (ele != NULL) {
		buf[i] = ele->content;
		ele = SH_TAILQ_NEXT(ele, sh_tes, sh_te);
		if (ele != NULL)
			buf[++i] = ' ';
		i++;
	}
	buf[i++] = '"';
	buf[i] = '\0';
	return (buf);
}

/* init a tail queue */
struct sh_tq *
sh_t_init(items)
	const char *items;
{
	const char *c = items;
	struct sh_te *ele = NULL, *last_ele = (struct sh_te*)-1;
	struct sh_tq *l = calloc(1, sizeof(struct sh_tq));

	SH_TAILQ_INIT(l);

	while (*c != '\0') {
		if (c[0] != ' ') {
			ele = calloc(1, sizeof(struct sh_te));
			ele->content = c[0];

			if (SH_TAILQ_EMPTY(l))
				SH_TAILQ_INSERT_HEAD(l, ele, sh_tes, sh_te);
			else
				SH_TAILQ_INSERT_AFTER(
				    l, last_ele, ele, sh_tes, sh_te);
			last_ele = ele;
		}
		c++;
	}
	return (l);
}

struct sh_tq *
sh_t_remove_head(l)
	struct sh_tq *l;
{
	struct sh_te *ele = SH_TAILQ_FIRST(l, sh_te);

	if (ele != NULL)
		SH_TAILQ_REMOVE(l, ele, sh_tes, sh_te);

	free(ele);

	return (l);
}

struct sh_tq *
sh_t_remove_tail(l)
	struct sh_tq *l;
{
	struct sh_te *ele = SH_TAILQ_FIRST(l, sh_te);

	if (SH_TAILQ_EMPTY(l))
		return (l);

	while (SH_TAILQ_NEXT(ele, sh_tes, sh_te) != NULL)
		ele = SH_TAILQ_NEXT(ele, sh_tes, sh_te);

	if (ele != NULL) {
		SH_TAILQ_REMOVE(l, ele, sh_tes, sh_te);
		free(ele);
	}

	return (l);
}

struct sh_tq *
sh_t_remove_item(l, item)
	struct sh_tq *l;
	const char *item;
{
	struct sh_te *ele = SH_TAILQ_FIRST(l, sh_te);

	while (ele != NULL) {
		if (ele->content == item[0])
			break;
		ele = SH_TAILQ_NEXT(ele, sh_tes, sh_te);
	}
	if (ele != NULL)
		SH_TAILQ_REMOVE(l, ele, sh_tes, sh_te);

	return (l);
}

struct sh_tq *
sh_t_insert_head(l, item)
	struct sh_tq *l;
	const char *item;
{
	struct sh_te *ele = calloc(1, sizeof(struct sh_te));

	ele->content = item[0];
	SH_TAILQ_INSERT_HEAD(l, ele, sh_tes, sh_te);

	return (l);
}

struct sh_tq *
sh_t_insert_tail(l, item)
	struct sh_tq *l;
	const char *item;
{
	struct sh_te *ele = 0;
	ele = calloc(1, sizeof(struct sh_te));
	ele->content = item[0];
	SH_TAILQ_INSERT_TAIL(l, ele, sh_tes);
	return l;
}

struct sh_tq *
sh_t_insert_before(l, item, before_item)
	struct sh_tq *l;
	const char *item;
	const char *before_item;
{
	struct sh_te *ele = NULL;
	struct sh_te *before_ele = SH_TAILQ_FIRST(l, sh_te);

	while (before_ele != NULL) {
		if (before_ele->content == before_item[0])
			break;
		before_ele = SH_TAILQ_NEXT(before_ele, sh_tes, sh_te);
	}

	if (before_ele != NULL) {
		ele = calloc(1, sizeof(struct sh_te));
		ele->content = item[0];
		SH_TAILQ_INSERT_BEFORE(l, before_ele, ele, sh_tes, sh_te);
	}

	return (l);
}

struct sh_tq *
sh_t_insert_after(l, item, after_item)
	struct sh_tq *l;
	const char *item;
	const char *after_item;
{
	struct sh_te *ele = NULL;
	struct sh_te *after_ele = SH_TAILQ_FIRST(l, sh_te);

	while (after_ele != NULL) {
		if (after_ele->content == after_item[0])
			break;
		after_ele = SH_TAILQ_NEXT(after_ele, sh_tes, sh_te);
	}

	if (after_ele != NULL) {
		ele = calloc(1, sizeof(struct sh_te));
		ele->content = item[0];
		SH_TAILQ_INSERT_AFTER(l, after_ele, ele, sh_tes, sh_te);
	}

	return (l);
}

void
sh_t_discard(l)
	struct sh_tq *l;
{
	struct sh_te *ele = NULL;

	while ((ele = SH_TAILQ_FIRST(l, sh_te)) != NULL) {
		SH_TAILQ_REMOVE(l, ele, sh_tes, sh_te);
		free(ele);
	}
	free(l);
}

int
sh_t_verify(l, items)
	struct sh_tq *l;
	const char *items;
{
	const char *c = items, *b = NULL;
	struct sh_te *ele = NULL, *lele = NULL;
	int i = 0, nele = 0;

	while (*c != '\0') {
		if (c[0] != ' ')
			nele++;
		c++;
	}

	/* use the FOREACH macro to walk the list */
	c = items;
	i = 0;
	SH_TAILQ_FOREACH(ele, l, sh_tes, sh_te) {
		if (ele->content != c[0])
			return (FOREACH_WALK_FAILED);
		i++;
		c +=2;
	}
	if (i != nele)
		return (FOREACH_WALK_FAILED);
	i = 0;
	if (items[0] != '\0') {
		/* walk the list forward */
		c = items;
		ele = SH_TAILQ_FIRST(l, sh_te);
		while (*c != '\0') {
			lele = ele;
			if (c[0] != ' ') {
				if (ele->content != c[0])
					return (FORWARD_WALK_FAILED);
				i++;
				ele = SH_TAILQ_NEXT(ele, sh_tes, sh_te);
			}
			c++;
		}

		if (i != nele)
			return (FOREACH_WALK_FAILED);

		if (lele != SH_TAILQ_LAST(l, sh_tes, sh_te))
			return (LIST_END_NOT_MARKED_FAILURE);
		ele = lele;

		/* ele should be the last element in the list... */
		/* ... so sle_next should be -1 */
		if (ele->sh_tes.stqe_next != -1)
			return (LIST_END_NOT_MARKED_FAILURE);

		/* and NEXT needs to be NULL */
		if (SH_TAILQ_NEXT(ele, sh_tes, sh_te) != NULL)
			return (LIST_END_NOT_MARKED_FAILURE);

		/* walk the list backwards using SH_LIST_PREV macro */
		c--;
		b = c;
		i = 0;
		while (c >= items) {
			if (c[0] != ' ') {
				lele = ele;
				if (ele->content != c[0])
					return (PREV_WALK_FAILED);
				ele = SH_TAILQ_PREV(l, ele, sh_tes, sh_te);
				i++;
			}
			c--;
		}
		ele = lele;

		if (i != nele)
			return (PREV_WALK_FAILED);

		if (ele != SH_TAILQ_FIRST(l, sh_te))
			return (-1);

		/* c should be the last character in the array, walk backwards
		from here using FOREACH_REVERSE and check the values again */
		c = b;
		i = 0;
		ele = SH_TAILQ_LAST(l, sh_tes, sh_te);
		SH_TAILQ_FOREACH_REVERSE(ele, l, sh_tes, sh_te) {
			if (ele->content != c[0])
				return (REVERSE_FOREACH_WALK_FAILED);
			i++;
			c -=2;
		}
		if (i != nele)
			return (REVERSE_FOREACH_WALK_FAILED);
	}
	return (0);
}

int
sh_t_verify_TAILQ_LAST(l, items)
	struct sh_tq *l;
	const char *items;
{
	const char *c = items;
	struct sh_te *ele = NULL;

	c = items;
	while (*c != '\0') {
		c++;
	}
	if (c == items) {
	  /* items is empty, so last should be NULL */
	  if (SH_TAILQ_LAST(l, sh_tes, sh_te) != NULL)
		return (-1);
	} else {
		c--;
		ele = SH_TAILQ_LAST(l, sh_tes, sh_te);
		if (ele->content != c[0])
			return (-1);
	}
	return (0);
}

typedef void *qds_t;
struct {
	const char *name;
	qds_t *(*f_init)(const char *);
	qds_t *(*f_remove_head)(qds_t *);
	qds_t *(*f_remove_tail)(qds_t *);
	qds_t *(*f_remove_item)(qds_t *, const char *);
	qds_t *(*f_insert_head)(qds_t *, const char *);
	qds_t *(*f_insert_tail)(qds_t *, const char *);
	qds_t *(*f_insert_before)(qds_t *, const char *, const char *);
	qds_t *(*f_insert_after)(qds_t *, const char *, const char *);
	qds_t *(*f_discard)(qds_t *);
	char *(*f_as_string)(qds_t *);
	int (*f_verify)(qds_t *, const char *);
} qfns[]= {
{	"sh_list",
	(qds_t*(*)(const char *))sh_l_init,
	(qds_t*(*)(qds_t *))sh_l_remove_head,
	(qds_t*(*)(qds_t *))sh_l_remove_tail,
	(qds_t*(*)(qds_t *, const char *))sh_l_remove_item,
	(qds_t*(*)(qds_t *, const char *))sh_l_insert_head,
	(qds_t*(*)(qds_t *, const char *))sh_l_insert_tail,
	(qds_t*(*)(qds_t *, const char *, const char *))sh_l_insert_before,
	(qds_t*(*)(qds_t *, const char *, const char *))sh_l_insert_after,
	(qds_t*(*)(qds_t *))sh_l_discard,
	(char *(*)(qds_t *))sh_l_as_string,
	(int(*)(qds_t *, const char *))sh_l_verify },
{	"sh_tailq",
	(qds_t*(*)(const char *))sh_t_init,
	(qds_t*(*)(qds_t *))sh_t_remove_head,
	(qds_t*(*)(qds_t *))sh_t_remove_tail,
	(qds_t*(*)(qds_t *, const char *))sh_t_remove_item,
	(qds_t*(*)(qds_t *, const char *))sh_t_insert_head,
	(qds_t*(*)(qds_t *, const char *))sh_t_insert_tail,
	(qds_t*(*)(qds_t *, const char *, const char *))sh_t_insert_before,
	(qds_t*(*)(qds_t *, const char *, const char *))sh_t_insert_after,
	(qds_t*(*)(qds_t *))sh_t_discard,
	(char *(*)(qds_t *))sh_t_as_string,
	(int(*)(qds_t *, const char *))sh_t_verify }
};

typedef enum {
	INSERT_BEFORE,
	INSERT_AFTER,
	INSERT_HEAD,
	INSERT_TAIL,
	REMOVE_HEAD,
	REMOVE_ITEM,
	REMOVE_TAIL,
} OP;

const char *op_names[] = {
	"INSERT_BEFORE",
	"INSERT_AFTER",
	"INSERT_HEAD",
	"INSERT_TAIL",
	"REMOVE_HEAD",
	"REMOVE_ITEM",
	"REMOVE_TAIL" };

struct {
	char *init;		/* initial state. */
	char *final;		/* final state. */
	char *elem;		/* element to operate on */
	char *insert;		/* element to insert */
	OP	op;		/* operation. */
} ops[] = {

	/* most operations on a empty list */
	{ "", "", NULL, NULL, REMOVE_HEAD },
	{ "", "", NULL, NULL, REMOVE_TAIL },
	{ "", "A", NULL, "A", INSERT_HEAD },
	{ "", "A", NULL, "A", INSERT_TAIL },

	/* all operations on a one element list */
	{ "A", "", NULL, NULL, REMOVE_HEAD },
	{ "A", "", NULL, NULL, REMOVE_TAIL },
	{ "A", "", "A", NULL, REMOVE_ITEM },
	{ "B", "A B", NULL, "A", INSERT_HEAD },
	{ "A", "A B", NULL, "B", INSERT_TAIL },
	{ "B", "A B", "B", "A", INSERT_BEFORE },
	{ "A", "A B", "A", "B", INSERT_AFTER },

	/* all operations on a two element list */
	{ "A B", "B", NULL, NULL, REMOVE_HEAD },
	{ "A B", "A", NULL, NULL, REMOVE_TAIL },
	{ "A B", "A", "B", NULL, REMOVE_ITEM },
	{ "A B", "B", "A", NULL, REMOVE_ITEM },
	{ "B C", "A B C", NULL, "A", INSERT_HEAD },
	{ "A B", "A B C", NULL, "C", INSERT_TAIL },
	{ "B C", "A B C", "B", "A", INSERT_BEFORE },
	{ "A C", "A B C", "C", "B", INSERT_BEFORE },
	{ "A C", "A B C", "A", "B", INSERT_AFTER },
	{ "A C", "A C B", "C", "B", INSERT_AFTER },

	/* all operations on a three element list */

	{ "A B C", "B C", NULL, NULL, REMOVE_HEAD },
	{ "A B C", "A B", NULL, NULL, REMOVE_TAIL },
	{ "A B C", "A B", "C", NULL, REMOVE_ITEM },
	{ "A B C", "A C", "B", NULL, REMOVE_ITEM },
	{ "A B C", "B C", "A", NULL, REMOVE_ITEM },
	{ "B C D", "A B C D", NULL, "A", INSERT_HEAD },
	{ "A B C", "A B C D", NULL, "D", INSERT_TAIL },
	{ "A B C", "X A B C", "A", "X", INSERT_BEFORE },
	{ "A B C", "A X B C", "B", "X", INSERT_BEFORE },
	{ "A B C", "A B X C", "C", "X", INSERT_BEFORE },
	{ "A B C", "A X B C", "A", "X", INSERT_AFTER },
	{ "A B C", "A B X C", "B", "X", INSERT_AFTER },
	{ "A B C", "A B C X", "C", "X", INSERT_AFTER },
};

int
main(argc, argv)
	int argc;
	char *argv[];
{
	void *list;
	int fc, tc;		/* tc is total count, fc is failed count */
	int eval, i, t, result;

	eval = 0;
	for (t = 0; t < sizeof(qfns) / sizeof(qfns[0]); ++t) {
		fc = tc = 0;
		printf("TESTING: %s\n", qfns[t].name);

		for (i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
			list = qfns[t].f_init(ops[i].init);
			result = qfns[t].f_verify(list, ops[i].init);
			if (result == 0) {
				fc++;
				putchar('.');
			} else {
				putchar('+'); /* + means failed before op */
				printf("\nVerify failed: %s\n",
				    failure_reason_names[result]);
				eval = 1;
			}
			if (!strcmp("sh_tailq", qfns[t].name)) {
				result =
				    sh_t_verify_TAILQ_LAST(list, ops[i].init);
			}
#ifdef VERBOSE
			printf("\ncase %d %s in %s init: \"%s\" desired: \"%s\" elem: \"%s\" insert: \"%s\"\n",
			    i, op_names[ops[i].op], qfns[t].name,
			    ops[i].init, ops[i].final,
			    ops[i].elem, ops[i].insert);
			fflush(stdout);
#endif
			tc++;
			switch (ops[i].op) {
			case REMOVE_HEAD:
				qfns[t].f_remove_head(list);
				break;
			case REMOVE_TAIL:
				qfns[t].f_remove_tail(list);
				break;
			case REMOVE_ITEM:
				qfns[t].f_remove_item(list, ops[i].elem);
				break;
			case INSERT_HEAD:
				qfns[t].f_insert_head(list, ops[i].insert);
				break;
			case INSERT_TAIL:
				qfns[t].f_insert_tail(list, ops[i].insert);
				break;
			case INSERT_BEFORE:
				qfns[t].f_insert_before(
				    list, ops[i].insert, ops[i].elem);
				break;
			case INSERT_AFTER:
				qfns[t].f_insert_after(
				    list, ops[i].insert, ops[i].elem);
				break;
			}
			if (!strcmp("sh_tailq", op_names[ops[i].op])) {
				result = sh_t_verify_TAILQ_LAST(list,
				    ops[i].final);
			}
			if (result == 0)
				result = qfns[t].f_verify(list, ops[i].final);
			if (result == 0) {
				fc++;
				putchar('.');
			} else {
				putchar('*'); /* * means failed after op */
				printf("\ncase %d %s in %s init: \"%s\" desired: \"%s\" elem: \"%s\" insert: \"%s\" got: %s - %s\n",
				    i, op_names[ops[i].op], qfns[t].name,
				    ops[i].init, ops[i].final,
				    ops[i].elem, ops[i].insert,
				    qfns[t].f_as_string(list),
				    failure_reason_names[result]);
				fflush(stdout);
				eval = 1;
			}

			tc++;
			qfns[t].f_discard(list);
		}

		printf("\t%0.2f%% passed (%d/%d).\n",
		    (((double)fc/tc) * 100), fc, tc);
	}
	return (eval);
}
