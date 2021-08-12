/* Tiny Snake: A Single-File C-Language Implementation of Snake for Linux TTYs
 * Copyright (C) 2021 Jyothiraditya Nellakra
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

struct termios cooked, raw;
int height, width;

int randx() { return rand() % width; }
int randy() { return rand() % height; }

void putch(int x, int y, char ch) { printf("\e[%d;%dH%c", y + 2, x + 1, ch); }
void reset_terminal() { tcsetattr(STDIN_FILENO, TCSANOW, &cooked); }

void pauseprg(long ns) { nanosleep((const struct timespec[]){{0, ns}}, NULL); }
void exitprg(int ret) { reset_terminal(); printf("\e[?25h"); exit(ret); }

#define FCNTL_SET_ERR "Error setting input to non-blocking with fcntl()."
#define TCGETATTR_ERR "Error getting terminal properties with tcgetattr()."
#define TCSETATTR_ERR "Error setting terminal properties with tcsetattr()."
#define SCREEN_HW_ERR "Error getting screen size with ANSI escape codes."
#define MEM_ALLOC_ERR "Error allocating memory with malloc()."
#define NON_REACH_ERR "This error shouldn't trigger; main() shouldn't exit."

#define BANNER "Tiny Snake - Use WASD to Move"
#define DESC "Space to Pause, Return to Exit, R to Speed Up, F to Slow Down."
#define BY "Tiny Snake Copyright (C) 2021 Jyothiraditya Nellakra"

struct segment { struct segment *next; size_t x, y; } *head, *tail;
char direction = 'd';
char *map_memory;

long delay = 125000000L;
int grace_moves = 3;
bool paused;

char map_get(int x, int y) { return map_memory[y * width + x]; }
void map_put(int x, int y, char ch) { map_memory[y * width + x] = ch; }
void put_two(int x, int y, char ch) { map_put(x, y, ch); putch(x, y, ch); }

int score = 0;
int length = 2;
int bonus = 0;

void game_over() { printf("\e[2J\e[H%s\nScore: %d\n", BY, score); exitprg(0); }

void game_main() {
	int x = head -> x, y = head -> y;

	switch(getchar()) {
		case 'r': delay -= delay / 10; bonus++;  break;
		case 'f': delay += delay / 10; bonus--; break;
		case ' ': paused = paused ? false : true; break;
		case '\n': game_over(); break;

	case 'w':
		if(direction == 's' || y == 0) break;
		if(map_get(x, y - 1) == '#') break;
		direction = 'w'; break;

	case 'a':
		if(direction == 'd' || x == 0) break;
		if(map_get(x - 1, y) == '#') break;
		direction = 'a'; break;

	case 's':
		if(direction == 'w' || y == height - 1) break;
		if(map_get(x, y + 1) == '#') break;
		direction = 's'; break;

	case 'd':
		if(direction == 'a' || x == width - 1) break;
		if(map_get(x + 1, y) == '#') break;
		direction = 'd';
	}

	if(paused) return;

	switch(direction) {
		case 'w': if(y > 0) { y--; break; } else goto end;
		case 'a': if(x > 0) { x--; break; } else goto end;
		case 's': if(y < height - 1) { y++; break; } else goto end;
		case 'd': if(x < width - 1) { x++; break; } else goto end;
	}

	switch(map_get(x, y)) {
	case '#': goto end;

	case '@':
		head -> next = malloc(sizeof(struct segment));
		if(!head -> next) { puts(MEM_ALLOC_ERR); exitprg(5); }

		head = head -> next; head -> x = x; head -> y = y;
		put_two(x, y, '#');

		while(map_get(x, y)) { x = randx(); y = randy(); }
		put_two(x, y, '@');

		length++; break;

	default:
		head -> next = tail;
		put_two(tail -> x, tail -> y, ' ');

		tail -> x = x; tail -> y = y;
		put_two(x, y, '#');

		tail = tail -> next; head = head -> next;
	}

	score += length + bonus + grace_moves;
	fflush(stdout);

	grace_moves = 3;
	return;

end:	if(!grace_moves) game_over();
	else grace_moves--;
}

int main() {
	int ret = fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	if(ret == -1) { puts(FCNTL_SET_ERR); exit(1); }

	ret = tcgetattr(STDIN_FILENO, &cooked);
	if(ret == -1) { puts(TCGETATTR_ERR); exit(2); }

	raw = cooked;
	raw.c_lflag &= ~(ICANON | ECHO);

	ret = tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	if(ret == -1) { puts(TCSETATTR_ERR); exit(3); }

	printf("\e[999;999H\e[6n");
	while(getchar() != '\e');

	ret = scanf("[%d;%dR", &height, &width); height--;
	if(ret != 2) { puts(SCREEN_HW_ERR); exitprg(4); }

	head = malloc(sizeof(struct segment));
	if(!head) { puts(MEM_ALLOC_ERR); exitprg(5); }

	head -> x = 1; head -> y = 0;

	tail = malloc(sizeof(struct segment));
	if(!tail) { puts(MEM_ALLOC_ERR); exitprg(5); }

	tail -> next = head; tail -> x = 0; tail -> y = 0;

	map_memory = calloc(height * width, sizeof(char));
	if(!map_memory) { puts(MEM_ALLOC_ERR); exitprg(5); }

	printf("\e[2J\e[H\e[7m%s", BANNER);

	if((unsigned) width < strlen(BANNER) + strlen(DESC) + 3) {
		int whitespace = width - strlen(BANNER);
		for(int i = 0; i < whitespace; i++) putchar(' ');
		puts("\e[0m\e[?25l");
	}

	else {
		int whitespace = width - strlen(BANNER) - strlen(DESC);
		for(int i = 0; i < whitespace; i++) putchar(' ');
		printf("%s\e[0m\e[?25l", DESC);
	}

	put_two(0, 0, '#'); put_two(1, 0, '#');
	srand((unsigned) time(NULL));

	int x = 0, y = 0;
	while(map_get(x, y)) { x = randx(); y = randy(); }
	map_put(x, y, '@'); putch(x, y, '@');

	fflush(stdout);
	while(true) { game_main(); pauseprg(delay); }
	puts(NON_REACH_ERR); exitprg(6);
}