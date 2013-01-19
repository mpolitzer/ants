#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "ants.h"
#include "util.h"

#define DBG(...) fprintf(debug, __VA_ARGS__)

#define AGENT(buff, agent, row, col) \
	_agents[buff][agent][mkindex(row, col, I)]

#define MIN(x, y) ((x) < (y) ? x : y)
#define MAX(x, y) ((x) > (y) ? x : y)

enum {
	FOOD = 0,
	ALLY,
	ENEMY,
	EXPLORE,
	MY_HILL,
	ENEMY_HILL,
	NUM_AGENTS
};

static FILE *debug;

static char *agent_string[] = {
	"food",
	"ally",
	"enemy",
	"explore",
	"enemy_hill",
	"NONE",
};

static int turn;

static int *_agents[2][NUM_AGENTS];

static int *circle;

static int *_last_seen;

static inline int agent_get(int buff, int agent, int row, int col,
		struct game_info *I)
{
	int buff_prev = (buff+1) % 2;

	return AGENT(buff_prev, agent, row+1, col)/4 +
		AGENT(buff_prev, agent, row-1, col)/4 +
		AGENT(buff_prev, agent, row, col+1)/4 +
		AGENT(buff_prev, agent, row, col-1)/4;
}

static inline void dump_last_seen(struct game_info *I)
{
	int row, col;
	int buff = turn % 2;
	int buff_next = (turn+1) % 2;

	DBG("last_seen\n");

	for (row=0; row < I->rows; row++) {
		for (col=0; col < I->cols; col++) {
			char *beg = "\033[0m";
			int v = _last_seen[mkindex(row, col, I)];

			if (MAP(mkindex(row, col, I)) == '*') beg = "\033[30;42m";
			else if (MAP(mkindex(row, col, I)) == 'a') beg = "\033[30;47m";
			else if (MAP(mkindex(row, col, I)) == 'A') beg = "\033[30;47m";
			else if (MAP(mkindex(row, col, I)) == '0') beg = "\033[30;47m";
			else if (MAP(mkindex(row, col, I)) == '%') beg = "\033[44m";
			else if (isalnum(MAP(mkindex(row, col, I)))) beg = "\033[41m";
			DBG("%s%5d\033[0m", beg, 10*v);
		}
		DBG("\n");
	}
	DBG("\n");
}

static inline void dump_agent(unsigned int agent,
		struct game_info *I)
{
	int row, col;
	int buff = turn % 2;
	int buff_next = (turn+1) % 2;

	DBG("agent: %s\n", agent_string[agent]);
	if (agent >= NUM_AGENTS) return;

	for (row=0; row < I->rows; row++) {
		for (col=0; col < I->cols; col++) {
			char *beg = "\033[0m";
			int v = agent_get(buff, agent, row, col, I);

			if (MAP(mkindex(row, col, I)) == '*') beg = "\033[30;42m";
			else if (MAP(mkindex(row, col, I)) == 'a') beg = "\033[30;47m";
			else if (MAP(mkindex(row, col, I)) == 'A') beg = "\033[30;47m";
			else if (MAP(mkindex(row, col, I)) == '0') beg = "\033[30;47m";
			else if (MAP(mkindex(row, col, I)) == '%') beg = "\033[44m";
			else if (isalnum(MAP(mkindex(row, col, I)))) beg = "\033[41m";
			DBG("%s%5.2f\033[0m", beg, (double)v / INT_MAX);
		}
		DBG("\n");
	}
	DBG("\n");
}

static inline void difuse_default(int buff, int agent, int row, int col,
		struct game_info *I)
{
	AGENT(buff, agent, row, col) = agent_get(buff, agent,
			row, col, I);
}

static inline void difuse_rules(int buff, int row, int col,
		struct game_state *G,
		struct game_info *I)
{
	int i;
	int buff_prev = (buff+1) % 2;
	int pos = MAP(mkindex(row, col, I));

	if (_last_seen[mkindex(row, col, I)] < turn)
		AGENT(buff_prev, EXPLORE, row, col)
			= INT_MAX
			- (0.001*INT_MAX)*_last_seen[mkindex(row, col, I)];

	if (pos == '%') {				/* wall */
		int agent;
		for (agent = 0; agent < NUM_AGENTS; agent++) {
			AGENT(buff_prev, agent, row, col) = 0;
		}
	} else if (pos == '*') {			/* food */
		AGENT(buff_prev, FOOD, row, col) = INT_MAX;
		for (i = 0; i < NUM_AGENTS; i++) if (i != FOOD)
			difuse_default(buff, i, row, col, I);
	} else if (pos == 'a') {		/* my ant */
		AGENT(buff_prev, ALLY, row, col) = INT_MAX;
		AGENT(buff_prev, ENEMY, row, col) = 0;
		AGENT(buff_prev, FOOD, row, col) = 0;
		AGENT(buff_prev, EXPLORE, row, col) = 0;
		difuse_default(buff, ENEMY_HILL, row, col, I);
	} else if (pos == 'A') {		/* my hill + my ant */
		AGENT(buff_prev, MY_HILL, row, col) = INT_MAX;
		AGENT(buff_prev, ALLY, row, col) = INT_MAX;
		AGENT(buff_prev, FOOD, row, col) = 0;
		AGENT(buff_prev, EXPLORE, row, col) = 0;
		difuse_default(buff, ENEMY_HILL, row, col, I);
	} else if (pos == '0') {		/* my hill */
		AGENT(buff_prev, MY_HILL, row, col) = INT_MAX;
		AGENT(buff_prev, EXPLORE, row, col) = 0;
	} else if (isdigit(pos) || isupper(pos)) {	/* enemy hill */
		AGENT(buff_prev, ENEMY_HILL, row, col) = INT_MAX;
	} else if (isalnum(pos)) {			/* enemy */
		AGENT(buff_prev, ENEMY, row, col) = INT_MAX;
		AGENT(buff_prev, ALLY, row, col) = INT_MIN;
	} else {
		for (i = 0; i < NUM_AGENTS; i++)
			difuse_default(buff, i, row, col, I);
	}
}

