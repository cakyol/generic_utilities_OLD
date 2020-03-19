
#include "tlv_manager.h"

#define BUFSIZE 100000
byte buffer [BUFSIZE];

#define DATASIZE 11
byte data[DATASIZE];

int main (int argc, char *argv[])
{
    tlvm_t tlvm;
    int i, j, max;
    int rc, tlv;
    one_tlv_t *tlvp;

    tlvm_attach(&tlvm, &buffer[0], BUFSIZE);

    printf("adding tlvs .. ");
    fflush(stdout);
    max = 0;
    for (i = 0; 1; i++) {
        for (j = 0; j < DATASIZE; j++) data[j] = i;
        printf("adding tlv type %d .. ", i);
        fflush(stdout);
        rc = tlvm_append(&tlvm, i, DATASIZE, &data[0]);
        if (rc) {
            printf("FAILED\n");
            break;
        } else {
            printf("done\n");
        }
        max++;
    }
    printf("\n%d tlvs added\n", max);

    printf(".... now parsing .. ");
    fflush(stdout);

    /* cheat so that parse does not complete fully */
    // tlvm.buf_size -= 113;

    rc = tlvm_parse(&tlvm);
    printf("done\n");
    if (rc || !tlvm.parse_complete) {
        printf("\n***** PARSE DID NOT COMPLETELY FINISH (%d) *****\n", rc);
    }
    fflush(stdout);

    printf(".... now verifying .. ");
    fflush(stdout);

    if (max != tlvm.n_tlvs) {
        printf("ERROR: %d tlvs parsed, should be %d\n",
            tlvm.n_tlvs, max);
        return -1;
    }

    for (i = 0, tlv = 0; i < max; i++, tlv++) {
        for (j = 0; j < DATASIZE; j++) data[j] = i;
        tlvp = &tlvm.tlvs[tlv];
        if (tlvp->type != i) {
            printf("tlv %d ERROR: mismatched type: %d, should be %d\n",
                tlv, tlvp->type, i);
        }
        if (tlvp->length != DATASIZE) {
            printf("tlv %d ERROR: mismatched length: %d, should be %d\n",
                tlv, tlvp->length, DATASIZE);
        }
        for (j = 0; j < DATASIZE; j++) {
            if (tlvp->value[j] != data[j]) {
                printf("tlv %d ERROR: mismatched data %d "
                    "at index %d, should be %d\n",
                tlv, tlvp->value[j], j, data[j]);
            }
        }
    }
    return 0;
}


