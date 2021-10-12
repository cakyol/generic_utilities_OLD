
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
** doubly linked list
**
*******************************************************************************
*******************************************************************************
*******************************************************************************
*******************************************************************************
******************************************************************************/

#include "dl_list_object.h"

#ifdef __cplusplus
extern "C" {
#endif

static dl_list_element_t *
dl_list_new_element (dl_list_t *list, void *object)
{
    dl_list_element_t *elem;

    elem = MEM_MONITOR_ZALLOC(list, sizeof(dl_list_element_t));
    if (0 == elem) return 0;
    elem->next = elem->prev = 0;
    elem->object = object;
    return elem;
}

/*
 * adds it to the head of the list.
 */
static void
thread_unsafe_dl_list_prepend_element (dl_list_t *list,
        dl_list_element_t *elem)
{
    if (list->n <= 0) {
        list->head = list->tail = elem;
    } else {
        elem->next = list->head;
        list->head->prev = elem;
        list->head = elem;
    }
    list->n++;
}

/*
 * adds it to the end of the list.
 */
static void
thread_unsafe_dl_list_append_element (dl_list_t *list,
        dl_list_element_t *elem)
{
    if (list->n <= 0) {
        list->head = list->tail = elem;
    } else {
        elem->prev = list->tail;
        list->tail->next = elem;
        list->tail = elem;
    }
    list->n++;
}

static int
thread_unsafe_dl_list_delete_element (dl_list_t *list,
        dl_list_element_t *elem)
{
    if (list->should_not_be_modified)
        return EBUSY;

    if (elem->next == 0) {
        if (elem->prev == 0) {
            list->head = list->tail = 0;
        } else {
            elem->prev->next = 0;
            list->tail = elem->prev;
        }
    } else {
        if (elem->prev == 0) {
            list->head = elem->next;
            elem->next->prev = 0;
        } else {
            elem->prev->next = elem->next;
            elem->next->prev = elem->prev;
        }
    }
    MEM_MONITOR_FREE(elem);
    list->n--;
    return 0;
}

static int 
thread_unsafe_dl_list_prepend_object (dl_list_t *list, void *object)
{
    dl_list_element_t *elem;
    
    if (list->should_not_be_modified)
        return EBUSY;

    elem = dl_list_new_element(list, object);
    if (elem) {
        thread_unsafe_dl_list_prepend_element(list, elem);
        return 0;
    }
    return ENOMEM;
}

static int
thread_unsafe_dl_list_append_object (dl_list_t *list, void *object)
{
    dl_list_element_t *elem;
    
    if (list->should_not_be_modified)
        return EBUSY;

    elem = dl_list_new_element(list, object);
    if (elem) {
        thread_unsafe_dl_list_append_element(list, elem);
        return 0;
    }
    return ENOMEM;
}

/************************** Public functions **************************/

int
dl_list_init (dl_list_t *list,
        boolean make_it_thread_safe,
        boolean statistics_wanted,
        mem_monitor_t *parent_mem_monitor)
{
    MEM_MONITOR_SETUP(list);
    LOCK_SETUP(list);
    STATISTICS_SETUP(list);

    list->head = list->tail = 0;
    list->n = 0;
    OBJ_WRITE_UNLOCK(list);
    return 0;
}

int
dl_list_prepend_object (dl_list_t *list, void *object)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    failed = thread_unsafe_dl_list_prepend_object(list, object);
    OBJ_WRITE_UNLOCK(list);
    return failed;
}

int
dl_list_append_object (dl_list_t *list, void *object)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    failed = thread_unsafe_dl_list_append_object(list, object);
    OBJ_WRITE_UNLOCK(list);
    return failed;
}

int
dl_list_delete_element (dl_list_t *list, dl_list_element_t *elem)
{
    int failed;

    OBJ_WRITE_LOCK(list);
    failed = thread_unsafe_dl_list_delete_element(list, elem);
    OBJ_WRITE_UNLOCK(list);
    return failed;
}

/*
 * Note that by incrementing & decrementing 'should_not_be_modified' we
 * allow traversing to be done within traversing.  As long as the list
 * is not modified (which 'should_not_be_modified' ensures), it IS
 * possible for multiple threads to traverse and even recursive traversals
 * within the same thread is possible.
 */
void
dl_list_traverse (dl_list_t *list,
    traverse_function_pointer tfn,
    void *p0, void *p1, void *p2, void *p3)
{
    dl_list_element_t *iterator;

    OBJ_READ_LOCK(list);
    list->should_not_be_modified++;
    iterator = list->head;
    while (iterator) {
        if (tfn(list, iterator, iterator->object, p0, p1, p2, p3)) {
            break;
        }
        iterator = iterator->next;
    }
    list->should_not_be_modified--;
    OBJ_READ_UNLOCK(list);
}

void
dl_list_destroy (dl_list_t *list)
{
    OBJ_WRITE_LOCK(list);
    while (list->head)
        thread_unsafe_dl_list_delete_element(list, list->head);
    assert(list->n == 0);
    assert(list->head == NULL);
    assert(list->tail == NULL);
    OBJ_WRITE_UNLOCK(list);
    lock_obj_destroy(list->lock);
}

#ifdef __cplusplus
} // extern C
#endif 




