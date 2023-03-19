#include <stdio.h>
#include <stdlib.h>

#define MACHINE_IO 1
#define OPT_IO(PR) if (!MACHINE_IO) PR;

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
int int_queue_nth(struct int_queue *, size_t);
int int_queue_first(struct int_queue *);
void int_queue_iter(struct int_queue *, iter_fn);
void int_queue_print(struct int_queue *);
void int_print_elem(int, size_t);
int *int_rand_perm(size_t, int);
int int_rand_range(int, int);
void int_swap(int *, int *);

struct int_queue *
int_queue(size_t max_size)
{
	struct int_queue *q = malloc(sizeof(struct int_queue));
	if (q == NULL) return NULL;

	q->buf = malloc(max_size * sizeof(int));
	if (q->buf == NULL) 
	{
		free(q);
		return NULL;
	}

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

	size_t real_off = INT_QUEUE_OFFSET(*q, 0);
	q->offset = INT_QUEUE_OFFSET(*q, 1);
	q->size--;

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
	if (i) printf(" %d", n);
	else printf("%d", n);
}

int *
int_rand_perm(size_t size, int seed)
{
	int *arr = malloc(size * sizeof(int));
	if (arr == NULL) return NULL;

	srand(seed);
	
	for (size_t i = 0; i < size; i++)
		*(arr+i) = i;

	for (size_t i = 0; i < size - 1; i++)
	{
		int k = int_rand_range(i, size - 1);
		int_swap(arr+i, arr+k);
	}

	return arr;
}

int
int_rand_range(int a, int b)
{
	int diff = b - a + 1;
	if (diff <= 1) return b;
	return rand() % diff + a;
}

void
int_swap(int *e1, int *e2) 
{
	int t = *e1;
	*e1 = *e2;
	*e2 = t;
}

enum game_mode { STANDARD, SIMPLE };

static struct game
{
	struct int_queue *player_a;
	struct int_queue *player_b;
	int seed;
	size_t conflicts;
	size_t max_conflicts;
	enum game_mode mode;
} game_state;

#define MAX_CARDS 52

int game_init(void);
int game_rand_cards(void);
int game_enter_war(void);

#define CARDS_EQ   0
#define CARD_A_GT  1
#define CARD_B_GT -1
int game_cmp_cards(int, int);
void game_grab_cards(struct int_queue *, struct int_queue *, size_t);
void game_cycle_cards(struct int_queue *, struct int_queue *);

#define END_MAX_CONFLICTS 1
#define END_NO_RESOLUTION 2
#define END_PLAYER_A      3
#define END_PLAYER_B      4
int game_tick(void);

#define END_WAR_MAX_CONFLICTS 1
#define END_WAR_NO_CARDS      2
#define END_WAR_PLAYER_A      3
#define END_WAR_PLAYER_B      4
int game_war_tick(size_t);
int game_custom_output(int);


int
game_init(void)
{
	game_state.player_a = int_queue(MAX_CARDS);
	game_state.player_b = int_queue(MAX_CARDS);

	if (game_state.player_a == NULL || game_state.player_b == NULL) return 1;	

	game_state.conflicts = 0;

	if (game_rand_cards()) return 1;

	return 0;
}

int
game_rand_cards(void)
{
	int *pool = int_rand_perm(MAX_CARDS, game_state.seed);
	if (pool == NULL) return 1;

	size_t half_size = MAX_CARDS / 2;

	int *pool_a = pool;
	int *pool_b = pool+half_size;

	for (size_t i = 0; i < half_size; i++)
	{
		int_queue_push(game_state.player_a, *(pool_a+i));
		int_queue_push(game_state.player_b, *(pool_b+i));
	}

	return 0;
}

