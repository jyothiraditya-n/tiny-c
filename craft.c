/* Tiny Craft: A Single-File C-Language Implementation of Minecraft for Linux
 * TTYs Copyright (C) 2021-2022 Jyothiraditya Nellakra
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

#include <inttypes.h> //
#include <math.h>     //
#include <stdbool.h>  //         m      "
#include <stdio.h>    //       mm#mm  mmm    m mm   m   m          mmm
#include <stdlib.h>   //         #      #    #"  #  "m m"         #"  "
#include <string.h>   //         #      #    #   #   #m#          #
#include <time.h>     //         "mm  mm#mm  #   #   "#           "#mm"
		      //                             m"
#include <fcntl.h>    //                            ""
#include <termios.h>  //
#include <unistd.h>   //

/* ========================== Global Declarations ========================== */

typedef struct { double x, y, z; } vec_t;

const int K_FCNTL_SET_ERR = 1, K_TCGETATTR_ERR = 2, K_TCSETATTR_ERR = 3;
const int K_SCREEN_HW_ERR = 4, K_MEM_ALLOC_ERR = 5, K_WRITE_SYS_ERR = 6;

int main(int argc, char **argv);
void K_panic(int error);
void K_exit();

struct termios C_raw, C_cooked;
int C_flags;

size_t C_height, C_width, C_max_render = 16;

typedef struct __attribute__((packed)) {
	char fg_header[7], fg_value[3], fg_post;
	char bg_header[7], bg_value[3], bg_post;
	char value;

} C_cell_t;

C_cell_t *C_buffer;

double *C_rvalue, *C_gvalue, *C_bvalue, *C_zvalue;
char *C_cvalue;

void C_initialise();
void C_reset();

void C_draw_line(vec_t start, vec_t end, vec_t colour);

void C_render();

/* ============================== Kernel Code ============================== */

#define K_FCNTL_SET_MSG "Error setting input to non-blocking with fcntl()."
#define K_TCGETATTR_MSG "Error getting terminal properties with tcgetattr()."
#define K_TCSETATTR_MSG "Error setting terminal properties with tcsetattr()."
#define K_SCREEN_HW_MSG "Error getting screen size with ANSI escape codes."
#define K_MEM_ALLOC_MSG "Error allocating memory with malloc()."
#define K_WRITE_SYS_MSG "Error writing using the write() system call."

int main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	C_initialise();
	srand((unsigned) time(NULL));

	vec_t a, b, c, d, colour;

	a.x = -1; a.z = -1;
	b.x = -1; b.z = 1;
	c.x = 1; c.z = -1;
	d.x = 1; d.z = 1;

	while(getchar() != ' ') for(double j = 1.0; j > 0 ; j -= 0.1) {
		C_reset();

		for(double i = 1 + j; i < C_max_render + j; i++) {
			a.y = b.y = c.y = d.y = i;

			colour.x = rand() / (double) RAND_MAX;
			colour.y = rand() / (double) RAND_MAX;
			colour.z = rand() / (double) RAND_MAX;

			C_draw_line(a, b, colour);
			C_draw_line(b, d, colour);
			C_draw_line(d, c, colour);
			C_draw_line(c, a, colour);
		}

		C_render();
	}

	K_exit();
}

void K_panic(int error) {
	switch(error) {
		case K_FCNTL_SET_ERR: puts(K_FCNTL_SET_MSG); break;

		case K_TCGETATTR_ERR: puts(K_TCGETATTR_MSG); goto cl2;
		case K_TCSETATTR_ERR: puts(K_TCSETATTR_MSG); goto cl2;

		case K_SCREEN_HW_ERR: puts(K_SCREEN_HW_MSG); goto cl1;
		case K_MEM_ALLOC_ERR: puts(K_MEM_ALLOC_MSG); goto cl1;
		case K_WRITE_SYS_ERR: puts(K_WRITE_SYS_MSG); goto cl1;

	cl1:	fcntl(STDIN_FILENO, F_SETFL, C_flags | O_NONBLOCK);
	cl2:	tcsetattr(STDIN_FILENO, TCSANOW, &C_cooked);
	}

	exit(error);
}

