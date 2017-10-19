
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol (gee_akyol@yahoo.com)
** Copyright: Cihangir Metin Akyol, November 2013
**
** This code is developed by and belongs to Cihangir Metin Akyol.  
** It is NOT owned by any company or consortium.  It is the sole
** property and work of one individual.
**
** It can be used by ANYONE or ANY company for ANY purpose as long 
** as NO ownership and/or patent claims are made to it by such persons 
** or companies.
**
** It ALWAYS is and WILL remain the property of Cihangir Metin Akyol.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "shared_memory_utils.h"

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** @@@@@ 
**
** Debugging support
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#define _BUFSIZE	32

/*
** By default, debugging is on, logging is off
*/
bool debugging_on = TRUE;
int current_debug_level = DEBUG_LEVEL_LOW;
static char *logging_filename = NULL;

/*
** generates time stamp
*/
static void sprintf_calendar_time (char *buf)
{
    time_t t;
    struct tm *tm;
    size_t s;

    buf[0] = 0;
    t = time(NULL);
    tm = localtime(&t);
    if (tm != NULL) {
	s = strftime(buf, _BUFSIZE, "%x %X", tm);
    }
}

void turn_logging_on (char *user_supplied_logging_filename)
{ logging_filename = user_supplied_logging_filename; }

void turn_logging_off (void)
{ logging_filename = NULL; }

static char *debug_string (int debug_level) 
{
    if (debug_level <= DEBUG_LEVEL_LOW) {
	return "DEBUG";
    } else if (debug_level <= DEBUG_LEVEL_NORMAL) {
	return "INFORMATION";
    } else if (debug_level <= DEBUG_LEVEL_ERROR) {
	return "ERROR";
    } else {
	return "CRITICAL ERROR";
    }
}

void print_error (int debug_level, const char *source_file, 
    const char *function_name, int line_number, char *fmt, ...)
{
    FILE *stream;
    va_list args;
    char timestring [_BUFSIZE];
    bool abort = (debug_level >= DEBUG_LEVEL_FATAL);

    // stream = (debug_level <= DEBUG_LEVEL_NORMAL) ?  stdout : stderr;
    stream = stdout;

    /* if logging required, we override the file stream */
    if (logging_filename != NULL)  {
	stream = fopen(logging_filename, "a");
	if (stream == NULL) {
	    stream = stdout;
	}
    }

    /* get time stamp */
    sprintf_calendar_time(timestring);

    /* print the message, flush & get out */
    fprintf(stream, "%s: %s: file %s function %s line %d:\n     ", 
	timestring, debug_string(debug_level), 
	source_file, function_name, line_number); 
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);
    fprintf(stream, "\n\n");
    if (abort) {
	fprintf(stream, "EXITING PROGRAM DUE TO CRITICAL ERROR\n\n");
    }
    fflush(stream);
    if (logging_filename != NULL) {
	fclose(stream);
    }

    /* if abort required, exit the process */
    if (abort) {
	_exit(1);
    }
}

/******************************************************************************
*******************************************************************************
**
** @@@@@ 
**
** CHECKS IF INITIALIZATION OF A SHARED 
** MEMORY BLOCK HAS BEEN DONE.  
**
*******************************************************************************
******************************************************************************/

static uint pattern [INITIALIZER_BLOCK_SIZE] = 
{
    0xDEA377F4, 3113, 543712, 111555, 15151, 
    0xDEADBEEF,
    5, 1234563, 555111000, 29, 0xBD8495F2
};

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** @@@@@ 
**
** RW LOCKS
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

static inline bool
readers_present (rwlock_t *rwl)
{ return rwl->readers > 0; }

static inline bool
no_readers_present (rwlock_t *rwl)
{ return rwl->readers <= 0; }

static inline bool
writers_pending (rwlock_t *rwl)
{ return rwl->writers > 0; }

static inline bool
no_writers_pending (rwlock_t *rwl)
{ return rwl->writers <= 0; }

static inline bool
writer_present (rwlock_t *rwl)
{ return rwl->writer_thread_id != 0; }

static inline bool
no_writer_present (rwlock_t *rwl)
{ return 0 == rwl->writer_thread_id; }

static inline bool
thread_already_has_write_lock (rwlock_t *rwl, pid_t thread_id)
{ return thread_id == rwl->writer_thread_id; }

static inline bool
thread_can_get_the_write_lock (rwlock_t* rwl, pid_t thread_id)
{
    /* there are readers, thread can not get the write lock */
    if (readers_present(rwl)) return FALSE;

    /* there is no actual writer, thread can get the lock */
    if (no_writer_present(rwl)) return TRUE;

    /* the requester already has the write lock, so its ok  */
    if (thread_already_has_write_lock(rwl, thread_id)) return TRUE;

    /* another thread has the write lock */
    return FALSE;
}

/*
** This function attempts to lock a mutex.  The mutex is initialized
** as a "robust" mutex.  Therefore, it always checks if the mutex is
** accidentally held by a process which has died or exited.  If that
** is the case, it tries to recover from this predicament.
*/

#define MAX_MUTEX_LOCK_ATTEMPTS   16

static inline void
safe_lock_mutex (rwlock_t *rwl)
{
    int err;
    int attempts = MAX_MUTEX_LOCK_ATTEMPTS;

    do {
        attempts--;

        err = pthread_mutex_lock(&rwl->mtx);

        /* success */
        if (0 == err) {
            return;
        }

        if (EOWNERDEAD == err) {
            pthread_mutex_consistent(&rwl->mtx);
            continue;
        }

    } while (attempts > 0);
}

/*
** Obtains a read lock.  The way this works is that a read access
** can be given only if there are no writers, pending or active.
**
** A pending writer is a thread waiting to obtain a write lock but
** has not quite got it yet.  It may be awaiting for all the readers
** to finish or another thread to finish writing.
**
** A pending writer always gets priority.  If writers are pending,
** further read accesses are not granted.  This is so that readers 
** cannot lock out a writer indefinitely.  If a writer is active/pending, 
** as soon as the current readers are done, the writer gets priority.  
** Until then, new read requests will not be granted.
**
** This function can be used in non blocking mode too.  Depending
** on the "block" parameter specified, it can either block until
** it can actually obtain the lock, or return immediately with
** a value of FALSE, if it cannot obtain the lock.  If it has
** successfully acquired the lock, it will return TRUE.
** 
** If it returns FALSE, it is the user's responsibility to make
** sure that critical sections are not accessed/modified.
*/
static bool 
rwlock_read_lock_engine (rwlock_t *rwl, bool block)
{
    safe_lock_mutex(rwl);
    while (writers_pending(rwl)) {
	pthread_mutex_unlock(&rwl->mtx);
	if (block) {
	    sched_yield();
	    safe_lock_mutex(rwl);
	    continue;
	}
	return FALSE;
    }
    rwl->readers++;
    pthread_mutex_unlock(&rwl->mtx);
    return TRUE;
}

