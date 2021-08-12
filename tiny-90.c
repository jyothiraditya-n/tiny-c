/* Tiny 90: A Single-File C-Language Implementation of the Rule 90 Cellular
 * Automaton for Linux TTYs Copyright (C) 2021 Jyothiraditya Nellakra
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
void putspaces(int spaces) { for(int i = 0; i < spaces; i++) putchar(' '); }
void refresh_screen() { puts(front_buf); fflush(stdout); }

void reset_terminal() { tcsetattr(STDIN_FILENO, TCSANOW, &cooked); }
void pauseprg(long ns) { nanosleep((const struct timespec[]){{0, ns}}, NULL); }
void exitprg(int ret) { reset_terminal(); printf("\e[?25h"); exit(ret); }

#define FCNTL_SET_ERR "Error setting input to non-blocking with fcntl()."
#define TCGETATTR_ERR "Error getting terminal properties with tcgetattr()."
#define TCSETATTR_ERR "Error setting terminal properties with tcsetattr()."
#define SCREEN_HW_ERR "Error getting screen size with ANSI escape codes."
#define MEM_ALLOC_ERR "Error allocating memory with malloc()."
#define NON_REACH_ERR "This error shouldn't trigger; main() shouldn't exit."

#define BANNER "Tiny 90 - Press Return to Exit"
#define DESC "Space to Pause, R to Speed Up, F to Slow Down"
#define CREDITS "Tiny 90 Copyright (C) 2021 Jyothiraditya Nellakra"

char buf_get(int i) {
	if(i >= width) i -= width;
	else if(i < 0) i += width;
	return front_buf[i];
}

void buf_put(int i, char ch) { back_buf[i] = ch; }
void game_over() { printf("\e[2J\e[H%s\n", CREDITS); exitprg(0); }

void game_main() {
	switch(getchar()) {
		case ' ': paused = paused ? false : true; break;
		case 'r': delay -= delay / 10; break;
		case 'f': delay += delay / 10; break;
		case '\n': game_over(); break;
	}

	if(paused) return;

	for(int i = 0; i < width; i++) {
		char a = buf_get(i - 1) == '#';
		char b = buf_get(i + 1) == '#';

		buf_put(i, a ^ b ? '#' : ' ');
	}

	swap_bufs(); refresh_screen();
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

	ret = scanf("[%d;%dR", &height, &width);
	if(ret != 2) { puts(SCREEN_HW_ERR); exitprg(4); }

	front_buf = malloc(sizeof(char) * width + 1);
	if(!front_buf) { puts(MEM_ALLOC_ERR); exitprg(5); }

	back_buf = malloc(sizeof(char) * width + 1);
	if(!back_buf) { puts(MEM_ALLOC_ERR); exitprg(5); }

	srand((unsigned) time(NULL));

	for(int i = 0; i <width; i++)
		front_buf[i] = rand() % 2 ? ' ' : '#';

	front_buf[width] = 0;
	printf("\r\e[7m%s", BANNER);

	if((unsigned) width < strlen(BANNER) + strlen(DESC) + 3) {
		putspaces(width - strlen(BANNER));
		printf("\e[0m\n%s\e[?25l\n", front_buf);
	}

	else {
		putspaces(width - strlen(BANNER) - strlen(DESC));
		printf("%s\e[0m\n%s\e[?25l\n", DESC, front_buf);
	}

	while(true) { game_main(); pauseprg(delay); }
	puts(NON_REACH_ERR); exitprg(6);
}