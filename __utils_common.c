
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol (gee.akyol@gmail.com, gee_akyol@yahoo.com)
** Copyright: Cihangir Metin Akyol, March 2016
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

#include "utils_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STRING_SIZE	64

/*
** FOR LINUX ONLY: POSIX COMPLIANT
**
** returns whether a process is actually running
** or dead already.  This routine uses the fact that
** in Linux, every process has a directory entry
** as "/proc/pidnumber".  If it does not exist,
** or it is not a directory, it means process is 
** dead.
*/
PUBLIC boolean 
process_is_dead (pid_t pid)
{
    char pid_string [64];
    struct stat st;

    sprintf(pid_string, "/proc/%d", pid);
    if (stat(pid_string, &st) == 0) {
	if (!S_ISDIR(st.st_mode)) {
	    return TRUE;
	}
	return FALSE;
    } else {
	return TRUE;
    }
}

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Time measurement functions
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#define NANOSEC_TO_SEC      ((int64) 1000000000)

void report_timer (timer_obj_t *tp, int64 iterations)
{
    int64 start_ns, end_ns, elapsed_ns;
    double elapsed_s, per_iter;

    start_ns = (tp->start.tv_sec * NANOSEC_TO_SEC) + tp->start.tv_nsec;
    end_ns = (tp->end.tv_sec * NANOSEC_TO_SEC) + tp->end.tv_nsec;
    elapsed_ns = end_ns - start_ns;
    elapsed_s = ((double) elapsed_ns) / ((double) (NANOSEC_TO_SEC));
    printf("elapsed time: %.9lf seconds (%llu nsecs) for %llu iterations\n",
	elapsed_s, elapsed_ns, iterations);
    per_iter = ((double) elapsed_ns / (double) iterations);
    printf("took %.3lf nano seconds per operation\n", per_iter);
}

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Some standard comparison functions
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

int 
compare_ints (datum_t d1, datum_t d2)
{ return d1.integer - d2.integer; }

int
compare_pointers (datum_t d1, datum_t d2)
{ return d1.pointer - d2.pointer; }

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ datum_list_t functions
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

/*
 * This is an object which is a list that continuously grows by
 * re-allocing itself.  It is typically used during tree traversals
 * to collect matching nodes into a linear table so that such nodes
 * can simply be iterated over later in the array.
 * Total number of datums at any one time in the array is 'n' and
 * are guaranteed to be consecutive (no holes in the array).
 *
 * Note that this object is intended to be used only as described,
 * for tree traversals.  Do NOT try to be clever.
 */
datum_list_t *
datum_list_allocate (int initial_size)
{
    datum_list_t *dlp;

    dlp = malloc(sizeof(datum_list_t));
    if (NULL == dlp) return NULL;
    dlp->datums = malloc(initial_size * sizeof(datum_t));
    if (NULL == dlp->datums) {
        free(dlp);
        return NULL;
    }
    dlp->max = initial_size;
    dlp->n = 0;

    return dlp;
}

error_t
datum_list_add (datum_list_t *dlp, datum_t data)
{
    int new_max;
    datum_t *new_datums;

    if (dlp->n < dlp->max) {
        dlp->datums[dlp->n] = data;
        dlp->n++;
        return 0;
    }
    new_max = dlp->max * 1.5;
    new_datums = realloc(dlp->datums, (new_max * sizeof(datum_t)));
    if (NULL == new_datums)
        return ENOMEM;
    dlp->max = new_max;
    dlp->datums = new_datums;
    return
        datum_list_add(dlp, data);
}

void
datum_list_free (datum_list_t *dlp)
{
    free(dlp->datums);
    free(dlp);
}

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ Memory accounting object
**       Keeps a very accurate measurement of how much dynamic
**       memory has been used, how many times the alloc has been
**       invoked and how many times the free has been invoked.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

void
mem_monitor_init (mem_monitor_t *memp)
{
    memp->bytes_used = 0;
    memp->allocations = 0;
    memp->frees = 0;
}

/*
 * An extra 8 bytes is allocated at the front to store the length.
 * Make sure this is 8 bytes aligned or it crashes in 64 bit systems.
 */