/*
** Very similar to obtaining a read lock as shown above.  The
** only difference is that write lock is exclusive and can be
** granted to only ONE thread at a time, whereas a read lock
** can be granted to many threads.  The way this works is that
** first a writer request is marked.  This blocks out any further
** readers.  If there are no readers, and also no other writers, 
** then an exclusive write access is granted.  As long as the
** "previous" readers are present or another thread is actually
** writing, this will block.
**
** Note that this lock is re-entrant by the SAME thread.  This
** means that a thread may write lock repeatedly as long as
** it was granted the lock in the first place.  If that is the
** case, the corresponding unlock MUST be issued just as many
** times to actually release the write lock.
**
** The "block" parameter behaves in the same way as described
** for the read lock.
*/
static bool 
rwlock_write_lock_engine (rwlock_t *rwl, pid_t thread_id, bool block)
{
    safe_lock_mutex(rwl);

    /* 
    ** This is VERY important, we are indicating that 
    ** a writer is pending.  This means that we are waiting
    ** to obtain the write lock but have not quite got it yet.
    ** This is an indication that new readers can not be allowed 
    ** any more.  Without this indication, readers can for ever 
    ** lock out a writer.
    */
    rwl->writers++;

    while (TRUE) {

	/* get it if we can */
	if (thread_can_get_the_write_lock(rwl, thread_id)) {
	    rwl->writer_thread_id = thread_id;
	    pthread_mutex_unlock(&rwl->mtx);
	    return TRUE;
	}
	
	/* could not do it; either block & try again or return failure */
	pthread_mutex_unlock(&rwl->mtx);
	if (block) {
	    sched_yield();
	    safe_lock_mutex(rwl);
	    continue;
	}

	return FALSE;
    }
}

/*****************************************************************************/

/*
** Initialize a lock object.
** This routine makes sure that the mutex CAN also be
** used between different processes as well as threads
** if it has been defined in a shared memory area.
*/
PUBLIC bool 
rwlock_init (rwlock_t *rwl)
{
    pthread_mutexattr_t mtxattr;
    int rc;

    /* get default mutex attributes */
    if ((rc = pthread_mutexattr_init(&mtxattr)))
	return FALSE;

    /* 
    ** make mutex recursively enterable, for 
    ** nested calls by the same process/thread
    */
    if ((rc = pthread_mutexattr_settype(&mtxattr, 
	PTHREAD_MUTEX_RECURSIVE)))
	    return FALSE;

    /* make mutex sharable between processes */
    if ((rc = pthread_mutexattr_setpshared(&mtxattr, 
	PTHREAD_PROCESS_SHARED)))
	    return FALSE;

    /* make mutex robust */
    if ((rc = pthread_mutexattr_setrobust(&mtxattr, 
	PTHREAD_MUTEX_ROBUST)))
	    return FALSE;

    /* init with the desired attributes */
    rc = pthread_mutex_init(&rwl->mtx, &mtxattr);
    if ((rc != 0) && (rc != EBUSY))
	return FALSE;

    rwl->readers = 
	rwl->writers = 
	    rwl->writer_thread_id = 0;

    return TRUE;
}

PUBLIC void
rwlock_read_lock (rwlock_t *rwl)
{ 
    (void) rwlock_read_lock_engine(rwl, TRUE); 
}

PUBLIC bool
rwlock_read_lock_unblocked (rwlock_t *rwl)
{
    return rwlock_read_lock_engine(rwl, FALSE); 
}

PUBLIC void 
rwlock_read_unlock (rwlock_t *rwl)
{
    safe_lock_mutex(rwl);
    rwl->readers--;
    pthread_mutex_unlock(&rwl->mtx);
}

PUBLIC void
rwlock_write_lock (rwlock_t *rwl)
{
    int tid = get_thread_id();
    (void) rwlock_write_lock_engine(rwl, TRUE, tid);
}

PUBLIC void
rwlock_write_lock_unblocked (rwlock_t *rwl)
{
    int tid = get_thread_id();
    (void) rwlock_write_lock_engine(rwl, FALSE, tid);
}

PUBLIC void 
rwlock_write_unlock (rwlock_t *rwl)
{
    safe_lock_mutex(rwl);
    rwl->writers--;
    rwl->writer_thread_id = 0;
    pthread_mutex_unlock(&rwl->mtx);
}

static inline void
cond_mutex_lock (bool lock, rwlock_t *rwl)
{ if (lock) pthread_mutex_lock(&rwl->mtx); }

static inline void
cond_mutex_unlock (bool lock, rwlock_t *rwl)
{ if (lock) pthread_mutex_unlock(&rwl->mtx); }

static inline void
cond_read_lock (bool lock, rwlock_t *rwl)
{ if (lock) rwlock_read_lock(rwl); }

static inline void
cond_read_unlock (bool lock, rwlock_t *rwl)
{ if (lock) rwlock_read_unlock(rwl); }

static inline void
cond_write_lock (bool lock, rwlock_t *rwl)
{ if (bool) rwlock_write_lock(rwl); }

static inline void
cond_write_unlock (bool lock, rwlock_t *rwl)
{ if (bool) rwlock_write_unlock(rwl); }

/***************************************************************
****************************************************************
**
** @@@@@
**
** STACK OVERLAY OBJECT; USED FOR ALL SHARED MEM STUFF.
** NOONE IS EXPECTED TO USE THIS OBJECT DIRECTLY.  THEREFORE
** WE CAN LOCK IT ONLY FOR WRITING.  THIS MEANS LOCK ONLY
** THE MUTEX PART.
**
****************************************************************
***************************************************************/

void shm_stack_init (shm_stack_t *stack, char *name, 
    bool init_data_part, uint maxsize)
{
    int i;

    lock_init(&stack->lock);
    lock_mutex_only(&stack->lock);
    if (init_data_part) {

	/* reverse init direction so topmost entry has index 0 */
	for (i = 0; i < maxsize; i++) {
	    stack->data[maxsize - 1 - i] = i;
	}
	stack->maxsize = maxsize;
	stack->n = maxsize;
    }
    unlock_mutex(&stack->lock);
}

bool shm_stack_empty (shm_stack_t *stack)
{
    return (stack->n <= 0);
}

/*
** push user data into stack.  If ok, return TRUE.
** If no more space left in stack, return FALSE;
*/
bool shm_stack_push (shm_stack_t *stack, int value,
    bool lock_it)
{
    bool rv;

    cond_mutex_lock(lock_it, &stack->lock);
    if (stack->n >= stack->maxsize) {
	rv = FALSE;
    } else {
	stack->data[stack->n++] = value;
	rv = TRUE;
    }
    cond_mutex_unlock(lock_it, &stack->lock);
    return rv;
}

/*
** pop data from stack.  If ok, return TRUE.
** If no more data left, return FALSE;
*/
bool shm_stack_pop (shm_stack_t *stack, int *value,
    bool lock_it)
{
    bool rv;

    cond_mutex_lock(lock_it, &stack->lock);
    if (stack->n <= 0) {
	rv = FALSE;
    } else {
	*value = stack->data[--stack->n];
	rv = TRUE;
    }
    cond_mutex_unlock(lock_it, &stack->lock);
    return rv;
}

/*********************************************************************
**********************************************************************
**
** @@@@@
**
** BIT LIST OBJECT
**
**********************************************************************
*********************************************************************/

/*
** use this ONLY when u know all parameters are already valid
*/
static inline 
int get_bit_value (shm_bitlist_t *bl, int bit_number)
{
    return 
	((bl->bits[bit_number/BITS_PER_INT]) &
	    (1 << (bit_number % BITS_PER_INT)));
}

