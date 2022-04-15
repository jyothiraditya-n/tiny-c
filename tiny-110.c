/* Tiny 110: A Single-File C-Language Implementation of the Rule 110 Cellular
 * Automaton for Linux TTYs Copyright (C) 2021-2022 Jyothiraditya Nellakra
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

#include <stdbool.h> //
#include <stdio.h>   //   m      "                         mmm    mmm     mmmm
#include <stdlib.h>  // mm#mm  mmm    m mm   m   m           #      #    m"  "m
#include <string.h>  //   #      #    #"  #  "m m"           #      #    #  m #
#include <time.h>    //   #      #    #   #   #m#            #      #    #    #
                     //   "mm  mm#mm  #   #   "#           mm#mm  mm#mm   #mm#
#include <fcntl.h>   //                       m"  
#include <termios.h> //                      ""
#include <unistd.h>  //

#define TITLE_LEFT    "Tiny 110 - Press Return to Exit"
#define TITLE_RIGHT   "Space to Pause, R to Speed Up, F to Slow Down"
#define COPYRIGHT     "Tiny 110 Copyright (C) 2021-2022 Jyothiraditya Nellakra"

#define FCNTL_SET_ERR "Error setting input to non-blocking with fcntl()."
#define TCGETATTR_ERR "Error getting terminal properties with tcgetattr()."
#define TCSETATTR_ERR "Error setting terminal properties with tcsetattr()."
#define SCREEN_HW_ERR "Error getting screen size with ANSI escape codes."
#define MEM_ALLOC_ERR "Error allocating memory with malloc()."

struct termios raw, cooked;
char *front_buf, *back_buf;
size_t height, width;

long delay = 41666667L;
bool paused = false;

void buf_put(size_t i, char ch) { back_buf[i] = ch; }
char buf_get(size_t i) { return front_buf[i >= width ? i - width : i]; }
void swap_bufs() { char *b = back_buf; back_buf = front_buf; front_buf = b; }

void print_spaces(size_t n) { for(size_t i = 0; i < n; i++) putchar(' '); }
void refresh_screen() { puts(front_buf); fflush(stdout); }

void reset_terminal() { tcsetattr(STDIN_FILENO, TCSANOW, &cooked); }
void pauseprg(long ns) { nanosleep((const struct timespec[]){{0, ns}}, NULL); }
void exitprg(size_t ret) { reset_terminal(); printf("\e[?25h"); exit(ret); }

int main_loop() {
	switch(getchar()) {
		case ' ': paused = paused ? false : true; break;
		case 'r': delay -= delay / 10; break;
		case 'f': delay += delay / 10; break;

		case '\n':
			printf("\e[2J\e[H%s\n", COPYRIGHT);
			return 0;
	}

	if(paused) return 1;

	for(size_t i = 0; i < width; i++) {
		char a = (buf_get(i - 1) == '#') << 1;
		a = (a + (buf_get(i) == '#')) << 1;
		a = a + (buf_get(i + 1) == '#');

		switch(a) {
			case 0: buf_put(i, ' '); break;
			case 1: buf_put(i, '#'); break;
			case 2: buf_put(i, '#'); break;
			case 3: buf_put(i, '#'); break;
			case 4: buf_put(i, ' '); break;
			case 5: buf_put(i, '#'); break;
			case 6: buf_put(i, '#'); break;
			case 7: buf_put(i, ' ');
		}
	}

	swap_bufs();
	refresh_screen();
	return 2;
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

	ret = scanf("[%zu;%zuR", &height, &width);
	if(ret != 2) { puts(SCREEN_HW_ERR); exitprg(4); }

	front_buf = malloc(sizeof(char) * width + 1);
	if(!front_buf) { puts(MEM_ALLOC_ERR); exitprg(5); }

	back_buf = malloc(sizeof(char) * width + 1);
	if(!back_buf) { puts(MEM_ALLOC_ERR); exitprg(5); }

	srand((unsigned) time(NULL));

	for(size_t i = 0; i <width; i++)
		front_buf[i] = rand() % 2 ? ' ' : '#';

	front_buf[width] = 0;
	printf("\r\e[7m%s", TITLE_LEFT);

	if(width < strlen(TITLE_LEFT) + strlen(TITLE_RIGHT) + 3) {
		print_spaces(width - strlen(TITLE_LEFT));
		printf("\e[0m\n%s\e[?25l\n", front_buf);
	}

	else {
		print_spaces(width - strlen(TITLE_LEFT) - strlen(TITLE_RIGHT));
		printf("%s\e[0m\n%s\e[?25l\n", TITLE_RIGHT, front_buf);
	}

	while(main_loop()) pauseprg(delay);
	exitprg(0);
}