
/******************************************************************************
 *
 * This is a chunk manager which manages a many same sized chunks, 
 * dynamically allocated & freed.  It is like malloc & free but much
 * faster since most chunks are pre-allocated and disbursing them
 * is therefore quite fast.
 *
 * There is one VERY important requirement of this implementation.
 * It is the fact that when this dynamically grows, the addresses of
 * all previously allocated chunks MUST stay the same.  Therefore,
 * a simple realloc scheme could not be used since at some point,
 * a realloc may change the adresses completely, violating this
 * principle.  The reason this is required is that if the user
 * caches the pointer values of these chunks, they must never change.
 *
 * To be able to provide the above, the concept of a 'chunk group' is
 * defined where all the addresses in that group always stay the same.
 * In each group, a big block of malloced area is divided into many chunks
 * and are maintained.  If chunks are all used up and more is needed,
 * we cannot simply realloc the block and expand it.  This is because
 * the realloc may change the addresses and this would not work.  To
 * provide this expansion requirement, an expanding array of chunk groups
 * are therefore maintained.  When all the chunks in a group is exhausted,
 * a new group is created.
 *
 * This concept of multiple grouping unfortunately means that the library
 * sometimes will fall into a situation where it has to search thru the
 * list of groups in which there is at least one chunk available.  This
 * is not ideal but the only way in which the service can be implemented.
 * By maintaining a group pointer to the group with at least one free
 * chunk sometimes eliminates the search when it is possible to do so.
 * This group pointer is 'has_free_chunks' shown below.
 *
 * The worst situation occurs when a group's last chunk is allocated.
 * In this situation, in the next request for a chunk, the entire
 * set of chunks must be searched to find the group in which the number
 * of chunks are maximum and the 'has_free_chunks' pointer must be set
 * to that group.  This is when 'n_free' of the group pointed to by
 * 'has_free_chunks' is zero and 'n_chunk_manager_free' is still positive.
 * This means that even though the current group's chunks are exhausted,
 * there are still other groups in the manager in which there are free
 * chunks available.  This is when the search needs to be made.
 *
 * If however at any time 'n_chunk_manager_free' is also zero, then a
 * totally new group must be allocated, chunkified and a chunk returned
 * from there.
 */

typedef struct chunk_header_s chunk_header_t;
typedef struct chunk_group_s chunk_group_t;
typedef struct chunk_manager_s chunk_manager_t;

struct chunk_header_s {

    /*
     * The chunk group I belong to so that when I am freed up,
     * I will return to the same group.
     */
    chunk_group_t *my_group;

    /* When I am in the free or used list, my neighbor chunk */
    chunk_header_t *next;

    /*
     * this is what is returned back to the user and it must
     * always be 8 bytes aligned, hence the use of long long int.
     */
    long long int data [0];
}

struct chunk_group_s {

    /* The chunk manager I belong to */
    chunk_manager_t *my_manager;

    /* big bulk of the memory, all chunks adjacent in one big block */
    void *chunks_block;

    /* how many free chunks & their list */
    int n_free;
    chunk_header_t *free_chunks;

};

struct chunk_manager_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;
    STATISTICS_VARIABLES;

    /* size of each chunk the user wants and how much it internally takes*/
    int chunk_size;
    int actual_chunk_size;

    /* how many chunks per group is needed */
    int chunks_per_group;

    /* total number of groups so far in the manager */
    int n_groups;

    /*
     * absolute total of free chunks in all the groups.  If this
     * is zero, it is time to create another group & add it to the
     * manager.
     */
    int n_chunk_manager_free;

    /* a linked list of all the groups */
    list_object_t chunk_groups_list;

    /*
     * kind of a 'cache' pointer which points to a group in which
     * there is always at least one free chunk.  This is so that
     * we do not have to search in all the groups to find the first
     * group with an available free chunk.
     */
    chunk_group_t *current_chunk_group;

} chunk_manager_t;

/* reasonable chunk size should be in between these numbers */
#define MIN_CHUNK_SIZE          8
#define MAX_CHUNK_SIZE          256

/*
 * reasonable number of chunks per group
 * should be in between these numbers.
 */
#define MIN_CHUNKS_PER_GROUP    64
#define MAX_CHUNKS_PER_GROUP    1024

