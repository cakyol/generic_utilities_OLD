
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "timer_object.h"

#define MAX_ENHANCED_COUNTERS		1024

typedef struct enhanced_counter_s {

	char *function_name;
	int line_number;
	long long int value;

} enhanced_counter_t;

typedef struct enhanced_counter_block_s {

	/* how many enhanced counters are in use */
	int n;

	/* actual counters */
	enhanced_counter_t ec_array [MAX_ENHANCED_COUNTERS];

	/* ordered pointers to the array */
	enhanced_counter_t *ec_index [MAX_ENHANCED_COUNTERS];

} enhanced_counter_block_t;

static void
init_one_enhanced_counter (enhanced_counter_t *ecp)
{
	ecp->function_name = 0;
	ecp->line_number = ecp->value = 0;
}

void
init_enhanced_counter_block (enhanced_counter_block_t *ecbp)
{
	int i;

	for (i = 0; i < MAX_ENHANCED_COUNTERS; i++)  {
		init_one_enhanced_counter(&ecbp->ec_array[i]);
		ecbp->ec_index[i] = 0;
	}
	ecbp->n = 0;
}

/*
 * moves blocks of enhanced counter pointers around
 * to shift up/down when insertions happen into
 * the index array.  Handles overlapping copies.
 */
static void
copy_enhanced_counters (enhanced_counter_t **src, enhanced_counter_t **dst,
	int count)
{
	if (dst < src) {
		while (count-- > 0) *dst++ = *src++;
	} else {
		src += count;
		dst += count;
		while (count-- > 0) *(--dst) = *(--src);
	}
}

/*
 * compares the keys of two enhanced pointers as if they were simply
 * a string of bytes and returns 0, <0 and >0 for equal, lt & gt.
 */
static inline int
compare_enhanced_counters (enhanced_counter_t *e1, enhanced_counter_t *e2)
{
	int result = e1->line_number - e2->line_number;
	if (result) return result;
	return strcmp(e1->function_name, e2->function_name);
}

/*
 * Binary search an enhanced counter.  An enhanced counter's unique
 * key is the function_name and its line_number.  This returns the
 * array index of the entry if found.  If not found, returns -1
 * but also places where in the array it should be placed, if it
 * is being inserted.
 */
static int 
enhanced_counter_find_position (enhanced_counter_block_t *ecbp,
        enhanced_counter_t *searched,
        int *insertion_point)
{
	register int mid, diff, lo, hi;

	lo = mid = diff = 0;
	hi = ecbp->n - 1;

	/* binary search */
	while (lo <= hi) {
		mid = (hi+lo) >> 1;
		diff = compare_enhanced_counters(searched, ecbp->ec_index[mid]);
		if (diff > 0) {
			lo = mid + 1;
		} else if (diff < 0) {
			hi = mid - 1;
		} else {
			return mid;
		}
	}

	/*
	 * not found, but record where the element should be
	 * inserted, in case it was required to be put into
	 * the array.
	 */
	*insertion_point = diff > 0 ? (mid + 1) : mid;
	return -1;
}

/*
 * Will insert the enhanced counter 'new_ecp' in its proper rank
 * into the index for fast binary searching.  If an entry
 * already exists, returns that in 'found'.  Note that this function
 * will never fail with the given circumstances, ie the storage areas
 * are pre defined arrays so memory exhaustion does not happen.
 */
void
enhanced_counter_insert (enhanced_counter_block_t *ecbp,
        enhanced_counter_t *new_ecp, enhanced_counter_t **found)
{
	int insertion_point = 0;	/* shut the -Werror up */
	int size, i;
	enhanced_counter_t **source;

	/* find the enhanced_counter_t in the index */
	i = enhanced_counter_find_position(ecbp, new_ecp, &insertion_point);

	/* already in index and NOT considered an error */
	if (i >= 0) {
		*found = ecbp->ec_index[i];
		return;
	}

	/* if we are here, it is not in the index, so insert it now */
	*found = 0;

	/* shift all of the pointers after "insertion_point" down by one */
	source = &(ecbp->ec_index[insertion_point]);
	if ((size = ecbp->n - insertion_point) > 0)
		copy_enhanced_counters(source, (source+1), size);

	/* fill in the new node values */
	ecbp->ec_index[insertion_point] = new_ecp;

	/* increment element count */
	ecbp->n++;
}

