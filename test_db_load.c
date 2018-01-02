
#include "generic_object_database.h"

object_database_t db;

int main (int argc, char *argv[])
{
    int rv;

    printf("loading database .. ");
    fflush(stdout);
    fflush(stdout);
    rv = database_load(1, &db);
    if (0 == rv) {
        printf("done, database has %d objects\n",
            table_member_count(&db.object_index));
    } else {
        fprintf(stderr, "FAILED\n");
    }
    return rv;
}






