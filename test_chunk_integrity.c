
#include "chunk_manager_object.h"

#define CHUNK_SIZE		128
#define MAX_CHUNKS		(2*MILLION)
#define LOOP                    10

byte *chunks [MAX_CHUNKS];
chunk_manager_t cmgr;
timer_obj_t tp;

static void
fill_chunk (void *chunk, int starting_value)
{
    int i, *iptr;

    iptr = (int*) chunk;
    for (i = 0; i < (CHUNK_SIZE/sizeof(int)); i++) {
        *iptr++ = starting_value;
        starting_value += i;
    }
}

static error_t
validate_chunk (void *chunk, int starting_value)
{
    int i, *iptr;

    iptr = (int*) chunk;
    for (i = 0; i < (CHUNK_SIZE/sizeof(int)); i++) {
        if (*iptr != starting_value) {
            fprintf(stderr, "error in chunk integrity\n");
            return error;
        }
        iptr++;
        starting_value += i;
    }
    return ok;
}

int main (int argc, char *argv[])
{
    int i, j;
    uint64 iter = 0;

    int rc = chunk_manager_init(&cmgr, false, 
                CHUNK_SIZE, MAX_CHUNKS/64, 1024, NULL);
    if (rc != ok) {
	printf("chunk_manager_init failed for %d chunks\n",
	    MAX_CHUNKS);
	return error;
    }

    start_timer(&tp);
    for (j = 0; j < LOOP; j++) {

	/* allocate chunks */
        //printf("allocating..\n");
	for (i = 0; i < MAX_CHUNKS; i++) {
	    chunks[i] = chunk_manager_alloc(&cmgr);
	    if (NULL == chunks[i]) {
		printf("chunk alloc failed outer loop %d inner loop %d\n",
		    j, i);
	    } else {
		//printf("chunk %d set to 0x%p\n", i, chunks[i]);
                iter++;
                fill_chunk(chunks[i], i);
	    }
	}

	/* now delete them */
        //printf("deleting..\n");
	for (i = 0; i < MAX_CHUNKS; i++) {
	    if (NULL != chunks[i]) {
                validate_chunk(chunks[i], i);
		chunk_manager_free(&cmgr, chunks[i]);
		//printf("chunk %d addr 0x%p freed\n", i, chunks[i]);
                iter++;
	    }
	}
    }
    end_timer(&tp);
    report_timer(&tp, iter);
    printf("chunk manager grew %llu times and shrank %llu times\n",
            cmgr.grow_count, cmgr.trim_count);
    return 0;
} 

