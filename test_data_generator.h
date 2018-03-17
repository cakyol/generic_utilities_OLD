
#ifndef __TEST_DATA_GENERATOR_H__
#define __TEST_DATA_GENERATOR_H__

/*
 * A generic data structure created to test all the data structures.
 */
#define KEY_SIZE                7
#define FK                      'A'
#define LK                      'Z'

typedef struct test_data_s {

    unsigned char key [KEY_SIZE];
    void *data;

} test_data_t;

extern int
compare_test_data (void *vt1, void *vt2);

extern test_data_t *
generate_test_data (int *how_many);

#endif // __TEST_DATA_GENERATOR_H__



