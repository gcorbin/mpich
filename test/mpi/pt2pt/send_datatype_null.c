/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Send an empty message with MPI_DATATYPE_NULL";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    int size;
    int rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2) {
        fprintf(stderr, "This test requires at least two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        MPI_Send(NULL, 0, MPI_DATATYPE_NULL, 1, 0, MPI_COMM_WORLD);
    } else if (rank == 1) {
        MPI_Recv(NULL, 0, MPI_DATATYPE_NULL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
