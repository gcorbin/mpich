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
    * Alltoallv; Exchange is not symmetric.
    * Send from left to right:
    * 20 * right_rank ints to each right rank
    * Send from right to left:
    * 100 * left_rank ints to each left rank
    */
    int *sendcounts = 0;
    int *recvcounts = 0;
    int *sdispls = 0;
    int *rdispls = 0;
    int *sendbuf = 0;
    int *recvbuf = 0;

    if ( in_left_group )
    {
        sendcounts = (int*) malloc( sizeof(int) * remote_size );
        recvcounts = (int*) malloc( sizeof(int) * remote_size );
        sdispls = (int*) malloc( sizeof(int) * remote_size );
        rdispls = (int*) malloc( sizeof(int) * remote_size );
        int sdisp = 0;
        int rdisp = 0;
        for ( int rank = 0; rank < remote_size; ++rank )
        {
            sendcounts[rank] = 20 * rank;
            sdispls[rank] = sdisp;
            sdisp += sendcounts[rank];

            recvcounts[rank] = 100 * local_rank;
            rdispls[rank] = rdisp;
            rdisp += recvcounts[rank];
        }
        sendbuf = (int*) malloc( sizeof(int) * sdisp );
        recvbuf = (int*) malloc( sizeof(int) * rdisp );
        expected = ( ByteCounts ){ .send = 4 * sdisp,
                                   .recv = 4 * rdisp };
    }
    else
    {
        sendcounts = (int*) malloc( sizeof(int) * remote_size );
        recvcounts = (int*) malloc( sizeof(int) * remote_size );
        sdispls = (int*) malloc( sizeof(int) * remote_size );
        rdispls = (int*) malloc( sizeof(int) * remote_size );
        int sdisp = 0;
        int rdisp = 0;
        for ( int rank = 0; rank < remote_size; ++rank )
        {
            sendcounts[rank] = 100 * rank;
            sdispls[rank] = sdisp;
            sdisp += sendcounts[rank];

            recvcounts[rank] = 20 * local_rank;
            rdispls[rank] = rdisp;
            rdisp += recvcounts[rank];
        }
        sendbuf = (int*) malloc( sizeof(int) * sdisp );
        recvbuf = (int*) malloc( sizeof(int) * rdisp );
        expected = ( ByteCounts ){ .send = 4 * sdisp,
                                   .recv = 4 * rdisp };
    }
    MTest_Alltoallv(sendbuf, sendcounts, sdispls, MPI_INT, recvbuf, recvcounts, rdispls,  MPI_INT, intercomm);
    print_expected_bytes(intercomm, in_left_group, expected);

    free(sendcounts);
    free(recvcounts);
    free(sdispls);
    free(rdispls);
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
