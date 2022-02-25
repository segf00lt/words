#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STB_DS_IMPLEMENTATION
#define STBDS_SIPHASH_2_4
#include "stb_ds.h"

#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

#define LINE_LEN 256

char* delims = " \t\n,.!@`~#$%^&()*_-=+[]{};:\"/?><";

char* progname = NULL;

char*
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

	return c_a <= c_b;
}

int
histcmp(const void* a, const void* b) {
	hist_t* h_a = (hist_t*)a;
	hist_t* h_b = (hist_t*)b;

	return arrlen(h_a->value) <= arrlen(h_b->value);
}

void
countrec(void* tbl, char* wd, void* arg) {
	arg = NULL; /* avoid unsused variable warning */
	count_table_t c_tbl = (count_table_t)tbl;
	size_t num = shget(c_tbl, wd);
	shput(c_tbl, wd, num + 1);
}

void
histrec(void* tbl, char* wd, void* arg) {
	hist_table_t h_tbl = (hist_table_t)tbl;
	size_t* lines = shget(table, wd);
	size_t l = *(size_t*)arg;
	arrput(lines, l);
	shput(h_tbl, wd, lines);
}

void
tabulate(FILE* fp, void* tbl, void(*rec)(void*, char*, void*)) {
	char buf[LINE_LEN];
	char* wd = NULL;

	size_t line = 1;

	/*
	WordTable table = NULL;
	sh_new_arena(table);

	if(table == NULL) {
		fprintf(stderr, "%s: failed to allocate table\n", progname);
		exit(1);
	}
	*/

	for(;fgets(buf, LENGTH(buf), fp) != NULL; ++line) {
		for(wd = strtok(buf, delims); wd != NULL; wd = strtok(NULL, delims)) {
			//wd = strtolower(wd);

			rec(tbl, wd, (void*)(&line));
			/*
			size_t* lines = shget(table, wd);
			arrput(lines, line);
			shput(table, wd, lines);
			*/
		}
	}

	//return table;
}

/*
void
report(WordTable table) {
	for(int i = 0; i < shlen(table); ++i) {
		char* word = table[i].key;
		size_t* lines = table[i].value;

		printf("%zu : %s [ ", arrlen(lines), word);

		size_t last = 0;
		for(int j = 0; j < arrlen(lines); ++j) {
			if(j == 0)
				printf("%zu ", lines[j]);
			if(lines[j] != last && last != 0)
				printf("%zu ", lines[j]);

			last = lines[j];
		}

		printf("]\n");
	}
}
*/

int main(int argc, char* argv[]) {
	progname = argv[0];

	if(argc == 1) {
		fprintf(stderr, "%s: no file given\n", progname);
		exit(1);
	}

	if(argc > 2) {
		fprintf(stderr, "%s: too many arguments\n", progname);
		exit(1);
	}

	FILE* fp = fopen(argv[1], "r");

	if(fp == NULL) {
		fprintf(stderr, "%s: %s: no such file\n", progname, argv[1]);
		exit(1);
	}


	WordTable table = tabulate(fp);

	qsort((void*)table, shlen(table), sizeof(WordTableEntrie), entrieCompare);

	report(table);

	for(int i = 0; i < shlen(table); ++i) {
		arrfree(table[i].value); 
	}
	shfree(table);

	fclose(fp);

	return 0;
}
