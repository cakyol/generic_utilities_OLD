
#include "stdio.h"
#include "chunk_manager_object.h"

#define CHUNK_SIZE		128
#define MAX_CHUNKS		(1024 * 1024)
#ifdef MEASURE_CHUNKS
#define LOOP                    4000
#else
#define LOOP                    400
#endif

unsigned char *chunks [MAX_CHUNKS];
chunk_manager_t cmgr;
timer_obj_t tp;

int main (int argc, char *argv[])
{
    int i, j;
    unsigned long long iter = 0;

#ifdef MEASURE_CHUNKS
    int rc = chunk_manager_init(&cmgr, 
                0, CHUNK_SIZE, MAX_CHUNKS+1, 0, NULL);
    if (rc != 0) {
	printf("chunk_manager_init failed for %d chunks\n",
	    MAX_CHUNKS);
	return -1;
    }
#endif

    timer_start(&tp);
    for (j = 0; j < LOOP; j++) {

	/* allocate chunks */
        //printf("allocating..\n");
	for (i = 0; i < MAX_CHUNKS; i++) {
#ifdef MEASURE_CHUNKS
	    chunks[i] = chunk_manager_alloc(&cmgr);
#else
	    chunks[i] = malloc(CHUNK_SIZE);
#endif	
	    if (NULL == chunks[i]) {
		printf("chunk alloc failed outer loop %d inner loop %d\n",
		    j, i);
	    } else {
		//printf("chunk %d set to 0x%p\n", i, chunks[i]);
                iter++;
	    }
	}

	/* now delete them */
        //printf("deleting..\n");
	for (i = 0; i < MAX_CHUNKS; i++) {
	    if (NULL != chunks[i]) {
#ifdef MEASURE_CHUNKS
		chunk_manager_free(&cmgr, chunks[i]);
#else
		free(chunks[i]);
#endif
		//printf("chunk %d addr 0x%p freed\n", i, chunks[i]);
                iter++;
	    }
	}
    }
    timer_end(&tp);
    timer_report(&tp, iter);
    return 0;
} 