void K_exit() {
	fcntl(STDIN_FILENO, F_SETFL, C_flags | O_NONBLOCK);
	tcsetattr(STDIN_FILENO, TCSANOW, &C_cooked);
	exit(0);
}

/* ============================ Console I/O Code =========================== */

#define C_LHEAD "Tiny Craft - Use WASD to Move, IJKL to Look; Return to Exit"
#define C_RHEAD "QE to Pick, UO to Place / Destroy Blocks; RF to Change FoV"

#define C_PROG_NAME "Tiny Craft"
#define C_COPYRIGHT "Copyright (C) 2021-2022 Jyothiraditya Nellakra"

void _put_spaces(size_t n) { for(size_t i = 0; i < n; i++) putchar(' '); }

void C_initialise() {
	C_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	int ret = fcntl(STDIN_FILENO, F_SETFL, C_flags | O_NONBLOCK);
	if(ret == -1) K_panic(K_FCNTL_SET_ERR);

	ret = tcgetattr(STDIN_FILENO, &C_cooked);
	if(ret == -1) K_panic(K_TCGETATTR_ERR);

	C_raw = C_cooked;
	C_raw.c_lflag &= ~(ICANON | ECHO);

	ret = tcsetattr(STDIN_FILENO, TCSANOW, &C_raw);
	if(ret == -1) K_panic(K_TCSETATTR_ERR);

	printf("\e[999;999H\e[6n");
	while(getchar() != '\e');

	ret = scanf("[%zu;%zuR", &C_height, &C_width); C_height -= 2;
	if(ret != 2) K_panic(K_SCREEN_HW_ERR);

	C_buffer = malloc(sizeof(C_cell_t) * C_height * C_width);
	C_cvalue = malloc(sizeof(char) * C_height * C_width);

	C_rvalue = malloc(sizeof(double) * C_height * C_width);
	C_gvalue = malloc(sizeof(double) * C_height * C_width);
	C_bvalue = malloc(sizeof(double) * C_height * C_width);
	C_zvalue = malloc(sizeof(double) * C_height * C_width);

	if(!C_cvalue || !C_buffer || !C_rvalue
	|| !C_gvalue || !C_bvalue || !C_zvalue)
		K_panic(K_MEM_ALLOC_ERR);

	for(size_t i = 0; i < C_height; i++)
		for(size_t j = 0; j < C_width; j++)
	{
		size_t offset = i * C_width + j;

		memcpy(C_buffer[offset].fg_header, "\e[38;5;", 7);
		memcpy(C_buffer[offset].fg_value, "015", 3);
		C_buffer[offset].fg_post = 'm';

		memcpy(C_buffer[offset].bg_header, "\e[48;5;", 7);
		memcpy(C_buffer[offset].bg_value, "000", 3);
		C_buffer[offset].bg_post = 'm';

		C_buffer[offset].value = ' ';
	}

	C_reset();

	if(C_width < strlen(C_LHEAD)) {
		printf("\e[2J\e[H\e[7m%s", C_PROG_NAME);
		_put_spaces(C_width - strlen(C_PROG_NAME));
		puts("\e[0m\e[?25l");
	}
	
	else if(C_width < strlen(C_LHEAD) + strlen(C_RHEAD) + 3) {
		printf("\e[2J\e[H\e[7m%s", C_LHEAD);
		_put_spaces(C_width - strlen(C_LHEAD));
		puts("\e[0m\e[?25l");
	}

	else {
		printf("\e[2J\e[H\e[7m%s", C_LHEAD);
		_put_spaces(C_width - strlen(C_LHEAD) - strlen(C_RHEAD));
		printf("%s\e[0m\e[?25l", C_RHEAD);
	}

	fflush(stdout);
}

void C_reset() {
	for(size_t i = 0; i < C_height; i++)
		for(size_t j = 0; j < C_width; j++)
	{
		size_t offset = i * C_width + j;

		C_rvalue[offset] = C_gvalue[offset] = C_bvalue[offset] = 0.0;
		C_zvalue[offset] = C_max_render;
		C_cvalue[offset] = ' ';
	}
}

