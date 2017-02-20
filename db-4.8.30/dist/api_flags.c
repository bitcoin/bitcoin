/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	char	*name;			/* API name */

	u_int	used_mask;		/* Bits used. */
} API;
API	**api_list, **api_end;

typedef struct {
	char	 *name;			/* Flag name */

	int	api_cnt;		/* APIs that use this flag. */
	API	**api, **api_end;

	u_int	value;			/* Bit value */
} FLAG;
FLAG	**flag_list, **flag_end;

int	verbose;
char	*progname;

int	add_entry(char *, char *);
void	define_print(char *, u_int);
void	dump_api(void);
void	dump_flags(void);
int	flag_cmp_alpha(const void *, const void *);
int	flag_cmp_api_cnt(const void *, const void *);
int	generate_flags(void);
int	parse(void);
void	print_api_mask(void);
void	print_api_remainder(void);
void	print_flag_value(void);
int	syserr(void);
int	usage(void);

int
main(int argc, char *argv[])
{
	enum { API_MASK, API_REMAINDER, FLAG_VALUE } output;
	int ch;

	if ((progname = strrchr(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		++progname;

	output = FLAG_VALUE;
	while ((ch = getopt(argc, argv, "mrv")) != EOF)
		switch (ch) {
		case 'm':
			output = API_MASK;
			break;
		case 'r':
			output = API_REMAINDER;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (parse() || generate_flags())
		return (EXIT_FAILURE);

	switch (output) {
	case API_MASK:
		print_api_mask();
		break;
	case API_REMAINDER:
		print_api_remainder();
		break;
	case FLAG_VALUE:
		print_flag_value();
		break;
	}

	if (verbose) {
		dump_api();
		dump_flags();
	}

	return (EXIT_SUCCESS);
}

int
parse()
{
	int lc;
	char *p, *api, buf[256];

	api = NULL;

	/*
	 * Read the method name/flag pairs.
	 */
	for (lc = 1; fgets(buf, sizeof(buf), stdin) != NULL; ++lc) {
		if ((p = strchr(buf, '\n')) != NULL)
			*p = '\0';
		else {
			fprintf(
			    stderr, "%s: %d: line too long\n", progname, lc);
			return (1);
		}

		/* Ignore any empty line or hash mark. */
		if (buf[0] == '\0' || buf[0] == '#')
			continue;

		/*
		 * A line without leading whitespace is an API name, a line
		 * with leading whitespace is a flag name.
		 */
		if (isspace(buf[0])) {
			if ((p = strtok(buf, " \t")) == NULL || *p == '#')
				continue;

			/* A flag without an API makes no sense. */
			if (api == NULL)
				goto format;

			/* Enter the pair into the array. */
			if (add_entry(api, p))
				return (1);
		} else {
			if ((p = strtok(buf, " \t")) == NULL)
				continue;
			if (api != NULL)
				free(api);
			if ((api = strdup(p)) == NULL)
				return (syserr());
		}
		if ((p = strtok(NULL, " \t")) != NULL && *p != '#')
			goto format;
	}

	return (0);

format:	fprintf(stderr, "%s: format error: line %d\n", progname, lc);
	return (1);
}

int
add_entry(char *api_name, char *flag_name)
{
	FLAG **fpp, *fp;
	API **app, *ap, **p;
	u_int cnt;

	/* Search for this api's API structure. */
	for (app = api_list;
	    app != NULL && *app != NULL && app < api_end; ++app)
		if (strcmp(api_name, (*app)->name) == 0)
			break;

	/* Allocate new space in the API array if necessary. */
	if (app == NULL || app == api_end) {
		cnt = app == NULL ? 100 : (u_int)(api_end - api_list) + 100;
		if ((api_list = realloc(api_list, sizeof(API *) * cnt)) == NULL)
			return (syserr());
		api_end = api_list + cnt;
		app = api_list + (cnt - 100);
		memset(app, 0, (u_int)(api_end - app) * sizeof(API *));
	}

	/* Allocate a new API structure and fill in the name if necessary. */
	if (*app == NULL &&
	    ((*app = calloc(sizeof(API), 1)) == NULL ||
	    ((*app)->name = strdup(api_name)) == NULL))
		return (syserr());

	ap = *app;

	/*
	 * There's a special keyword, "__MASK=<value>" that sets the initial
	 * flags value for an API, and so prevents those flag bits from being
	 * chosen for that API's flags.
	 */
	if (strncmp(flag_name, "__MASK=", sizeof("__MASK=") - 1) == 0) {
		ap->used_mask |=
		    strtoul(flag_name + sizeof("__MASK=") - 1, NULL, 0);
		return (0);
	}

	/* Search for this flag's FLAG structure. */
	for (fpp = flag_list;
	    fpp != NULL && *fpp != NULL && fpp < flag_end; ++fpp)
		if (strcmp(flag_name, (*fpp)->name) == 0)
			break;

	/* Realloc space in the FLAG array if necessary. */
	if (fpp == NULL || fpp == flag_end) {
		cnt = fpp == NULL ? 100 : (u_int)(flag_end - flag_list) + 100;
		if ((flag_list =
		    realloc(flag_list, sizeof(FLAG *) * cnt)) == NULL)
			return (syserr());
		flag_end = flag_list + cnt;
		fpp = flag_list + (cnt - 100);
		memset(fpp, 0, (u_int)(flag_end - fpp) * sizeof(FLAG *));
	}

	/* Allocate a new FLAG structure and fill in the name if necessary. */
	if (*fpp == NULL &&
	    ((*fpp = calloc(sizeof(FLAG), 1)) == NULL ||
	    ((*fpp)->name = strdup(flag_name)) == NULL))
		return (syserr());

	fp = *fpp;
	++fp->api_cnt;

	/* Check to see if this API is already listed for this flag. */
	for (p = fp->api; p != NULL && *p != NULL && p < fp->api_end; ++p)
		if (strcmp(api_name, (*p)->name) == 0) {
			fprintf(stderr,
			    "duplicate entry: %s / %s\n", api_name, flag_name);
			return (1);
		}

	/* Realloc space in the FLAG's API array if necessary. */
	if (p == NULL || p == fp->api_end) {
		cnt = p == NULL ? 20 : (u_int)(fp->api_end - fp->api) + 20;
		if ((fp->api = realloc(fp->api, sizeof(API *) * cnt)) == NULL)
			return (syserr());
		fp->api_end = fp->api + cnt;
		p = fp->api + (cnt - 20);
		memset(p, 0, (u_int)(fp->api_end - fp->api) * sizeof(API *));
	}
	*p = ap;

	return (0);
}

void
dump_api()
{
	API **app;

	printf("=============================\nAPI:\n");
	for (app = api_list; *app != NULL; ++app)
		printf("%s (%#x)\n", (*app)->name, (*app)->used_mask);
}

void
dump_flags()
{
	FLAG **fpp;
	API **api;
	char *sep;

	printf("=============================\nFLAGS:\n");
	for (fpp = flag_list; *fpp != NULL; ++fpp) {
		printf("%s (%#x, %d): ",
		    (*fpp)->name, (*fpp)->value, (*fpp)->api_cnt);
		sep = "";
		for (api = (*fpp)->api; *api != NULL; ++api) {
			printf("%s%s", sep, (*api)->name);
			sep = ", ";
		}
		printf("\n");
	}
}

int
flag_cmp_api_cnt(const void *a, const void *b)
{
	FLAG *af, *bf;

	af = *(FLAG **)a;
	bf = *(FLAG **)b;

	if (af == NULL) {
		if (bf == NULL)
			return (0);
		return (1);
	}
	if (bf == NULL) {
		if (af == NULL)
			return (0);
		return (-1);
	}
	if (af->api_cnt > bf->api_cnt)
		return (-1);
	if (af->api_cnt < bf->api_cnt)
		return (1);
	return (strcmp(af->name, bf->name));
}

int
generate_flags()
{
	FLAG **fpp;
	API **api;
	u_int mask;

	/* Sort the FLAGS array by reference count, in reverse order. */
	qsort(flag_list,
	    (u_int)(flag_end - flag_list), sizeof(FLAG *), flag_cmp_api_cnt);

	/*
	 * Here's the plan: walk the list of flags, allocating bits.  For
	 * each flag, we walk the list of APIs that use it and find a bit
	 * none of them are using.  That bit becomes the flag's value.
	 */
	for (fpp = flag_list; *fpp != NULL; ++fpp) {
		mask = 0xffffffff;			/* Set to all 1's */
		for (api = (*fpp)->api; *api != NULL; ++api)
			mask &= ~(*api)->used_mask;	/* Clear API's bits */
		if (mask == 0) {
			fprintf(stderr, "%s: ran out of bits at flag %s\n",
			   progname, (*fpp)->name);
			return (1);
		}
		(*fpp)->value = mask = 1 << (ffs(mask) - 1);
		for (api = (*fpp)->api; *api != NULL; ++api)
			(*api)->used_mask |= mask;	/* Set bit for API */
	}

	return (0);
}

int
flag_cmp_alpha(const void *a, const void *b)
{
	FLAG *af, *bf;

	af = *(FLAG **)a;
	bf = *(FLAG **)b;

	if (af == NULL) {
		if (bf == NULL)
			return (0);
		return (1);
	}
	if (bf == NULL) {
		if (af == NULL)
			return (0);
		return (-1);
	}
	return (strcmp(af->name, bf->name));
}

void
print_api_mask()
{
	API **app;
	char *p, buf[256];

	/* Output a mask for the API. */
	for (app = api_list; *app != NULL; ++app) {
		(void)snprintf(
		    buf, sizeof(buf), "_%s_API_MASK", (*app)->name);
		for (p = buf; *p != '\0'; ++p)
			if (islower(*p))
				*p = toupper(*p);
			else if (!isalpha(*p))
				*p = '_';
		define_print(buf, (*app)->used_mask);
	}
}

void
print_api_remainder()
{
	API **app;
	int unused, i;

	/* Output the bits remaining for the API. */
	for (app = api_list; *app != NULL; ++app) {
		for (i = unused = 0; i < 32; ++i)
			if (!((*app)->used_mask & (1 << i)))
				++unused;
		printf("%s: %d bits unused\n", (*app)->name, unused);
	}
}

void
print_flag_value()
{
	FLAG **fpp;

	/* Sort the FLAGS array in alphabetical order. */
	qsort(flag_list,
	    (u_int)(flag_end - flag_list), sizeof(FLAG *), flag_cmp_alpha);

	/* Output each flag's value. */
	for (fpp = flag_list; *fpp != NULL; ++fpp)
		define_print((*fpp)->name, (*fpp)->value);
}

void
define_print(char *name, u_int value)
{
	char *sep;

	switch (strlen(name) / 8) {
	case 0:
		sep = "\t\t\t\t\t";
		break;
	case 1:
		sep = "\t\t\t\t";
		break;
	case 2:
		sep = "\t\t\t";
		break;
	case 3:
		sep = "\t\t";
		break;
	default:
		sep = "\t";
		break;
	}
	printf("#define\t%s%s%#010x\n", name, sep, value);
}

int
syserr(void)
{
	fprintf(stderr, "%s: %s\n", progname, strerror(errno));
	return (1);
}

int
usage()
{
	(void)fprintf(stderr, "usage: %s [-mrv]\n", progname);
	return (EXIT_FAILURE);
}
