
#include <stdio.h>
#include "bitlist.h"

#define LOW	-1000
#define HI	10000

int main (int argc, char *argv[])
{
    bitlist_t bl;
    int rv, i, bit;

    if (bitlist_init(&bl, LOW, HI, 0) != 0) {
	fprintf(stderr, "bitlist_init failed for lo %d hi %d\n",
	    LOW, HI);
	return -1;
    }

    /* access outside the limits, these should fail */
    for (i = LOW - 100; i < LOW; i++) {
	rv = bitlist_get(&bl, i, &bit);
	if (0 == rv) {
	    fprintf(stderr, "erroneously returned bit number %d\n", i);
	}
    }
    for (i = HI + 1; i < HI + 100; i++) {
	rv = bitlist_get(&bl, i, &bit);
	if (0 == rv) {
	    fprintf(stderr, "erroneously returned bit number %d\n", i);
	}
    }

    /* these should all return 0 */
    for (i = LOW; i <= HI; i++) {
	rv = bitlist_get(&bl, i, &bit);
	if ((rv != 0) || (bit != 0)) {
	    fprintf(stderr, "returned wrong bit %d for bit number %d\n",
		bit, i);
	}
    }

    /* check for first set bits */
    for (i = LOW; i <= HI; i++) {
	rv = bitlist_set(&bl, i);
	if (rv) {
	    fprintf(stderr, "setting bit %d failed\n", i);
	}
	rv = bitlist_first_set_bit(&bl);
	if (rv != i) {
	    fprintf(stderr, "first set bit should be %d but it is %d\n",
		i, rv);
	}
	rv = bitlist_clear(&bl, i);
	if (rv) {
	    fprintf(stderr, "clearing bit %d failed\n", i);
	}
    }

    for (i = LOW; i <= HI; i++) {
	rv = bitlist_set(&bl, i);
	if (rv) {
	    fprintf(stderr, "setting bit %d failed\n", i);
	}
    }

    /* now clear from the end and verify first clear bit */
    for (i = HI; i >= LOW; i--) {
	rv = bitlist_clear(&bl, i);
	if (rv) {
	    fprintf(stderr, "clearing bit %d failed\n", i);
	}
	rv = bitlist_first_clear_bit(&bl);
	if (rv != i) {
	    fprintf(stderr, "first clear bit should be %d but it is %d\n",
		i, rv);
	}
    }
    printf("if no error messages were printed, bitlist is sane\n");
    return 0;
}

