#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#include "mpitest.h"
#include "mpicolltest.h"
#include "ic_bytes_common.h"

int run_test( MPI_Comm intercomm, int in_left_group )
{
    int errs = 0;
    ByteCounts expected = { .send = 0, .recv = 0 };
    int local_rank, local_size, remote_size;
    MPI_Comm_rank(intercomm, &local_rank);
    MPI_Comm_size(intercomm, &local_size);
    MPI_Comm_remote_size(intercomm, &remote_size);

    /*
    * Test:
    * Allreduce
    */
    int count = 0;
    int *sendbuf = 0;
    int *recvbuf = 0;

    if ( in_left_group )
    {
        count = 100;
        sendbuf = (int*) malloc( sizeof(int) * count );
        recvbuf = (int*) malloc( sizeof(int) * count );
        expected = ( ByteCounts ){ .send = 4 * count * remote_size,
                                   .recv = 4 * count * remote_size };
    }
    else
    {
        count = 100;
        sendbuf = (int*) malloc( sizeof(int) * count );
        recvbuf = (int*) malloc( sizeof(int) * count );
        expected = ( ByteCounts ){ .send = 4 * count * remote_size,
                                   .recv = 4 * count * remote_size };
    }
    MTest_Allreduce(sendbuf, recvbuf, count, MPI_INT, MPI_SUM, intercomm);
    print_expected_bytes(intercomm, in_left_group, expected);

    free(sendbuf);
    free(recvbuf);

    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    /*
     * Create Intercomm
     */
    int in_left_group;
    MPI_Comm intercomm = MPI_COMM_NULL;
    getIntercomm(2, 4, &intercomm, &in_left_group);
    if ( intercomm != MPI_COMM_NULL )
    {
        errs += run_test(intercomm, in_left_group );
    }
    MTest_Finalize_PMPI(errs);
    return MTestReturnValue(errs);
}
