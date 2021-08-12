/* Tiny Life: A Single-File C-Language Implementation of Conway's Game of Life
 * for Linux TTYs Copyright (C) 2021 Jyothiraditya Nellakra
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
char *back_buf, *front_buf;
int height, width;

long delay = 125000000L;
bool paused = false;
int x, y;

void swap_bufs() { char *b = back_buf; back_buf = front_buf; front_buf = b; }
void refresh_screen() { puts("\e[1;1H"); puts(front_buf); }

void putch(char ch) { printf("\e[%d;%dH\e[7m%c\e[0m", y + 2, x + 1, ch); }
void putspaces(int spaces) { for(int i = 0; i < spaces; i++) putchar(' '); }

void reset_terminal() { tcsetattr(STDIN_FILENO, TCSANOW, &cooked); }
void pauseprg(long ns) { nanosleep((const struct timespec[]){{0, ns}}, NULL); }
void exitprg(int ret) { reset_terminal(); printf("\e[?25h"); exit(ret); }

#define FCNTL_SET_ERR "Error setting input to non-blocking with fcntl()."
#define TCGETATTR_ERR "Error getting terminal properties with tcgetattr()."
#define TCSETATTR_ERR "Error setting terminal properties with tcsetattr()."
#define SCREEN_HW_ERR "Error getting screen size with ANSI escape codes."
#define MEM_ALLOC_ERR "Error allocating memory with malloc()."
#define NON_REACH_ERR "This error shouldn't trigger; main() shouldn't exit."

#define BANNER "Tiny Life - Use WASD to Move, Space to Pause, Return to Exit"
#define DESC "RF to Alter Speed, IO for Cell State, X to Reset, C to Clear"
#define NAME "Tiny Life"
#define CREDITS "Copyright (C) 2021 Jyothiraditya Nellakra"

char buf_get(int x, int y) {
	if(x >= width) x -= width; else if(x < 0) x += width;
	if(y >= height) y -= height; else if(y < 0) y += height;
	return front_buf[y * width + x];
}

void back_buf_put(int x, int y, char ch) { back_buf[y * width + x] = ch; }
void front_buf_put(int x, int y, char ch) { front_buf[y * width + x] = ch; }
void game_over() { printf("\e[2J\e[H%s %s\n", NAME, CREDITS); exitprg(0); }

int count_neighbours(int x, int y) {
	return (buf_get(x - 1, y - 1) == '#') + (buf_get(x, y - 1) == '#')
	     + (buf_get(x + 1, y - 1) == '#') + (buf_get(x + 1, y) == '#')
	     + (buf_get(x + 1, y + 1) == '#') + (buf_get(x, y + 1) == '#')
	     + (buf_get(x - 1, y + 1) == '#') + (buf_get(x - 1, y) == '#');
}

void next_generation() {
	for(int x = 0; x < width; x++) for(int y = 0; y < height; y++) {
		int count = count_neighbours(x, y);
		
		if(buf_get(x, y) == '#' && (count < 2 || count > 3))
			back_buf_put(x, y, ' ');

		else if(buf_get(x, y) == ' ' && count == 3)
			back_buf_put(x, y, '#');

		else back_buf_put(x, y, buf_get(x, y));
	}

	swap_bufs();
}

void game_main() {
	switch(getchar()) {
		case 'w': if(y > 0) { y--; } break;
		case 'a': if(x > 0) { x--; } break;
		case 's': if(y < height - 1) { y++; } break;
		case 'd': if(x < width - 1) { x++; } break;

		case 'i': front_buf_put(x, y, '#'); break;
		case 'o': front_buf_put(x, y, ' '); break;

		case ' ': paused = paused ? false : true; break;
		case 'r': delay -= delay / 10; break;
		case 'f': delay += delay / 10; break;
		case '\n': game_over(); break;

	case 'c':
		for(int i = 0; i < width * height; i++) front_buf[i] = ' ';
		goto redisp;

	case 'x':
		for(int i = 0; i < width * height; i++)
			front_buf[i] = rand() % 2 ? ' ' : '#';

	redisp:	if(paused) refresh_screen();
	}

	if(!paused) { next_generation(); refresh_screen(); }
	putch(buf_get(x, y)); fflush(stdout);
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

	ret = scanf("[%d;%dR", &height, &width); height -= 2;
	if(ret != 2) { puts(SCREEN_HW_ERR); exitprg(4); }

	front_buf = malloc(sizeof(char) * height * width + 1);
	if(!front_buf) { puts(MEM_ALLOC_ERR); exitprg(5); }

	back_buf = malloc(sizeof(char) * height * width + 1);
	if(!back_buf) { puts(MEM_ALLOC_ERR); exitprg(5); }

	srand((unsigned) time(NULL));

	for(int i = 0; i < height * width; i++)
		front_buf[i] = rand() % 2 ? ' ' : '#';

	front_buf[height * width] = 0;

	if((unsigned) width < strlen(BANNER)) {
		printf("\e[2J\e[H\e[7m%s", NAME);
		putspaces(width - strlen(NAME));
		puts("\e[0m\e[?25l");
	}
	
	else if((unsigned) width < strlen(BANNER) + strlen(DESC) + 3) {
		printf("\e[2J\e[H\e[7m%s", BANNER);
		putspaces(width - strlen(BANNER));
		puts("\e[0m\e[?25l");
	}

	else {
		printf("\e[2J\e[H\e[7m%s", BANNER);
		putspaces(width - strlen(BANNER) - strlen(DESC));
		printf("%s\e[0m\e[?25l", DESC);
	}

	refresh_screen();
	while(true) { game_main(); pauseprg(delay); }
	puts(NON_REACH_ERR); exitprg(6);
}