
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

    elem = MEM_MONITOR_ALLOC(list, sizeof(dl_list_element_t));
    if (NULL == elem) return NULL;
    elem->next = elem->prev = NULL;
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

static void
thread_unsafe_dl_list_delete_element (dl_list_t *list,
        dl_list_element_t *elem)
{
    if (elem->next == NULL) {
        if (elem->prev == NULL) {
            list->head = list->tail = NULL;
        } else {
            elem->prev->next = NULL;
            list->tail = elem->prev;
        }
    } else {
        if (elem->prev == NULL) {
            list->head = elem->next;
            elem->next->prev = NULL;
        } else {
            elem->prev->next = elem->next;
            elem->next->prev = elem->prev;
        }
    }
    MEM_MONITOR_FREE(list, elem);
    list->n--;
}

/*
 * if an equality function for the list is defined, we use that
 * to determine whether the two user objects are considered to
 * be the same.  Otherwise we test only whether the actual object
 * pointers are the same.
 */
static inline int
objects_are_equal (dl_list_t *list, void *obj1, void *obj2)
{
    if (list->objects_match_function) {
        return
            list->objects_match_function(obj1, obj2);
    }
    return
        (obj1 == obj2);
}

static int
thread_unsafe_dl_list_find_element (dl_list_t *list, void *object,
        dl_list_element_t **found_element)
{
    dl_list_element_t *elem;

    *found_element = NULL;
    elem = list->head;
    while (elem) {
        if (objects_are_equal(list, object, elem->object)) {
            *found_element = elem;
            return 0;
        }
        elem = elem->next;
    }
    return ENODATA;
}

static int 
thread_unsafe_dl_list_prepend_object (dl_list_t *list, void *object)
{
    int err = ENOMEM;
    dl_list_element_t *elem;
    
    elem = dl_list_new_element(list, object);
    if (elem) {
        thread_unsafe_dl_list_prepend_element(list, elem);
        err = 0;
    }
    return err;
}

static int
thread_unsafe_dl_list_append_object (dl_list_t *list, void *object)
{
    int err = ENOMEM;
    dl_list_element_t *elem;
    
    elem = dl_list_new_element(list, object);
    if (elem) {
        thread_unsafe_dl_list_append_element(list, elem);
        err = 0;
    }
    return err;
}

/***************************** Non static functions *****************************/

int
dl_list_init (dl_list_t *list,
        int make_it_thread_safe,
        object_comparer objects_match_function,
        mem_monitor_t *parent_mem_monitor)
{
    MEM_MONITOR_SETUP(list);
    LOCK_SETUP(list);

    list->head = list->tail = NULL;
    list->n = 0;
    list->objects_match_function = objects_match_function;

    WRITE_UNLOCK(list);

    return 0;
}

int
dl_list_prepend_object (dl_list_t *list, void *object)
{
    int failed;

    WRITE_LOCK(list);
    failed = thread_unsafe_dl_list_prepend_object(list, object);
    WRITE_UNLOCK(list);
    return failed;
}

int
dl_list_append_object (dl_list_t *list, void *object)
{
    int failed;

    WRITE_LOCK(list);
    failed = thread_unsafe_dl_list_append_object(list, object);
    WRITE_UNLOCK(list);
    return failed;
}

int
dl_list_find_object (dl_list_t *list, void *object,
        dl_list_element_t **found_element)
{
    int err;

    READ_LOCK(list);
    err = thread_unsafe_dl_list_find_element(list, object, found_element);
    WRITE_UNLOCK(list);
    return err;
}

void
dl_list_iterate (dl_list_t *list, object_comparer iterator,
        void *extra_arg, int stop_if_fails)
{
    dl_list_element_t *elem;
        
    READ_LOCK(list);
    elem = list->head;
    while (elem) {
        if (iterator(elem->object, extra_arg) && stop_if_fails) break;
        elem = elem->next;
    }
    READ_UNLOCK(list);
}

int
dl_list_delete_object (dl_list_t *list, void *object)
{
    int failed = ENODATA;
    dl_list_element_t *elem;

    WRITE_LOCK(list);
    if (thread_unsafe_dl_list_find_element(list, object, &elem) == 0) {
        thread_unsafe_dl_list_delete_element(list, elem);
        failed = 0;
    }
    WRITE_UNLOCK(list);
    return failed;
}

void
dl_list_destroy (dl_list_t *list)
{
    WRITE_LOCK(list);
    while (list->head) thread_unsafe_dl_list_delete_element(list, list->head);
    assert(list->n == 0);
    assert(list->head == NULL);
    assert(list->tail == NULL);
    WRITE_UNLOCK(list);
    lock_obj_destroy(list->lock);
}

#ifdef __cplusplus
} // extern C
#endif 




