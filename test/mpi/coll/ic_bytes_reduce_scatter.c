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
    * Reduce_scatter
    */
    int basecount = 100;
    int remote_factor = (remote_size * (remote_size - 1))/2;
    int recvcounts[local_size];
    int count = 0;
    for ( int i = 0; i < local_size; ++i)
    {
        recvcounts[i] = basecount * i * remote_factor;
        count += recvcounts[i];
    }
    int *sendbuf = (int*) malloc( sizeof(int) * count );
    int *recvbuf = (int*) malloc( sizeof(int) * recvcounts[local_rank] );

    expected = ( ByteCounts ){ .send = 4 * count,
                               .recv = 4 * recvcounts[local_rank] * remote_size };

    MTest_Reduce_scatter(sendbuf, recvbuf, recvcounts, MPI_INT, MPI_SUM, intercomm);
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
