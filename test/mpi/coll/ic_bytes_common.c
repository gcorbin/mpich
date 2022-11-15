#include "ic_bytes_common.h"

void getIntercomm(int lsize, int rsize, MPI_Comm* intercomm, int* in_left_group)
{
    const int total_requested_size = lsize + rsize;

    int wsize, wrank;
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    if ( wsize != total_requested_size )
    {
        fprintf(stderr, "Exactly %d processes required, but world size is only %d\n", total_requested_size, wsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /*
     * Split MPI_COMM_WORLD into two comms and make an intercomm from those.
     * The left comm contains ranks 0 to lsize-1. Leader is rank 0.
     * The right comm contains ranks lsize to total_requested_size-1. Leader is rank lsize.
     * Extra ranks don't participate.
     */
    int group = 2;
    if ( wrank < lsize )
    {
        group = 0;
    }
    else if ( wrank < total_requested_size )
    {
        group = 1;
    }
    else
    {
        group = 2;
    }
    int remote_leader = -1;
    MPI_Comm split_comm;
    MPI_Comm_split(MPI_COMM_WORLD, group, wrank, &split_comm);


    if ( wrank == 0 )
    {
        remote_leader = lsize;
    }
    else if ( wrank == lsize )
    {
        remote_leader = 0;
    }
    if ( group < 2 )
    {
        MPI_Intercomm_create(split_comm, 0, MPI_COMM_WORLD, remote_leader, 12345, intercomm);
    }
    else
    {
        *intercomm = MPI_COMM_NULL;
    }
    *in_left_group = ( group == 0);
}


void print_expected_bytes(MPI_Comm intercomm, int in_left_group, ByteCounts bytes)
{
    int local_rank;
    PMPI_Comm_rank(intercomm, &local_rank);

    MPI_Comm merged_comm;
    PMPI_Intercomm_merge(intercomm, !in_left_group, &merged_comm);

    int msize, mrank;
    PMPI_Comm_size(merged_comm, &msize);
    PMPI_Comm_rank(merged_comm, &mrank);

    int wrank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    uint64_t sendbytes[msize], recvbytes[msize];
    int left_right[msize];
    int local_ranks[msize];
    int world_ranks[msize];
    PMPI_Gather(&bytes.send, 1, MPI_UINT64_T, sendbytes, 1, MPI_UINT64_T, 0, merged_comm);
    PMPI_Gather(&bytes.recv, 1, MPI_UINT64_T, recvbytes, 1, MPI_UINT64_T, 0, merged_comm);
    PMPI_Gather(&in_left_group, 1, MPI_INT, left_right, 1, MPI_INT, 0, merged_comm);
    PMPI_Gather(&local_rank, 1, MPI_INT, local_ranks, 1, MPI_INT, 0, merged_comm);
    PMPI_Gather(&wrank, 1, MPI_INT, world_ranks, 1, MPI_INT, 0, merged_comm);
    if ( mrank == 0 )
    {
//         for (int rank = 0; rank < msize; ++rank)
//         {
//             MTestPrintfMsg(1, "Rank %3d(%3d in %s group): sendbytes = %8ld, recvbytes = %8ld\n", world_ranks[rank], local_ranks[rank], (left_right[rank])?"left ":"right", sendbytes[rank], recvbytes[rank]);
//         }
        MTestPrintfMsg(1, "sendbytes: ");
        for (int rank = 0; rank < msize; ++rank)
        {
            MTestPrintfMsg(1, "%8ld ", sendbytes[rank]);
        }
        MTestPrintfMsg(1, "\n");
        MTestPrintfMsg(1, "recvbytes: ");
        for (int rank = 0; rank < msize; ++rank)
        {
            MTestPrintfMsg(1, "%8ld ", recvbytes[rank]);
        }
        MTestPrintfMsg(1, "\n");
    }
}
