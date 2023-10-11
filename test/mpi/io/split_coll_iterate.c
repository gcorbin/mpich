/*
 * Translated from writeallbef90.f90
 *
 */
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void handle_error(int errcode, const char *str)
{
    char msg[MPI_MAX_ERROR_STRING];
    int resultlen;
    MPI_Error_string(errcode, msg, &resultlen);
    fprintf(stderr, "%s: %s\n", str, msg);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

#define MPI_CHECK(fn) { int errcode; errcode = (fn); if (errcode != MPI_SUCCESS) handle_error(errcode, #fn); }


int main(int argc, char **argv)
{
    int rank, size;
    const int n = 4000;
    const int b = 2;
    int intsize;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_File file;
    int errs = 0;


    MTest_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    MPI_Type_size(MPI_INT, &intsize);

    MPI_Datatype filetype;
    MPI_Offset offset;
    MPI_Type_vector(b, n, n*size, MPI_INT, &filetype);
    MPI_Type_commit(&filetype);
    offset = rank * n * intsize;

    int* buf = (int*)malloc(n*sizeof(int));

    MPI_CHECK(MPI_File_open(comm, "iotest.txt", MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &file));
    MPI_CHECK(MPI_File_set_view(file, offset, MPI_INT, filetype, "native", MPI_INFO_NULL));
    for ( int k = 1; k <= b; ++k)
    {
        for ( int i =1; i<=n; ++i)
        {
            buf[i-1] = rank * n + (k-1)*n*size + i - 1;
        }
        MPI_CHECK(MPI_File_write_all_begin(file, buf, n, MPI_INT));
        MPI_Status status;
        MPI_CHECK(MPI_File_write_all_end(file, buf, &status));
    }
    MPI_CHECK(MPI_File_close(&file));

    MPI_CHECK(MPI_File_open(comm, "iotest.txt", MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &file));
    MPI_CHECK(MPI_File_set_view(file, offset, MPI_INT, filetype, "native", MPI_INFO_NULL));
    for ( int k = 1; k <= b; ++k)
    {
        for ( int i =1; i<=n; ++i)
        {
            buf[i-1] = 0;
        }
        MPI_CHECK(MPI_File_read_all_begin(file, buf, n, MPI_INT));
        MPI_Status status;
        MPI_CHECK(MPI_File_read_all_end(file, buf, &status));
        for ( int i = 1; i <= n; ++i)
        {
            int expected = rank * n + (k-1)*n*size + i - 1;
            if (buf[i-1] != expected)
            {
                errs++;
                printf("buf[%d] = %d, expected %d\n", i-1, buf[i-1], expected);
            }
        }
    }
    MPI_CHECK(MPI_File_close(&file));

    MPI_Barrier(comm);
    if (rank == 0)
    {
        MPI_CHECK(MPI_File_delete("iotest.txt", MPI_INFO_NULL));
    }
    MPI_Barrier(comm);

    MPI_CHECK(MPI_Type_free(&filetype));

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