void shm_bitlist_init (shm_bitlist_t *bl, char *name, 
    uint desired_size_in_bits)
{
    lock_init(&bl->lock);
    get_write_lock(&bl->lock);
    bl->size_in_bits = desired_size_in_bits;
    bl->size_in_ints = INTS_FOR_BITS(desired_size_in_bits);
    bl->size_in_bytes = (bl->size_in_ints * sizeof(int));
    bl->n = 0;
    bzero(bl->bits, bl->size_in_bytes);
    release_write_lock(&bl->lock);
}

bool shm_bitlist_get_bit (shm_bitlist_t *bl, int bit_number,
    int *returned_bit_value, bool lock_it)
{
    cond_read_lock(lock_it, &bl->lock);
    if ((bit_number < 0) || (bit_number >= bl->size_in_bits)) {
	PRINT_ERROR("shm_bitlist bit out of range: "
	    "%d, range is 0 to %d",
	    bit_number, ((bl->size_in_bits)-1));
	cond_read_unlock(lock_it, &bl->lock);
	return FALSE;
    }
    *returned_bit_value = get_bit_value(bl, bit_number);
    cond_read_unlock(lock_it, &bl->lock);
    return TRUE;
}

bool shm_bitlist_set_bit (shm_bitlist_t *bl, int bit_number,
    bool lock_it)
{
    cond_write_lock(lock_it, &bl->lock);

    if ((bit_number < 0) || (bit_number >= bl->size_in_bits)) {
	PRINT_ERROR("shm_bitlist bit out of range: "
	    "%d, range is 0 to %d",
	    bit_number, ((bl->size_in_bits)-1));
	cond_write_unlock(lock_it, &bl->lock);
	return FALSE;
    }

    /* increment the 1 count ONLY if not already set */
    if (get_bit_value(bl, bit_number) == 0) {
	bl->bits[bit_number/BITS_PER_INT] |= 
	    (1 << (bit_number % BITS_PER_INT));
	bl->n++;
    }

    cond_write_unlock(lock_it, &bl->lock);

    return TRUE;
}

bool shm_bitlist_clear_bit (shm_bitlist_t *bl, int bit_number,
    bool lock_it)
{
    cond_write_lock(lock_it, &bl->lock);

    if ((bit_number < 0) || (bit_number >= bl->size_in_bits)) {
	PRINT_ERROR("shm_bitlist bit out of range: "
	    "%d, range is 0 to %d",
	    bit_number, ((bl->size_in_bits)-1));
	cond_write_unlock(lock_it, &bl->lock);
	return FALSE;
    }

    /* decrement the 1 count ONLY if not already 0 */
    if (get_bit_value(bl, bit_number) != 0) {
	bl->bits[bit_number/BITS_PER_INT] &= 
	    (~(1 << (bit_number % BITS_PER_INT)));
	bl->n--;
    }

    cond_write_unlock(lock_it, &bl->lock);

    return TRUE;
}

void shm_bitlist_collect_ones (shm_bitlist_t *bl, 
    int *returned_values, int *howmany, bool lock_it)
{
    int i, limit, count;
    int value, word_bit, overall_bit;

    cond_read_lock(lock_it, &bl->lock);

    /* nothing in bitlist */
    if (bl->n <= 0) {
	*howmany = 0;
	goto end_bitlist_collect_ones;
    }

    /* cannot get more than what is in the list */
    limit = *howmany;
    if (limit > bl->n) {
	*howmany = limit = bl->n;
    }

    /* start collecting */
    for (i = 0, count = 0; i < bl->size_in_ints; i++) {
	if ((value = bl->bits[i]) != 0) {
	    while (value) {
		word_bit = ffs(value) - 1;
		value &= ~(1 << word_bit);
		overall_bit = (i * BITS_PER_INT) + word_bit;
		returned_values[count++] = overall_bit;
		if (count >= limit) {
		    goto end_bitlist_collect_ones;
		}
	    }
	}
    }

end_bitlist_collect_ones:
    cond_read_unlock(lock_it, &bl->lock);
}

void shm_bitlist_reset (shm_bitlist_t *bl, bool lock_it)
{
    cond_write_lock(lock_it, &bl->lock);
    bzero(bl->bits, bl->size_in_bytes);
    bl->n = 0;
    cond_write_unlock(lock_it, &bl->lock);
}

/***************************************************************
****************************************************************
**
** @@@@@
**
** SHARED MEMORY MALLOC & FREE
**
****************************************************************
****************************************************************/

#define _USED_		0	// chunk is used
#define _FREE_		1	// chunk is available

/*
** Each mempool block has so many chunks of 
** the given chunk size
*/
#define MALLOC_CHUNK_SIZE		256
#define MALLOC_MAX_CHUNKS		(16 * 1024)	
#define MAX_HEAP_CONTROL_BLOCKS		256

/*
** administration info for freeing a 
** memory block using shm_malloc.
*/
typedef struct free_admin_info_s {
    int block_number;
    int start_chunk_index;
    int chunks_used;
} free_admin_info_t;

/*
** size of one chunk, ie, minimum allocable memory
*/
typedef struct chunk_s {
    byte memory [MALLOC_CHUNK_SIZE];
} chunk_t;

/*
** each one of these is a self contained block of memory.
** When it fills up, we chain another one to it, to the
** blocks list below in the mempool structure.
*/
typedef struct mempool_control_block_s {
    int block_number;
    int first_free_chunk_index;
    int free_chunks_left;
    byte freelist[MALLOC_MAX_CHUNKS];
    chunk_t chunks[MALLOC_MAX_CHUNKS];
} mempool_control_block_t;

/*
** This resides in shared memory only.
** Block names are derived using the base_name and appending
** _%d block number to it.  For example, if base_name is
** "my_memory", block 0 will be named "my_memory_0", block
** number 1 named as "my_memory_1" and so on.
*/
typedef struct shm_mempool_s {
    rwlock_t lock;
    byte base_name [48];
    int number_of_blocks;
} shm_mempool_t;

/*
** This resides in LOCAL MEMORY of each process.
** When shmem mempool is mapped, depending on how 
** many blocks have been mapped in, the local
** process can map them in.  
**
** We allow MAX_HEAP_CONTROL_BLOCKS of expansion space.
** With a value of 256, this means the absolute max
** shared memory for one shm can grow to 256 * (16 * 1024)
** chunks of 256 bytes each.  This is approx 1 Gig.
*/
typedef struct local_mempool_s {

    shm_mempool_t *shm_memp;
    int number_of_blocks;
    mempool_control_block_t 
	*block_address_table [MAX_HEAP_CONTROL_BLOCKS];

} local_mempool_t;

/*
** with the current configuration above, maximum possible
** contiguous memory allocable is the entire mempool control
** block size minus the administration info at the front of it.
*/
static inline int
MAX_MALLOCABLE_SIZE (void)
{ 
    return (MALLOC_MAX_CHUNKS * MALLOC_CHUNK_SIZE) - 
	sizeof(free_admin_info_t); 
}

/************************************************************************/