int
game_enter_war(void)
{
	size_t war_ndx = 0;
	int tick_result;

	while (!(tick_result = game_war_tick(war_ndx))) 
		war_ndx += 2;

	switch (tick_result)
	{	// TODO don't know how it should work (example useless because of it's ambigouity)
		case END_WAR_PLAYER_A:
			game_grab_cards(game_state.player_a, game_state.player_a, war_ndx + 2);
			game_grab_cards(game_state.player_b, game_state.player_a, war_ndx + 2);
			break;
		case END_WAR_PLAYER_B:
			game_grab_cards(game_state.player_b, game_state.player_b, war_ndx + 2);
			game_grab_cards(game_state.player_a, game_state.player_b, war_ndx + 2);
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
game_grab_cards(struct int_queue *from, struct int_queue *to, size_t amount)
{
	int card;
	for (size_t i = 0; i < amount; i++)
	{
		card = int_queue_pop(from);
		// printf("take %d\n", card);
		if (card == -1) return;
		int_queue_push(to, card);
	}
}

void
game_cycle_cards(struct int_queue *a, struct int_queue *b)
{
	int_queue_push(a, int_queue_pop(a));
	int_queue_push(b, int_queue_pop(b));
}

int
game_tick(void)
{
	// printf("\nplayer a");
	// int_queue_print(game_state.player_a);
	// printf("\nplayer b");
	// int_queue_print(game_state.player_b);
	// printf("\n");

	// printing before check
	// printf("tick %zu of %zu\n", game_state.conflicts + 1, game_state.max_conflicts);
	if (game_state.conflicts >= game_state.max_conflicts) return END_MAX_CONFLICTS;
	
	if (game_state.player_b->size <= 0) return END_PLAYER_A;
	if (game_state.player_a->size <= 0) return END_PLAYER_B;

	int card_a = int_queue_first(game_state.player_a);
	int card_b = int_queue_first(game_state.player_b);

	int cmp_card = game_cmp_cards(card_a, card_b);
	int war_result = 0;

	switch (cmp_card)
	{
		case CARDS_EQ:
			if (game_state.mode == STANDARD)
				war_result = game_enter_war();
			else
				game_cycle_cards(game_state.player_a, game_state.player_b);	
			break;
		case CARD_A_GT:
			game_grab_cards(game_state.player_a, game_state.player_a, 1);
			game_grab_cards(game_state.player_b, game_state.player_a, 1);
			game_state.conflicts++;
			break;
		case CARD_B_GT:
			game_grab_cards(game_state.player_b, game_state.player_b, 1);
			game_grab_cards(game_state.player_a, game_state.player_b, 1);
			game_state.conflicts++;
			break;
	}

	switch (war_result)
	{
		case END_WAR_MAX_CONFLICTS:
			return END_MAX_CONFLICTS;
		case END_WAR_NO_CARDS:
			return END_NO_RESOLUTION;
	}

	return 0;
}

int
game_war_tick(size_t ndx)
{
	if (ndx >= game_state.player_a->size || ndx >= game_state.player_b->size) return END_WAR_NO_CARDS;
	int card_a = int_queue_nth(game_state.player_a, ndx);
	int card_b = int_queue_nth(game_state.player_b, ndx);

	// printf("war: %d vs %d (%d, %d)\n", card_a & 0xfc, card_b & 0xfc, card_a, card_b);

	if (game_state.conflicts > game_state.max_conflicts) return END_WAR_MAX_CONFLICTS;

	game_state.conflicts += 2;

	switch (game_cmp_cards(card_a, card_b))
	{
		case CARDS_EQ:
			return 0;
		case CARD_A_GT:
			return END_WAR_PLAYER_A;
		case CARD_B_GT:
			return END_WAR_PLAYER_B;
	}

	return 0;
}

int
game_custom_output(int outcome)
{
	printf("%d ", outcome - 1);
	switch (outcome)
	{
		case END_MAX_CONFLICTS:
		case END_NO_RESOLUTION:
			printf("%zu %zu\n", game_state.player_a->size, game_state.player_b->size);
			break;
		case END_PLAYER_A:
			printf("%zu\n", game_state.conflicts);
			break;
		case END_PLAYER_B:
			int_queue_print(game_state.player_b);
			break;
	}

	return outcome - 1;
}

int
main(void)
{
	OPT_IO(puts("seed: "))
	scanf("%d", &game_state.seed);

	OPT_IO(puts("mode: "))
	scanf("%d", &game_state.mode);

	OPT_IO(puts("max conflicts: "))
	scanf("%zu", &game_state.max_conflicts);

	if (game_init()) return 1;

	int res = 0;
	while (!(res = game_tick()));

	OPT_IO(printf("end: %d\n", res))

	return game_custom_output(res);
};
