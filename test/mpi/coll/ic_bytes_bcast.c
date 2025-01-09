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
    * Bcast; root is rank 0 in left group
    */
    const int count = 100;
    int buf[count];
    int root;

    if ( in_left_group )
    {
        if ( local_rank == 0 )
        {
            root = MPI_ROOT;
            expected = ( ByteCounts ){ .send = 4 * count * remote_size,
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
                                .recv = 4 * count };
    }
    MTest_Bcast(buf, count, MPI_INT, root, intercomm);
    print_expected_bytes(intercomm, in_left_group, expected);

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
