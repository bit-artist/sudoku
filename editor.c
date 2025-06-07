/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2023 Rainer Holzner <rholzner@web.de> */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
//#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "term.h"
#include "tui.h"
#include "config.h"
#include "util.h"

#define NUM_ELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#define LEN(s) (sizeof(s)-1)
#define EMPTY(s) (!s || (s[0] == '\0'))
#define NOT_EMPTY(s) (s && (s[0] != '\0'))
#define UNUSED_PARAM(p) (void)(p)

#define TEMPFILE_SUFFIX "-edit.tmp"
#define TEMPFILE "/tmp/sudoku-XXXXXX"TEMPFILE_SUFFIX
#define REDIRECT_INPUT " < "
#define REDIRECT_STDERR_TO_STDOUT " 2>&1"
#define SOLVE_CMD_TEMPLATE \
	SOLVER REDIRECT_INPUT TEMPFILE REDIRECT_STDERR_TO_STDOUT

/* Origin coordinates of the Sudoku grid on screen */
#define FRAME_POS_X 0
#define FRAME_POS_Y 1

static const char *argv0;
static const char *msg_cdir = "Current dir:";
static const char *msg_cancel = "'Ctrl-C' or leave empty to cancel.";
static char message[1024];
static char filepath[PATH_MAX];
/* The current working directory */
static char wdir[PATH_MAX];
/* The Sudoku map */
static int cells[9][9];
static bool fixed[9][9];
/* Indicates which cell is currently selected */
static size_t cell_x, cell_y;
/* Status bar */
static char status_text[128];
static Color status_color;

/* Translate cell coordinates into text buffer coordinats */
static void translate(size_t cl_x, size_t cl_y, size_t *buf_x, size_t *buf_y)
{
	size_t x = cl_x, y=cl_y;
	if (x > 5)
		x+=3;
	else if (x > 2)
		x+=2;
	else
		x+=1;

	if (y > 5)
		y+=3;
	else if(y > 2)
		y+=2;
	else
		y+=1;

	*buf_x=x;
	*buf_y=y;
}

static void place_cursor(void)
{
	size_t x, y;

	translate(cell_x, cell_y, &x, &y);
	/* Add offsets */
	x+=FRAME_POS_X;
	y+=FRAME_POS_Y;
	set_cursor_pos(x+1, y+1);
}

static void update_grid(void)
{
	size_t x, y;
	Text_Cell tc;

	tui_text_cell_init(&tc);
	for (size_t y0 = 0; y0 < 9; y0++)
	{
		for (size_t x0 = 0; x0 < 9; x0++)
		{
			int val = cells[y0][x0];
			tc.c = (val > 0 && val <= 9) ? ('0' + val) : ' ';
			if (val > 0 && val <= 9)
			{
				if (fixed[y0][x0])
					tc.fg = YELLOW;
				else
					tc.fg = SYS_DEFAULT;
			}
			else
			{
				tc.fg = SYS_DEFAULT;
			}
			translate(x0, y0, &x, &y);
			tui_frame_set(x, y, &tc);
		}
	}
}

static void draw_all(void)
{
	tui_clear_screen();
	puts("Sudoku Editor v" VERSION);
	tui_frame_draw();
	if (status_text[0] != '\0')
	{
		putchar('\n');
		tui_print(status_text, status_color, SYS_DEFAULT, NORMAL);
	}
	puts("\n\nPress '?' for instructions.");
	place_cursor();
}

/* Prompt the user for input
 * Returns the user's input, else NULL.
 */
static char *prompt(const char *message)
{
	static char input[PATH_MAX];
	char *ret = NULL;

	tui_clear_screen();
	/* Make input canonical again. This allows for limited line editing. */
	terminal_restore();
	printf("%s", message);
	
	/* Reading stops after EOF or '\n'. Reads on less than size.
	 * A terminating null byte is stored after the last character.
	 */
	ret = fgets(input, sizeof(input), stdin);
	if (ret == input)
	{
		ret = strchr(input, '\n');
		if (ret != NULL)
		{
			*ret = '\0';
		}
		ret = input;
	}
	/* Switch back direct input */
	terminal_init();

	return ret;
}

