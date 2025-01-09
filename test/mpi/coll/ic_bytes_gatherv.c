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
    * Gatherv; root is rank 0 in left group
    * Send 100 * local_rank ints from each rank in the right group to the root
    */
    int sendcount = 0;
    int *recvcounts = 0;
    int *displs = 0;
    int *sendbuf = 0;
    int *recvbuf = 0;
    int root;

    if ( in_left_group )
    {
        if ( local_rank == 0 )
        {
            root = MPI_ROOT;
            recvcounts = (int*) malloc( sizeof(int) * remote_size );
            displs = (int*) malloc( sizeof(int) * remote_size );

            int disp = 0;
            for ( int i = 0; i < remote_size; ++i )
            {
                recvcounts[i] = i * 100;
                displs[i] = disp;
                disp += recvcounts[i];
            }
            recvbuf = (int*) malloc( sizeof(int) * disp );
            expected = ( ByteCounts ){ .send = 0,
                                       .recv = 4 * 100 * (remote_size * (remote_size-1))/2 };
        }
        else
        {
            root = MPI_PROC_NULL;
            expected = ( ByteCounts ){ .send = 0, .recv = 0 };
        }
    }
    else
    {
        sendcount = local_rank * 100;
        sendbuf = (int*) malloc ( sizeof(int) * sendcount );
        root = 0;
        expected = ( ByteCounts ){ .send = 4 * sendcount,
                                   .recv = 0 };
    }
    MTest_Gatherv(sendbuf, sendcount, MPI_INT, recvbuf, recvcounts, displs, MPI_INT, root, intercomm);
    print_expected_bytes(intercomm, in_left_group, expected);

    if ( in_left_group && local_rank == 0 )
    {
        free(recvcounts);
        free(displs);
        free(recvbuf);
    }
    if ( !in_left_group )
    {
        free(sendbuf);
    }


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
