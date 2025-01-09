#ifndef IC_BYTES_COMMON_H
#define IC_BYTES_COMMON_H

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

void getIntercomm(int lsize, int rsize, MPI_Comm* intercomm, int* in_left_group);

typedef struct ByteCounts
{
    uint64_t send;
    uint64_t recv;
} ByteCounts;

void print_expected_bytes(MPI_Comm intercomm, int in_left_group, ByteCounts bytes);
#endif