static inline void difuse_map(int buff,
		struct game_state *G, struct game_info *I)
{
	int row, col;

	buff = buff % 2;

	for (row = 0; row < I->rows; row++) {
		for (col = 0; col < I->cols; col++) {
			difuse_rules(buff, row, col, G, I);
		}
	}
}

static inline int choose_agent(int ant,
		struct game_state *G,
		struct game_info *I)
{
	int row = ANTS(ant).row, col = ANTS(ant).col;
	int buff = turn % 2;
	int buff_next = (turn+1) % 2;

	int agent;
	int max = 0, max_i = 0;

	int explore = agent_get(buff, EXPLORE, row, col, I);
	int food = agent_get(buff, FOOD, row, col, I);
	int enemy = agent_get(buff, ENEMY, row, col, I);
	int ally = agent_get(buff, ALLY, row, col, I);
	int enemy_hill = agent_get(buff, ENEMY_HILL, row, col, I);

	if (0.5*enemy_hill > enemy) return ENEMY_HILL;
#if 1
	if (enemy > 0.05*INT_MAX) {
		if (enemy > 0.08*ally)
			return ALLY;
		if (ally > 0.06*enemy)
			return ENEMY;
	}
#endif
	if (food > 0.3*explore) return FOOD;

	return EXPLORE;
}

static inline int choose_ally(int ant,
		struct game_state *G,
		struct game_info *I)
{
	int dir, max_dir = 0;
	int min=INT_MAX;
	int row = ANTS(ant).row, col = ANTS(ant).col;
	int buff_next = (turn+1) % 2;

	for (dir = 0; dir < ARRAY_SIZE(dirvec); dir++) {
		int val = AGENT(buff_next, ALLY, row+dirvec[dir][0],
				col+dirvec[dir][1]);
			if (val < min) {
				min = val;
				max_dir = dir;
			}
	}
	return max_dir;
}

static inline void follow_agent(int ant,
		struct game_state *G,
		struct game_info *I)
{
	int dir = 0;
	int buff_next = (turn+1) % 2;
	int min = INT_MAX, max = 0, max_dir = 0;
	int agent = choose_agent(ant, G, I);
	int row = ANTS(ant).row, col = ANTS(ant).col;
	DBG("chose: %s\n", agent_string[agent]);

	for (dir = 0; dir < ARRAY_SIZE(dirvec); dir++) {
		int val = AGENT(buff_next, agent, row+dirvec[dir][0],
				col+dirvec[dir][1]);
		if (val > max) {
			max = val;
			max_dir = dir;
		}
	}
	if (max_dir > 4) {
		max_dir = rand() % 4;
	}
	move_safe(ant, dirmap[max_dir], G, I);
}

static inline void dump_map(struct game_info *I)
{
	int i;
	for (i = 0; i < I->rows * I->cols; i++) {
		if (i % I->cols == 0) DBG("\n");
		else DBG("%c", I->map[i]);
	}
	DBG("\n");
}

void mark_seen(struct game_state *G, struct game_info *I)
{
	int i, ant;
	for (ant = 0; ant < G->my_count; ant++) {
		int row = ANTS(ant).row, col = ANTS(ant).col;
		for (i=1; i < circle[0]; i+=2) {
			_last_seen[mkindex(row+circle[i], col+circle[i+1], I)]
				= turn;
		}
	}
}

void do_init(struct game_state *G, struct game_info *I)
{
	int i;
	debug = fopen("./cbot/debug.txt", "w");

	for (i = 0; i < NUM_AGENTS; i++) {
		_agents[0][i] = malloc(sizeof(int) * I->cols * I->rows);
		_agents[1][i] = malloc(sizeof(int) * I->cols * I->rows);
		memset(_agents[0][i], 0, I->cols * I->rows);
		memset(_agents[1][i], 0, I->cols * I->rows);
	}
	_last_seen = malloc(sizeof(int) * I->cols * I->rows);
	memset(_last_seen, 0, I->cols * I->rows);

	circle = mkcircle(ceil(sqrt(I->viewradius_sq)));
}

void do_turn(struct game_state *G, struct game_info *I)
{
	int i;
	int ant;
	int agent;

	turn++;

	mark_seen(G, I);
	for (i = 0; i < 2*ceil(sqrt(I->viewradius_sq)) + 1; i++)
		difuse_map(i, G, I);
	DBG("turn: %d\n", turn);
	dump_agent(ALLY, I);
	dump_agent(ENEMY, I);

	for (ant = 0; ant < G->my_count; ant++) {
		follow_agent(ant, G, I);
	}
}
