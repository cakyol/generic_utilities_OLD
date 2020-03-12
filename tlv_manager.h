
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

#ifndef __TLV_MANAGER_H__
#define __TLV_MANAGER_H__

#include "common.h"

/*
 * max allowed length in bytes of the value part of a tlv.
 * Parsing will fail if any tlv value exceeds this many bytes.
 */
#define MAX_TLV_VALUE_BYTES     512

/*
 * representation of a single tlv (serialized form)
 */
typedef struct one_tlv_s {

    int type;
    int length;     /* length of the 'value' only */
    byte *value;

} one_tlv_t;

/*
 * tlv manager "assist" structure.
 * Depending on whether this is being used to create a list of
 * tlvs, or being used to parse an existing list of tlvs, appropriate
 * filelds are used in appropriate ways.
 */
typedef struct tlvm_s {

    /*
     * If we are creating the tlv list, this is the buffer
     * being written into.  If we are parsing a tlv list,
     * this is the buffer which contains the existing tlvs.
     */
	byte *buffer;
    int buf_size;

    /*
     * read or write index into the buffer depending on whether
     * a tlv list is being created or being parsed.
     */
    int idx;

    /*
     * how many bytes are left to write into or read from,
     * depending on whether a tlv list is being created or
     * an existing tlv list is being parsed.
     */
	int remaining_size;

    /*
     * these are used for parsing an existing list of tlvs.
     * At the end of the parse, n_tlvs is set to how many tlvs
     * has been parsed and tlvs are an array of the tlvs, each
     * one points to the specific tlv.  The array will be malloced
     * and re-alloced if it grows.  Therefore, do NOT store
     * the pointers into the array since they MAY change.
     * If everything "seemed" ok during the parsing, then the
     * field 'parse_complete' will be set to non zero.  If there
     * was ANY issue during the parse, it will be set to zero
     * and parsing will be terminated indicating only partial
     * extraction has been made.
     */
    int n_tlvs;
    int parse_complete;
    one_tlv_t *tlvs;

} tlvm_t;

/*
 * initializes a tlv manager object to a buffer which is externally 
 * provided with the given size.  This can be a buffer which may
 * be written into if a tlv list is being created or an existing
 * list of tlvs to be parsed.
 */
extern void
tlvm_init (tlvm_t *tlvmp,
	byte *externally_supplied_buffer,
    int externally_supplied_buffer_size);

/*
 * append a tlv to the managed tlv buffer.
 * Note that the 'length' is ONLY the length of the data (value).
 * 0 is returned for no error or an errno if fails.
 */
extern int
tlvm_append (tlvm_t *tlvmp,
    int type, int length, byte *value);

extern int
tlvm_append_tlv (tlvm_t *tlvmp, one_tlv_t *tlv);

/*
 * parses all the tlvs in an initialised tlv manager and assigns
 * all the tlv array structures to the correct places in the buffer
 */
extern int
tlvm_parse (tlvm_t *tlvmp);

extern void
tlvm_reset (tlvm_t *tlvmp);

#endif /* __TLV_MANAGER_H__ */

