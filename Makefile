# Tiny C - Small Projects Implemented as Single-File C-Language Programs
# Copyright (C) 2021 Jyothiraditya Nellakra
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

progs = $(patsubst %.c,%,$(wildcard *.c))
cur_progs = $(foreach prog,$(progs),$(wildcard $(prog)))

CC ?= gcc
CFLAGS ?= -std=gnu99 -Wall -Wextra -Werror -O3
DESTDIR ?= ~/.local/bin

CLEAN = $(foreach prog,$(cur_progs),rm $(prog);)
INSTALL = $(foreach prog,$(progs),cp $(prog) $(DESTDIR)/$(prog);)

$(progs) : % : %.c
	$(CC) $(CFLAGS) $< -o $@

.DEFAULT_GOAL = all
.PHONY : all clean install

all : $(progs)

clean :
	$(CLEAN)

install : $(progs)
	$(INSTALL)