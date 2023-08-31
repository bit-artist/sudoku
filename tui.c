/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2023 Rainer Holzner <rholzner@web.de> */

#include <stdio.h>
#include <string.h>
#include "tui.h"

static Text_Cell tui_frame_buffer[FRAME_SIZE_Y][FRAME_SIZE_X];
static size_t tui_frame_pos_x, tui_frame_pos_y;
static size_t tui_frame_size_x, tui_frame_size_y;

void cursor_off(void)
{
	printf("\x1b[?25l");
}

void cursor_on(void)
{
	printf("\x1b[?25h");
}

void get_cursor_pos(unsigned *x, unsigned *y)
{
	printf("\x1b[6n");
	if (scanf("\x1b[%u;%uR", y, x) != 2)
	{
		*x = 0;
		*y = 0;
	}
}

void set_cursor_pos(unsigned x, unsigned y)
{
	printf("\x1b[%u;%uH", y, x);
}

void move_cursor_home(void)
{
	printf("\x1b[H");
}

void move_cursor_up(unsigned lines)
{
	printf("\033[%uA", lines);
}

void clear_screen(void)
{
	printf("\x1b[2J");
}

void clear_screen_from_cursor_down(void)
{
	printf("\x1b[J");
}

void tui_print(const char *txt, Color fg, Color bg, Style sty)
{
	printf("\033[%d", sty);
	if (fg != SYS_DEFAULT)
		printf(";%d", 30 + fg);
	if (bg != SYS_DEFAULT)
		printf(";%d", 40 + bg);
	printf("m%s\033[m", txt);
}

void tui_text_cell_init(Text_Cell *tc)
{
	tc->fg = SYS_DEFAULT;
	tc->bg = SYS_DEFAULT;
	tc->sty = NORMAL;
	tc->c = ' ';
}

void tui_frame_init(size_t x, size_t y, size_t size_x, size_t size_y)
{
	tui_frame_pos_x = x;
	tui_frame_pos_y = y;
	tui_frame_size_x = size_x;
	tui_frame_size_y = size_y;
}

void tui_init(void)
{
	tui_clear_screen();
}

void tui_deinit(void)
{
	tui_clear_screen();
}

void tui_clear_screen(void)
{
	move_cursor_home();
	clear_screen();
}

void tui_frame_clear(void)
{
	Text_Cell reset_value;
   	
	tui_text_cell_init(&reset_value);
	for (size_t y = 0; y < tui_frame_size_y; y++)
	{
		for (size_t x = 0; x < tui_frame_size_x; x++)
		{
			tui_frame_buffer[y][x] = reset_value;
		}
	}
}

void tui_frame_draw(void)
{
	for (size_t y = 0; y < tui_frame_size_y; y++)
	{
		set_cursor_pos(tui_frame_pos_x+1, tui_frame_pos_y+y+1);
		for (size_t x = 0; x < tui_frame_size_x; x++)
		{
			const Text_Cell *tc = &tui_frame_buffer[y][x];
			printf("\033[%d", tc->sty);
			if (tc->fg != SYS_DEFAULT)
				printf(";%d", 30 + tc->fg);
			if (tc->bg != SYS_DEFAULT)
				printf(";%d", 40 + tc->bg);
			printf("m%c\033[m", tc->c);
	//		printf("\033[%d;%d;%dm%c\033[m",
	//				tc->sty,
	//				tc->fg == SYS_DEFAULT ? 0 : 30 + tc->fg,
	//				tc->bg == SYS_DEFAULT ? 0 : 40 + tc->bg,
	//				tc->c);
		}
	}
}

int tui_frame_set(size_t x, size_t y, const Text_Cell *tc)
{
	int ret = 0;

	if (x < tui_frame_size_x && y < tui_frame_size_y)
	{
		tui_frame_buffer[y][x] = *tc;
	}
	else
	{
		ret = -1;
	}
	return ret;
}

void tui_frame_fill(Color fg, Color bg, Style sty, const char *txt)
{
	size_t x, y, i = 0, len = 0;
	Text_Cell tc = { fg, bg, sty, ' '};

	if (txt)
		len = strlen(txt);
	
	for (y = 0; y < tui_frame_size_y; y++)
	{
		for (x = 0; x < tui_frame_size_x; x++)
		{
			if (len)
			{
				i = i % len;
				tc.c = txt[i++];
			}
			tui_frame_buffer[y][x] = tc;
		}
	}
}
