
/*
 * A generic data structure created to test all the data structures.
 */
#define KEY_SIZE                6
#define FK                      'A'
#define LK                      'Z'

typedef struct test_data_s {

    unsigned char key [KEY_SIZE];
    void *data;

} test_data_t;

test_data_t *
generate_test_data (int how_many)
{
    int i, j, k, l, m, n;
    int count = 0;
    test_data_t *data_malloced, *tmp;

    data_malloced = (test_data_t*) malloc(how_many * sizeof(test_data_t));
    assert(0 != data_malloced);
    for (i = FK; i <= LK; i++) {
        for (j = FK; j <= LK; j++) {
            for (k = FK; k <= LK; k++) {
                for (l = FK; l <= LK; l++) {
                    for (m = FK; m <= LK; m++) {
                        for (n = FK; n <= LK; n++) {
                            tmp = &data_malloced[count];
                            tmp->key[0] = i;
                            tmp->key[1] = j;
                            tmp->key[2] = k;
                            tmp->key[3] = l;
                            tmp->key[4] = m;
                            tmp->key[5] = n;
                            tmp->data = tmp;
                            if (++count >= how_many) goto finished;
                        }
                    }
                }
            }
        }
    }
finished:
    return data_malloced;
}

int
verify_test_data (test_data_t *d1, test_data_t *d2)
{
    int i;

    assert(d1 && d2);
    if (d1 == d2) return 1;
    if (d1->data != d2->data) return 0;
    for (i = 0; i < KEY_SIZE; i++) {
        if (d1->key[i] != d2->key[i]) return 0;
    }
    return 1;
}


