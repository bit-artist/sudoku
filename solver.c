/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2023 Rainer Holzner <rholzner@web.de> */

#include <stdio.h>
#include "config.h"

typedef enum _cell_type
{
	CT_BLANK,
	CT_FIXED,
	CT_VALUE
} Cell_Type;

typedef struct _cell
{
	unsigned value;
	Cell_Type ct;
} Cell;

#define CL(a) (&cells[a])

static Cell cells[9*9];
static Cell *rows[9][9];
static Cell *cols[9][9];
static Cell *boxes[9][9];

static const char *argv0;

static unsigned cell_to_box(unsigned cell_no)
{
	unsigned box_y, box_x;

	box_y = (cell_no/9)/3;
	box_x = (cell_no%9)/3;
	return box_y*3+box_x;
}

static void init(void)
{
	unsigned i, j, box;
	Cell empty = { .value = 0, .ct = CT_BLANK };

	for (i = 0; i < 9; i++)
	{
		for (j = 0; j < 9; j++)
		{
			cells[i*9+j] = empty;
			rows[i][j] = &cells[i*9+j];
			cols[i][j] = &cells[j*9+i];
		}
	}

	for (box = 0; box < 9; box++)
	{
		j = 0;
		for (i = 0; i < 81; i++)
		{
			if (cell_to_box(i) == box)
			{
				boxes[box][j++] = CL(i);
			}
		}
	}
}

/* Reads at most 81 characters from stdin.
 * Digits between [1..9] represent fixed cell values.
 * CR and LF are ignored.
 * All other characters represent empty cells.
 * 
 * Returns 0 on success, else -1.
 */
static int read_puzzle(void)
{
	size_t i = 0;
	int c;
	Cell *cl = CL(0);

	while (i < 81 && (c = getchar()) != EOF)
	{
		if (c > '0' && c <= '9')
		{
			cl->value = (unsigned)(c - '0');
			cl->ct = CT_FIXED;
		}
		else if (c == '\n' || c == '\r')
		{
			continue;
		}
		/* else: the character represents an empty cell */
		i++;
		cl++;
	}

	return (i == 81) ? 0 : -1;
}

static void print_cells(void)
{
	unsigned x, y;

	for (y = 0; y < 9; y++)
	{
		for (x = 0; x < 9; x++)
		{
			Cell *cl = &cells[y*9+x];
			if (cl->ct == CT_BLANK)
			{
				printf(". ");
			}
			else
			{
				printf("%u ", cl->value);
			}
		}
		putchar('\n');
	}
}

static int check_unique(const Cell *a[])
{
	unsigned bits = 0;
	unsigned mask;
	unsigned i;
	const Cell *cl;

	for (i = 0; i < 9; i++)
	{
		cl = a[i];
		if (cl->ct == CT_VALUE || cl->ct == CT_FIXED)
		{
			mask = 1 << cl->value;
			if (bits & mask)
			{
				return 0;
			}
			bits |= mask;
		}
	}
	return 1;
}

static int check_cell(int cell_no)
{
	return check_unique((const Cell **)rows[cell_no/9]) &&
		check_unique((const Cell **)cols[cell_no%9]) &&
		check_unique((const Cell **)boxes[cell_to_box(cell_no)]);
}

/* Checks the entire puzzle for validity.
 * Returns 1 if puzzle is valid, else 0.
 */
static int check_all(void)
{
	unsigned cell_no;

	for (cell_no = 0; cell_no < 81; cell_no++)
	{
		if (check_cell(cell_no) == 0)
		{
			return 0;
		}
	}
	return 1;
}

/* Return: 81 Reach end, < 81 go back */
/* Can continue from back */
static int forward(int cell_no)
{
	Cell *cl;

	while (cell_no < 81)
	{
		cl = &cells[cell_no];
		if (cl->ct == CT_BLANK)
		{
			cl->value = 1;
			cl->ct = CT_VALUE;
		}
		else if (cl->ct == CT_FIXED)
		{
			cell_no++;
			continue;
		}
		else
		{
			if (cl->value >= 9)
			{
				return cell_no;
			}
			else
			{
				cl->value++;
			}
		}
		while (check_cell(cell_no) == 0)
		{
			if (cl->value < 9)
			{
				cl->value++;
			}
			else
			{
				/* go back */
				return cell_no;
			}
		}
		/* go forward */
		cell_no++;
	}
	return cell_no;
}

static int back(int cell_no)
{
	Cell *cl = &cells[cell_no];

	/* Set current cell empty */
	cl->value = 0;
	cl->ct = CT_BLANK;
	/* Find cell with (not-fixed) value */
	while (--cell_no >= 0)
	{
		cl = &cells[cell_no];
		if (cl->ct == CT_FIXED)
		{
			continue;
		}
		else if (cl->ct == CT_VALUE)
		{
			break;
		}
	}
	return cell_no;
}

int main(int argc, char *argv[])
{
	int cell_no = 0;
	int i = 0;
	int verbose = 0;

	argv0 = argv[0];
	if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'v')
	{
		verbose = 1;
	}

	init();
	if (read_puzzle() < 0)
	{
		fprintf(stderr, "%s: Error: Cannot read puzzle data!\n", argv0);
		return 1;
	}

	/* Pre-check */
	cell_no = check_all();
	if (cell_no != 1)
	{
		fprintf(stderr, "%s: Error: The puzzle is invalid!\n", argv0);
		return 2;
	}

	if (verbose)
		print_cells();
   
	cell_no = 0;
	do
	{
		i++;
		cell_no = forward(cell_no);
		if (verbose)
		{
			printf("forward, ret=%d\n", cell_no);
			print_cells();
		}
		if (cell_no >= 0 && cell_no < 81)
		{
			cell_no = back(cell_no);
			if (verbose)
			{
				printf("back, ret=%d\n", cell_no);
				print_cells();
			}
		}
	}
	while (cell_no >= 0 && cell_no < 81);

	if (cell_no != 81 || check_all() == 0)
	{
		fprintf(stderr, "%s: Error: No solution found!\n", argv0);
		return 3;
	}

	printf("i=%d\n", i);
	print_cells();
	return 0;
}