static int read_puzzle(const char *path)
{
	FILE *fp;
	int c;
	size_t i, x, y;
	int tmp_cells[9][9];
	bool tmp_fixed[9][9];

	i = x = y = 0;
	fp = fopen(path, "r");
	if (fp)
	{
		while (i < 81 && (c = fgetc(fp)) != EOF)
		{
			y = i/9;
			x = i%9;
			if (c > '0' && c <= '9')
			{
				c = c-'0';
				tmp_fixed[y][x] = true;
			}
			else
			{
				c = 0;
				tmp_fixed[y][x] = false;
			}
			tmp_cells[y][x] = c;
			i++;
		}
		fclose(fp);
	}
	else
		perror("fopen");
	if (i == 81)
	{
		memcpy(cells, tmp_cells, sizeof(cells));
		memcpy(fixed, tmp_fixed, sizeof(fixed));
		return 0;
	}
	return -1;
}

static int fwrite_puzzle(FILE *fp)
{
	size_t i;
	int c;

	i = 0;
	if (fp)
	{
		for (i = 0; i < 81; i++)
		{
			c = cells[i/9][i%9];
			if (c > 0 && c <= 9)
				c = '0' + c;
			else
				c = '-';
			c = fputc(c, fp);
			if (c == EOF)
				break;
		}
	}
	if (i == 81)
		return 0;
	return -1;
}

static int write_puzzle(const char *path)
{
	int ret;
	FILE *fp;

	ret = -1;
	fp = fopen(path, "w");
	if (fp)
	{
		ret = fwrite_puzzle(fp);
		fclose(fp);
	}
	else
		perror("fopen");
	return ret;
}

/* Hint: This is not the solving algorithm rather the code
 * that invokes it.
 */
static int solve(struct timespec *tp, size_t *iterations)
{
	char template[] = TEMPFILE;
	char cmd[sizeof(SOLVE_CMD_TEMPLATE)];
	int ret, fd;
	FILE *fp;
	int c;
	size_t i, x, y;
	struct timespec tp0, tp1;

	i = 0;
	ret = -1;
	fd = mkstemps(template, LEN(TEMPFILE_SUFFIX));
	if (fd >= 0)
	{
		fp = fdopen(fd, "w");
		if (fp)
		{
			ret = fwrite_puzzle(fp);
			fclose(fp);
		}
		else
		{
			perror("fdopen");
			close(fd);
		}
	}
	if (ret == 0)
	{
		if (!make_string(cmd, sizeof(cmd),
				SOLVER REDIRECT_INPUT "%s" REDIRECT_STDERR_TO_STDOUT,
				template))
		{
			ret = -1;
		}
	}
	if (ret == 0)
	{
		clock_gettime(CLOCK_MONOTONIC, &tp0);
		fp = popen(cmd, "r");
		if (fp)
		{
			if (fscanf(fp, "i=%zu\n", iterations) == 1)
			{
				while (i < 81 && (c = fgetc(fp)) != EOF)
				{
					if (c > '0' && c <= '9')
					{
						c -= '0';
						y = i/9;
						x = i%9;
						/* If it's a fixed cell, then the returned value
						 * must equal the stored value.
						 */
						if (fixed[y][x])
						{
							if (c == cells[y][x])
							{
								i++;
							}
							else
							{
								break;
							}
						}
						else
						{
							cells[y][x] = c;
							i++;
						}
					}
				}
			}
			/* This is the return code of the solver or calling Shell */
			ret = pclose(fp);
			clock_gettime(CLOCK_MONOTONIC,&tp1);
			tp->tv_sec = tp1.tv_sec - tp0.tv_sec;
			if (tp0.tv_nsec > tp1.tv_nsec)
			{
				tp->tv_sec--;
				tp->tv_nsec = tp0.tv_nsec - tp1.tv_nsec;
			}
			else
			{
				tp->tv_nsec = tp1.tv_nsec - tp0.tv_nsec;
			}
		}
	}
	if (fd >= 0)
	{
		/* Delete the temp file */
		unlink(template);
	}
	if (i == 81 && ret == 0)
	{
		return 0;
	}
	return (ret == 0) ? -1 : -ret;
}

static void instructions(void)
{
	prompt("INSTRUCTIONS\n"
	"\n"
	" [hjkl], arrow keys : Select cell\n"
	" 1-9 : Place number under cursor\n"
	" DEL : Delete number under cursor\n"
	"   c : Clear all numbers\n"
	"   r : Read Sudoku from file\n"
	"   w : Write Sudoku to file\n"
	"   s : Solve\n"
	"   q : Quit\n"
	"\n"
	" [ Press enter ]");
}