static int
find_consecutive_free_chunks (mempool_control_block_t *mcb, 
    int howmany)
{
    int i, first, last;

    first = mcb->first_free_chunk_index;

TRY_AGAIN:

    /* index of last chunk */
    last = first + howmany - 1;

    /* we are past the end, none in this block */
    if (last >= MALLOC_MAX_CHUNKS) {
	return -1;
    }

    /* if last entry is used, skip entire block */
    if (mcb->freelist[last] == _USED_) {
	first = last + 1;
	goto TRY_AGAIN;
    }

    /* check if all bytes between first & last inclusive are free */
    for (i = first; i <= last; i++) {

	/* skip past the used to start checking again */
	if (mcb->freelist[i] == _USED_) {
	    first = i + 1;
	    goto TRY_AGAIN;
	}
    }

    /* we found it at this position */
    return first;
}

static void mark_chunks (mempool_control_block_t *mcb, int first, 
    int howmany, byte value)
{ 
    memset(&mcb->freelist[first], value, howmany); 
}

static void 
mcb_init_data (mempool_control_block_t *mcb, int block_number)
{
    /* first zero out everything */
    memset(mcb, 0, sizeof(mempool_control_block_t));

    /* some initializations */
    mcb->block_number = block_number;
    mcb->first_free_chunk_index = 0;
    mcb->free_chunks_left = MALLOC_MAX_CHUNKS;
    
    /* mark all freelist as available */
    memset(&mcb->freelist[0], _FREE_, MALLOC_MAX_CHUNKS);
}

/*
** maps/creates/initializes a missing/new mcb
*/
static mempool_control_block_t *
generic_map_mcb (shm_mempool_t *smp, int block_number, bool create)
{
    mempool_control_block_t *mcb;
    char block_name [48];

    sprintf(block_name, "%s_%d", smp->base_name, block_number);
    mcb = (mempool_control_block_t*) 
	map_shared_memory(block_name, 
	    sizeof(mempool_control_block_t), create);
    if (mcb) {
	//PRINT_INFO("mapped %s to process space", block_name);
	if (create) {
	    (smp->number_of_blocks)++;
	    mcb_init_data(mcb, block_number);
	}
    }
    return mcb;
}

/*
** If this guy has some missing mcbs that has been
** created before, map those new ones in.  Note that
** this call does NOT create NEW mcbs, only maps in
** the missing ones.
*/
static void map_unmapped_mcbs (local_mempool_t *lmp)
{
    int local_count, shm_count;
    shm_mempool_t *smp = lmp->shm_memp;
    mempool_control_block_t *mcb;

    shm_count = smp->number_of_blocks;
    local_count = lmp->number_of_blocks;

    while (local_count < shm_count) {
	mcb = generic_map_mcb(smp, local_count, FALSE);
	if (NULL == mcb) {
	    PRINT_ERROR("map_mcb failed for %s count %d",
		smp->base_name, local_count);
	    break;
	}
	//PRINT_INFO("successfully mapped block %d for %s",
	    //local_count, smp->base_name);
	lmp->block_address_table[local_count] = mcb;
	local_count++;
    }
    lmp->number_of_blocks = local_count;
}

static byte *
mcb_malloc (mempool_control_block_t *mcb, int size)
{
    int start_idx, howmany_chunks_needed;
    byte *addr;
    free_admin_info_t *fap;

    if (NULL == mcb) {
	return NULL;
    } 

    howmany_chunks_needed = size / MALLOC_CHUNK_SIZE;
    if (size % MALLOC_CHUNK_SIZE) {
	howmany_chunks_needed++;
    }
    
    /* we dont have enuf chunks left, consecutive or otherwise */
    if (mcb->free_chunks_left < howmany_chunks_needed) {
	return NULL;
    }

    /* find the chunks */
    start_idx = find_consecutive_free_chunks(mcb, 
	howmany_chunks_needed);

    /* nothing of that size left */
    if (start_idx < 0) {
	return NULL;
    }

    /* mark the chunks as used up */
    mark_chunks(mcb, start_idx, howmany_chunks_needed, _USED_);
    mcb->free_chunks_left -= howmany_chunks_needed;
    mcb->first_free_chunk_index += howmany_chunks_needed;

    /* this is the start of chunk memory, keep admin info in there */
    addr = &(mcb->chunks[start_idx].memory[0]);
    fap = (free_admin_info_t*) addr;
    fap->block_number = mcb->block_number;
    fap->start_chunk_index = start_idx;
    fap->chunks_used = howmany_chunks_needed;

    /* this is what we return to user */
    addr += sizeof(free_admin_info_t);

    return addr;
}

/************************************************************************/

void *shm_allocator_init (char *name, bool create)
{
    local_mempool_t *lmp = malloc(sizeof(local_mempool_t));

    if (NULL == lmp) {
	PRINT_ERROR("malloc for local_mempool_t (%d bytes) failed",
	    sizeof(local_mempool_t));
	return NULL;
    }

    memset(lmp, 0, sizeof(local_mempool_t));
    lmp->shm_memp = (shm_mempool_t*) 
	map_shared_memory(name, sizeof(shm_mempool_t), create);
    if (lmp->shm_memp) {
	strncpy((char*) &(lmp->shm_memp->base_name[0]), 
	    (const char*) name, 48);
	if (create) {
	    lock_init(&lmp->shm_memp->lock);
	    lmp->shm_memp->number_of_blocks = 0;
	}
	return lmp;
    }
    return NULL;
}

void *shm_allocator_create (char *name)
{ return shm_allocator_init(name, TRUE); }

void *shm_allocator_attach (char *name)
{ return shm_allocator_init(name, FALSE); }

byte *shm_alloc (void *handle, int size, shm_address_t *shm_addr,
    bool lock_it)
{
    local_mempool_t *lmp = (local_mempool_t*) handle;
    int block_number, shm_count;
    mempool_control_block_t *mcb;
    byte *addr;
    shm_mempool_t *shm_memp = lmp->shm_memp;

    /* these sizes are completely illegal */
    if ((size <= 0) || (size > MAX_MALLOCABLE_SIZE())) {
	return NULL;
    }

    cond_write_lock(lock_it, &shm_memp->lock);

    /* we use some admin space internally for freeing */
    size += sizeof(free_admin_info_t);

    /* map missing mcb's in case some was created before that we missed */
    map_unmapped_mcbs(lmp);

    shm_count = shm_memp->number_of_blocks;
    block_number = 0;
    while (block_number < shm_count) {
	mcb = lmp->block_address_table[block_number];
	addr = mcb_malloc(mcb, size);
	if (addr) {
	    goto FOUND_MEMORY;
	}
	block_number++;
    }

    /* 
    ** if we are here, we could not find any in existing blocks, 
    ** therefore allocate a NEW block and append it to the blocks
    */
    if (block_number >= MAX_HEAP_CONTROL_BLOCKS) {
	PRINT_ERROR("reached end of mappable blocks");
	cond_write_unlock(lock_it, &shm_memp->lock);
	return NULL;
    }

    //PRINT_INFO("adding block %d to %s", 
	//block_number, shm_memp->base_name);
    mcb = generic_map_mcb(shm_memp, block_number, TRUE);
    lmp->block_address_table[block_number] = mcb;
    lmp->number_of_blocks++;
    addr = mcb_malloc(mcb, size);

FOUND_MEMORY:

    if (addr) {
	if (shm_addr) {
	    shm_addr->block_number = block_number;
	    shm_addr->offset = (addr - ((byte*) mcb));
	}
	cond_write_unlock(lock_it, &shm_memp->lock);
	return addr;
    }

    /* 
    ** If we are here, something wrong.  
    ** We SHOULD have found a free chunk 
    ** since we just mapped a NEW block
    */
    PRINT_ERROR("could not find a chunk of %d bytes "
	"in new block %d of %s",
	size, block_number, shm_memp->base_name);
    cond_write_unlock(lock_it, &shm_memp->lock);
    return NULL;
}

