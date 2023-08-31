/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2023 Rainer Holzner <rholzner@web.de> */

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdbool.h>
#include "term.h"

static struct termios term_old_settings;
static bool term_undo_settings;

int terminal_init(void)
{
	int status;
	struct termios new_settings;

	status = tcgetattr(0, &term_old_settings);
	if (status == -1)
	{
		perror("tcgetattr");
		return status;
	}

	new_settings = term_old_settings;
	cfmakeraw(&new_settings);
	new_settings.c_lflag |= ISIG;
	new_settings.c_oflag |= OPOST;
	status = tcsetattr(0, TCSAFLUSH, &new_settings);
	if (status == -1)
	{
		perror("tcsetattr");
	}
	term_undo_settings = true;
	return status;
}

int terminal_read_key(void)
{
	enum { S_0, S_ESC, S_CSI, S_SKIP };
	static int state;
	int c;

	while ((c = getchar()) != EOF)
	{
		switch (state)
		{
		case S_0:
			if (c == 0x1b)
			{
				state = S_ESC;
			}
			break;
		case S_ESC:
			if (c == 0x5b)
			{
				state = S_CSI;
			}
			else if (c == 0x1b)
			{
				
			}
			else
			{
				state = S_0;
			}
			break;
		case S_CSI:
			if (c >= 0x30 && c <= 0x3f)
			{
				/* optional parameter byte */
				state = S_SKIP;
			}
			else if (c >= 0x20 && c <= 0x2f)
			{
				/* optional intermediate bytes */
				state = S_SKIP;
			}
			else if (c >= 0x40 && c <= 0x7e)
			{
				/* final byte */
				state = S_0;
				c = (0x1b5b<<8) + c;
			}
			else if (c == 0x1b)
			{
				state = S_ESC;
			}
			else
			{
				state = S_0;
			}
			break;
		case S_SKIP:
			if (c >= 0x30 && c <= 0x3f)
			{
				/* optional parameter byte */
			}
			else if (c >= 0x20 && c <= 0x2f)
			{
				/* optional intermediate bytes */
			}
			else if (c == 0x1b)
			{
				state = S_ESC;
			}
			else
			{
				state = S_0;
			}
			break;
		}
		if (state == S_0)
		{
			break;
		}
	}
	return c;
}
void terminal_restore(void)
{
	if (term_undo_settings)
	{
		if (tcsetattr(0, TCSAFLUSH, &term_old_settings) == -1)
		{
			perror("Failed to restore terminal settings!");
		}
		term_undo_settings = false;
	}
}