static void handle_key_r(void)
{
	int status = -1;
	char *answer = NULL;

	filepath[0] = '\0';
	if (make_string(message, sizeof(message),
			"%s\n%s\n\n%s\n\nOpen file\n> ",
			msg_cdir, wdir, msg_cancel))
	{
		answer = prompt(message);
		trim(answer);
		if (NOT_EMPTY(answer))
		{
			if (get_realpath(answer, filepath, sizeof(filepath)) == filepath)
			{
				status = read_puzzle(filepath);
			}
			else
			{
				perror("realpath");
			}
		}
		else
		{
			status = -2;
		}
	}
	if (status < 0)
	{
		if (status == -2)
		{
			tui_print("\nCancelled!", YELLOW, SYS_DEFAULT, NORMAL);
		}
		else
		{
			if (!make_string(message, sizeof(message),
				"Failed to read file: `%s'",
				(filepath[0] != '\0') ? filepath :
			   	(answer != NULL) ? answer : ""))
			{
				//TODO: die()
			}
			tui_print(message, RED, SYS_DEFAULT, NORMAL);
		}
		printf("\n\n[Press key]");
		terminal_read_key();
	}
}

static void handle_key_w(void)
{
	int status = -1;
	char *answer = NULL;

	filepath[0] = '\0';
	if (make_string(message, sizeof(message),
			"%s\n%s\n\n%s\n\nWrite to\n> ",
			msg_cdir, wdir, msg_cancel))
	{
		answer = prompt(message);
		trim(answer);
		if (NOT_EMPTY(answer))
		{
			/* Normalize the filepath */
			errno = 0;
			if (get_realpath(answer, filepath, sizeof(filepath)))
			{
				/* File does exist */
				if (make_string(message, sizeof(message),
						"File `%s' already exists.\n\n"
						"Overwrite? (y/N) ",
						filepath))
				{
					answer = prompt(message);
					if (NOT_EMPTY(answer) &&
							(strcmp(answer, "y") == 0 ||
							strcmp(answer, "Yes") == 0))
					{
						status = write_puzzle(filepath);
					}
					else
					{
						status = -5;
					}
				}
			}
			else
			{
				if (errno == ENOENT)
				{
					/* File does not exist, but hopefully the path is
					 * resolved.
					 */
					status = write_puzzle(filepath);
				}
				else
				{
					filepath[0] = '\0';
				}
			}
		}
		else
		{
			status = -2;
		}
	}

	if (status == 0)
	{
		if (make_string(message, sizeof(message),
				"Saved to: `%s'", filepath) == NULL)
		{
			//TODO: die()
		}
		tui_print(message, GREEN, SYS_DEFAULT, NORMAL);
	}
	else
	{
		if (status == -2)
		{
			tui_print("\nCancelled!", YELLOW, SYS_DEFAULT, NORMAL);
		}
		else if (status == -5)
		{
			tui_print("\nNot overwriting!", YELLOW, SYS_DEFAULT, NORMAL);
		}
		else
		{
			if (!make_string(message, sizeof(message),
				"Failed to write file: `%s'",
				(filepath[0] != '\0') ? filepath : answer))
			{
				
				//TODO: die()
			}
			tui_print(message, RED, SYS_DEFAULT, NORMAL);
		}
	}
	printf("\n\n[Press key]");
	terminal_read_key();
}

static void handle_key_s(void)
{
	struct timespec tp;
	size_t i;

	strncpy(status_text, "Solving. Please wait..", LEN(status_text));
	status_color = GREEN;
	draw_all();
	if (solve(&tp, &i) != 0)
	{
		strncpy(status_text, "Cannot solve this Sudoku!", LEN(status_text));
		status_color = RED;
	}
	else
	{
		if (!make_string(status_text, sizeof(status_text),
				"Solved!\nIterations=%zu, Time=%fs", i,
				(double) tp.tv_sec + ((double) tp.tv_nsec)/1e9))
		{
			//TODO: die()
		}
		status_color = GREEN;
	}
}