/*
 * Given the function_name & line number, this function finds the corresponding
 * counter in the enhanced_counter_block_t.  If it cannot be found, this must be 
 * a new call, so it creates the entry.  It then increments the value.
 * If a new entry in the array is consumed, it advances the entry count (n).
 */
int
increment_enhanced_counter (enhanced_counter_block_t *ecbp,
	char *function_name, int line_number)
{
	enhanced_counter_t *newone, *found;

	/* MUST check this here, dont allow it to go thru past this point */
	if (ecbp->n >= MAX_ENHANCED_COUNTERS) {
		return ENOSPC;
	}

	/*
	 * assume entry is NOT there so get ready to insert the new one.
	 * If it happens to already exist, no problem.	If it is NOT
	 * there, we will have already prepared it.  The new one is
	 * the next empty entry in the array and is used as a scratchpad
	 * entry.
	 */
	newone = &ecbp->ec_array[ecbp->n];
	newone->function_name = function_name;
	newone->line_number = line_number;
	enhanced_counter_insert(ecbp, newone, &found);

	/* if the entry was already in the index object, just use that */
	if (found) {
		found->value++;
	} else {
		newone->value++;
	}

	return 0;
}

#define STANDALONE_TESTING

#ifdef STANDALONE_TESTING

enhanced_counter_block_t ecb;
long long int total_calls = 0;
timer_obj_t tmr;

#define TEST_EC(ecbp) \
	do { \
		increment_enhanced_counter(ecbp, (char*) __FUNCTION__, (int) __LINE__); \
		total_calls++; \
	} while (0)

void worst_case_really_really_long_function_name_1 (void)
{
	int i;
	for (i = 0; i < 123; i++) TEST_EC(&ecb);
}

void worst_case_really_really_long_function_name_2 (void)
{
	int i;
	for (i = 0; i < 312; i++) TEST_EC(&ecb);
}

void worst_case_really_really_long_function_name_3 (void)
{
	int i;
	for (i = 0; i < 28; i++) TEST_EC(&ecb);
}

void worst_case_really_really_long_function_name_4 (void)
{
	int i;
	for (i = 0; i < 1000; i++) TEST_EC(&ecb);
}

void worst_case_really_really_long_function_name_5 (void)
{
	int i;
	for (i = 0; i < 428; i++) TEST_EC(&ecb);
}

void short_name (void)
{
	TEST_EC(&ecb);
}

void dump_enhanced_counter_block (enhanced_counter_block_t *ecbp)
{
	int i;
	enhanced_counter_t *ecp;

	for (i = 0; i < ecbp->n; i++) {
		ecp = ecbp->ec_index[i];
		printf("func %s line %d entered %lld times\n",
			ecp->function_name, ecp->line_number, ecp->value);	

	}
}

#define ITERATIONS	50000

int main (int argc, char *argv[])
{
	int i;

	init_enhanced_counter_block(&ecb);
	timer_start(&tmr);
	for (i = 0; i < ITERATIONS; i++) {
		worst_case_really_really_long_function_name_5();
		worst_case_really_really_long_function_name_4();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_5();
		worst_case_really_really_long_function_name_4();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_5();
		worst_case_really_really_long_function_name_4();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_5();
		worst_case_really_really_long_function_name_4();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_5();
		worst_case_really_really_long_function_name_4();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_3();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_2();
		worst_case_really_really_long_function_name_1();
		worst_case_really_really_long_function_name_3();
		short_name();
	}
	timer_end(&tmr);
	timer_report(&tmr, total_calls, 0);
	dump_enhanced_counter_block(&ecb);
}

#endif /* STANDALONE_TESTING */






