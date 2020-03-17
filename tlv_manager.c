
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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include "tlv_manager.h"

/*
 * does NOT handle overlapping regions
 */
static inline void
byte_copy (void *src, void *dst, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        ((byte*) dst)[i] = ((byte*) src)[i];
    }
}

void
tlvm_reset (tlvm_t *tlvmp)
{
    tlvmp->idx = 0;
    tlvmp->remaining_size = tlvmp->buf_size;
    tlvmp->n_tlvs = 0;
    tlvmp->parse_complete = 0;
    if (tlvmp->tlvs) free(tlvmp->tlvs);
    tlvmp->tlvs = NULL;
}

void
tlvm_init (tlvm_t *tlvmp,
	byte *externally_supplied_buffer,
    int externally_supplied_buffer_size)
{
    tlvmp->buffer = externally_supplied_buffer;
    tlvmp->buf_size = externally_supplied_buffer_size;
    tlvmp->tlvs = NULL;
    tlvm_reset(tlvmp);
}

int
tlvm_append (tlvm_t *tlvmp,
    int type, int length, byte *value)
{
    int size, len, idx;

    /* minimum total number of bytes needed in the buffer */
    size = length + sizeof(type) + sizeof(length);
    if (size > tlvmp->remaining_size) return ENOSPC;

    idx = tlvmp->idx;

    /* encode type and write it into the buffer */
    type = htonl(type);
    byte_copy(&type, &tlvmp->buffer[idx], sizeof(type));
    idx += sizeof(type);

    /* encode length and write it into the buffer */
    len = length;
    length = htonl(length);
    byte_copy(&length, &tlvmp->buffer[idx], sizeof(length));
    idx += sizeof(length);

    /* copy the value into the buffer */
    byte_copy(value, &tlvmp->buffer[idx], len);
    idx += len;

    /* update the write index & counters */
    tlvmp->idx = idx;
    tlvmp->remaining_size -= size;
    tlvmp->n_tlvs++;

    return 0;
}

int
tlvm_append_tlv (tlvm_t *tlvmp, one_tlv_t *tlv)
{
    return
        tlvm_append(tlvmp, tlv->type, tlv->length, tlv->value);
}

int
tlvm_parse (tlvm_t *tlvmp)
{
    byte *bptr, *end;
    one_tlv_t *tlvp, *new_tlvs;
    int type, length;

    bptr = &tlvmp->buffer[0];
    end = bptr + tlvmp->buf_size;
    tlvm_reset(tlvmp);
    while (bptr <= end) {

        /*
         * get type, while checking it does not 
         * go past the end of the buffer.
         */
        if ((bptr + sizeof(type)) >= end) return ENOMEM;
        memcpy(&type, bptr, sizeof(type));
        type = ntohl(type);
        bptr += sizeof(type);

        /*
         * get length, while checking it does not
         * go past the end of the buffer.
         */
        if (bptr + sizeof(length) >= end) return ENOMEM;
        memcpy(&length, bptr, sizeof(length));
        length = ntohl(length);

        /* check validity of length */
        if ((length <= 0) || (length > MAX_TLV_VALUE_BYTES)) return ENOMEM;
        bptr += sizeof(length);

        /*
         * Add this newly parsed tlv to the end of the tlvs array by
         * expanding the array by one extra tlv structure using realloc.
         * If no more space left to expand the array, it stops parsing
         * and leaves the tlvs parsed so far intact but does not set
         * 'parse_complete' as an indication that something went wrong
         * during parsing.  The function return value is 0 if all the
         * tlvs are parsed successfuly or an errno if parsing terminated
         * prematurely as a result of an error.
         */
        new_tlvs = realloc(tlvmp->tlvs, (tlvmp->n_tlvs+1) * sizeof(one_tlv_t));
        if (NULL == new_tlvs) return ENOMEM;
        tlvmp->tlvs = new_tlvs;
        tlvp = &tlvmp->tlvs[tlvmp->n_tlvs];
        tlvp->type = type;
        tlvp->length = length;
        tlvp->value = bptr;

        /* get ready to parse the next tlv */
        bptr += length;

        /* one more tlv parsed */
        tlvmp->n_tlvs++;
    }
    tlvmp->parse_complete = 1;
    return 0;
}

void
tlvm_destroy (tlvm_t *tlvmp)
{
    tlvmp->buffer = NULL;
    tlvmp->buf_size = 0;
    tlvm_reset(tlvmp);
}







