#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STB_DS_IMPLEMENTATION
#define STBDS_SIPHASH_2_4
#include "stb_ds.h"

#define USAGE "Usage: %s [-HCh] [file...]\n"

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

#define LINE_LEN 256

enum {
	WORDS,
	HISTOGRAM,
	COUNTOCCUR,
} mode = WORDS;

char* delims = " \t\n,.!@`~#$%^&()*-=+[]{};:\"/?><";

char* progname = NULL;

size_t tablelen = 0;
size_t itemsize = 0;

static inline char*
strtolower(char* s) {
	for(char* c = s; *c != '\0'; ++c)
		*c = tolower(*c);
	return s;
}

typedef struct { char* key; size_t value; } count_t;
typedef struct { char* key; size_t* value; } hist_t;

typedef count_t* count_table_t;
typedef hist_t* hist_table_t;

int
countcmp(const void* a, const void* b) {
	count_t* c_a = (count_t*)a;
	count_t* c_b = (count_t*)b;
	return c_a->value <= c_b->value;
}

int
histcmp(const void* a, const void* b) {
	hist_t* h_a = (hist_t*)a;
	hist_t* h_b = (hist_t*)b;
	return arrlen(h_a->value) <= arrlen(h_b->value);
}

void*
countrec(void* tbl, char* wd, void* arg) {
	count_table_t c_tbl = (count_table_t)tbl;
	size_t n = shget(c_tbl, wd);
	shput(c_tbl, wd, n + 1);

	tablelen = shlen(c_tbl);

	return (void*)c_tbl;
}

void*
histrec(void* tbl, char* wd, void* arg) {
	hist_table_t h_tbl = (hist_table_t)tbl;
	size_t* lines = shget(h_tbl, wd);
	size_t l = *(size_t*)arg;
	arrput(lines, l);
	shput(h_tbl, wd, lines);

	tablelen = shlen(h_tbl);

	return (void*)h_tbl;
}

void
wordrpt(void* tbl) {
	count_table_t w_tbl = (count_table_t)tbl;

	for(size_t i = 0; i < shlen(w_tbl); ++i) {
		char* word = w_tbl[i].key;
		printf("%s\n", word);
	}
}

void
countrpt(void* tbl) {
	count_table_t c_tbl = (count_table_t)tbl;

	for(size_t i = 0; i < shlen(c_tbl); ++i) {
		char* word = c_tbl[i].key;
		size_t count = c_tbl[i].value;
		printf("%zu:%s\n", count, word);
	}
}

void
histrpt(void* tbl) {
	hist_table_t h_tbl = (hist_table_t)tbl;

	for(size_t i = 0; i < shlen(h_tbl); ++i) {
		char* word = h_tbl[i].key;
		size_t* lines = h_tbl[i].value;

		printf("%zu:%s:", arrlen(lines), word);

		size_t last = 0;
		for(int j = 0; j < arrlen(lines); ++j) {
			if(lines[j] != last)
				printf("%zu ", lines[j]);

			last = lines[j];
		}

		printf("\n");
	}
}

void
countcleanup(void* tbl) {
	if(tbl == NULL)
		return;

	count_table_t c_tbl = (count_table_t)tbl;
	shfree(c_tbl);
}

void
histcleanup(void* tbl) {
	if(tbl == NULL)
		return;

	hist_table_t h_tbl = (hist_table_t)tbl;
	for(size_t i = 0; i < shlen(h_tbl); ++i) {
		arrfree(h_tbl[i].value); 
	}
	shfree(h_tbl);
}

void*
tabulate(FILE* fp, void* tbl, void*(*rec)(void*, char*, void*)) {
	char buf[LINE_LEN];
	char* wd = NULL;

	size_t line = 1;

	for(;fgets(buf, LENGTH(buf), fp) != NULL; ++line) {
		for(wd = strtok(buf, delims); wd != NULL; wd = strtok(NULL, delims)) {
			//wd = strtolower(wd);

			tbl = rec(tbl, wd, (void*)(&line));
		}
	}

	return tbl;
}

int main(int argc, char* argv[]) {
	progname = argv[0];
	void* tbl = NULL;
	void*(*rec)(void*, char*, void*) = NULL;
	int(*cmp)(const void*, const void*) = NULL;
	void(*rpt)(void*) = NULL;
	void(*cleanup)(void*) = NULL;

	count_table_t c_tbl = NULL;
	hist_table_t h_tbl = NULL;

	/* parse arguments */
	size_t optind;
	for(optind = 1; optind < argc && argv[optind][0] == '-'; ++optind) {
		switch(argv[optind][1]) {
			case 'H':
				sh_new_arena(h_tbl);
				tbl = (void*)h_tbl;

				itemsize = sizeof(hist_t);

				rec = histrec;
				cmp = histcmp;
				rpt = histrpt;

				cleanup = histcleanup;

				mode = HISTOGRAM;
				break;
			case 'C':
				sh_new_arena(c_tbl);
				tbl = (void*)c_tbl;

				itemsize = sizeof(count_t);

				rec = countrec;
				cmp = countcmp;
				rpt = countrpt;

				cleanup = countcleanup;

				mode = COUNTOCCUR;
				break;
			case 'h':
				fprintf(stderr, USAGE, progname);
				if(cleanup != NULL)
					cleanup(tbl);
				exit(1);
			default:
				fprintf(stderr, "%s: unknown argument %s\n", progname, argv[optind]);
				if(cleanup != NULL)
					cleanup(tbl);
				exit(1);
		}
	}

	if(mode == WORDS) {
		sh_new_arena(c_tbl);
		tbl = (void*)c_tbl;

		itemsize = sizeof(count_t);

		rec = countrec;
		cmp = countcmp;
		rpt = wordrpt;

		cleanup = countcleanup;
	}

	if(optind == argc) {
		fprintf(stderr, "%s: no file given\n", progname);
		cleanup(tbl);
		exit(1);
	}

	FILE* fp = fopen(argv[optind], "r");

	if(fp == NULL) {
		fprintf(stderr, "%s: %s: no such file\n", progname, argv[optind]);
		cleanup(tbl);
		exit(1);
	}

	tbl = tabulate(fp, tbl, rec);

	qsort(tbl, tablelen, itemsize, cmp);

	rpt(tbl);

	cleanup(tbl);

	fclose(fp);

	return 0;
}
