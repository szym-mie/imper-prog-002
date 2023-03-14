#include <stdio.h>
#include <stdlib.h>

struct int_queue
{
	int *buf;
	size_t offset;
	size_t size;
	size_t max_size;
};

typedef void (*iter_fn)(int, size_t);

#define INT_QUEUE_OFFSET(QUEUE, NDX) ((QUEUE).offset + NDX) % (QUEUE).max_size

struct int_queue *int_queue(size_t);
int int_queue_push(struct int_queue *, int);
int int_queue_pop(struct int_queue *);
int int_queue_ndx(struct int_queue *, size_t);
int int_queue_first(struct int_queue *);
void int_queue_iter(struct int_queue *, iter_fn);
void int_queue_print(struct int_queue *);
void int_print_elem(int, size_t);

struct int_queue *
int_queue(size_t max_size)
{
	struct int_queue *q = malloc(sizeof(struct int_queue));
	if (q == NULL) return NULL;

	q->buf = malloc(max_size * sizeof(int));
	if (q->buf == NULL) return NULL;

	q->offset = 0;
	q->size = 0;
	q->max_size = max_size;

	return q;
}

int
int_queue_push(struct int_queue *q, int elem)
{
	if (q->size >= q->max_size) return 1;

	size_t real_off = INT_QUEUE_OFFSET(*q, q->size++);
	*(q->buf+real_off) = elem;

	return 0;
}

int
int_queue_pop(struct int_queue *q)
{
	if (q->size <= 0) return -1;

	size_t real_off = INT_QUEUE_OFFSET(*q, q->size--);
	q->offset = INT_QUEUE_OFFSET(*q, 1);

	return *(q->buf+real_off);
}

int
int_queue_nth(struct int_queue *q, size_t ndx)
{
	if (ndx >= q->size) return -1;

	size_t real_off = INT_QUEUE_OFFSET(*q, ndx);
	
	return *(q->buf+real_off);
}

int
int_queue_first(struct int_queue *q)
{
	return int_queue_nth(q, 0);
}

void
int_queue_iter(struct int_queue *q, iter_fn fn)
{
	for (size_t i = 0; i < q->size; i++)
		fn(int_queue_nth(q, i), i);
}

void
int_queue_print(struct int_queue *q)
{
	int_queue_iter(q, int_print_elem);
}

void
int_print_elem(int n, size_t i)
{
	char prefix = i ? ',' : ' ';
	printf("%c %d", prefix, n);
}

// TODO permutation, selection

enum game_mode { STANDARD, SIMPLE };

static struct game
{
	struct int_queue *player_a;
	struct int_queue *player_b;
	int seed;
	int conflicts;
	int max_conflicts;
	enum game_mode mode;
} game_state;

#define MAX_CARDS 52

int game_init(void);
int game_enter_war(void);
int game_cmp_cards(int, int);
void game_grab_cards(struct int_queue *, struct int_queue *, size_t);

#define END_MAX_CONFLICTS 1
#define END_NO_RESOLUTION 2
#define END_PLAYER_A      3
#define END_PLAYER_B      4
int game_tick(void);

#define END_WAR_NO_CARDS  1
#define END_WAR_PLAYER_A  2
#define END_WAR_PLAYER_B  3
int game_war_tick(size_t);

int
game_init(void)
{
	game_state.player_a = int_queue(MAX_CARDS);
	game_state.player_b = int_queue(MAX_CARDS);

	game_state.conflicts = 0;

	if (game_state.player_a == NULL || game_state.player_b == NULL) return 1;	

	return 0;
}

int
game_enter_war(void)
{
	size_t war_ndx = 1;
	int tick_result;

	while (!(tick_result = game_war_tick(war_ndx))) 
		war_ndx += 2;

	switch (tick_result)
	{
		case END_WAR_PLAYER_A:
			game_grab_cards(game_state.player_b, game_state.player_a, war_ndx);
			break;
		case END_WAR_PLAYER_B:
			game_grab_cards(game_state.player_a, game_state.player_b, war_ndx);
			break;
	}

	return tick_result;
}

int
game_cmp_cards(int card_a, int card_b)
{
	int age_a = card_a & 0xfc;
	int age_b = card_b & 0xfc;
	
	if (age_a == age_b) return 0;
	else return age_a > age_b ? 1 : -1;
}

void
game_grab_cards(struct int_queue *from, struct int_queue *to, size_t ticks)
{
	int card;
	for (size_t i = 0; i < ticks; i++)
	{
		card = int_queue_pop(from);
		if (card == -1) return;
		int_queue_push(to, card);
	}
}


int
game_tick(void)
{
	// TODO print
	if (game_state.conflicts > game_state.max_conflicts) return END_MAX_CONFLICTS;
	
	if (game_state.player_b->size == 0) return END_PLAYER_A;
	if (game_state.player_a->size == 0) return END_PLAYER_B;

	int card_a = int_queue_first(game_state.player_a);
	int card_b = int_queue_first(game_state.player_b);

	game_state.conflicts++;

	int cmp_card = game_cmp_cards(card_a, card_b);
	int war_result;

	switch (cmp_card)
	{
		case 0:
			war_result = game_enter_war();	
			break;
		case 1:
			game_grab_cards(game_state.player_b, game_state.player_a, 1);
			break;
		case -1:
			game_grab_cards(game_state.player_a, game_state.player_b, 1);
			break;
	}

	if (war_result) return 1;

	return 0;
}

int
game_war_tick(size_t ndx)
{
	if (ndx >= game_state.player_a->size || ndx >= game_state.player_b->size) return END_WAR_NO_CARDS;
	int card_a = int_queue_ndx(game_state.player_a, ndx);
	int card_b = int_queue_ndx(game_state.player_b, ndx);

	game_state.conflicts += 2;

	int cmp_card = game_cmp_cards(card_b, card_b);
	switch (cmp_card)
	{
		case 0:
			return 0;
		case 1:
			return END_WAR_PLAYER_A;
		case -1:
			return END_WAR_PLAYER_B;
	}

	return 0;
}

int
main(void)
{
	puts("seed: ");
	scanf("%d", &game_state.seed);

	puts("mode: ");
	scanf("%d", &game_state.mode);

	puts("max conflicts: ");
	scanf("%d", &game_state.max_conflicts);

	if (game_init()) return 1;

	int res;
	while (!res)
		res = game_tick();

	printf("%d ", res - 1);

	switch (res)
	{
		
	}

	return 0;
};
