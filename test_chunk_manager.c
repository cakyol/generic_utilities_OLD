
#include "stdio.h"
#include "chunk_manager.h"

#define CHUNK_SIZE              256
#define MAX_CHUNKS              (10 * 1024 * 1024)
#ifdef USE_MALLOC
#define LOOP                    150
#else
#define LOOP                    500
#endif

unsigned char *chunks [MAX_CHUNKS];
chunk_manager_t cmgr;
timer_obj_t tp;

int main (int argc, char *argv[])
{
    int i, j;
    unsigned long long iter = 0;

#ifndef USE_MALLOC
    printf("initializing chunk object .. ");
    fflush(stdout);
    int rc = chunk_manager_init(&cmgr, 
                0, CHUNK_SIZE, MAX_CHUNKS+1, 0, NULL);
    if (rc != 0) {
        printf("chunk_manager_init failed for %d chunks\n",
            MAX_CHUNKS);
        return -1;
    }
    printf("done\n");
#endif

    iter = 0;
    printf("started chunk alloc/free test\n");
    timer_start(&tp);
    for (j = 0; j < LOOP; j++) {

        /* allocate chunks */
        //printf("allocating..\n");
        for (i = 0; i < MAX_CHUNKS; i++) {
#ifdef USE_MALLOC
            chunks[i] = malloc(CHUNK_SIZE);
#else
            chunks[i] = chunk_manager_alloc(&cmgr);
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
#ifdef USE_MALLOC
                free(chunks[i]);
#else
                chunk_manager_free(&cmgr, chunks[i]);
#endif
                //printf("chunk %d addr 0x%p freed\n", i, chunks[i]);
                iter++;
            }
        }
    }
    timer_end(&tp);
    timer_report(&tp, iter);
#ifndef USE_MALLOC
    assert(MAX_CHUNKS+1 == chunk_manager_trim(&cmgr));
    assert(0 == chunk_manager_trim(&cmgr));

    /* allocate more again just to se if it works after a trim */
    void *ch1 = chunk_manager_alloc(&cmgr);
    void *ch2 = chunk_manager_alloc(&cmgr);
    assert(ch1 && ch2);
    assert(EBUSY == chunk_manager_destroy(&cmgr));
    chunk_manager_free(&cmgr, ch1);
    chunk_manager_free(&cmgr, ch2);
    int total = cmgr.total_chunk_count;
    assert(total == chunk_manager_trim(&cmgr));
    assert(0 == chunk_manager_trim(&cmgr));
    assert(0 == chunk_manager_destroy(&cmgr));
#endif
    return 0;
} 

