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
    * Allgatherv; Exchange is not symmetric.
    * Send from left to right:
    * 20 * left_rank ints
    * Send from right to left:
    * 100 * right_rank ints
    */
    int sendcount = 0;
    int *recvcounts = 0;
    int *sendbuf = 0;
    int *recvbuf = 0;
    int *displs = 0;

    if ( in_left_group )
    {
        sendcount = 20 * local_rank;
        recvcounts = (int*) malloc( sizeof(int) * remote_size );
        displs = (int*) malloc( sizeof(int) * remote_size );

        int disp = 0;
        for ( int i = 0; i < remote_size; ++i )
        {
            recvcounts[i] = i * 100;
            displs[i] = disp;
            disp += recvcounts[i];
        }

        sendbuf = (int*) malloc( sizeof(int) * sendcount );
        recvbuf = (int*) malloc( sizeof(int) * disp );
        expected = ( ByteCounts ){ .send = 4 * sendcount * remote_size,
                                   .recv = 4 * disp };
    }
    else
    {
        sendcount = 100 * local_rank;
        recvcounts = (int*) malloc( sizeof(int) * remote_size );
        displs = (int*) malloc( sizeof(int) * remote_size );

        int disp = 0;
        for ( int i = 0; i < remote_size; ++i )
        {
            recvcounts[i] = i * 20;
            displs[i] = disp;
            disp += recvcounts[i];
        }

        sendbuf = (int*) malloc( sizeof(int) * sendcount );
        recvbuf = (int*) malloc( sizeof(int) * disp );
        expected = ( ByteCounts ){ .send = 4 * sendcount * remote_size,
                                   .recv = 4 * disp };
    }
    MTest_Allgatherv(sendbuf, sendcount, MPI_INT, recvbuf, recvcounts, displs, MPI_INT, intercomm);
    print_expected_bytes(intercomm, in_left_group, expected);

    free(recvcounts);
    free(displs);
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