byte *shm_addr2addr (void *handle, shm_address_t *shmaddr,
    bool lock_it)
{
    local_mempool_t *lmp = (local_mempool_t*) handle;
    shm_mempool_t *shm_memp = lmp->shm_memp;
    byte *addr;

    cond_write_lock(lock_it, &shm_memp->lock);
    map_unmapped_mcbs(lmp);
    addr = (byte*) (lmp->block_address_table[shmaddr->block_number]);
    addr += shmaddr->offset;
    cond_write_unlock(lock_it, &shm_memp->lock);
    return addr;
}

void shm_free (void *handle, byte *ptr, bool lock_it)
{
    local_mempool_t *lmp = (local_mempool_t*) handle;
    mempool_control_block_t *mcb;
    int block_number, chunk_start, number_chunks;
    free_admin_info_t *fap;
    shm_mempool_t *shm_memp = lmp->shm_memp;

    cond_write_lock(lock_it, &shm_memp->lock);

    map_unmapped_mcbs(lmp);

    /* obtain admin information */
    ptr -= sizeof(free_admin_info_t);
    fap = (free_admin_info_t*) ptr;
    block_number = fap->block_number;
    chunk_start = fap->start_chunk_index;
    number_chunks = fap->chunks_used;

    /* zero out the memory */
    memset(ptr, 0, (number_chunks * MALLOC_CHUNK_SIZE));

    /* we regained these chunks */
    mcb = lmp->block_address_table[block_number];
    mcb->free_chunks_left += number_chunks;
    mark_chunks(mcb, chunk_start, number_chunks, _FREE_);

    /*
    ** if any chunks have been freed BEFORE the 
    ** last recorded one, update first_free_chunk_index
    ** with this new value
    */
    if (chunk_start < mcb->first_free_chunk_index) {
	mcb->first_free_chunk_index = chunk_start;
    }
    
    cond_write_unlock(lock_it, &shm_memp->lock);
}

void shm_address_free (void *handle, shm_address_t *shm_addr,
    bool lock_it) 
{
    byte *addr = shm_addr2addr(handle, shm_addr, lock_it);
    shm_free(handle, addr, lock_it);
}

void nullify_shm_address (shm_address_t *shmaddr)
{
    shmaddr->block_number = -1;
    shmaddr->offset = -1;
}

bool null_shm_address (shm_address_t *shmaddr)
{
    return ((shmaddr->block_number < 0) || (shmaddr->offset < 0));
}

/***************************************************************
****************************************************************
**
** @@@@@
**
** SHARED MEMORY INDEX OBJECT
**
****************************************************************
****************************************************************/

/* handles overlapping regions */
static inline
void copy_shm_indxobj_elements (shm_indxobj_node_t *src, 
    shm_indxobj_node_t *dst, register int size)
{
    if (dst < src)
	while (size-- > 0) *dst++ = *src++;
    else {
	src += size;
	dst += size;
	while (size-- > 0) *(--dst) = *(--src);
    }
}

void shm_indxobj_init (shm_indxobj_t *indxobj, char *name, 
    int max_size, bool duplicates_allowed)
{
    lock_init(&indxobj->lock);
    get_write_lock(&indxobj->lock);
    indxobj->maxsize = max_size;
    indxobj->duplicates_allowed = duplicates_allowed;
    indxobj->n = 0;
    release_write_lock(&indxobj->lock);
}

/*
** THIS IS THE GUTS SEARCH OF THE OBJECT.
** IT HAS TO BE HIGHLY OPTIMAL
**
** searches an EXACT match in the index that matches the 
** key "key".  Returns the index if found, otherwise returns 
** -1.  If the element is NOT found, the routine returns the 
** insertion point of the element in "insertion_point".  This 
** is the place that the element should be inserted into if 
** it was to be put into the index object.  On the other hand, 
** if the element is found, the value in "insertion_point"
** is NOT changed.  Note that "insertion_point" can be specified 
** as NULL.
*/
static int shm_indxobj_find_position (shm_indxobj_t *ind,
    comparing_function cmp, void *key, int *insertion_point)
{
    register int mid, diff, lo, hi;
    shm_indxobj_node_t *elems = ind->elements;

    lo = mid = diff = 0;
    hi = ind->n - 1;

    /* binary search */
    while (lo <= hi) {

	/* integer divide by 2, but shift right by 1 is faster */
	mid = (hi+lo) >> 1;
	diff = (*cmp)(key, elems[mid].data);
	if (diff > 0) {
	    lo = mid + 1;
	} else if (diff == 0) {
	    return mid;
	} else {
	    hi = mid - 1;
	}
    }

    /*
    ** not found, but record where the element should be 
    ** inserted, in case it was required to be put into 
    ** the index.
    */
    if (insertion_point) {
	*insertion_point = (diff > 0 ? (mid+1) : mid);
    }
    return (-1);
}

void shm_indxobj_insert (shm_indxobj_t *ind, comparing_function cmp, 
    void *key, int data, int *refcount, bool lock_it)
{
    int insertion_point;
    int size, idx;
    shm_indxobj_node_t *source;
    shm_indxobj_node_t *elem;

    /* protect */
    cond_write_lock(lock_it, &ind->lock);

    /*
    ** see if element is already there and if not,
    ** note the insertion point in "insertion_point".
    */
    idx = shm_indxobj_find_position(ind, cmp, key, &insertion_point);

    /* 
    ** if already in there, manipulate ref counts 
    */
    if (idx != (-1)) {
	elem = &ind->elements[idx];
	if (ind->duplicates_allowed) {
	    elem->refcount++;
	}
	*refcount = elem->refcount;
	cond_write_unlock(lock_it, &ind->lock);
	return;
    }

    /* if we are here, it is NOT there */

    /* if ran out of space not much we can do :-( */
    if (ind->n >= ind->maxsize) {
	*refcount = 0;
	cond_write_unlock(lock_it, &ind->lock);
	return;
    }

    /*
    ** shift all of the pointers after 
    ** "insertion_point" right by one 
    */
    elem = source = &(ind->elements[insertion_point]);
    if ((size = ind->n - insertion_point) > 0)
	copy_shm_indxobj_elements(source, (source+1), size);
    
    /* insert the new element pointer into its rightful place */
    elem->refcount = 1;
    elem->data = data;

    /* increment element count */
    ind->n++;

    /* successfully completed */
    *refcount = elem->refcount;
    
    /* reallow access */
    cond_write_unlock(lock_it, &ind->lock);
}