static int
chunk_manager_add_group (chunk_manager_t *cmgrp)
{
    chunk_group_t *cgp;

    cgp = MEM_MONITOR_ALLOC(cmgrp, sizeof(chunk_group_t));
    if (NULL == cgp) return ENOMEM;
    cgp->chunks_block = MEM_MONITOR_ALLOC(cmgrp,
            (cmgrp->actual_chunk_size * cmgrp->chunks_per_group));
    if (NULL == cgp->chunks_block) {
        MEM_MONITOR_FREE(cgp);
        return ENOMEM;
    }

    /* add the new group to the list of chunk groups */
    err = list_object_insert_head(&cmgrp->chunk_groups_list, cgp, null);
    if (err) {
        MEM_MONITOR_FREE(cgp->chunks_block);
        MEM_MONITOR_FREE(cgp);
        return err;
    }

    /* now update everything everything */

    cgp->my_manager = cmgrp;
    cgp->n_free = cmgrp->chunks_per_group;
    cgp->free_chunks = (chunk_header_t*) cgp->chunks_block;

    cmgrp->n_chunk_manager_free += cmgrp->chunks_per_group;
    cmgrp->n_groups += 1;

    /* link the chunks to each other */
    for (i = 0; i < cmgrp->chunks_per_group; i++) {
    }

    /* the new free chunk cache pointer becomes this group */
    cmgrp->current_chunk_group = cgp;

    return 0;
}

PUBLIC int
chunk_manager_init (chunk_manager_t *cmgrp,
    boolean make_it_thread_safe,
    boolean statistics_wanted,
    int chunk_size, int chunks_per_group,
    mem_monitor_t *parent_mem_monitor)
{
    int err;

    /* basic sanity checks */
    if ((chunk_size < MIN_CHUNK_SIZE) ||
        (chunk_size > MAX_CHUNK_SIZE)) {
            return EINVAL;
    }
    if ((chunks_per_group < MIN_CHUNKS_PER_GROUP) ||
        (chunks_per_group > MAX_CHUNKS_PER_GROUP)) {
            return EINVAL;
    }

    /* clear absolutely everything */
    memset(cmgrp, 0, sizeof(chunk_manager_t));

    MEM_MONITOR_SETUP(cmgrp);
    LOCK_SETUP(cmgrp);
    STATISTICS_SETUP(cmgrp);

    cmgrp->chunk_size = chunk_size;
    cmgrp->actual_chunk_size = ((chunk_size + 7) & ~7) sizeof(chunk_header_t);
    cmgrp->chunks_per_group = chunks_per_group;

    err = list_object_init(&cmgrp->chunk_groups_list, false, false, null,
        0, cmgrp->mem_mon_p);

    OBJ_WRITE_UNLOCK(cmgrp);

    return err;
}

static chunk_group_t *
find_group_with_most_free_chunks (chunk_manager_t *cmgrp)
{
    int n = 0;
    list_node_t *node;
    chunk_group_t *grp, *highest_grp;

    /* there are absolutely no groups available in this cmgr */
    if (cmgrp->chunk_groups_list.n <= 0)
        return NULL;

    /* iterate thru finding the group with the highest number of free chunks */
    node = cmgrp->chunk_groups_list.head;
    grp = highest_grp = (chunk_group_t*) node->data;
    while (n < cmgrp->chunk_groups_list.n) {
        if (grp->n_free > highest_grp->n_free) {
            highest_grp = grp;
        }
        n++;
        node = node->next;
        grp = (chunk_group_t*) node->data;
    }
    return highest_grp;
}

/*
 * Be careful with this.  Here is the subtle logic:
 *
 * If there are free chunks in the chunk manager, AND the current
 * group has at least one free chunk, then return that.  If however,
 * the current group is exhausted (but there are still free chunks in
 * other groups), search for the group with the highest number of
 * chunks, make it the current group and recursively call the alloc
 * function again.  This will return a chunk from the newly assigned
 * current group.
 *
 * If however, there was absolutely no chunks left anywhere in the
 * manager, then create a new group and make the current group equal
 * to that and recursively call the alloc function again.
 *
 * If the creation of the new group failed, then there is nothing we
 * can do, there is no memory left, so return NULL.
 *
 */
static void *
thread_unsafe_chunk_alloc (chunk_manager_t *cmgrp)
{
    chunk_header_t *ret;
    chunk_group_t *grp;

    if (cmgrp->n_chunk_manager_free > 0) {
        grp = cmgrp->current_chunk_group;
        if (grp->n_free > 0) {
            ret = grp->free_chunks;
            grp->free_chunks = grp->free_chunks->next;
            grp->n_free--;
            cmgrp->n_chunk_manager_free--;
            return ret;
        }
        cmgrp->current_chunk_group = find_group_with_most_free_chunks(cmgrp);
        return thread_unsafe_chunk_alloc(cmgrp);
    }

    if (chunk_manager_add_group(cmgrp)) return NULL;
    return thread_unsafe_chunk_alloc(cmgrp);
}

void
chunk_free (chunk)
{
    return the chunk to the group it belongs, increment free counters.
    If the current group's free count is now > current's free count,
    make the current group the new group.
}

void
chunk_manager_trim (chunk_manager_t *cmgrp)
{
    For all the groups in the chunk manager
        delete/free up all groups whose chunks are completely free
            ie, none of the chunks are used.
    end
}

