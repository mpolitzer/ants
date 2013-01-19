#ifndef UTIL_H
#define UTIL_H

#include "ants.h"

int distance(int row1, int col1, int row2, int col2, struct game_info *Info);
void move(int index, char dir, struct game_state* Game, struct game_info* Info);

#define ANTS(_k)	G->my_ants[_k]
#define FOOD(_k)	G->food[_k]
#define ENEMY(_k)	G->enemy_ants[_k]
#define HILL(_k)	G->hill[_k]
#define DEAD(_k)	G->dead_ants[_k]
#define MAP(_k)		I->map[_k]

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*x))

#define INF	0x0F0F0F0F
#define NINF	0xC0C0C0C0

const static dirmap[] = {'N', 'W', 'E', 'S'};
const static dirvec[][2] = {
		{ -1, 0 },
	{ 0,-1 },	{ 0, 1 },
		{  1, 0 },
};

static inline unsigned int mkindex(int row, int col, struct game_info* Info)
{
	int idx;

	row = (row + Info->rows) % Info->rows;
	col = (col + Info->cols) % Info->cols;

	idx = row * Info->cols + col;
	return idx;
}

static inline int isdirwalkable(int i, char d,
		struct game_state* Game, struct game_info* Info)
{
/*
	%	= Walls (the official spec calls this water, in either case it's
		  simply space that is occupied
	.	= Land        (territory that you can walk on)
	a	= Your Ant
	[b..z]	= Enemy Ants
	[A..Z]	= Dead Ants   (disappear after one turn)
	*	= Food
	?	= Unknown     (not used, assumed to be land)
*/
	struct my_ant ant = Game->my_ants[i];
	int idx, row, col;
	char c;

	if (d == 'N') { row = ant.row-1;col = ant.col;  }
	if (d == 'S') { row = ant.row+1;col = ant.col;  }
	if (d == 'E') { row = ant.row;	col = ant.col+1;}
	if (d == 'W') { row = ant.row;	col = ant.col-1;}

	idx = mkindex(row, col, Info);
	c = Info->map[idx];

	if (c == '.' || c == '?' || isdigit(c)) return 1;
	return 0;
}

static inline int move_safe(int idx, char dir,
		struct game_state* Game, struct game_info* Info)
{
	if (isdirwalkable(idx, dir, Game, Info)){
		move(idx, dir, Game, Info);
		return 1;
	}
	return 0;
}

static inline int *mkcircle(int r)
{
	int i;
	int row, col, n = 1; /* buff[0] contains its size. */
	int *buff = malloc(2 * (r*r * M_PI) * sizeof(int) + 1);

	for (col = 0; col < r; col++) {
		int row = ceil(sqrt(r*r - col*col));
		for (i=0; i<row; i++) { buff[n++] = i; buff[n++] = col; }
	}
	for (col = 1; col < r; col++) {
		int row = ceil(sqrt(r*r - col*col));
		for (i=0; i<row; i++) { buff[n++] = i; buff[n++] = -col; }
	}
	for (col = 0; col < r; col++) {
		int row = ceil(sqrt(r*r - col*col));
		for (i=1; i<row; i++) { buff[n++] = -i; buff[n++] = col; }
	}
	for (col = 1; col < r; col++) {
		int row = ceil(sqrt(r*r - col*col));
		for (i=1; i<row; i++) { buff[n++] = -i; buff[n++] = -col; }
	}
	buff[0] = n;
	return buff;
}

#endif /* UTIL_H */