/* 
** search an element in the index 
*/
void shm_indxobj_search (shm_indxobj_t *ind, comparing_function cmp, 
    void *key, int *refcount, int *returned_data,
    bool lock_it)
{
    int idx;
    shm_indxobj_node_t *elem;

    /* protect */
    cond_read_lock(lock_it, &ind->lock);

    idx = shm_indxobj_find_position(ind, cmp, key, NULL);

    /* not found */
    if (idx == (-1)) {
	*refcount = 0;
	cond_read_unlock(lock_it, &ind->lock);
	return;
    }

    elem = &ind->elements[idx];
    *refcount = elem->refcount;
    *returned_data = elem->data;

    /* reallow access */
    cond_read_unlock(lock_it, &ind->lock);
}

void shm_indxobj_remove (shm_indxobj_t *ind, comparing_function cmp, 
    void *key, int how_many_to_remove, 
    int *how_many_actually_removed, int *how_many_remaining,
    int *data_value_removed,
    bool lock_it)
{
    int idx, size, count;
    shm_indxobj_node_t *elem;

    /* protect */
    cond_write_lock(lock_it, &ind->lock);

    /* first see if it is there */
    idx = shm_indxobj_find_position(ind, cmp, key, NULL);

    /* not in table */
    if (idx == (-1)) {
	*how_many_actually_removed = 
	*how_many_remaining = 0;
	cond_write_unlock(lock_it, &ind->lock);
	return;
    }

    /* found the element, remember the user data for returning */
    elem = &ind->elements[idx];
    *data_value_removed = elem->data;

    /* manipulate ref counts */
    count = elem->refcount;
    if (how_many_to_remove > count) {
	how_many_to_remove = count;
    }
    elem->refcount -= how_many_to_remove;
    *how_many_actually_removed = how_many_to_remove;
    *how_many_remaining = elem->refcount;

    /* is it time to remove the whole node ? */
    if (elem->refcount > 0) {
	cond_write_unlock(lock_it, &ind->lock);
	return;
    }

    /* decrement count; one less element */
    ind->n--;

    /* pull the elements AFTER "index" to the left by one */
    if ((size = ind->n - idx) > 0) {
	shm_indxobj_node_t *source = &(ind->elements[idx+1]);
	copy_shm_indxobj_elements(source, (source-1), size);
    }
    
    /* reallow access */
    cond_write_unlock(lock_it, &ind->lock);
}

/*
** return n'th element from the index
*/
int shm_indxobj_nth (shm_indxobj_t *ind, int n, bool *success,
    bool lock_it)
{
    int data = 0;

    cond_read_lock(lock_it, &ind->lock);
    *success = FALSE;
    if ((n > 0) && (n < ind->n)) {
	data = ind->elements[n].data;
	*success = TRUE;
    }
    cond_read_unlock(lock_it, &ind->lock);
    return data;
}

/*
** returns the LAST element in the list *AND* takes it
** OUT of the list.  If no elems exist, success is set 
** to FALSE.
*/
int shm_indxobj_end_pop (shm_indxobj_t *ind, bool *success,
    bool lock_it)
{
    int data = 0;

    cond_write_lock(lock_it, &ind->lock);
    *success = FALSE;
    if (ind->n > 0) {
	ind->n--;
	data = ind->elements[ind->n].data;
	*success = TRUE;
    }
    cond_write_unlock(lock_it, &ind->lock);
    return data;
}

/*
** empty out all elements from the index
*/
void shm_indxobj_reset (shm_indxobj_t *ind, bool lock_it)
{
    cond_write_lock(lock_it, &ind->lock);
    ind->n = 0;
    cond_write_unlock(lock_it, &ind->lock);
}

/*
** These are provided for the user to explicitly be able to lock
** or unlock an index object.  They can be useful when the
** user wants to perform many consecutive protected actions
** on the index object, which by default are not protected.
** Since the mutex object type is set to be RECURSIVE at init time,
** this should not cause any problems.
*/
static inline
void shm_indxobj_lock_operation (shm_indxobj_t *ind, bool lock)
{
    if (lock) {
	get_read_lock(&ind->lock);
    } else {
	release_read_lock(&ind->lock);
    }
}

void shm_indxobj_read_lock (shm_indxobj_t *ind)
{
    shm_indxobj_lock_operation(ind, TRUE);
}

void shm_indxobj_release_read_lock (shm_indxobj_t *ind)
{
    shm_indxobj_lock_operation(ind, FALSE);
}

/***************************************************************
****************************************************************
** 
** @@@@@
**
** SHARED MEMORY CHUNK OBJECT
**
****************************************************************
***************************************************************/

/* not done yet */

/***************************************************************
****************************************************************
** 
** @@@@@
**
** SHARED MEMORY AVL TREES
**
****************************************************************
***************************************************************/

/* 
** given a node pointer, get its index
*/
static inline
int P2I (shm_avl_node_t *node) 
{ return (node == NULL) ? -1 : node->index; }

/* 
** given an index, get the node pointer
*/
static inline
shm_avl_node_t *I2P (shm_avl_tree_t *tree, int index)
{ return (index < 0) ? NULL :  &tree->shm_avl_nodes[index]; }

static inline 
bool is_root (shm_avl_node_t *node)
{ return (node->parent < 0); }

static inline 
int get_balance (shm_avl_node_t *node)
{ return node->balance; }

static inline 
void set_balance (shm_avl_node_t *node, int balance)
{ node->balance = balance; }

static inline 
int inc_balance (shm_avl_node_t *node)
{ return ++node->balance; }

static inline 
int dec_balance(shm_avl_node_t *node)
{ return --node->balance; }

static inline 
shm_avl_node_t *get_parent (shm_avl_tree_t *tree, shm_avl_node_t *node)
{ return I2P(tree, node->parent); }

static inline 
void set_parent (shm_avl_node_t *parent, shm_avl_node_t *node)
{ node->parent = P2I(parent); }

static inline 
shm_avl_node_t *get_first (shm_avl_tree_t *tree, shm_avl_node_t *node)
{
    while (node->left >= 0)
	node = I2P(tree, node->left);
    return node;
}

static inline 
shm_avl_node_t *get_last (shm_avl_tree_t *tree, shm_avl_node_t *node)
{
    while (node->right >= 0)
	node = I2P(tree, node->right);
    return node;
}

static inline
shm_avl_node_t *avltree_first (shm_avl_tree_t *tree)
{ return I2P(tree, tree->first); }

static inline
shm_avl_node_t *avltree_last (shm_avl_tree_t *tree)
{ return I2P(tree, tree->last); }

static
shm_avl_node_t *avltree_next (shm_avl_tree_t *tree, shm_avl_node_t *node)
{
    shm_avl_node_t *parent;

    if (node->right >= 0)
	return get_first(tree, I2P(tree, node->right));

    while ((parent = get_parent(tree, node)) && 
	(parent->right == P2I(node)))
	    node = parent;

    return parent;
}

static 
shm_avl_node_t *avltree_prev (shm_avl_tree_t *tree, shm_avl_node_t *node)
{
    shm_avl_node_t *parent;

    if (node->left >= 0)
	return get_last(tree, I2P(tree, node->left));

    while ((parent = get_parent(tree, node)) && 
	(parent->left == P2I(node)))
	    node = parent;
	
    return parent;
}

