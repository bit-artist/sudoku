/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2023 Rainer Holzner <rholzner@web.de> */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "util.h"

char *make_string(char *buf, size_t buf_size, const char *fmt, ...)
{
	int size;
	va_list ap;

	va_start(ap, fmt);
	size = vsnprintf(buf, buf_size, fmt, ap);
	va_end(ap);

	if (size < 0 || (size_t) size >= buf_size)
	{
		if (buf_size)
		{
			buf[buf_size-1] = '\0';
		}
		return NULL;
	}
	return buf;
}

void trim(char *str)
{
	char *start, *end, *str_end;

	if (!str || str[0] == '\0')
		return;

	start = str;
	end = str + strlen(str);
	str_end = end;

	while (start < end)
	{
		if (isspace(*start))
			start++;
		else
			break;
	}
	while (end > start)
	{
		if (isspace(*(end-1)))
			end--;	
		else
			break;
	}
	if (start != str || end != str_end)
	{
		memmove(str, start, (size_t) (end - start));
		str[end - start] = '\0';
	}
}

char *make_filepath(const char *dir, const char *filename,
		char *filepath, size_t size)
{
	char *ret = NULL;

	if (make_string(filepath, size, "%s/%s", dir, filename) == filepath)
	{
		ret = filepath;
	}
	return ret;
}

/* This will substitude '.', '..' but not '~' which is a Shell feature
 * If the named file does not exists (ENOENT), the resolved path may
 * still be usable. 
 */
char *get_realpath(const char *filepath, char *resolved_path,
		size_t resolved_path_size)
{
	char *ret = NULL;

	if (resolved_path_size < PATH_MAX)
	{
		errno = ENOMEM;
	}
	else
	{
		if (realpath(filepath, resolved_path) == resolved_path)
		{
			ret = resolved_path;
		}
	}
	return ret;
}

