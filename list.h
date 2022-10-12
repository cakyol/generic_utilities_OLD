
/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Author: Cihangir Metin Akyol, gee.akyol@gmail.com, gee_akyol@yahoo.com
** Copyright: Cihangir Metin Akyol, April 2014 -> ....
**
** All this code has been personally developed by and belongs to 
** Mr. Cihangir Metin Akyol.  It has been developed in his own 
** personal time using his own personal resources.  Therefore,
** it is NOT owned by any establishment, group, company or 
** consortium.  It is the sole property and work of the named
** individual.
**
** It CAN be used by ANYONE or ANY company for ANY purpose as long 
** as ownership and/or patent claims are NOT made to it by ANYONE
** or ANY ENTITY.
**
** It ALWAYS is and WILL remain the property of Cihangir Metin Akyol.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

/******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
**
** Doubly linked list container object.
**
** This can be used as a lifo, fifo, queue, stk... basically
** any data structure which needs some kind of linked list.
** Since it is doubly linked, it is extremely fast to delete a
** node from it.
**
** Note that this list can also be used as an ordered list, where the
** ordering is maintained by the user specified function named 'cmp'
** which is specified at the list initialization time.  By default,
** if this function is NOT null, the list is considered ordered.  If
** the list is ordered, ALL insert functions (regardless of the function
** names) will follow the order.
**
** IMPORTANT 1:
** The only criteria used to determine whether a list is ordered or not
** is fixed at the initialization time by checking the value of
** the 'cmp' function.  If specied as NULL, the list will be marked as
** UN-ordered, otherwise ordered.
**
** IMPORTANT 2:
** The ordering can be head to tail (forward) or tail to head (backward).
** This will be determined by the comparison function supplied by the user.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

typedef struct list_node_s list_node_t;
typedef struct list_s list_t;

struct list_node_s {

    list_node_t *next, *prev;
    void *data;
};

struct list_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;
    STATISTICS_VARIABLES;

    list_node_t *head, *tail;

    /*
     * used to order the list and/or search the list
     * for a specific data which matches.  Supplied
     * by the user at init time.  CAN be null.
     */
    object_comparer cmp;

    /* how many nodes are in the list currently */
    int n;

    /* if > 0, specifies the max number of allowed nodes */
    int n_max;

};

/******************************************************************************
 * Initialize the list.  Return value is 0 for success
 * or a non zero errno.  
 *
 * If 'n_max' > 0, then the number of nodes in the list is limited
 * to that number.  Further insertions will not be allowed.  If
 * 'n_max' <= 0, there is no limit except the memory for insertions.
 *
 * The object comparer pointer is a function
 * pointer used for comparing two 'data' objects to determine
 * their equality.  It should return a value similar to the strcmp
 * function, based on how you want to order your data.
 * If 'cmp' is NOT NULL, then only ordered insertions will be
 * performed, regardless of which insert function is called.
 *
 * The ordering (increasing in value or decreasing in value) can
 * be controlled by how you specify the ordering function.
 */
extern int
list_init (list_t *list,
    boolean make_it_thread_safe,
    boolean enable_statistics,
    int n_max,
    object_comparer cmp,
    mem_monitor_t *parent_mem_monitor);

/******************************************************************************
 * Adds user data based on the ordering of the list as specified by
 * the comparison function specified at the initialization time.
 *
 * If this function is called for a list in which the comparison
 * function has not been specified (NULL), then a non zero error
 * code (EPERM) will be returned and insertion will fail, list will
 * not change.
 *
 * Successful insertion return 0.
 */
extern int
list_insert_ordered (list_t *list, void *data);

/******************************************************************************
 * Add user data to the beginning of the list.
 * Return value is 0 for success or a non zero
 * errno value.
 *
 * *** IMPORTANT ***
 * This function will insert differently if the list is ordered.
 * It will NOT insert to head but into the position determined by
 * how the 'cmp' function behaves.  It effecively executes a call to
 * 'list_insert_ordered' shown above.
 *
 */
