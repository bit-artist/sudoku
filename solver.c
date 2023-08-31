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
static Cell *boxes[9][9] = {
	{ CL( 0), CL( 1), CL( 2), CL( 9), CL(10), CL(11), CL(18), CL(19), CL(20) },
	{ CL( 3), CL( 4), CL( 5), CL(12), CL(13), CL(14), CL(21), CL(22), CL(23) },
	{ CL( 6), CL( 7), CL( 8), CL(15), CL(16), CL(17), CL(24), CL(25), CL(26) },
	{ CL(27), CL(28), CL(29), CL(36), CL(37), CL(38), CL(45), CL(46), CL(47) },
	{ CL(30), CL(31), CL(32), CL(39), CL(40), CL(41), CL(48), CL(49), CL(50) },
	{ CL(33), CL(34), CL(35), CL(42), CL(43), CL(44), CL(51), CL(52), CL(53) },
	{ CL(54), CL(55), CL(56), CL(63), CL(64), CL(65), CL(72), CL(73), CL(74) },
	{ CL(57), CL(58), CL(59), CL(66), CL(67), CL(68), CL(75), CL(76), CL(77) },
	{ CL(60), CL(61), CL(62), CL(69), CL(70), CL(71), CL(78), CL(79), CL(80) }
};

static void init(void)
{
	size_t i, j;
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
}

/* Reads 81 digits [1..9] from stdin initializing the cells.
 * Ignores everything else. Returns 0 on success, else -1.
 */
static int read_puzzle(void)
{
	size_t i = 0;
	char c;
	Cell *cl = CL(0);

	while (i < 81 && (c = getchar()) != EOF)
	{
		if (c > '0' && c <= '9')
		{
			cl->value = (unsigned)(c - '0');
			cl->ct = CT_FIXED;
		}
		/* else: skip all other characters */
		i++;
		cl++;
	}

	return (i == 81) ? 0 : -1;
}

static void print_cells(void)
{
	size_t x, y;

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
	const Cell *cl;

	for (size_t i = 0; i < 9; i++)
	{
		cl = a[i];
		if (cl->ct == CT_VALUE || cl->ct == CT_FIXED)
		{
			if (bits & (1 << cl->value))
			{
				return 0;
			}
			bits |= (1 << cl->value);
		}
	}
	return 1;
}

static int cell_to_box(int cell_no)
{
	int box_y, box_x;

	box_y = (cell_no/9)/3;
	box_x = (cell_no%9)%3;
	return box_y*3+box_x;
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
	int cell_no;

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

	if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'v')
	{
		verbose = 1;
	}

	init();
	if (read_puzzle() < 0)
	{
		fprintf(stderr,
				"%s: Error: Cannot read puzzle data!\n",
				SOLVER);
		return 1;
	}

	/* Pre-check */
	cell_no = check_all();
	if (cell_no != 1)
	{
		fprintf(stderr,
			   	"%s: Error: The puzzle is invalid!\n",
				SOLVER);
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

	cell_no = check_all();
	if (cell_no != 1)
	{
		fprintf(stderr,
			   	"%s: Error: No solution found!\n",
				SOLVER);
		return 3;
	}

	printf("i=%d\n", i);
	print_cells();
	return 0;
}
