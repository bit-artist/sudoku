/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2023 Rainer Holzner <rholzner@web.de> */

#ifndef _UTIL_H_
#define _UTIL_H_

char *make_string(char *buf, size_t buf_size, const char *fmt, ...);
char *make_filepath(const char *dir, const char *filename,
		char *filepath, size_t size);
char *get_realpath(const char *filepath, char *resolved_path,
		size_t resolved_path_size);
void trim(char *str);

#endif