extern int 
list_prepend_data (list_t *list, void *data);

/******************************************************************************
 * Add user data to the end of the list.
 * Return value is 0 for success or a non zero
 * errno value.
 *
 * *** IMPORTANT ***
 * If the list is ordered, it will implicitly execute 'list_insert_ordered'.
 *
 */
extern int
list_append_data (list_t *list, void *data);

/******************************************************************************
 * Add user data after the node specified.  If the node specified
 * is not in the list specified, results will not be good.  The
 * function does NOT check this, it assumes it is called correctly.
 *
 * This function will fail if the list is ordered.
 *
 */
extern int
list_insert_data_after_node (list_t *list,
    list_node_t *node, void *data);

/******************************************************************************
 * Add user data before the node specified.  If the node specified
 * is not in the list specified, results will not be good.  The
 * function does NOT check this, it assumes it is called correctly.
 *
 * If the list is ordered, it will implicitly execute 'list_insert_ordered'.
 */
extern int
list_insert_data_before_node (list_t *list,
    list_node_t *node, void *data);

/******************************************************************************
 * Finds the data stored in the list and if found, returns
 * the node in which it is srtored.  If not found, it returns
 * null.
 */
extern list_node_t *
list_find_data_node (list_t *list, void *data);

/******************************************************************************
 * Delete a node in the list, used when you
 * already know the node to be removed.
 */
extern int
list_remove_node (list_t *list, list_node_t *node);

/******************************************************************************
 * Delete the user data from the list.  If a comparison function
 * to compare user data pointers was defined at the init time of the
 * list, then that is used to find the node/data which is then
 * removed.  If the function was not specified at the initialization
 * time, then just a simple pointer comparison is done.
 * Return value is 0 for success or a non zero errno value.
 */
extern int
list_remove_data (list_t *list, void *data);

/******************************************************************************
 * Destruction will be complete, list cannot be used until
 * it is re-initialized again properly.
 */
extern void
list_destroy (list_t *list);

/******************************************************************************
 * fifo implementation using the list
 */

typedef list_t fifo_t;

static inline int
fifo_init (fifo_t *fifo,
    bool make_it_thread_safe,
    bool enable_statistics,
    int n_max,
    mem_monitor_t *memp)
{
    return list_init((list_t*) fifo,
                make_it_thread_safe, enable_statistics,
                n_max, NULL, memp);
}

static inline int
fifo_queue (fifo_t *fifo, void *data)
{
    /* add to END of list */
    return list_append_data((list_t*) fifo, data);
}

static inline void *
fifo_peek (fifo_t *fifo)
{
    return ((list_t*) fifo)->head;
}

static inline void *
fifo_dequeue (fifo_t *fifo)
{
    void *data = ((list_t*) fifo)->head->data;

    list_remove_node((list_t*) fifo, ((list_t*) fifo)->head);
    return data;
}

static inline void
fifo_destroy (fifo_t *fifo)
{
    list_destroy((list_t*) fifo);
}
    
/******************************************************************************
 * stack implementation using the list
 */

typedef list_t stk_t;

static inline int
stk_init (stk_t *stk,
    bool make_it_thread_safe,
    bool enable_statistics,
    int n_max,
    mem_monitor_t *memp)
{
    return list_init((list_t*) stk,
                make_it_thread_safe, enable_statistics,
                n_max, NULL, memp);
}

static inline int
stk_insert (stk_t *stk, void *data)
{
    /* add to HEAD of list */
    return list_prepend_data((list_t*) stk, data);
}

static inline void *
stk_peek (stk_t *stk)
{
    return ((list_t*) stk)->head;
}

static inline void *
stk_remove (stk_t *stk)
{
    void *data = ((list_t*) stk)->head->data;

    list_remove_node((list_t*) stk, ((list_t*) stk)->head);
    return data;
}

static inline void
stk_destroy (stk_t *stk)
{
    list_destroy((list_t*) stk);
}

#ifdef __cplusplus
} // extern C
#endif 

#endif // __LIST_H__


