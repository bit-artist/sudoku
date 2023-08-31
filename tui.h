/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2023 Rainer Holzner <rholzner@web.de> */

#ifndef _TUI_H_
#define _TUI_H_

#define FRAME_SIZE_X 13
#define FRAME_SIZE_Y 13

typedef enum _color
{
	BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, SYS_DEFAULT
} Color;

typedef enum _style
{
	NORMAL, //RESET
	BOLD,
	FAINT,
	ITALICS,
	UNDERLINED,
	BLINK,
	DO_NOT_USE,
	INVERTED,
	INVISIBLE_TEXT
} Style;

typedef struct _text_cell
{
	Color fg, bg;
	Style sty;
	char c;
} Text_Cell;

/*
 * VT100 Cursor and screen control functions
 */
void cursor_off(void);
void cursor_on(void);

/* Hint: You want to disable local echo before calling this function. */
void get_cursor_pos(unsigned *x, unsigned *y);
/* Hint: Starts counting at 1. */
void set_cursor_pos(unsigned x, unsigned y);
void move_cursor_home(void);
/* Move cursor num printed lines up */
void move_cursor_up(unsigned lines);
void clear_screen(void);
void clear_screen_from_cursor_down(void);

/*
 * TUI control
 */
void tui_init(void);
void tui_deinit(void);
void tui_clear_screen(void);
void tui_print(const char *txt, Color fg, Color bg, Style sty);
void tui_text_cell_init(Text_Cell *tc);
void tui_frame_init(size_t x, size_t y, size_t size_x, size_t size_y);
void tui_frame_clear(void);
void tui_frame_draw(void);
int tui_frame_set(size_t x, size_t y, const Text_Cell *tc);
void tui_frame_fill(Color fg, Color bg, Style sty, const char *txt);

#endif
