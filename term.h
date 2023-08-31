/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2023 Rainer Holzner <rholzner@web.de> */

#ifndef _TERM_H_
#define _TERM_H_

#define KEY_ARROW_UP    0x1b5b41
#define KEY_ARROW_DOWN  0x1b5b42
#define KEY_ARROW_RIGHT 0x1b5b43
#define KEY_ARROW_LEFT  0x1b5b44
#define KEY_DEL  		0x7e

int terminal_init(void);
int terminal_read_key(void);
void terminal_restore(void);

#endif