void _set_char(size_t x, size_t y, double z, char ch, vec_t colour) {
	size_t offset = y * C_width + x;

	if(z < C_zvalue[offset]) {
		C_rvalue[offset] = colour.x / z;
		C_gvalue[offset] = colour.y / z;
		C_bvalue[offset] = colour.z / z;

		C_cvalue[offset] = ch;
		C_zvalue[offset] = z;
	}
}

void C_draw_line(vec_t start, vec_t end, vec_t colour) {
	size_t width = (C_width - 2) >> 1, height = (C_height - 2) >> 1;

	size_t x1 = start.x / start.y * width + width;
	size_t y1 = start.z / start.y * height + height;

	size_t x2 = end.x / end.y * width + width;
	size_t y2 = end.z / end.y * height + height;

	double z1 = start.y;
	double z2 = end.y;

	if(y1 >= C_height - 1 || y2 >= C_height - 1) return;
	if(x1 >= C_width - 1 || x2 > C_width - 1) return;
	if(x1 < 1 || x2 < 1 || y1 < 1 || y2 < 1) return;

	intmax_t dx, dy, d;
	double dz, z;
	size_t x, y;
	char ch;

	if(x1 > x2) {
		if(y1 > y2) {
			dx = x1 - x2; dy = y1 - y2; dz = z1 - z2;
			dz /= sqrt(dx * dx + dy * dy);

			if(dx > dy) {
				d = (2 * dy) - dx; y = y2; z = z2; ch = '_';
				for(x = x2; x <= x1; ++x) {
					_set_char(x, y, z, ch, colour);

					if(d > 0) {
						d -= 2 * dx; ++y;
						ch = '\\';
					}

					ch = '_'; d += 2 * dy; z += dz;
				}
			}

			else if(dx < dy) {
				d = (2 * dx) - dy; x = x1; z = z1; ch = '|';
				for(y = y1; y >= y2; --y) {
					_set_char(x, y, z, ch, colour);

					if(d > 0) {
						d -= 2 * dy; --x; ch = '\\';
					}
					
					ch = '|'; d += 2 * dx; z -= dz;
				}
			}

			else {
				y = y2; z = z2; ch = '\\';
				for(x = x2; x <= x1; ++x) {
					_set_char(x, y, z, ch, colour);
					z += dz; ++y;
				}
			}
		}

		else if(y1 < y2) {
			dx = x1 - x2; dy = y2 - y1; dz = z2 - z1;
			dz /= sqrt(dx * dx + dy * dy);

			if(dx > dy) {
				d = (2 * dy) - dx; y = y1; z = z1; ch = '_';
				for(x = x1; x >= x2; --x) {
					_set_char(x, y, z, ch, colour);

					if(d > 0) {
						d -= 2 * dx; ++y; ch = '/';
					}
					
					ch = '_'; d += 2 * dy; z += dz;
				}
			}

			else if(dx < dy) {
				d = (2 * dx) - dy; x = x1; z = z1; ch = '|';
				for(y = y1; y <= y2; ++y) {
					_set_char(x, y, z, ch, colour);

					if(d > 0) {
						d -= 2 * dy; --x; ch = '/';
					}
					
					ch = '|'; d += 2 * dx; z += dz;
				}
			}

			else {
				y = y2; z = z2; ch = '/';
				for(x = x2; x <= x1; ++x) {
					_set_char(x, y, z, ch, colour);
					z -= dz; --y;
				}
			}
		}

		else {
			dz = (z1 - z2) / (x1 - x2); y = y2; z = z2; ch = '_';
			for(x = x2; x <= x1; ++x) {
				_set_char(x, y, z, ch, colour);
				z += dz;
			}
		}
	}

	else if(x1 < x2) {
		if(y1 > y2) {
			dx = x2 - x1; dy = y1 - y2; dz = z1 - z2;
			dz /= sqrt(dx * dx + dy * dy);

			if(dx > dy) {
				d = (2 * dy) - dx; y = y2; z = z2; ch = '_';
				for(x = x2; x >= x1; --x) {
					_set_char(x, y, z, ch, colour);

					if(d > 0) {
						d -= 2 * dx; ++y; ch = '/';
					}
					
					ch = '_'; d += 2 * dy; z += dz;
				}
			}

			else if(dx < dy) {
				d = (2 * dx) - dy; x = x2; z = z2; ch = '|';
				for(y = y2; y <= y1; ++y) {
					_set_char(x, y, z, ch, colour);

					if(d > 0) {
						d -= 2 * dy; --x; ch = '/';
					}
					
					ch = '|'; d += 2 * dx; z += dz;
				}
			}

			else {
				y = y1; z = z1; ch = '/';
				for(x = x1; x <= x2; ++x) {
					_set_char(x, y, z, ch, colour);
					z -= dz; --y;
				}
			}
		}

		else if(y1 < y2) {
			dx = x2 - x1; dy = y2 - y1; dz = z2 - z1;
			dz /= sqrt(dx * dx + dy * dy);

			if(dx > dy) {
				d = (2 * dy) - dx; y = y1; z = z1; ch = '_';
				for(x = x1; x <= x2; ++x) {
					_set_char(x, y, z, ch, colour);

					if(d > 0) {
						d -= 2 * dx; ++y; ch = '\\';
					}
					
					ch = '_'; d += 2 * dy; z += dz;
				}
			}

			else if(dx < dy) {
				d = (2 * dx) - dy; x = x2; z = z2; ch = '|';
				for(y = y2; y >= y1; --y) {
					_set_char(x, y, z, ch, colour);

					if(d > 0) {
						d -= 2 * dy; --x; ch = '\\';
					}
					
					ch = '|'; d += 2 * dx; z -= dz;
				}
			}

			else {
				y = y1; z = z1; ch = '\\';
				for(x = x1; x <= x2; ++x) {
					_set_char(x, y, z, ch, colour);
					z += dz; ++y;
				}
			}
		}

		else {
			dz = (z2 - z1) / (x2 - x1); y = y1; z = z1; ch = '_';
			for(x = x1; x <= x2; ++x) {
				_set_char(x, y, z, ch, colour);
				z += dz;
			}
		}
	}

	else {
		if(y1 > y2) {
			dz = (z1 - z2) / (y1 - y2); x = x2; z = z2; ch = '|';
			for(y = y2; y <= y1; ++y) {
				_set_char(x, y, z, ch, colour);
				z += dz;
			}
		}

		else if(y1 < y2) {
			dz = (z2 - z1) / (y2 - y1); x = x1; z = z1; ch = '|';
			for(y = y1; y <= y2; ++y) {
				_set_char(x, y, z, ch, colour);
				z += dz;
			}
		}

		else {
			x = x1; y = y1; z = z1; ch = '+';
			_set_char(x, y, z, ch, colour);
		}
	}
}

