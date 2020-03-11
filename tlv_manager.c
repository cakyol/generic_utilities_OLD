
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

void
tlvm_reset (tlvm_t *tlvmp)
{
    tlvmp->idx = 0;
    tlvmp->remaining_size = tlvmp->buf_size;
    if (tlvmp->tlvs)
        free(tlvmp->tlvs);
    tlvmp->tlvs = 0;
    tlvmp->n_tlvs = 0;
}

void
tlvm_init (tlvm_t *tlvmp,
	byte *externally_supplied_buffer,
    int externally_supplied_buffer_size)
{
    tlvmp->buffer = externally_supplied_buffer;
    tlvmp->buf_size = externally_supplied_buffer_size;
    tlvmp->tlvs = 0;
    tlvm_reset(tlvmp);
}

int
tlvm_append (tlvm_t *tlvmp,
    int type, int length, byte *value)
{
    int size, len;

    len = length;
    size = length + sizeof(type) + sizeof(length);
    if (size > tlvmp->remaining_size) {
        return ENOMEM;
    }

    /* encode type */
    type = htonl(type);
    memcpy(&tlvmp[tlvmp->idx], &type, sizeof(type));
    tlvmp->idx += sizeof(type);

    /* encode length */
    length = htonl(length);
    memcpy(&tlvmp[tlvmp->idx], &length, sizeof(length));
    tlvmp->idx += sizeof(length);

    /* copy rest of the data */
    memcpy(&tlvmp[tlvmp->idx], value, len);
    tlvmp->idx += len;

    return 0;
}

int
tlvm_append_tlv (tlvm_t *tlvmp, one_tlv_t *tlv)
{
    return
        tlvm_append(tlvmp, tlv->type, tlv->length, tlv->value);
}

