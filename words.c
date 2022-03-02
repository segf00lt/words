#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define STB_DS_IMPLEMENTATION
#define STBDS_SIPHASH_2_4
#include "stb_ds.h"

#define USAGE "Usage: %s [-iHNh] [file...]\n"

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

enum {
	WORDS,
	HISTOGRAM,
	NOCCUR,
} mode = WORDS;

char* delims = " \t\n,.!@`~#$%^&()*-=+[]{};:\"/?><";

char* progname = NULL;

size_t tablelen = 0;
size_t itemsize = 0;
bool insentitive = false;

static inline char*
strtolower(char* s) {
	for(char* c = s; *c != '\0'; ++c)
		*c = tolower(*c);
	return s;
}

typedef struct { char* key; size_t value; } noccur_t;
typedef struct { char* key; size_t* value; } hist_t;

typedef noccur_t* noccur_table_t;
typedef hist_t* hist_table_t;

int
noccurcmp(const void* a, const void* b) {
	noccur_t* c_a = (noccur_t*)a;
	noccur_t* c_b = (noccur_t*)b;
	return c_a->value <= c_b->value;
}

int
histcmp(const void* a, const void* b) {
	hist_t* h_a = (hist_t*)a;
	hist_t* h_b = (hist_t*)b;
	return arrlen(h_a->value) <= arrlen(h_b->value);
}

void*
noccurrec(void* tbl, char* wd, void* arg) {
	noccur_table_t c_tbl = (noccur_table_t)tbl;
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
	noccur_table_t w_tbl = (noccur_table_t)tbl;

	for(size_t i = 0; i < shlen(w_tbl); ++i) {
		char* word = w_tbl[i].key;
		printf("%s\n", word);
	}
}

void
noccurrpt(void* tbl) {
	noccur_table_t c_tbl = (noccur_table_t)tbl;

	for(size_t i = 0; i < shlen(c_tbl); ++i) {
		char* word = c_tbl[i].key;
		size_t noccur = c_tbl[i].value;
		printf("%zu:%s\n", noccur, word);
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
noccurcleanup(void* tbl) {
	if(tbl == NULL)
		return;

	noccur_table_t c_tbl = (noccur_table_t)tbl;
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
	char buf[BUFSIZ];
	char* wd = NULL;

	size_t line = 1;

	for(;fgets(buf, LENGTH(buf), fp) != NULL; ++line) {
		for(wd = strtok(buf, delims); wd != NULL; wd = strtok(NULL, delims)) {
			if(insentitive)
				wd = strtolower(wd);

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

	FILE* fp = NULL;

	noccur_table_t c_tbl = NULL;
	hist_table_t h_tbl = NULL;

	/* parse arguments */
	size_t optind;
	for(optind = 1; optind < argc && argv[optind][0] == '-'; ++optind) {
		switch(argv[optind][1]) {
			case 'i':
				insentitive = true;
				break;
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
			case 'N':
				sh_new_arena(c_tbl);
				tbl = (void*)c_tbl;

				itemsize = sizeof(noccur_t);

				rec = noccurrec;
				cmp = noccurcmp;
				rpt = noccurrpt;

				cleanup = noccurcleanup;

				mode = NOCCUR;
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

		itemsize = sizeof(noccur_t);

		rec = noccurrec;
		cmp = noccurcmp;
		rpt = wordrpt;

		cleanup = noccurcleanup;
	}

	if(optind == argc)
		tbl = tabulate(stdin, tbl, rec);

	while(optind < argc) {
		fp = fopen(argv[optind++], "r");

		if(fp == NULL) {
			fprintf(stderr, "%s: %s: no such file or directory\n", progname, argv[optind-1]);
			cleanup(tbl);
			exit(1);
		}

		tbl = tabulate(fp, tbl, rec);
		fclose(fp);
	}

	qsort(tbl, tablelen, itemsize, cmp);

	rpt(tbl);

	cleanup(tbl);

	return 0;
}