void C_render() {
	for(size_t i = 0; i < C_height; i++)
		for(size_t j = 0; j < C_width; j++)
	{
		size_t offset = i * C_width + j;

		int rvalue = C_rvalue[offset] * 5.0 + 0.5;
		int gvalue = C_gvalue[offset] * 5.0 + 0.5;
		int bvalue = C_bvalue[offset] * 5.0 + 0.5;
		int colour = 36 * rvalue + 6 * gvalue + 1 * bvalue + 16;

		char buffer[16];
		sprintf(buffer, "%03d", colour);
		memcpy(C_buffer[offset].bg_value, buffer, 3);

		rvalue = 5 - rvalue; gvalue = 5 - gvalue; bvalue = 5 - bvalue;
		colour = 36 * rvalue + 6 * gvalue + 1 * bvalue + 16;

		sprintf(buffer, "%03d", colour);
		memcpy(C_buffer[offset].fg_value, buffer, 3);

		C_buffer[offset].value = C_cvalue[offset];
	}

	ssize_t ret = write(STDOUT_FILENO, "\e[2;1H", 6);
	if(ret == -1) K_panic(K_WRITE_SYS_ERR);

	ret = write(STDOUT_FILENO, (void *) C_buffer, C_height * C_width * 23);
	if(ret == -1) K_panic(K_WRITE_SYS_ERR);
}

/* ========================= Chunk Management Code ========================= */