static
void rotate_left (shm_avl_node_t *node, shm_avl_tree_t *tree)
{
    shm_avl_node_t *p = node;
    shm_avl_node_t *q = I2P(tree, node->right);	/* can't be NULL */
    shm_avl_node_t *parent = get_parent(tree, p);

    if (!is_root(p)) {
	if (parent->left == P2I(p))
	    parent->left = P2I(q);
	else
	    parent->right = P2I(q);
    } else
	tree->root = P2I(q);
    set_parent(parent, q);
    set_parent(q, p);
    p->right = q->left;
    if (p->right >= 0)
	set_parent(p, I2P(tree, p->right));
    q->left = P2I(p);
}

static 
void rotate_right (shm_avl_node_t *node, shm_avl_tree_t *tree)
{
    shm_avl_node_t *p = node;
    shm_avl_node_t *q = I2P(tree, node->left);	/* can't be NULL */
    shm_avl_node_t *parent = get_parent(tree, p);

    if (!is_root(p)) {
	if (parent->left == P2I(p))
	    parent->left = P2I(q);
	else
	    parent->right = P2I(q);
    } else
	tree->root = P2I(q);
    set_parent(parent, q);
    set_parent(q, p);

    p->left = q->right;
    if (p->left >= 0)
	set_parent(p, I2P(tree, p->left));
    q->right = P2I(p);
}

static inline 
shm_avl_node_t *do_lookup (shm_avl_tree_t *tree, comparing_function cmpfp, 
    void *key, shm_avl_node_t **pparent, shm_avl_node_t **unbalanced,
    bool *is_left)
{
    shm_avl_node_t *node = I2P(tree, tree->root);
    int res = 0;

    *pparent = NULL;
    *unbalanced = node;
    *is_left = FALSE;

    while (node) {
	if (get_balance(node) != 0)
	    *unbalanced = node;
	res = cmpfp(key, node->data);
	if (res == 0)
	    return node;
	*pparent = node;
	if ((*is_left = (res < 0)))
	    node = I2P(tree, node->left);
	else
	    node = I2P(tree, node->right);
    }
    return NULL;
}

static inline
shm_avl_node_t *new_shm_avl_node (shm_avl_tree_t *tree)
{
    shm_stack_t *stk;
    int index;

    stk = (shm_stack_t*) (((byte*) tree) + tree->offset_to_stack);
    if (shm_stack_pop(stk, &index, FALSE)) {
	tree->count++;
	return &tree->shm_avl_nodes [index];
    }
    return NULL;
}

static inline 
void free_shm_avl_node (shm_avl_tree_t *tree, shm_avl_node_t *node)
{
    shm_stack_t *stk;

    stk = (shm_stack_t*) (((byte*) tree) + tree->offset_to_stack);
    shm_stack_push(stk, node->index, FALSE);
    tree->count--;
    if (tree->count <= 0) {
	assert(tree->root < 0);
    } else {
	assert(tree->root >= 0);
    }
}


static 
void set_child (shm_avl_node_t *child, shm_avl_node_t *node, 
    bool left)
{
    if (left)
	node->left = P2I(child);
    else
	node->right = P2I(child);
}

/************************* public routines *************************/

void 
shm_avl_tree_init (shm_avl_tree_t *tree, char *name,
    int size, bool duplicates_allowed)
{
    int i;
    shm_stack_t *stk;

    lock_init(&tree->lock);
    get_write_lock(&tree->lock);

    /* stack starts after all the nodes */
    tree->offset_to_stack = (((long) &tree->shm_avl_nodes[0]) - 
	((long) tree)) + (size * sizeof(shm_avl_node_t));
    stk = (shm_stack_t*) (((long) tree) + tree->offset_to_stack);
    shm_stack_init(stk, name, TRUE, size);

    /* stack is initted in reverse order, make sure this also complies */
    for (i = 0; i < size; i++) {
	tree->shm_avl_nodes[i].index = i;
    }

    tree->size = size;
    tree->duplicates_allowed = duplicates_allowed;
    tree->root = -1;
    tree->count = 0;

    release_write_lock(&tree->lock);
}

/* 
** Insertion never needs more than 2 rotations 
*/
void shm_avl_tree_insert (shm_avl_tree_t *tree, comparing_function cmpfp,
    void *key, int data, int *refcount, bool lock_it)
{
    shm_avl_node_t *found, *parent, *unbalanced, *node;
    bool is_left;

    cond_write_lock(lock_it, &tree->lock);

    found = do_lookup(tree, cmpfp, key, &parent, 
	&unbalanced, &is_left);
    if (found) {
	if (tree->duplicates_allowed) {
	    found->refcount++;
	}
	*refcount = found->refcount;
	cond_write_unlock(lock_it, &tree->lock);
	return;
    }

    node = new_shm_avl_node(tree);

    /* no new node; no more space */
    if (node == NULL) {
	*refcount = 0;
	cond_write_unlock(lock_it, &tree->lock);
	return;
    }

    /* initialize new node */
    node->left = node->right = node->parent = (-1);
    node->balance = 0;
    node->data = data;
    node->refcount = 1;

    if (!parent) {
	tree->root = P2I(node);
	tree->first = tree->last = P2I(node);
	*refcount = 1;
	cond_write_unlock(lock_it, &tree->lock);
	return;
    }

    if (is_left) {
	if (P2I(parent) == tree->first)
	    tree->first = P2I(node);
    } else {
	if (P2I(parent) == tree->last)
	    tree->last = P2I(node);
    }
    set_parent(parent, node);
    set_child(node, parent, is_left);
    for (;;) {
	if (parent->left == P2I(node))
	    dec_balance(parent);
	else
	    inc_balance(parent);
	if (parent == unbalanced)
	    break;
	node = parent;
	parent = get_parent(tree, parent);
    }

    switch (get_balance(unbalanced)) {
    case  1: 
    case -1:
    case 0:
	break;
	
    case 2: 
	{
	    shm_avl_node_t *right = I2P(tree, unbalanced->right);

	    if (get_balance(right) == 1) {
		set_balance(unbalanced, 0);
		set_balance(right, 0);
	    } else {
		switch (get_balance(I2P(tree, right->left))) {
		case 1:
		    set_balance(unbalanced, -1);
		    set_balance(right, 0);
		    break;
		case 0:
		    set_balance(unbalanced, 0);
		    set_balance(right, 0);
		    break;
		case -1:
		    set_balance(unbalanced, 0);
		    set_balance(right, 1);
		    break;
		}
		set_balance(I2P(tree, right->left), 0);
		rotate_right(right, tree);
	    }
	    rotate_left(unbalanced, tree);
	    break;
	}
    case -2: 
	{
	    shm_avl_node_t *left = I2P(tree, unbalanced->left);

	    if (get_balance(left) == -1) {
		set_balance(unbalanced, 0);
		set_balance(left, 0);
	    } else {
		switch (get_balance(I2P(tree, left->right))) {
		case 1:
		    set_balance(unbalanced, 0);
		    set_balance(left, -1);
		    break;
		case 0:
		    set_balance(unbalanced, 0);
		    set_balance(left, 0);
		    break;
		case -1:
		    set_balance(unbalanced, 1);
		    set_balance(left, 0);
		    break;
		}
		set_balance(I2P(tree, left->right), 0);
		rotate_left(left, tree);
	    }
	    rotate_right(unbalanced, tree);
	    break;
	}
    }
    *refcount = 1;
    cond_write_unlock(lock_it, &tree->lock);
}

