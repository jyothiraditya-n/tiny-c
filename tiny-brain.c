/* Tiny Brain: A Single-File C-Language Implementation of Brian's Brain for
 * Linux TTYs Copyright (C) 2021-2022 Jyothiraditya Nellakra
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
#include <stdio.h>   //          m      "
#include <stdlib.h>  //        mm#mm  mmm    m mm   m   m          mmm
#include <string.h>  //          #      #    #"  #  "m m"         #"  "
#include <time.h>    //          #      #    #   #   #m#          #
                     //          "mm  mm#mm  #   #   "#           "#mm"
#include <fcntl.h>   //                              m"
#include <termios.h> //                             ""
#include <unistd.h>  //

#define TITLE_L "Tiny Brain - Use WASD to Move, Space to Pause, Return to Exit"
#define TITLE_R "RF to Alter Speed, UIO for Cell State, X to Reset, C to Clear"
#define PROGRAM "Tiny Brain"
#define CREDITS "Copyright (C) 2021-2022 Jyothiraditya Nellakra"

#define FCNTL_SET_ERR "Error setting input to non-blocking with fcntl()."
#define TCGETATTR_ERR "Error getting terminal properties with tcgetattr()."
#define TCSETATTR_ERR "Error setting terminal properties with tcsetattr()."
#define SCREEN_HW_ERR "Error getting screen size with ANSI escape codes."
#define MEM_ALLOC_ERR "Error allocating memory with malloc()."
#define NON_REACH_ERR "This error shouldn't trigger; main() shouldn't exit."

struct termios cooked, raw;
char *back_buf, *front_buf;
ssize_t height, width;

long delay = 41666667L;
bool paused = false;
ssize_t x, y;

void swap_bufs() { char *b = back_buf; back_buf = front_buf; front_buf = b; }
void refresh_screen() { puts("\e[1;1H"); puts(front_buf); }

void print_ch(char ch) { printf("\e[%zu;%zuH\e[7m%c\e[0m", y + 2, x + 1, ch); }
void print_spaces(size_t n) { for(size_t i = 0; i < n; i++) putchar(' '); }

void reset_terminal() { tcsetattr(STDIN_FILENO, TCSANOW, &cooked); }
void pauseprg(long ns) { nanosleep((const struct timespec[]){{0, ns}}, NULL); }
void exitprg(size_t ret) { reset_terminal(); printf("\e[?25h"); exit(ret); }

char buf_get(ssize_t x, ssize_t y) {
	x = x < 0 ? x + width : x >= width ? x - width : x;
	y = y < 0 ? y + height : y >= height ? y - height : y;

	return front_buf[y * width + x];
}

void back_buf_put(ssize_t x, ssize_t y, char ch) {
	back_buf[y * width + x] = ch;
}

void front_buf_put(ssize_t x, ssize_t y, char ch) {
	front_buf[y * width + x] = ch;
}

size_t count_around(ssize_t x, ssize_t y) {
	return (buf_get(x - 1, y - 1) == '#') + (buf_get(x, y - 1) == '#')
	     + (buf_get(x + 1, y - 1) == '#') + (buf_get(x + 1, y) == '#')
	     + (buf_get(x + 1, y + 1) == '#') + (buf_get(x, y + 1) == '#')
	     + (buf_get(x - 1, y + 1) == '#') + (buf_get(x - 1, y) == '#');
}

void next_generation() {
	for(ssize_t x = 0; x < width; x++)
		for(ssize_t y = 0; y < height; y++) {

	switch(buf_get(x, y)) {
		case '#': back_buf_put(x, y, '+'); break;
		case '+': back_buf_put(x, y, ' '); break;

	case ' ':
		if(count_around(x, y) == 2) back_buf_put(x, y, '#');
		else back_buf_put(x, y, ' ');
	}}

	swap_bufs();
	refresh_screen();
}

int main_loop() {
	switch(getchar()) {
		case 'w': if(y > 0) { y--; } break;
		case 'a': if(x > 0) { x--; } break;
		case 's': if(y < height - 1) { y++; } break;
		case 'd': if(x < width - 1) { x++; } break;

		case 'u': front_buf_put(x, y, '#'); break;
		case 'i': front_buf_put(x, y, '+'); break;
		case 'o': front_buf_put(x, y, ' '); break;

		case ' ': paused = paused ? false : true; goto wait;
		case 'r': delay -= delay / 10; break;
		case 'f': delay += delay / 10; break;
		case '\n': return 0;

	case 'c':
		for(size_t i = 0; i < (unsigned) (width * height); i++)
			front_buf[i] = ' ';

		goto redisp;

	case 'x':
		for(size_t i = 0; i < (unsigned) (height * width); i++) {
			
		switch(rand() % 3) {
			case 0: front_buf[i] = ' '; break;
			case 1: front_buf[i] = '+'; break;
			case 2: front_buf[i] = '#';
		}}

	redisp:	if(paused) refresh_screen(); break;
	wait:	if(paused) fcntl(STDIN_FILENO, F_SETFL, ~O_NONBLOCK);
		else fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	}

	if(!paused) next_generation();
	print_ch(buf_get(x, y));
	fflush(stdout);
	return 1;
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

	ret = scanf("[%zd;%zdR", &height, &width); height -= 2;
	if(ret != 2) { puts(SCREEN_HW_ERR); exitprg(4); }

	front_buf = malloc(sizeof(char) * height * width + 1);
	if(!front_buf) { puts(MEM_ALLOC_ERR); exitprg(5); }

	back_buf = malloc(sizeof(char) * height * width + 1);
	if(!back_buf) { puts(MEM_ALLOC_ERR); exitprg(5); }

	srand((unsigned) time(NULL));

	for(size_t i = 0; i < (unsigned) (height * width); i++) {
		switch(rand() % 3) {
			case 0: front_buf[i] = ' '; break;
			case 1: front_buf[i] = '+'; break;
			case 2: front_buf[i] = '#';
		}
	}

	front_buf[height * width] = 0;

	if((unsigned) width < strlen(TITLE_L)) {
		printf("\e[2J\e[H\e[7m%s", PROGRAM);
		print_spaces(width - strlen(PROGRAM));
		puts("\e[0m\e[?25l");
	}
	
	else if((unsigned) width < strlen(TITLE_L) + strlen(TITLE_R) + 3) {
		printf("\e[2J\e[H\e[7m%s", TITLE_L);
		print_spaces(width - strlen(TITLE_L));
		puts("\e[0m\e[?25l");
	}

	else {
		printf("\e[2J\e[H\e[7m%s", TITLE_L);
		print_spaces(width - strlen(TITLE_L) - strlen(TITLE_R));
		printf("%s\e[0m\e[?25l", TITLE_R);
	}

	refresh_screen();
	while(main_loop()) pauseprg(delay);

	printf("\e[2J\e[H%s %s\n", PROGRAM, CREDITS);
	exitprg(0);
}