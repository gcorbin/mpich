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
    * Scatter; root is rank 0 in left group
    */
    const int sendcount = 100;
    const int recvcount = 100;
    int recvbuf[recvcount];
    int* sendbuf;
    int root;

    if ( in_left_group )
    {
        if ( local_rank == 0 )
        {
            root = MPI_ROOT;
            sendbuf = (int*) malloc( sizeof(int) * sendcount * remote_size );
            expected = ( ByteCounts ){ .send = 4 * sendcount * remote_size,
                                       .recv = 0 };
        }
        else
        {
            root = MPI_PROC_NULL;
            expected = ( ByteCounts ){ .send = 0, .recv = 0 };
        }
    }
    else
    {
        root = 0;
        expected = ( ByteCounts ){ .send = 0,
                                   .recv = 4 * recvcount };
    }
    MTest_Scatter(sendbuf, sendcount, MPI_INT, recvbuf, recvcount, MPI_INT, root, intercomm);
    print_expected_bytes(intercomm, in_left_group, expected);

    if ( in_left_group && local_rank == 0 )
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