void shm_avl_tree_search (shm_avl_tree_t *tree, comparing_function cmpfp,
    void *key, int *refcount, int *returned_data,
    bool lock_it)
{
    shm_avl_node_t *parent, *unbalanced, *node;
    bool is_left;

    cond_read_lock(lock_it, &tree->lock);
    node = do_lookup(tree, cmpfp, key, 
		&parent, &unbalanced, &is_left);
    if (node) {
	*refcount = node->refcount;
	*returned_data = node->data;
    } else {
	*refcount = 0;
    } 
    cond_read_unlock(lock_it, &tree->lock);
}

void shm_avl_tree_remove (shm_avl_tree_t *tree, comparing_function cmpfp,
    void *key, int howmany_entries_to_remove, 
    int *howmany_actually_removed, int *still_remaining, 
    int *data_value_removed,
    bool lock_it)
{
    shm_avl_node_t *node, *to_be_deleted;
    shm_avl_node_t *parent, *unbalanced;
    shm_avl_node_t *left;
    shm_avl_node_t *right;
    shm_avl_node_t *next;
    bool is_left;
    
    cond_write_lock(lock_it, &tree->lock);

    /* find the matching node first */
    node = do_lookup(tree, cmpfp, key, &parent, 
	&unbalanced, &is_left);

    /* not there */
    if (!node) {
	*howmany_actually_removed = 0;
	*still_remaining = 0;
	cond_write_unlock(lock_it, &tree->lock);
	return;
    }

    /* if we are here, we found it */
    *data_value_removed = node->data;
    if (node->refcount < howmany_entries_to_remove) {
	howmany_entries_to_remove = node->refcount;
    }
    node->refcount -= howmany_entries_to_remove;
    *howmany_actually_removed = howmany_entries_to_remove;
    *still_remaining = node->refcount;
    if (node->refcount > 0) {
	cond_write_unlock(lock_it, &tree->lock);
	return;
    }

    /* 
    ** if we are here, refcount is <= 0, time to 
    ** remove the node itself completely 
    */

    /* cache it for later freeing */
    to_be_deleted = node;

    parent = get_parent(tree, node);
    left = I2P(tree, node->left);
    right = I2P(tree, node->right);

    if (P2I(node) == tree->first)
	tree->first = P2I(avltree_next(tree, node));
    if (P2I(node) == tree->last)
	tree->last = P2I(avltree_prev(tree, node));

    if (!left)
	next = right;
    else if (!right)
	next = left;
    else
	next = get_first(tree, right);

    if (parent) {
	is_left = (parent->left == P2I(node));
	set_child(next, parent, is_left);
    } else
	tree->root = P2I(next);

    if (left && right) {
	set_balance(next, get_balance(node));
	next->left = P2I(left);
	set_parent(next, left);
	if (next != right) {
	    parent = get_parent(tree, next);
	    set_parent(get_parent(tree, node), next);
	    node = I2P(tree, next->right);
	    parent->left = P2I(node);
	    next->right = P2I(right);
	    set_parent(next, right);
	    is_left = TRUE;
	} else {
	    set_parent(parent, next);
	    parent = next;
	    node = I2P(tree, parent->right);
	    is_left = FALSE;
	}
	assert(parent != NULL);
    } else
	node = next;

    if (node)
	set_parent(parent, node);

    while (parent) {

	int balance;
	    
	node = parent;
	parent = get_parent(tree, parent);
	if (is_left) {
	    is_left = (parent && (parent->left == P2I(node)));
	    balance = inc_balance(node);
	    if (balance == 0)		/* case 1 */
		continue;
	    if (balance == 1) {		/* case 2 */
		goto END_OF_DELETE;
	    }
	    right = I2P(tree, node->right);		/* case 3 */
	    switch (get_balance(right)) {
	    case 0:				/* case 3.1 */
		set_balance(node, 1);
		set_balance(right, -1);
		rotate_left(node, tree);
		goto END_OF_DELETE;
	    case 1:				/* case 3.2 */
		set_balance(node, 0);
		set_balance(right, 0);
		break;
	    case -1:			/* case 3.3 */
		switch (get_balance(I2P(tree, right->left))) {
		case 1:
		    set_balance(node, -1);
		    set_balance(right, 0);
		    break;
		case 0:
		    set_balance(node, 0);
		    set_balance(right, 0);
		    break;
		case -1:
		    set_balance(node, 0);
		    set_balance(right, 1);
		    break;
		}
		set_balance(I2P(tree, right->left), 0);
		rotate_right(right, tree);
	    }
	    rotate_left(node, tree);
	} else {
	    is_left = (parent && (parent->left == P2I(node)));
	    balance = dec_balance(node);
	    if (balance == 0)
		continue;
	    if (balance == -1) {
		goto END_OF_DELETE;
	    }
	    left = I2P(tree, node->left);
	    switch (get_balance(left)) {
	    case 0:
		set_balance(node, -1);
		set_balance(left, 1);
		rotate_right(node, tree);
		goto END_OF_DELETE;
	    case -1:
		set_balance(node, 0);
		set_balance(left, 0);
		break;
	    case 1:
		switch (get_balance(I2P(tree, left->right))) {
		case 1:
		    set_balance(node, 0);
		    set_balance(left, -1);
		    break;
		case 0:
		    set_balance(node, 0);
		    set_balance(left, 0);
		    break;
		case -1:
		    set_balance(node, 1);
		    set_balance(left, 0);
		    break;
		}
		set_balance(I2P(tree, left->right), 0);
		rotate_left(left, tree);
	    }
	    rotate_right(node, tree);
	}
    }

END_OF_DELETE:
		
    free_shm_avl_node(tree, to_be_deleted);
    cond_write_unlock(lock_it, &tree->lock);
}

/***************************************************************
****************************************************************
**
** @@@@@
**
** Common to entire module
**
****************************************************************
***************************************************************/

DEFINE_DEBUG_VARIABLE(shm_generic_debug);

/*
** Creates and maps the required shared memory into
** the process address space.  Returns the base address
** of the mapped memory.
*/
byte *map_shared_memory (char *shared_memory_name, int size, 
    bool create)
{
    int flags;
    int fd;
    unsigned char *base = NULL;

    /* create and/or map the memory segment */
    flags = O_RDWR;
    if (create) {
	flags |= O_CREAT;
    }
    fd = shm_open(shared_memory_name, flags,
	(S_IRWXU | S_IRWXG | S_IRWXO));
    if (fd < 0) {
	FATAL_ERROR("open for %s failed: %s",
	    shared_memory_name, ERR);
    }

    /* set its size correctly */
    if (ftruncate(fd, size) != 0) {
	FATAL_ERROR("ftruncate for %s to size %d failed: %s",
	    shared_memory_name, size, ERR);
    }

    /* now map it to process space */
    base = mmap(NULL, size,
	(PROT_READ | PROT_WRITE | PROT_EXEC),
	MAP_SHARED,
	fd, 0);
    if (base == MAP_FAILED) {
	FATAL_ERROR("mmap for %s failed: %s",
	    shared_memory_name, ERR);
    }

    //PRINT_DEBUG(shm_generic_debug, 
	//"memory for %s mapped to 0x%x total %d bytes", 
	//shared_memory_name, base, size);

    return (byte*) base;
}

