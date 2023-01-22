/* Tiny BF: A Single-File C-Language Interpreter for Brainfuck for Linux TTYs
 * Copyright (C) 2021-2023 Jyothiraditya Nellakra
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

char *name; char data[30000]; size_t dptr; char *code; size_t clen;

size_t execute(size_t cptr, int run) {
	size_t head = cptr; int exec;
top:	exec = data[dptr]? 1: 0;
	switch(code[cptr]) {
	case '[':
		cptr = execute(++cptr, run && exec); goto end;

	case ']':
		if(!run || !exec) return cptr;
		else {cptr = head - 1;} goto end;
	}

	if(!run) goto end;
	else switch(code[cptr]) {
		case '+': ++data[dptr]; goto end;
		case '-': --data[dptr]; goto end;
		case '.': putchar(data[dptr]); goto end;
		case ',': data[dptr] = getchar(); goto end;
		case '>': ++dptr; break;
		case '<': --dptr; break;
	}

	if(dptr > 30000) {printf("%s: error: ptr error.\n", name); exit(5);}
end:	if(cptr == clen) return clen;
	else {cptr++;} goto top;
}

int main(int argc, char **argv) {
	name = argv[0];
	if(argc != 2) {
		printf("%s: usage: %s [FILE].\n", argv[0], argv[0]); return 1;
	}

	FILE *fp = fopen(argv[1], "rb");
	if(!fp) {printf("%s: error: can't open file.\n", argv[0]); return 2;}

	int fd = fileno(fp); fseek(fp, 0L, SEEK_END); clen = ftell(fp);
	code = mmap(NULL, clen, PROT_READ, MAP_SHARED, fd, 0);
	if(code == MAP_FAILED) {
		printf("%s: error: can't map file.\n", argv[0]); return 3;
	}

	int loop = 0;
	for(size_t i = 0; i < clen; i++) {
		loop += code[i] == '['; loop -= code[i] == ']';
	}
	
	if(loop != 0) {printf("%s: error: unmatched [.\n", argv[0]); return 4;}
	else {execute(0, 1);} return 0;
}