static int handle_key_press(int c)
{
	if (c == 'c' || c == 'r' || c == KEY_DEL || (c <= '9' && c > 0))
	{
		/* Status may no longer fit to the current Sudoku */
		strncpy(status_text, "", LEN(status_text));
	}
	switch (c)
	{
	case 'h':
	case KEY_ARROW_LEFT:
		if (cell_x > 0)
			cell_x--;
		break;
	case 'j':
	case KEY_ARROW_DOWN:
		if (cell_y < 8)
			cell_y++;
		break;
	case 'k':
	case KEY_ARROW_UP:
		if (cell_y > 0)
			cell_y--;
		break;
	case 'l':
	case KEY_ARROW_RIGHT:
		if (cell_x < 8)
			cell_x++;
		break;
	case KEY_DEL:
		cells[cell_y][cell_x] = 0;
		fixed[cell_y][cell_x] = false;
		break;
	case 'q':
		return 1;
		break;
	case 'c':
		memset(cells, 0, sizeof(cells));
		memset(fixed, 0, sizeof(fixed));
		break;
	case 'r':
		handle_key_r();
		break;
	case 'w':
		handle_key_w();
		break;
	case 's':
		handle_key_s();
		break;
	case '?':
		instructions();
		break;
	default:
		if (c > '0' && c <= '9')
		{
			cells[cell_y][cell_x] = c-'0';
			fixed[cell_y][cell_x] = true;
		}
		break;
	}
	return 0;
}

static char *get_default_workdir(void)
{
	struct stat sb;
	int ret = -1;
	char *home;
 
	/* Already chdir into it */
	if (wdir[0] != '\0')
		return wdir;

	home = getenv("HOME");
	if (home)
	{
		if (make_filepath(home, WORKDIR, wdir, sizeof(wdir))
			   	== wdir)
		{
			ret = stat(wdir, &sb);
			if (ret == 0)
			{
				if (!S_ISDIR(sb.st_mode))
				{
					/* Fatal: path exists but is not a directory */
					fprintf(stderr, "Error: `%s' is not a directory!\n", wdir);
					ret = -1;
				}
			}
			else
			{
				ret = mkdir(wdir, S_IRWXU);
				if (ret == -1)
				{
					/* Fatal */
					perror("mkdir");
				}
			}
		}
	}
	if (ret != 0)
	{
		wdir[0] = '\0';
		return NULL;
	}
	return wdir;
}

static void handle_signal(int signum)
{
	UNUSED_PARAM(signum);
//	static const char mesg[]="received signal\n";
//	write(1, mesg, LEN(mesg));
}

int main(int argc, char *argv[])
{
	int c;
	char *pwd;
	struct sigaction sigact;

	if (argc >= 1)
		argv0 = argv[0];

	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = handle_signal;
	sigaction(SIGINT, &sigact, NULL);

	if (argc == 1)
	{
		pwd = get_default_workdir();
		if (pwd)
		{
			errno = 0;
			if (chdir(pwd) == -1)
			{
				perror("chdir");
				return 1;
			}
		}
		else
		{
			return 1;
		}
	}
	else if (argc == 2)
	{
		/* Set the default workdir */
		pwd = getenv("PWD");
		if (pwd)
		{
			strncpy(wdir, pwd, LEN(wdir));
		}
		errno = 0;
		if (get_realpath(argv[1], filepath, sizeof(filepath)) == filepath)
		{
			if (read_puzzle(filepath) != 0)
			{
				return 1;
			}
		}
		else if (errno == ENOENT)
		{
			/* TODO: Keep resolved filepath for write */
		}
		else
		{
			return 1;
		}
	}
	else
	{
		printf("usage: %s [file]\n", argv0);
		return 1;
	}
	terminal_init();
	tui_init();
	tui_frame_init(FRAME_POS_X, FRAME_POS_Y, FRAME_SIZE_X, FRAME_SIZE_Y);
	static const char grid[] =
		"+---+---+---+"
		"|   |   |   |"
		"|   |   |   |"
		"|   |   |   |"
		"+---+---+---+"
		"|   |   |   |"
		"|   |   |   |"
		"|   |   |   |"
		"+---+---+---+"
		"|   |   |   |"
		"|   |   |   |"
		"|   |   |   |"
		"+---+---+---+";
	tui_frame_fill(SYS_DEFAULT, SYS_DEFAULT, NORMAL, grid);
	update_grid();
	draw_all();
	
	while ((c = terminal_read_key()) != EOF)
	{
		if (handle_key_press(c) != 0)
			break;
		update_grid();
		draw_all();
	}

	tui_deinit();
	terminal_restore();
	return 0;
}