void *
mem_monitor_allocate (mem_monitor_t *memp, int size)
{
    int total_size = size + sizeof(int64);
    int64 *block = (int64*) malloc(total_size);

    if (block) {
        memset(block, 0, total_size);
	*block = total_size;
	memp->bytes_used += total_size;
	memp->allocations++;
	block++;
	return (void*) block;
    }
    return NULL;
}

void
mem_monitor_free (mem_monitor_t *memp, void *ptr)
{
    int64 *block = (int64*) ptr;

    block--;
    memp->bytes_used -= *block;
    memp->frees++;
    free(block);
}

void *
mem_monitor_reallocate (mem_monitor_t *memp, void *ptr, int newsize)
{
    int oldsize;
    int64 *block = (int64*) ptr;
    int64 *new_block;

    if (NULL == ptr)
	return 
            mem_monitor_allocate(memp, newsize);
    block--;
    oldsize = *block;
    newsize += sizeof(int64);
    new_block = (int64*) realloc((void*) block, newsize);
    if (new_block) {
        *new_block = newsize;
        memp->bytes_used -= oldsize;
        memp->bytes_used += newsize;
        new_block++;
        memp->allocations++;
        memp->frees++;
        return (void*) new_block;
    }
    return NULL;
}

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
** 
** @@@@@ file/socket operations
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

/*
 * maximum tolarable times a read or a write call can consecutively fail
 */
#define MAX_CONSECUTIVE_FAILURES_ALLOWED        64

/*
 * makes sure it reads/writes all the requested size.  It will not
 * give up until it completes every single byte, unless too many 
 * consecutive number of read/write errors have occured.
 *
 * If reading & writing into a pipe or a socket, 'can_block' should
 * be specified as 'true'.
 */
static error_t 
relentless_read_write (boolean perform_read,
        int fd, void *buffer, int size, boolean can_block)
{
    int total, rc, failed;

    total = failed = 0;
    while (size > 0) {

        // read/write maximum requested amount if it can
        //
        if (perform_read) {
            rc = read(fd, &((char*) buffer)[total], size);
        } else {
            rc = write(fd, &((char*) buffer)[total], size);
        }

        // process the read/written amount
        //
	if (rc > 0) {
	    total += rc;
	    size -= rc;
            failed = 0;
	    continue;
	}

        // operation failed; has it failed consecutively many times ?
        //
        if (++failed >= MAX_CONSECUTIVE_FAILURES_ALLOWED) {
            return error;
        }

        // these errors may occur rarely and should be recovered from,
        // unless they keep on happening consecutively
        //
        if ((can_block && (EWOULDBLOCK == errno)) ||
            (EINTR == errno) || (EAGAIN == errno)) {
                continue;
        }

        // an unacceptable error occured, cannot continue
        //
        return error;
    }

    // everything finished ok
    return ok;
}

PUBLIC error_t
read_exact_size (int fd, void *buffer, int size, boolean can_block)
{
    return
        relentless_read_write(true, fd, buffer, size, can_block);
}

PUBLIC error_t
write_exact_size (int fd, void *buffer, int size, boolean can_block)
{
    return
        relentless_read_write(false, fd, buffer, size, can_block);
}

PUBLIC error_t
filecopy (char *in_file, char *out_file)
{
    error_t rv = ok;
    int in_fd = open(in_file, O_RDONLY);
    int out_fd = open(out_file, (O_WRONLY | O_CREAT));
    char *buf = malloc(8192);
    ssize_t result;

    // basic sanity checks
    //
    if ((in_fd < 0) || (out_fd < 0) || (NULL == buf)) {
        rv = error;
        goto filecopy_finished;
    }

    // read from one file, write to the other
    //
    while (1) {

        // read from input file
        //
        result = read(in_fd, &buf[0], sizeof(buf));

        // everything read, finished
        //
        if (0 == result) break;

        // a read error occured, can we recover ?
        //
        if (result < 0) {
            if ((errno == EINTR) || (errno == EAGAIN)) continue;
            rv = error;
            goto filecopy_finished;
        }

        // an unrecoverable write error occured
        //
        if (write_exact_size(out_fd, &buf[0], result, false) != ok) {
            rv = error;
            goto filecopy_finished;
        }
    }

filecopy_finished:

    if (buf) free(buf);
    fsync(out_fd);
    close(in_fd);
    close(out_fd);

    return rv;
}

#ifdef __cplusplus
} // extern C
#endif 


