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
    * Alltoall; Exchange is not symmetric.
    */
    int sendcount = 0;
    int recvcount = 0;
    int *sendbuf = 0;
    int *recvbuf = 0;

    if ( in_left_group )
    {
        sendcount = 0;
        recvcount = 100;

        sendbuf = (int*) malloc( sizeof(int) * sendcount * remote_size );
        recvbuf = (int*) malloc( sizeof(int) * recvcount * remote_size );
        expected = ( ByteCounts ){ .send = 4 * sendcount * remote_size,
                                   .recv = 4 * recvcount * remote_size };
    }
    else
    {
        sendcount = 100;
        recvcount = 0;

        sendbuf = (int*) malloc( sizeof(int) * sendcount * remote_size );
        recvbuf = (int*) malloc( sizeof(int) * recvcount * remote_size );
        expected = ( ByteCounts ){ .send = 4 * sendcount * remote_size,
                                   .recv = 4 * recvcount * remote_size };
    }
    MTest_Alltoall(sendbuf, sendcount, MPI_INT, recvbuf, recvcount, MPI_INT, intercomm);
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
