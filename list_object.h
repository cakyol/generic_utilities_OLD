
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
** It ALWAYS is and WILL remain the sole property of Cihangir Metin Akyol.
**
** For proper indentation/viewing, regardless of which editor is being used,
** no tabs are used, ONLY spaces are used and the width of lines never
** exceed 80 characters.  This way, every text editor/terminal should
** display the code properly.  If modifying, please stick to this
** convention.
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
** This is a very simple linked list which can best be used as a fifo
** or a lifo.  What makes it extremely fast is the fact that you can
** add a data node only either to its head or its tail, which makes it
** very suitable to be used as either a fifo or a lifo.
**
** If you want to use it as a fifo, always add to its tail.
** If you want to use it as a lifo, always add to its head.  Then when you
** pop each element, you will always pop the nodes in the correct order.
** Popping the element will actually remove it from the list.
**
** Insertions & deletions are extremely fast, since the list always
** maintains a 'tail' pointer which always points to an 'empty' node
** which can be manipulated in deletions very quickly.
**
** The list also can be searched and random nodes can be deleted.
** Searches are linear but deletions are extremely fast.
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#ifndef __LIST_OBJECT_H__
#define __LIST_OBJECT_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "common.h"
#include "mem_monitor_object.h"
#include "lock_object.h"

typedef struct list_node_s list_node_t;
typedef struct list_object_s list_object_t;

/*
 * end of list is typically signified with a node which has
 * both 'data' and 'next' to be NULL *AND* this special marker
 * node is ALWAYS pointed to by the 'tail' pointer of the list.
 */
struct list_node_s {

    /* user data */
    void *data;

    /* next node */
    list_node_t *next;
};

typedef struct list_object_s {

    MEM_MON_VARIABLES;
    LOCK_VARIABLES;

    /* how many elements are currently in the list */
    int n;

    /* start of list, and the end node of list */
    list_node_t *head, *tail;

    /* to search a certain object in the list; May be NULL */
    object_comparer cmp;

    statistics_block_t stats;

} list_object_t;

/*
 * initialize a list object.  The comparison function is used to
 * search the list to see if matching data is found.  It is
 * supplied by the user.  The comparison is just performed by the
 * list object & checks equality.  It should return 0 for equal
 * data or non zero otherwise.
 *
 * If the comparison function is NULL, the list object compares
 * just the pointers and decides based only on that.
 */
extern int
list_object_init (list_object_t *list,
    boolean make_it_thread_safe,
    object_comparer cmpf,
    mem_monitor_t *parent_mem_monitor);

/*
 * Insert the user data to the head of list,
 * typically to be used as a lifo.
 */
extern int
list_object_insert_head (list_object_t *list, void *data);

/*
 * Insert the user data to the end of list,
 * typically to be used as a fifo.
 */
extern int
list_object_insert_tail (list_object_t *list, void *data);

/*
 * Returns true if the user data is already in the list.
 * This uses the comparison function specified at the list
 * initialization.  It is a linear search and is therefore
 * likely to be slow.
 * /
extern boolean
list_object_contains (list_object_t *list, void *searched);

static inline int
list_object_size (list_object_t *list)
{
    return list->n;
}

/*
 * searches & then removes the data from the list.  Returns 0
 * if data was indeed in the list.  Returns error if not.
 */
extern int
list_object_remove_data (list_object_t *list, void *data);

/*
 * Always pops the user data from the head of list.  Depending
 * on how the user data was inserted into the list, this can be
 * useful as a fifo or a lifo.
 */
extern void *
list_object_pop_data (list_object_t *list);

/*
 * Completely destroys the list.  Note that the actual user data items
 * in the list are not touched.  The list becomes un-usable and needs
 * to be re-initialized for further use.
 */
extern void
list_object_destroy (list_object_t *list);

#ifdef __cplusplus
} // extern C
#endif

#endif // __LIST_OBJECT_H__

